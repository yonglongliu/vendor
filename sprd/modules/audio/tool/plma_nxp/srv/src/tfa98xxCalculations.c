/*
 * tfa98xxCalculations.c
 *
 *  Created on: Okt 23, 2014
 *      Author: Jeffrey
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <math.h>
#include <string.h>
#include "tfa_service.h"
#include "tfa98xxCalculations.h"

// Local functions and the corresponding data structures definition
typedef struct 
{
  double     xSoftStartDb;
  double     xSoftEndDb;
  double     softKneeRatio;
  double     ySoftEndDb;
} s_Soft_Knee;

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

/////////////////////////////////////////////////////////
// Sorensen in-place split-radix FFT for real values
// data: array of doubles:
// re(0),re(1),re(2),...,re(size-1)
//
// output:
// re(0),re(1),re(2),...,re(size/2),im(size/2-1),...,im(1)
// normalized by array length
//
// Source:
// Sorensen et al: Real-Valued Fast Fourier Transform Algorithms,
// IEEE Trans. ASSP, ASSP-35, No. 6, June 1987
void realfft_split(double *data, long n)
{
        long i, j, k, i5, i6, i7, i8, i0, id, i1, i2, i3, i4, n2, n4, n8;
        double t1, t2, t3, t4, t5, t6, a3, ss1, ss3, cc1, cc3, a, e, sqrt2;
        double pi = 3.1415926f;
        sqrt2 = sqrt(2.0);
        n4 = n - 1;
//data shuffling
        for (i = 0, j = 0, n2 = n / 2; i < n4; i++) {
                if (i < j) {
                        t1 = data[j];
                        data[j] = data[i];
                        data[i] = t1;
                }
                k = n2;
                while (k <= j) {
                        j -= k;
                        k >>= 1;
                }
                j += k;
        }
    /*----------------------*/
//length two butterflies
        i0 = 0;
        id = 4;
        do {
                for (; i0 < n4; i0 += id) {
                        i1 = i0 + 1;
                        t1 = data[i0];
                        data[i0] = t1 + data[i1];
                        data[i1] = t1 - data[i1];
                }
                id <<= 1;
                i0 = id - 2;
                id <<= 1;
        } while (i0 < n4);
    /*----------------------*/
//L shaped butterflies
        n2 = 2;
        for (k = n; k > 2; k >>= 1) {
                n2 <<= 1;
                n4 = n2 >> 2;
                n8 = n2 >> 3;
                e = 2 * pi / (n2);
                i1 = 0;
                id = n2 << 1;
                do {
                        for (; i1 < n; i1 += id) {
                                i2 = i1 + n4;
                                i3 = i2 + n4;
                                i4 = i3 + n4;
                                t1 = data[i4] + data[i3];
                                data[i4] -= data[i3];
                                data[i3] = data[i1] - t1;
                                data[i1] += t1;
                                if (n4 != 1) {
                                        i0 = i1 + n8;
                                        i2 += n8;
                                        i3 += n8;
                                        i4 += n8;
                                        t1 = (data[i3] + data[i4]) / sqrt2;
                                        t2 = (data[i3] - data[i4]) / sqrt2;
                                        data[i4] = data[i2] - t1;
                                        data[i3] = -data[i2] - t1;
                                        data[i2] = data[i0] - t2;
                                        data[i0] += t2;
                                }
                        }
                        id <<= 1;
                        i1 = id - n2;
                        id <<= 1;
                } while (i1 < n);
                a = e;
                for (j = 2; j <= n8; j++) {
                        a3 = 3 * a;
                        cc1 = cos(a);
                        ss1 = sin(a);
                        cc3 = cos(a3);
                        ss3 = sin(a3);
                        a = j * e;
                        i = 0;
                        id = n2 << 1;
                        do {
                                for (; i < n; i += id) {
                                        i1 = i + j - 1;
                                        i2 = i1 + n4;
                                        i3 = i2 + n4;
                                        i4 = i3 + n4;
                                        i5 = i + n4 - j + 1;
                                        i6 = i5 + n4;
                                        i7 = i6 + n4;
                                        i8 = i7 + n4;
                                        t1 = data[i3] * cc1 + data[i7] * ss1;
                                        t2 = data[i7] * cc1 - data[i3] * ss1;
                                        t3 = data[i4] * cc3 + data[i8] * ss3;
                                        t4 = data[i8] * cc3 - data[i4] * ss3;
                                        t5 = t1 + t3;
                                        t6 = t2 + t4;
                                        t3 = t1 - t3;
                                        t4 = t2 - t4;
                                        t2 = data[i6] + t6;
                                        data[i3] = t6 - data[i6];
                                        data[i8] = t2;
                                        t2 = data[i2] - t3;
                                        data[i7] = -data[i2] - t3;
                                        data[i4] = t2;
                                        t1 = data[i1] + t5;
                                        data[i6] = data[i1] - t5;
                                        data[i1] = t1;
                                        t1 = data[i5] + t4;
                                        data[i5] -= t4;
                                        data[i2] = t1;
                                }
                                id <<= 1;
                                i = id - n2;
                                id <<= 1;
                        } while (i < n);
                }
        }
// energy component is disabled
//division with array length
        // for (i = 0; i < n; i++)
        //     data[i] /= n;
}

/** Compute the Hanning window
        Matlab code:
      %                             h1 = hanning( lenW * 2 );
      %                             h1 = h1((end/2+1):end);
      h1 = .5*(1 - cos(2*pi*(1:lenW)'/(lenW*2+1)));
      h1 = h1(end:-1:1);
*/
void windowing_filter(double *data, int L)
{
        int i = 0;
        double pi = 3.1415926f;

        for (i=1; i<=L; i++) {
                data[L-i] = 0.5*(1.0 - cos(2.0*pi*(double)i / ((double)L*2.0 + 1.0)));
        }
}

void leakageFilter(double *data, int cnt, double lf)
{
        int i;
        double phi;
        float pi = 3.1415926f;

        for (i = 0; i < cnt; i++) {
                phi = sin(0.5 * pi * i / (cnt));
                phi *= phi;
                data[i] = sqrt(pow(1 + lf, 2) - 4 * lf * phi);
        }
}

/**
* Untangle the real and imaginary parts and take the modulus for max2
*/
void untangle_max2(double *waveform, double *hAbs, int L)
{
        int i, cnt = L / 2;

        hAbs[0] = waveform[0];

        for (i = 1; i < cnt; i++) {
                hAbs[i] = sqrt((waveform[i] * waveform[i]) +
                         (waveform[L - i] * waveform[L - i]));
        }
}

/**
* Untangle the real and imaginary parts and take the modulus for max1
*/
void untangle(double *waveform, double *hAbs, int L)
{
        int i, cnt = L / 2;

        hAbs[0] =
            sqrt((waveform[0] * waveform[0]) +
                 (waveform[L / 2] * waveform[L / 2]));
        for (i = 1; i < cnt; i++)
                hAbs[i] =
                    sqrt((waveform[i] * waveform[i]) +
                         (waveform[L - i] * waveform[L - i]));

}

/** Compute Zf
        Matlab code:
     Z = 1.0./fft(self.lsmod.wY,lenW);
     Z = Z(1:lenW/2+1);
     %                             Z =freqz(1.0, self.lsmod.wY , fVec, self.lsmod.fs);
     self.hTrcs.(selTrcfldNm{c1}).update( fVec, abs(Z) );
*/
void compute_Zf(double *wY, double *absZ,  double *scratch, int fft_size)
{
        int i = 0;

        memcpy(scratch, wY, fft_size*sizeof(double));
        realfft_split(scratch, fft_size);
   
        untangle_max2(scratch, absZ, fft_size);

        for (i = 0; i < fft_size/2; i++) {
                absZ[i] = 1 / (absZ[i] + 1e-6); // avoid division by 0 adding a small nr.
        }
}

/** Compute Xf
        Matlab code:
      wY = self.lsmod.wY' .* h1;
  
      % lagW is 12 by default
      lagW = 12;
  
      bX         = - self.lsmod.ReCorrection * self.lsmod.ReZ .* wY;
      bX(lagW+1) = bX(lagW+1) + 1;
      bX         = bX ./ self.lsmod.Bl /self.lsmod.fs * 1000 ; % 1/fs from aX
  
      % Compute denominator coefficients of excursion filter
      aX = [1 -self.lsmod.leakageFactor]';
  
      X = fft(bX,lenW)./fft(aX,lenW);
      X = X(1:lenW/2+1);
      %                             X =freqz(bX, aX , fVec, self.lsmod.fs);
  
      self.hTrcs.(selTrcfldNm{c1}).update( fVec, abs(X) );
*/
void compute_Xf(double *wY, double *absX, double *aw, double *scratch, int lagW, 
  double fs, double leakageFactor, double ReCorrection, double Bl, double ReZ, int fft_size)
{
        int i = 0;

        for (i = 0; i < fft_size; i++) {
                scratch[i] = - ReCorrection * ReZ * wY[i] * aw[i];

                if (i == lagW)
                        scratch[i] += 1.0;

                scratch[i] = 1000 * scratch[i] / (Bl * fs);
        }

        realfft_split(scratch, fft_size);
        untangle_max2(scratch, absX, fft_size);

        // get aX
        scratch[0] = 1.0;
        scratch[1] = -leakageFactor;
        memset(&scratch[2], 0, (fft_size-2)*sizeof(double));

        realfft_split(scratch, fft_size);
        untangle_max2(scratch, scratch, fft_size);  // compute in place as it is allowed by the untangle function

        for (i = 0; i < fft_size/2; i++) {
                absX[i] = absX[i] / (scratch[i] + 1e-6); // avoid division by 0 adding a small nr.
        }
}

/**
* Untangle the real and imaginary parts and take the modulus 
*  and divide by the leakage filter
*/
void untangle_leakage(double *waveform, double *hAbs, int L, double leakage)
{
        int i, cnt = L / 2;
        double m_Aw[64];

        // create the filter
        leakageFilter(m_Aw, cnt, leakage);
        // 
        hAbs[0] = sqrt((waveform[0] * waveform[0]) + (waveform[L / 2] * waveform[L / 2])) / m_Aw[0];
        for (i = 1; i < cnt; i++)
                hAbs[i] = sqrt((waveform[i] * waveform[i]) + (waveform[L - i] * waveform[L - i])) / m_Aw[i];
}

/*
 * for max2 emulate max1 stateinfo
 *
 *  refer to (max2)HostSDK/doc/stateinfo_emulate.xlsx for mapping
 */
struct flag_bits {
	int Activity:1;
	int S_Ctrl:1;
	int Muted:1;
	int X_Ctrl:1;
	int T_Ctrl:1;
	int NewModel:1;
	int VolumeRdy:1;
	int Damaged:1;
	int SignalClipping:1;
};
static int get_stateinfo_flags(int *data) {
	union {
		int flags;
		struct flag_bits bits;
	} map;
	map.bits.Activity = data[8];
	map.bits.S_Ctrl = 0;
	map.bits.Muted = 0;
	map.bits.X_Ctrl = 0;
	map.bits.T_Ctrl = 0;
	map.bits.NewModel = 0;
	map.bits.VolumeRdy = 0;
	map.bits.Damaged = data[9];
	map.bits.SignalClipping = data[10];
	return map.flags;
}
/*
 * pInfo must point to 2 structs
 */
enum Tfa98xx_Error
tfa98xx_calculate_state_info(const unsigned char bytes[], struct Tfa98xx_StateInfo *pInfo)
{
	enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
	int data[MEMTRACK_MAX_WORDS];	/* allocate worst case */
	int stateSize = MEMTRACK_MAX_WORDS;
	int i;

	tfa98xx_convert_bytes2data(3 * stateSize, bytes, data);

	/* data[0] : time measurement of blocks since last get cmd */
	for(i=0;i<2;i++) {
        pInfo[i].agcGain = (float)data[i+1] /65536;
        pInfo[i].limGain = (float)data[i+2] /65536;
        pInfo[i].sMax = (float)data[i+3] / 65536;//?
        pInfo[i].T = data[i+4]/ 16384;
        pInfo[i].statusFlag = get_stateinfo_flags(&data[i+0]);
        pInfo[i].X1 = (float)data[i+5] / 8388608;
        /* deal with type specific variations */

        	pInfo[i].X2 = (float)0;
        	pInfo[i].Re = (float)data[i+6] / 16384;//?
        	pInfo[i].shortOnMips = data[i+7];
	}

	return error;
}

double lin2db(int fac)
{
        return 20*log10(fac);
}

/** Compute Biquad frequency transfer function

Matlab code:
self.fs = 48000;
fVec = (0:((FFT_SIZE_DRC/2)-1))' * self.fs / FFT_SIZE_DRC;

% Replace the Matlab 'freqz' by the fft computation below
% X = abs(freqz(self.CoefB, self.CoefA, fVec, self.fs));

X = 20*log10(fft(self.CoefB,FFT_SIZE_DRC)./fft(self.CoefA,FFT_SIZE_DRC));
*/
void compute_biquad_freq_transf_function(double *outFreqTrFunc, double *coefB, double *coefA, double *scratch, int fft_size)
{
  int i = 0;

  memset(&scratch[3], 0, (fft_size-3)*sizeof(double));
  memcpy(scratch, coefB, 3*sizeof(double));
  realfft_split(scratch, fft_size);
  untangle(scratch, outFreqTrFunc, fft_size);

  memset(&scratch[3], 0, (fft_size-3)*sizeof(double));
  memcpy(scratch, coefA, 3*sizeof(double));
  realfft_split(scratch, fft_size);
  untangle(scratch, scratch, fft_size);

  for (i = 0; i < fft_size/2; i++){
    outFreqTrFunc[i] = 20*log10(outFreqTrFunc[i] / (scratch[i] + 1e-6)); // avoid division by 0 adding a small nr.
  }
}

//**************************************************************************************************
// The input filter is responsible for the input signal filtering which derives the current subband energy estimation.
// Unless the subband parameters: 'subBandCenter' or 'subBandFilterQ' change (which could be done by GUI knobs or from an xml parameter file),
// this filter coefficients remain unchanged in time. That means the filter frequency tranfer function remains also usually unchanged,
// and thus it is very important to check if the coefficients have changed before recomputing this filter transfer function.
//
// The output is a vector of size fft_size/2, which contains the transfer function values in dB
//            corresponding to a linear (e.g. not logarithmic) frequency scale from 0 Hz to fs/2 Hz;
//**************************************************************************************************
void compute_input_filter_transf_function(double *outFreqTrFunc, s_biquad_t *bqCoef, double *scratch, int fft_size, double lowLimit)
{
  int i = 0;
  double coefB[3];
  double coefA[3];
  double scaleFactor = pow(2.0, bqCoef->scale);

  // scale the coefficients
  coefA[0] = 1.0;
  coefA[1] = scaleFactor * bqCoef->a1;
  coefA[2] = scaleFactor * bqCoef->a2;
  coefB[0] = scaleFactor * bqCoef->b0;
  coefB[1] = scaleFactor * bqCoef->b1;
  coefB[2] = scaleFactor * bqCoef->b2;

  compute_biquad_freq_transf_function(outFreqTrFunc, coefB, coefA, scratch, fft_size);

  for (i = 0; i < fft_size/2; i++){
    if (outFreqTrFunc[i] < lowLimit){
      outFreqTrFunc[i] = lowLimit;
    }
  }
}

//**************************************************************************************************
// The gain filter is responsible for the subband output gain, and changes the transfer function depending 
// on the DSP computed 'subBandGain' at any frame. It is also dependant on the subband parameters: 
// 'subBandCenter' or 'subBandGainQ' which could be updated by GUI knobs or from an xml parameter file.
//
// The output is a vector of size fft_size/2, which contains the transfer function values in dB
//            corresponding to a linear (e.g. not logarithmic) frequency scale from 0 Hz to fs/2 Hz;
//**************************************************************************************************
void compute_gain_filter_transf_function(double *outFreqTrFunc, s_biquad_t *bqCoef, double subBandGain, double *scratch, int fft_size)
{
  int i = 0;
  double coefB[3];
  double coefA[3];
  double scaleFactor = pow(2.0, bqCoef->scale);

  // scale the coefficients
  coefA[0] = 1.0;
  coefA[1] = scaleFactor * bqCoef->a1;
  coefA[2] = scaleFactor * bqCoef->a2;
  coefB[0] = scaleFactor * bqCoef->b0;
  coefB[1] = scaleFactor * bqCoef->b1;
  coefB[2] = scaleFactor * bqCoef->b2;

  // Update CoefB according to the subBandGain
  for (i=0; i<3; i++)
  {
    coefB[i] = coefA[i] - (1.0 - subBandGain)*coefB[i];
  }

  compute_biquad_freq_transf_function(outFreqTrFunc, coefB, coefA, scratch, fft_size);
}

//**************************************************************************************************
// This function adds in frequency all the subbands transfer functions in order to get the MBDRC total 
// transfer function
//**************************************************************************************************
void compute_gain_combined_transf_function(double *outFreqTrFunc, double subBandsTrFunc[][FFT_SIZE_DRC/2], int nr_subbands, int buffLength)
{
  int i = 0;
  int j = 0;

  memcpy(outFreqTrFunc, subBandsTrFunc[0], buffLength*sizeof(double));

  for (i=1; i<nr_subbands; i++)
  {
    for (j=0; j<buffLength; j++)
    {
      outFreqTrFunc[j] += subBandsTrFunc[i][j];
    }
  }
}

//**************************************************************************************************
// The soft knee is applicable only to DRC_TYPE_LOG_SMP_COMP, DRC_TYPE_LOG_BLK_PEAK_COMP, 
// DRC_TYPE_LOG_BLK_AVG_COMP model types
//**************************************************************************************************
void setSoftKnee( s_Soft_Knee *softKnee, s_DRC_Params *paramsDRC, double crRatio )
{    
  // The 'kneeDb' point corresponds to the module parameter 'thresdB'
  if (paramsDRC->kneePercentage != 0.0) // check if there is a soft knee.            
  {
    double halfKnee = (MIN(fabs(paramsDRC->thresdB+100.0)/2.0, fabs(paramsDRC->thresdB)/2.0))*paramsDRC->kneePercentage/100.0;
    softKnee->xSoftStartDb = paramsDRC->thresdB - halfKnee;
    softKnee->xSoftEndDb = paramsDRC->thresdB + halfKnee;
    softKnee->softKneeRatio = 1.0/(softKnee->xSoftEndDb - softKnee->xSoftStartDb);
    softKnee->ySoftEndDb = paramsDRC->thresdB + (softKnee->xSoftEndDb - paramsDRC->thresdB)*crRatio;
  }
}

//**************************************************************************************************
// Get the soft knee transfer function using quadratic Bezier courves
//**************************************************************************************************
double computeSoftKneePoint( s_Soft_Knee *softKnee, double xIn, double thresdB )        
{   
  double xSoft = (xIn - softKnee->xSoftStartDb) * softKnee->softKneeRatio;
  double ySoft = softKnee->xSoftStartDb * (1.0 - xSoft) * (1.0 - xSoft) + 2.0 * thresdB * (1.0 - xSoft) * xSoft 
                  + softKnee->ySoftEndDb * xSoft * xSoft;
    
  ySoft = xIn - ySoft;

  return ySoft;
}

//**************************************************************************************************
// Function which returns the DRC compressor / expander time domain I/O transfer function.
// The DRC modes / types are the ones used in SB1.5.
// 
// The input is an synthetically generated envelope which exercise all the levels in dB between DRC_GUI_LOW_LIMIT_DB 
// and DRC_GUI_HIGH_LIMIT_DB, in 1 dB increment steps.
//
// The output is the DRC transfer function which corresponds to the input envelope.
//
//**************************************************************************************************
void compute_drc_transf_function(double *transfFunctDb, s_DRC_Params *paramsDRC, double *envelopeDb, int length)
{
  int i = 0;
  double crRatio = 0.0;
  double slope = 0.0;
  s_Soft_Knee softKnee;

  if (paramsDRC->on == 0)
  {
    for (i=0; i<length; i++)
    {
      transfFunctDb[i] = envelopeDb[i];
    }
    return;
  }

  if (paramsDRC->type == DRC_TYPE_LIN_BLK_XPN)
  {
    crRatio = paramsDRC->ratio;
  }else{
    // if soft limit, compute reciprocal, otherwise keep ratio = 0, for hard limit
    if (paramsDRC->ratio == 0.0)
    {
       crRatio = paramsDRC->ratio;
    }else{
       crRatio = 1.0/paramsDRC->ratio;
    }
  }
  
  slope = 1.0 - crRatio;
  setSoftKnee( &softKnee, paramsDRC, crRatio );

  // use transfFunctDb as a temporary storage for the gain values
  memset (transfFunctDb, 0, length*sizeof(double));
    
  switch( paramsDRC->type )
  {
    case DRC_TYPE_LOG_SMP_COMP:
    case DRC_TYPE_LOG_BLK_PEAK_COMP:
    case DRC_TYPE_LOG_BLK_AVG_COMP:
      if (paramsDRC->kneePercentage == 0.0)
      {
        // no soft knee
        for (i=0; i<length; i++)
        {
          if (envelopeDb[i] > paramsDRC->thresdB)
          {
            transfFunctDb[i] = -slope * (envelopeDb[i] - paramsDRC->thresdB);
          }
        }
      }else
      {
        for (i=0; i<length; i++)
        {
          // find the indexes for the soft part
          if ((envelopeDb[i] >= softKnee.xSoftStartDb)
            &&(envelopeDb[i] <= softKnee.xSoftEndDb))
          {
            transfFunctDb[i] = -computeSoftKneePoint( &softKnee, envelopeDb[i], paramsDRC->thresdB );
          }
          else if (envelopeDb[i] > softKnee.xSoftEndDb)
          {
            transfFunctDb[i] = -slope*(envelopeDb[i] - paramsDRC->thresdB);
          }
        }
      }
      break;

    case DRC_TYPE_LIN_BLK_XPN:
      for (i=0; i<length; i++)
      {
        // the soft knee is not supported by the expander as
        // it doesn't make sense given the very low / close
        // to noise floor level where the expander operates
        if ( envelopeDb[i] < paramsDRC->thresdB )
        {
          transfFunctDb[i] = paramsDRC->ratio * (envelopeDb[i] - paramsDRC->thresdB);

          // limit the gain to be applied to -100 dB
          if (transfFunctDb[i] < -100.0){
            transfFunctDb[i] = -100.0;
          }
        }else
        {
          transfFunctDb[i] = 0.0;
        }
      }
      break;
    case DRC_NB_OF_TYPES:
            /* Avoid warning */
        break;
  }

  for (i=0; i<length; i++)
  {
    /*
    // if necessary, limit the gain
    if (paramsDRC->maxCompGaindB >= -40.0)
    {
      if (transfFunctDb[i] < paramsDRC->maxCompGaindB){
        transfFunctDb[i] = paramsDRC->maxCompGaindB;
      }
    }
    */

    transfFunctDb[i] += envelopeDb[i] + paramsDRC->makeUpGaindB;
  
    if (transfFunctDb[i] < DRC_GUI_LOW_LIMIT_DB)
    {
      transfFunctDb[i] = DRC_GUI_LOW_LIMIT_DB;
    }
  }
}

//**************************************************************************************************
// Function which combines two DRC transfer functions. This is required by the MBDRC combination
// of 2 DRCs / subband.
//**************************************************************************************************
void compute_drc_combined_transf_function(double *combinedTransfFunctDb, double transfFunctDb[2][DRC_GUI_PLOT_LENGTH], double *envelopeDb, int length)
{
  int i = 0;

  for (i=0; i<length; i++)
  {
    combinedTransfFunctDb[i] = transfFunctDb[0][i] + transfFunctDb[1][i] - envelopeDb[i];
  }
}


void tfa98xx_calculate_dynamix_variables_drc(int *data, s_subBandDynamics *sbDynamics, int index_subband)
{
	int i;

	/* Max value is 6 subbands * 17 = 102 */
	for(i=1; i<103; i++) {
		if(i == 1+index_subband*17) {
			sbDynamics->BQ_1.scale = data[i-1];
		} else if(i >= 2+index_subband*17 && i <= 6+index_subband*17) {
			if(i == 2+index_subband*17)
				sbDynamics->BQ_1.b2 = ((double)data[i-1] / (double)8388608);
			if(i == 3+index_subband*17)
				sbDynamics->BQ_1.b1 = ((double)data[i-1] / (double)8388608);
			if(i == 4+index_subband*17)
				sbDynamics->BQ_1.b0 = ((double)data[i-1] / (double)8388608);
			if(i == 5+index_subband*17)
				sbDynamics->BQ_1.a2 = ((double)data[i-1] / (double)-8388608);
			if(i == 6+index_subband*17)
				sbDynamics->BQ_1.a1 = ((double)data[i-1] / (double)-8388608);
		} else if(i == 7+index_subband*17) {
			sbDynamics->BQ_2.scale = data[i-1];
		} else if(i >= 8+index_subband*17 && i <= 12+index_subband*17) {
			if(i == 8+index_subband*17)
				sbDynamics->BQ_2.b2 = ((double)data[i-1] / (double)8388608);
			if(i == 9+index_subband*17)
				sbDynamics->BQ_2.b1 = ((double)data[i-1] / (double)8388608);
			if(i == 10+index_subband*17)
				sbDynamics->BQ_2.b0 = ((double)data[i-1] / (double)8388608);
			if(i == 11+index_subband*17)
				sbDynamics->BQ_2.a2 = ((double)data[i-1] / (double)-8388608);
			if(i == 12+index_subband*17)
				sbDynamics->BQ_2.a1 = ((double)data[i-1] / (double)-8388608);
		} else if(i == 13+index_subband*17) {
			sbDynamics->Gains.subBandGain = ((double)data[i-1] / (double)65536);
		} else if(i == 14+index_subband*17) {
			sbDynamics->Gains.DRC1gain = ((double)data[i-1] / (double)65536);
		} else if(i == 15+index_subband*17) {
			sbDynamics->Gains.DRC1envelopeDb = ((double)data[i-1] / (double)65536);
		} else if(i == 16+index_subband*17) {
			sbDynamics->Gains.DRC2gain = ((double)data[i-1] / (double)65536);
		} else if(i == 17+index_subband*17) {
			sbDynamics->Gains.DRC2envelope = ((double)data[i-1] / (double)65536);
			break;
		} 
	}
}

/* Count the number of bits to get the number of subbands */
int bitCount_subbands(int index_subband)
{
	unsigned int count = 0;
	/* until all bits are zero */
	while (index_subband > 0) {
		/* check lower bit */
		if ((index_subband & 1) == 1) {
			count++;
		}
		/* shift bits, removing lower bit */
		index_subband >>= 1;              
	}
	return count;
}
