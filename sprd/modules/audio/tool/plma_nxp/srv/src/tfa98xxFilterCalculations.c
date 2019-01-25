/**
 * tfa98xxCalculations.c
 *
 * Convert the filter input parameters into coeff
 */

#include <math.h>
#include <stdio.h> /* only needed for FILENAME_MAX */
#include <stdlib.h>

#include "tfa98xx_parameters.h"
#include "tfaContainerWrite.h"
#include "tfa_container.h"
#include "dbgprint.h"

#ifndef PI
#define PI 3.141592653589793f
#endif

extern int tfa98xx_cnt_write_verbose;

/*
* Integrator filter definitions
*/
#define INTEGRATOR_LPF   0
#define INTEGRATOR_HPF   1

// local definition which is not worth to put in the header file
typedef struct coeffs_float {
	float a2;
	float a1;	
	float b2;	
	float b1;	
	float b0;	
}coeffs_float_t;

static void print_bq(nxpTfaBiquad_t *bq, int tfa98xx_family_type)
{
	if(tfa98xx_family_type == 2) {
		PRINT ("\tb2=%d,b1=%d,b0=%d,-a2=%d,-a1=%d\n",bq->a2, bq->a1, bq->b2, -bq->b1, -bq->b0);
		PRINT ("\tb2=0x%06x,b1=0x%06x,b0=0x%06x,a2=0x%06x,a1=0x%06x\n", abs(bq->a2), abs(bq->a1), abs(bq->b2), abs(bq->b1), abs(bq->b0));
	} else {
		PRINT ("\t-a2=%d,-a1=%d,b2=%d,b1=%d,b0=%d\n", -bq->a2, -bq->a1, bq->b2, bq->b1, bq->b0);
		PRINT ("\ta2=0x%06x,a1=0x%06x,b2=0x%06x,b1=0x%06x,b0=0x%06x\n", abs(bq->a2), abs(bq->a1), abs(bq->b2), abs(bq->b1), abs(bq->b0));
	}
}

// get the scaled output coeffs
void translate_coeffs_to_fixed_point (nxpTfaBiquad_t *coeffsOut, coeffs_float_t *coeffsIn, int scale, int tfa98xx_family_type)
{
  if(tfa98xx_family_type == 2) {
	/* For the 88 device this needs to be reordered */
	coeffsOut->a2 = (int)( coeffsIn->b2 * pow(2.0f, 23-scale));
	coeffsOut->a1 = (int)( coeffsIn->b1 * pow(2.0f, 23-scale));
	coeffsOut->b2 = (int)( coeffsIn->b0 * pow(2.0f, 23-scale));	
	coeffsOut->b1 = (int)( -coeffsIn->a2 * pow(2.0f, 23-scale));
	coeffsOut->b0 = (int)( -coeffsIn->a1 * pow(2.0f, 23-scale));
  } else {
	coeffsOut->a1 = (int)(-coeffsIn->a1 * pow(2.0f, 23-scale));
	coeffsOut->a2 = (int)(-coeffsIn->a2 * pow(2.0f, 23-scale));
	coeffsOut->b0 = (int)( coeffsIn->b0 * pow(2.0f, 23-scale));
	coeffsOut->b1 = (int)( coeffsIn->b1 * pow(2.0f, 23-scale));
	coeffsOut->b2 = (int)( coeffsIn->b2 * pow(2.0f, 23-scale));
  }
}



/************************************************************************/
/**
 *  Function filter_ls_compensation
 *
 *        Computes Loudspeaker Bandwidth Extension and/or Resonant compensation filter. 
 * 
 *  @param   [in/out]     self:     Pointer to the filter structure with the input parameters & output coefficients
 *
 */
/************************************************************************/
void filter_ls_compensation (nxpTfaLsCompensationFilter_t *self)
{
    coeffs_float_t coefsFloat;
    float fBwExt = self->fBwExt;   // resonance compesation + bandwidth extension

    if (self->bwExtOn == 0){
      // only resonance compensation
      fBwExt = self->fRes;
    }

    {
      float a0 = 0.0f;
      float K = self->fRes/fBwExt;                    // DC gain
      float Qz = 1.0f/self->Qt;                       // Q-factor for the zeros
      float Qp = (float)(2.0f/sqrt(2.0f));            // Q-factor for the poles
      float w0 = PI * fBwExt/self->samplingFreq;      // Cut-off Frequency for poles (Zeros are indirectly set via K)

      float sn = (float)sin(w0);    
      float cs = (float)cos(w0);    

      a0 = (float)(pow(cs, 2.0f) + sn*cs*Qp + pow(sn, 2.0f));

      coefsFloat.a1 = (float)((2.0f * (pow(sn, 2.0f) - pow(cs, 2.0f)))/a0);
      coefsFloat.a2 = (float)((pow(cs, 2.0f) - sn*cs*Qp + pow(sn, 2.0f))/a0);

      coefsFloat.b0 = (float)((pow(cs, 2.0f) + K*sn*cs*Qz + pow(K*sn, 2.0f))/a0);
      coefsFloat.b1 = (float)((2 * (pow(K*sn, 2.0f) - pow(cs, 2.0f)))/a0);
      coefsFloat.b2 = (float)((pow(cs, 2.0f) - K*sn*cs*Qz + pow(K*sn, 2.0f))/a0);

      // get the scaled output coeffs
      translate_coeffs_to_fixed_point(&self->biquad, &coefsFloat, 3, 1);
    }
}



/************************************************************************/
/**
 *  Function compute_1stOrder_Butterworth
 *
 *           Computes the 1st order High / Low Pass filter coeficients.
 *
 *
 *  @param   [in]       type        : filter type (to differenciate LPF and HPF)
 *  @param   [in]       fc          : cut off frequency
 *  @param   [in]       fs          : sampling frequency
 *  @param   [out]      *coefsOut   : Output computed coeffs in the order [a1, b0, b1]. a0 = 1.0, thus not transfered
 *
 */
/************************************************************************/
static void compute_1stOrder_Butterworth( int type, float fc, float fs, float *coefsOut)
{
    float  w = 2.0f * PI * fc / fs;
    float  K = (float)tan( w / 2.0f );
    float  alpha = 1.0f + K;

    coefsOut[0] = ( K - 1.0f ) / alpha; // a1

    if (type == INTEGRATOR_LPF){
      coefsOut[1] = K / alpha;      // b0
      coefsOut[2] =  K / alpha;     // b1
    }else{
      // implicit HPF
      coefsOut[1] = 1.0f / alpha;   // b0
      coefsOut[2] = -1.0f / alpha;  // b1
    }
}



/************************************************************************/
/**
 *  Function filter_coeff_integrator
 *
 *          Performs the convolution between a 1st order Butterworth filter and a 1st order integrator
 *          The resulting filter is a biquad with fixed point coefficients scaled / right shifted by 1
 * 
 *  @param   [in/out]     in_param:     Pointer to the filter structure with the input parameters & output coefficients
 *
 */
/************************************************************************/
void filter_coeff_integrator(nxpTfaIntegratorFilter_t *self, int tfa98xx_family_type)
{
  float coefsBw[3];   // [a1, b0, b1]
  coeffs_float_t coefsConv;
  int   scale = 1;    // hardcoded value as the scaling is fixed on the DSP for this particular biquad filter

  compute_1stOrder_Butterworth( self->type, self->cutOffFreq, self->samplingFreq, coefsBw );

  // Perform convolution with the first order integrator
  coefsConv.a1 = (coefsBw[0] - self->leakage) / scale;  // a1
  coefsConv.a2 = -coefsBw[0] * self->leakage  / scale;  // a2

  coefsConv.b0 = coefsBw[1] / scale; // b0
  coefsConv.b1 = coefsBw[2] / scale; // b1 
  coefsConv.b2 = 0.0;                // b2
  
  // get the scaled output coeffs
  translate_coeffs_to_fixed_point(&self->biquad, &coefsConv, scale, tfa98xx_family_type);

  if(tfa98xx_cnt_write_verbose) {
	  PRINT("filter[%d]=%s,%.0f,%.2f,%.0f\n", 13, tfa_cont_eqtype_name(self->type),
			self->cutOffFreq, self->leakage, self->samplingFreq);
	  print_bq(&self->biquad, tfa98xx_family_type);
  }
}



//------------------------------------------------------------------------------



// EQ General processing functions
static void filter_adjust_gain(float peak_gain, float *b0, float *b1, float *b2)
{
    float v = (float)pow(10.0f, peak_gain / 20.0f);

    *b0 = *b0 * v;
    *b1 = *b1 * v;
    *b2 = *b2 * v;
}

static void filter_calc_intermediate(float fc, float fs, float q_factor, float *k_out, float *q_out)
{
    *k_out = (float)tan(PI * fc / fs);

    *q_out = q_factor;

    if (q_factor < 0.001f){
        *q_out = 0.001f;
    }
}
    

// EQ filter computations
static void flat(float peak_gain, coeffs_float_t *out)
{
    out->b0 = 1.0f;
    out->b1 = 0.0f;
    out->b2 = 0.0f;
    out->a1 = 0.0f;
    out->a2 = 0.0f;

    filter_adjust_gain(peak_gain, &out->b0, &out->b1, &out->b2);
}


static void fst_lowpass(float peak_gain, float k, coeffs_float_t *out)
{
    float norm = 1.0f / (1.0f + k);

    out->b0 = k * norm;
    out->b1 = out->b0;
    out->a1 = norm * (k - 1.0f);
    out->b2 = out->a2 = 0.0f;

    filter_adjust_gain(peak_gain, &out->b0, &out->b1, &out->b2);
}


static void fst_highpass(float peak_gain, float k, coeffs_float_t *out)
{
    float norm = 1.0f / (1.0f + k);

    out->b0 = norm;
    out->b1 = -1.0f * out->b0;
    out->a1 = norm * (k - 1.0f);
    out->b2 = out->a2 = 0.0f;

    filter_adjust_gain(peak_gain, &out->b0, &out->b1, &out->b2);
}


static void lowpass(float peak_gain, float k, float q, coeffs_float_t *out)
{
    float norm = 1.0f / (1.0f + k / q + k * k);

    out->b0 = k * k * norm;
    out->b1 = 2.0f * out->b0;
    out->b2 = out->b0;
    out->a1 = 2.0f * (k * k - 1.0f) * norm;
    out->a2 = (1.0f - k / q + k * k) * norm;

    filter_adjust_gain(peak_gain, &out->b0, &out->b1, &out->b2);
}


static void highpass(float peak_gain, float k, float q, coeffs_float_t *out)
{
    float norm = 1.0f / (1.0f + k / q + k * k);

    out->b0 = norm;
    out->b1 = -2.0f * out->b0;
    out->b2 = out->b0;
    out->a1 = 2.0f * (k * k - 1.0f) * norm;
    out->a2 = (1.0f - k / q + k * k) * norm;

    filter_adjust_gain(peak_gain, &out->b0, &out->b1, &out->b2);
}


static void bandpass(float peak_gain, float k, float q, coeffs_float_t *out)
{
    float norm = 1.0f / (1.0f + k / q + k * k);

    out->b0 = k / q * norm;
    out->b1 = 0.0f;
    out->b2 = -1.0f * out->b0;
    out->a1 = 2.0f * (k * k - 1.0f) * norm;
    out->a2 = (1.0f - k / q + k * k) * norm;
    
    filter_adjust_gain(peak_gain, &out->b0, &out->b1, &out->b2);
}


// peak_gain is applicable to the untouched by the notch part of the spectrum
static void notch(float peak_gain, float fc, float fs, float q, coeffs_float_t *out)
{
    float omega = 2.0f * PI * fc/fs;
    float sn = (float)sin(omega);
    float cs = (float)cos(omega);
    float alpha = sn/(2.0f*q);
    float a0 = 1.0f + alpha;

    out->a1 = -2.0f * cs / a0;
    out->a2 = (1.0f - alpha) / a0;

    out->b0 = 1.0f / a0;
    out->b1 = -2.0f * cs / a0;
    out->b2 = 1.0f / a0;

    filter_adjust_gain(peak_gain, &out->b0, &out->b1, &out->b2);
}

// peak_gain is applicable to the peak level. The remaining spectrum is unbiased
static void peak(float peak_gain, float fc, float fs, float q, coeffs_float_t *out)
{
    float pow2q = (float)pow(q, 2.0f);
    float BW = (float)log((2.0f*pow2q + 1.0f)/(2.0f*pow2q) + sqrt(pow((2.0f*pow2q + 1.0f)/pow2q, 2.0f)/4.0f - 1.0f));

    float A = (float)pow(10.0f, peak_gain / 40.0f);
    float omega = 2.0f * PI * fc/fs;
    float sn = (float)sin(omega);
    float cs = (float)cos(omega);

    float alpha = sn * (float)sinh(1.0f / 2.0f * BW * omega / sn);
    float a0 = 1.0f + (alpha / A);

    out->a1 = -2.0f * cs / a0;
    out->a2 = (1.0f - (alpha / A)) / a0;

    out->b0 = (1.0f + (alpha * A)) / a0;
    out->b1 = -2.0f * cs / a0;
    out->b2 = (1.0f - (alpha * A)) / a0;
}

static void low_shelf(float peak_gain, float fc, float fs, float q, coeffs_float_t *out)
{
    float A = (float)pow(10.0f, peak_gain / 40.0f);
    float omega = 2.0f * PI * fc/fs;
    float sn = (float)sin(omega);
    float cs = (float)cos(omega);
    float alpha = sn/2.0f*(float)sqrt((A + 1.0f/A)*(1.0f/q - 1.0f) + 2.0f);
    float beta = 2.0f * (float)sqrt(A) * alpha;
    float a0 = (A + 1.0f) + (A - 1.0f) * cs + beta;

    out->b0 = (A * ((A + 1.0f) - (A - 1.0f) * cs + beta)) / a0;
    out->b1 = 2 * A * ((A - 1.0f) - (A + 1.0f) * cs) / a0;
    out->b2 = A * ((A + 1.0f) - (A - 1.0f) * cs - beta) / a0;
    
    out->a1 = -2 * ((A - 1.0f) + (A + 1.0f) * cs) / a0;
    out->a2 = ((A + 1.0f) + (A - 1.0f) * cs - beta) / a0;

}


static void high_shelf(float peak_gain, float fc, float fs, float q, coeffs_float_t *out)
{
    float A = (float)pow(10.0f, peak_gain / 40.0f);
    float omega = 2.0f * PI * fc/fs;
    float sn = (float)sin(omega);
    float cs = (float)cos(omega);
    float alpha = sn/2.0f * (float)sqrt((A + 1.0f/A)*(1.0f/q - 1.0f) + 2.0f);
    float beta = 2.0f * (float)sqrt(A) * alpha;
    float a0 = (A + 1.0f) - (A - 1.0f) * cs + beta;

    out->b0 = (A * ((A + 1.0f) + (A - 1.0f) * cs + beta)) / a0;
    out->b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cs)) / a0;
    out->b2 = (A * ((A + 1.0f) + (A - 1.0f) * cs - beta)) / a0;

    out->a1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cs)) / a0;
    out->a2 = ((A + 1.0f) - (A - 1.0f) * cs - beta) / a0;

}



/************************************************************************/
/**
 *  Function filter_coeff_equalizer
 *
 *          Computes the Eq type filter coefficients
 * 
 *  @param   [in/out]     in_param:     Pointer to the filter structure with the input parameters & output coefficients
 *
 */
/************************************************************************/
void filter_coeff_equalizer(nxpTfaEqFilter_t *self, int tfa98xx_family_type)
{
  coeffs_float_t  coeffs;  // [a2, a1, b2, b1, b0]
  float k = 0.0;
  float q = 0.0;

  filter_calc_intermediate(self->cutOffFreq, self->samplingFreq, self->Q, &k, &q);

  switch (self->type)
  {
    case fFlat:
      flat(self->gainDb, &coeffs); break;

    case fLowpass:
      lowpass(self->gainDb, k, q, &coeffs); break;

    case fHighpass:
      highpass(self->gainDb, k, q, &coeffs); break;

    case fBandpass:
      bandpass(self->gainDb, k, q, &coeffs); break;

    case fNotch:
      notch(self->gainDb, self->cutOffFreq, self->samplingFreq, q, &coeffs); break;

    case fPeak:
      peak(self->gainDb, self->cutOffFreq, self->samplingFreq, q, &coeffs); break;

    case fLowshelf:
      low_shelf(self->gainDb, self->cutOffFreq, self->samplingFreq, q, &coeffs); break;

    case fHighshelf:
      high_shelf(self->gainDb, self->cutOffFreq, self->samplingFreq, q, &coeffs); break;

    case f1stLP:
      fst_lowpass(self->gainDb, k, &coeffs); break;

    case f1stHP:
      fst_highpass(self->gainDb, k, &coeffs); break;

    default:
    	PRINT_ERROR("unsupported eq filter type:%d\n", self->type);
      // handle somehow the exception for the non valid filter type here
      return;
  }

  // check that the coefficients can fit within the range [-2.0, 1.99) which correspond to the imposed scaling factor = 1;
  // if not decrease the gainDb in steps of 20% from the current value until the coefficients match the allowed range
  if ((coeffs.a1 > 1.99) || (coeffs.a1 < -2.0)
   || (coeffs.a2 > 1.99) || (coeffs.a2 < -2.0)
   || (coeffs.b0 > 1.99) || (coeffs.b0 < -2.0)
   || (coeffs.b1 > 1.99) || (coeffs.b1 < -2.0)
   || (coeffs.b2 > 1.99) || (coeffs.b2 < -2.0))
  {
    self->gainDb -= 0.5f; // adjust the gain and recompute the coefficients in a recursive matter
    // care should be taken so that the gain is not excessively large and the depth of the recursive nr of calls would not overflow the stack
    filter_coeff_equalizer(self, tfa98xx_family_type);
    /* say we're recursive to avoid confusion in verbose print */
    if(tfa98xx_cnt_write_verbose)
		  PRINT("..recursive call: ");
  }
  else
  {
    // get the fixed point coefficients, given that they match within the allowed range
    translate_coeffs_to_fixed_point(&self->biquad, &coeffs, 1, tfa98xx_family_type); // hardcoded to 1 scaling factor as this is fixed on the DSP for these particular biquad filters
  }

  if(tfa98xx_cnt_write_verbose) {
	  PRINT("filter[10..12]=%s,%.0f,%.2f,%.1f,%.0f\n", tfa_cont_eqtype_name(self->type),
			self->cutOffFreq, self->Q, self->gainDb, self->samplingFreq);
	  print_bq(&self->biquad, tfa98xx_family_type);
  }

}




//------------------------------------------------------------------------------
// Eliptic Filter 2nd order computation

#define ELLIPTIC_FILTER_ORDER           2   // not working for higher order, although it could with some small modifications
#define ELLIPTIC_FILTER_ORDER_DIV2     (ELLIPTIC_FILTER_ORDER/2)
#define ELLIPTIC_FILTER_ORDER_MOD2     (ELLIPTIC_FILTER_ORDER%2)
#define PI_DOUBLE	 3.1415926535897932384626433832795028841971


typedef struct cmpx {
  double real;
  double imag;
} cmpx_t; 


// Complete elliptic integral of the first kind.
static double ellipticK (double k)
{
  double m = k*k;
  double a = 1;
  double b = sqrt (1 - m);
  double c = a - b;
  double co;
  double ao;

  do
  {
    co = c;
    c = (a - b) / 2;
    ao = (a + b) / 2;
    b = sqrt (a*b);
    a = ao;
  }
  while (c<co);
 
  return PI_DOUBLE / (a + a);
}

// generate the product of (z+s1[i]) for i = 1 .. sn and store it in b1[]
// (i.e. f[z] = b1[0] + b1[1] z + b1[2] z^2 + ... b1[sn] z^sn)
static void prodpoly( int sn, double *s1, double *a1, double *b1 )
{
  int i, j;

  b1[0] = s1[1];
  b1[1] = 1;

  for (j = 2; j <= sn; j++)
  {
    a1[0] = s1[j]*b1[0];
    for (i = 1; i <= j-1; i++)
      a1[i] = b1[i-1]+s1[j]*b1[i];
    for (i = 0; i != j; i++)
      b1[i] = a1[i];
    b1[j] = 1;
  }
}

// determine f(z)^2
static void calcfz2( int i, double *c1, double *a1 )
{
  int ji = 0;
  int jf = 0;
  int j = 0;

  if (i < 2*((ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)/2)+2)
  {
    ji = 0;
    jf = i;
  }
  if (i > 2*((ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)/2))
  {
    ji = i-2*((ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)/2);
    jf = 2*((ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)/2);
  }
  c1[i] = 0.0;

  for(j = ji; j <= jf; j += 2){
    c1[i] += a1[j]*(a1[i-j]*pow(10., (ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)-i/2));
  }
}

// calculate f(z)
static void calcfz( double e, double *s1, double *z1, double *c1, double *a1, double *b1 )
{
  int i = 1;

#if ELLIPTIC_FILTER_ORDER_MOD2 == 1
  s1[i++] = 1.0;
#endif

  for (; i <= ELLIPTIC_FILTER_ORDER_MOD2+ELLIPTIC_FILTER_ORDER_DIV2; i++){
    s1[i] = s1[i+ELLIPTIC_FILTER_ORDER_DIV2] = z1[i-ELLIPTIC_FILTER_ORDER_MOD2];
  }

  prodpoly(ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2, s1, a1, b1);

  for (i = 0; i <= 2*((ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)/2); i += 2){
    a1[i] = e*b1[i];
  }

  for (i = 0; i <= 2*2*((ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)/2); i += 2){
    calcfz2(i, c1, a1);
  }
}


// determine q(z)
static void calcqz( double *s1, double *z1, double *d1, double *a1, double *b1 )
{
  int i;
  int dd;

  // DEADCODE as ELLIPTIC_FILTER_ORDER_MOD2 == 0:
  for (i = 1; i <= ELLIPTIC_FILTER_ORDER_MOD2; i++)
    s1[i] = -10.0;

  for (; i <= ELLIPTIC_FILTER_ORDER_MOD2+ELLIPTIC_FILTER_ORDER_DIV2; i++)
    s1[i] = -10*z1[i-ELLIPTIC_FILTER_ORDER_MOD2]*z1[i-ELLIPTIC_FILTER_ORDER_MOD2];

  for (; i <= ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2; i++)
    s1[i] = s1[i-ELLIPTIC_FILTER_ORDER_DIV2];

  prodpoly(ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2, s1, a1, b1);

  dd = ((ELLIPTIC_FILTER_ORDER_MOD2 & 1) == 1) ? -1 : 1;

  for (i = 0; i <= 2*(ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2); i += 2)
    d1[i] = dd*b1[i/2];
}


// compute factors
static double findfact(int t, double *p1, double *q1, double *c1, double *a1, double *b1)
{
  int i;
  int i1 = 0;
  double a = 0;

  for (i = 1; i <= t; i++){
    a1[i] /= a1[0];
  }

  a1[0] = b1[0] = c1[0] = 1.0;

  for(;;)
  {
    double p0 = 0, q0 = 0;

    if (t <= 2){
      break;
    }
    
    i1++;
    for(;;)
    {
      int x1, x2, x3;
      double x4, ddp, dq;

      b1[1] = a1[1] - p0;
      c1[1] = b1[1] - p0;

      for (i = 2; i <= t; i++){
        b1[i] = a1[i]-p0*b1[i-1]-q0*b1[i-2];
      }

      for (i = 2; i < t; i++){
        c1[i] = b1[i]-p0*c1[i-1]-q0*c1[i-2];
      }

      x1 = t-1;
      x2 = t-2;
      x3 = t-3;
      x4 = c1[x2]*c1[x2]+c1[x3]*(b1[x1]-c1[x1]);

      if (x4 == 0){
        x4 = 1e-3;
      }

      ddp = (b1[x1]*c1[x2]-b1[t]*c1[x3])/x4;
      p0 += ddp;

      dq = (b1[t]*c1[x2]-b1[x1]*(c1[x1]-b1[x1]))/x4;
      q0 += dq;

      if (fabs(ddp+dq) < 1e-6){
        break;
      }
    }

    p1[i1] = p0;
    q1[i1] = q0;
    a1[1] = a1[1]-p0;
    t -= 2;

    for (i = 2; i <= t; i++){
      a1[i] -= p0*a1[i-1]+q0*a1[i-2];
    }

    if (t <= 2){
      break;
    }
  }

  if (t == 2)
  {
    i1++;
    p1[i1] = a1[1];
    q1[i1] = a1[2];
  }

  if (t == 1){
    a = -a1[1];
  }

  return a;
}

static double calcsn(double u, double K, double Kprime)
{
  double sn = 0;
  int j;
  // q = modular constant
  double q = exp(-PI_DOUBLE*Kprime/K);
  double v = PI_DOUBLE*.5*u/K;

  for (j = 0; ; j++)
  {
    double w = pow(q, j+.5);
    sn += w*sin((2*j+1)*v)/(1-w*w);
    if (w < 1e-7){
      break;
    }
  }
  return sn;
}


static int LowPassBilinearTransform (double fc, cmpx_t *digital, cmpx_t *analog)
{
  cmpx_t temp0, temp1;

  // frequency transform
  temp0.real = fc * analog->real; 
  temp0.imag = fc * analog->imag;
  
  temp1 = temp0;

  // bilinear low pass transform
  // (1.0 + c) / (1.0 - c);
  temp0.real = temp0.real + 1.0;
  temp1.real = 1.0 - temp1.real;
  temp1.imag = - temp1.imag;
  
  // compute temp0/temp1
  {
    double ratio = temp1.real * temp1.real + temp1.imag * temp1.imag;

    if (ratio == 0.0){
      // impossible to compute the division
      return 0;
    }else{
      digital->real = (temp0.real * temp1.real + temp0.imag * temp1.imag)/ratio;
      digital->imag = (temp0.imag * temp1.real - temp0.real * temp1.imag)/ratio;
      return 1;
    }
  }
}

static int LowPassFilterTransform ( double fc,     // normalized cut off frequency: cutOffFreq/samplingFreq
                             cmpx_t *poleDigital,                            
                             cmpx_t *zeroDigital,
                             cmpx_t *poleAnalog,
                             cmpx_t *zeroAnalog,
                             int nrPoles)
{
  // prewarp
  int i, ret0, ret1;
  double f = tan (PI_DOUBLE * fc);

  const int pairs = nrPoles / 2;

  for (i = 0; i < pairs; ++i)
  {
    ret0 = LowPassBilinearTransform (f, &poleDigital[2*i], &poleAnalog[2*i]);
    ret1 = LowPassBilinearTransform (f, &zeroDigital[2*i], &zeroAnalog[2*i]);

    if ((ret0 == 0) || (ret1 == 0)){
      // impossible to compute the bilinear transform for the given analog poles
      return 0;
    }

    // complex conjugate
    poleDigital[2*i + 1] = poleDigital[2*i];
    poleDigital[2*i + 1].imag = -poleDigital[2*i + 1].imag;

    zeroDigital[2*i + 1] = zeroDigital[2*i];
    zeroDigital[2*i + 1].imag = -zeroDigital[2*i + 1].imag;
  }

  if (nrPoles & 1)
  {
    ret0 = LowPassBilinearTransform (fc, &poleDigital[2*pairs], &poleAnalog[2*pairs]);
    ret1 = LowPassBilinearTransform (fc, &zeroDigital[2*pairs], &zeroAnalog[2*pairs]);

    if ((ret0 == 0) || (ret1 == 0)){
      // impossible to compute the bilinear transform for the given analog poles
      return 0;
    }
  }

  return 1;
}


static void getCoeffs2ndOrder(cmpx_t *poleDigital,                            
                        cmpx_t *zeroDigital,
                        coeffs_float_t *coeffs)
{
    if (poleDigital[0].imag != 0)
    {
      coeffs->a1 = (float)(-2 * poleDigital[0].real);
      coeffs->a2 = (float)(poleDigital[1].real * poleDigital[1].real + poleDigital[1].imag * poleDigital[1].imag);
    }
    else
    {
      coeffs->a1 = (float)(-(poleDigital[0].real + poleDigital[1].real));
      coeffs->a2 = (float)(  poleDigital[0].real * poleDigital[1].real);
    }

    coeffs->b0 = 1.0f;
    coeffs->b1 = (float)(-2 * zeroDigital[1].real);
    coeffs->b2 = (float)(zeroDigital[1].real * zeroDigital[1].real + zeroDigital[1].imag * zeroDigital[1].imag);
}


static void normalizeImpulseResponse (double normalGain, coeffs_float_t  *coeffs)
{
    float scale;
    float ch = coeffs->b0  + coeffs->b1 + coeffs->b2;
    float cbot = 1.0f + coeffs->a1  + coeffs->a2;

    scale = (float)(normalGain / fabs(ch / cbot));

    coeffs->b0 *= scale;
    coeffs->b1 *= scale;
    coeffs->b2 *= scale;
}


/************************************************************************/
/**
 *  Function filter_coeff_anti_alias
 *
 *          Computes the Elliptic Low Pass second order filter coefficients
 *          Doesn't support higher order filter topologies, although it could with some small modifications
 * 
 *  @param   [in/out]     in_param:     Pointer to the filter structure with the input parameters & output coefficients
 *
 */
/************************************************************************/
void filter_coeff_anti_alias(nxpTfaAntiAliasFilter_t *self, int tfa98xx_family_type)
{
    // only one filter topology is supported here: Eliptic 2nd order Low Pass Filter
    int i = 0;
    int r = 0;
    const int n = ELLIPTIC_FILTER_ORDER;  // Number of Poles = order of the filter
    double f[ELLIPTIC_FILTER_ORDER];
    double s1[2*ELLIPTIC_FILTER_ORDER];
    double z1[2*ELLIPTIC_FILTER_ORDER];
    double p1[4*(ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)];
    double q1[4*(ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)];
    double a1[4*(ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)];
    double b1[4*(ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)];
    double c1[4*(ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)];
    double d1[4*(ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)];
    cmpx_t poleAnalog[ELLIPTIC_FILTER_ORDER];
    cmpx_t zeroAnalog[ELLIPTIC_FILTER_ORDER];
    cmpx_t poleDigital[ELLIPTIC_FILTER_ORDER];
    cmpx_t zeroDigital[ELLIPTIC_FILTER_ORDER];
    coeffs_float_t coeffs;

    double e2=pow(10.0, self->rippleDb/10.0)-1.0;
    double xi = 5.0 * exp (self->rolloff - 1.0) + 1.0;
    int ni = ((ELLIPTIC_FILTER_ORDER & 1) == 1) ? 0 : 1;
    
    double K = ellipticK(1.0/xi);
    double Kprime = ellipticK(sqrt(1.0-1.0/(xi*xi)));
    double fb = 1/(2*PI_DOUBLE);

    for (i = 1; i <= n/2; i++)
    {
      double u = (2*i-ni)*K/n;
      double sn = calcsn(u, K, Kprime);
      sn *= 2*PI_DOUBLE/K;
      f[i] = 1/sn;
    }

    for (i = 1; i <= ELLIPTIC_FILTER_ORDER_DIV2; i++)
    {
      double x = f[ELLIPTIC_FILTER_ORDER_DIV2+1-i];
      z1[i] = sqrt(1-1/(x*x));
    }

    calcfz(sqrt(e2), s1, z1, c1, a1, b1);
    calcqz(s1, z1, d1, a1, b1);

#if (ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2) > 2*((ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)/2)
      c1[2*(ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)] = 0.0;
#endif

    for (i = 0; i <= 2*(ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2); i += 2){
      a1[ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2-i/2] = c1[i] + d1[i];
    }

    if (findfact(ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2, p1, q1, c1, a1, b1) != 0.0)
    {
      // could not design the filter as the eliptic integral didn't converge
      // throw if possible a warning / exception here and return
      return;
    }

    while (r < ((ELLIPTIC_FILTER_ORDER_MOD2+2*ELLIPTIC_FILTER_ORDER_DIV2)/2))
    {
      r++;
      p1[r] /= 10.0;
      q1[r] /= 100.0;

      b1[r] = (1+p1[r]/2.0)*fb*fb/(1.0+p1[r]+q1[r]);
      q1[r] = fb/pow(1.0+p1[r]+q1[r], 0.25);
      p1[r] = 1/sqrt(fabs(2.0*(1-b1[r]/(q1[r]*q1[r]))));
      q1[r] = 2*PI_DOUBLE*q1[r];

      poleAnalog[r-1].real = -0.5*q1[r]/p1[r];
      poleAnalog[r-1].imag = 0.5*sqrt(fabs(q1[r]*q1[r]/(p1[r]*p1[r]) - 4.0*q1[r]*q1[r]));

      // complex conjugate of poleAnalog[r-1]
      poleAnalog[r].real = -0.5*q1[r]/p1[r];
      poleAnalog[r].imag = -(0.5*sqrt(fabs(q1[r]*q1[r]/(p1[r]*p1[r]) - 4.0*q1[r]*q1[r])));

      zeroAnalog[r-1].real = 0.0;
      zeroAnalog[r-1].imag = f[r];

      // complex conjugate of xero[r-1]
      zeroAnalog[r].real = 0.0;
      zeroAnalog[r].imag = -f[r];
    }

    if (LowPassFilterTransform (self->cutOffFreq/self->samplingFreq,
                                poleDigital, zeroDigital,
                                poleAnalog, zeroAnalog, n) == 0)
    {
      // could not design the filter as the poles are 0
      // throw if possible a warning / exception here and return
      return;
    }

#if ELLIPTIC_FILTER_ORDER == 2 
      getCoeffs2ndOrder(poleDigital, zeroDigital, &coeffs);
#elif ELLIPTIC_FILTER_ORDER == 1
      getCoeffs1stOrder(poleDigital, zeroDigital, &coeffs);
#endif

    normalizeImpulseResponse((n&1) ? 1.0 : pow (10.0, -self->rippleDb / 20.0), &coeffs);

    // check that the coefficients can fit within the range [-2.0, 1.99) which correspond to the imposed scaling factor = 1;
    if ((coeffs.a1 > 1.99) || (coeffs.a1 < -2.0)
     || (coeffs.a2 > 1.99) || (coeffs.a2 < -2.0)
     || (coeffs.b0 > 1.99) || (coeffs.b0 < -2.0)
     || (coeffs.b1 > 1.99) || (coeffs.b1 < -2.0)
     || (coeffs.b2 > 1.99) || (coeffs.b2 < -2.0))
    {
      // could not design the filter as the coefficients are larger then the supported range
      // this is quite unlikely to happen given the common / normal parameters
      // throw if possible a warning / exception here and return
      return;
    }
    else
    {
      // get the fixed point coefficients, given that they match within the allowed range
      translate_coeffs_to_fixed_point(&self->biquad, &coeffs, 1, tfa98xx_family_type); // hardcoded to 1 scaling factor as this is fixed on the DSP for these particular biquad filters
    }

    if(tfa98xx_cnt_write_verbose) {
  	  PRINT("filter[10..12]=%s,%.0f,%.2f,%.1f,%.1f ;anti-alias low pass\n", "eclip",
  			self->cutOffFreq, self->rippleDb, self->rolloff, self->samplingFreq);
  	  print_bq(&self->biquad, tfa98xx_family_type);
    }
}

