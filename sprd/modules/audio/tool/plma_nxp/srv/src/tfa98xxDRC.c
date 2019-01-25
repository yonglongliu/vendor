//Only needed for cli
#include <stdio.h>
#if defined(WIN32) || defined(_X64)
#include <tchar.h>
#pragma warning(disable : 4204) /* Disable: warning C4204: nonstandard extension used : non-constant aggregate initializer */
#endif
#include <sys/stat.h>
#include <stdlib.h>
//===================

#include <memory.h>	//memcpy()
#include <math.h>	//log10()
#include "tfa98xxDRC.h"
#include "dbgprint.h"

#ifndef MIN
#define MIN(A,B) (A<B?A:B)
#endif

//Conversion from and to int24
int int24_to_int(int24_t src) {
	//byte[0] = msb
	int c = (src.byte[0]<< 16) | (src.byte[1] << 8) | src.byte[2];
	if ((src.byte[0] & 0x80) == 0x80 ) c = -((1 << 24) - c); // Negative two's comp.
	return c;
}
  
int24_t int_to_int24(int value) {
	int24_t ret;
	if (value >= 0) value = MIN(value, (1 << 23) - 1);
	ret.byte[2] = value & 0xFF; //lsb
	ret.byte[1] =( value >> 8) & 0xFF;
	ret.byte[0] = (value >> 16) & 0xFF;
	return ret;
}

//Some helper functions
float getQfactor(struct drcBiquad *bq) {
	return ((float) int24_to_int(bq->Q)) / SCALE_FACTOR(DSP_BQ_Q_EXP);
}

void setQfactor(struct drcBiquad *bq, float q) {
	bq->Q = int_to_int24((int) (0.5 + q * SCALE_FACTOR(DSP_BQ_Q_EXP)));
}

float getGain(struct drcBiquad *bq) {
	return ((float) int24_to_int(bq->gain)) / SCALE_FACTOR(DSP_BQ_K_EXP);
}

void setGain(struct drcBiquad *bq, float gain) {
	bq->gain = int_to_int24((int) (0.5 + gain * SCALE_FACTOR(DSP_BQ_K_EXP)));
}

int getFrequency(struct drcBiquad *bq) {
	return int24_to_int(bq->freq);
}

void setFrequency(struct drcBiquad *bq, int freq) {
	bq->freq = int_to_int24(freq);
}

enum dspBqFiltType getFltrType(struct drcBiquad *bq) {
	return (enum dspBqFiltType) int24_to_int(bq->type);
}

void setFltrType(struct drcBiquad *bq, enum dspBqFiltType type) {
	bq->type = int_to_int24(type);
}

float getThreshold(struct drc *drc) {
	return ((float) int24_to_int(drc->thresDb)) / SCALE_FACTOR(EXP2LIN_20LOG10_IN_EXP);
}

void setThreshold(struct drc *drc, float threshold) {
	drc->thresDb = int_to_int24((int) (0.5 + threshold * SCALE_FACTOR(EXP2LIN_20LOG10_IN_EXP)));
}

void saturateBiquad(struct drcBiquad *bq, int maxFreq) {
	setFrequency(bq, MIN(getFrequency(bq), maxFreq));
}

void addThresholdOffset(struct drc *drc, float offset) {
	setThreshold(drc, getThreshold(drc) + offset);
}





/************************************************************************
 * On input, parambuf contains generic DRC parameters. parambuf should be
 * twice the size of the DRC parameterblock. This function adds a copy of
 * the generic parameters in the second half of parambuf, with some 
 * adaptions:
 * Adds gain offset to threshold parameters. Gain offset is vlsCal.
 * Satuartes filter center frequencies to 80% of half the samplerate.
 * Ensures X-over is a 2nd order Linkwitz-Riley filter with a bandwidth 
 * of at least 1/3 octave (~ 80%).
 *
 * @parambuf with original generic drc parameter block from drc file on
 *    input, original plus adapted parameters on output
 * @sz is the length of parambuf and should be at least 
 *    2 * sizeof(drcParamBlock_t)
 * @vlsCal is the value of parameter vlsCal in the global parameter block
 * @sr is the current samplerate
 *
 * Returns 0 on succes, 
 *         1 if sz < 2 * sizeof(drcParamBlock_t).
 *         2 if vlsCal < 0.001
 *         3 if sr < 8000
  ************************************************************************/
int drc_generic_to_specific(char *parambuf,   
					 	 int sz,			
						 float vlsCal, 
						 int sr) 
{

	int maxFreq = 0, xoHi = 0, xoLo = 0;
	float offset = 0.0;
	drcParamBlock_t *params;

	//Check function arguments
	if ((unsigned int)sz < (2u * sizeof(drcParamBlock_t))) return 1;
	if (vlsCal < 0.001) return 2;
	if (sr < 8000) return 3;

	//Copy the generic parameters to the second half
	params = (drcParamBlock_t *)(parambuf + sizeof(drcParamBlock_t));
	memcpy(params, parambuf, sizeof(drcParamBlock_t));

	// max frequency filters can support safely
	maxFreq = (int) ( 0.4 * (float)sr); // 80% of Nyquits

	// Get the two x-over frequencies.
	// Assumption is that mi1 has the lo frequency and mi3 has the hi frequency
	// Could also be turned into function arguments
	// First saturate mi3
	saturateBiquad(&params->mi3bq, maxFreq);
	xoHi = getFrequency(&params->mi3bq);
	xoLo = MIN(getFrequency(&params->mi1bq), (int) ( 0.8 * xoHi));
	// xoHi is now max 80% of nyquits
	// xoLo is now max 80% of xoHi

	// Set the cross over filters. 
	// All butterworth LP2 or HP2, Q = sqrt2/2, gain = 0 
	// and freq = xoLo or xoHi
	setFrequency(&params->lo1bq, xoLo);
	setFrequency(&params->lo2bq, xoLo);
	setFrequency(&params->mi1bq, xoLo);
	setFrequency(&params->mi2bq, xoLo);
	setFrequency(&params->mi3bq, xoHi);		// allready set
	setFrequency(&params->mi4bq, xoHi);
	setFrequency(&params->hi1bq, xoHi);
	setFrequency(&params->hi2bq, xoHi);

	setQfactor(&params->lo1bq, SQRT2_2);
	setQfactor(&params->lo2bq, SQRT2_2);
	setQfactor(&params->mi1bq, SQRT2_2);
	setQfactor(&params->mi2bq, SQRT2_2);
	setQfactor(&params->mi3bq, SQRT2_2);
	setQfactor(&params->mi4bq, SQRT2_2);
	setQfactor(&params->hi1bq, SQRT2_2);
	setQfactor(&params->hi2bq, SQRT2_2);

	setGain(&params->lo1bq, 0);
	setGain(&params->lo2bq, 0);
	setGain(&params->mi1bq, 0);
	setGain(&params->mi2bq, 0);
	setGain(&params->mi3bq, 0);
	setGain(&params->mi4bq, 0);
	setGain(&params->hi1bq, 0);
	setGain(&params->hi2bq, 0);

	setFltrType(&params->lo1bq, secondOrderLP);
	setFltrType(&params->lo2bq, secondOrderLP);
	setFltrType(&params->mi1bq, secondOrderHP);
	setFltrType(&params->mi2bq, secondOrderHP);
	setFltrType(&params->mi3bq, secondOrderLP);
	setFltrType(&params->mi4bq, secondOrderLP);
	setFltrType(&params->hi1bq, secondOrderHP);
	setFltrType(&params->hi2bq, secondOrderHP);

	// Saturate the other filters
	saturateBiquad(&params->po1bq, maxFreq);
	saturateBiquad(&params->po2bq, maxFreq);
	saturateBiquad(&params->bl.biquad, maxFreq);

	// lo12, mi1234 and hi12 now form a Linkwitz-Riley 2nd order 3-band cross-over network
	// with xoLo and xoHi as the center frequencies
	// All frequencies are below 80% of the Nyquist frequency and 
	// the mid band is at least 1/3 of an octave wide.

	//Now adjust the threshold gains
	offset = 20 * (float)log10(vlsCal);
	addThresholdOffset(&params->lo1drc, offset);
	addThresholdOffset(&params->lo2drc, offset);
	addThresholdOffset(&params->mi1drc, offset);
	addThresholdOffset(&params->mi2drc, offset);
	addThresholdOffset(&params->hi1drc, offset);
	addThresholdOffset(&params->hi2drc, offset);
	addThresholdOffset(&params->po1drc, offset);
	addThresholdOffset(&params->po2drc, offset);
	addThresholdOffset(&params->bl.limiter, offset);
	return 0;
}

/************************************************************************
 * This function tests if any of the thresholds is larger than zero. If
 * so, all threshold values are lowered with 20*log10(vlsCal).
 * Values are changed 'in place'.
 *
 * @drc with original generic drc parameter block from drc file on
 *    input, original plus adapted parameters on output
 * @vlsCal is the value of parameter vlsCal in the global parameter block
 *
 * Returns 0 on succes (No change needed, or changes applied), 
 *         1 if drc == NULL.
 *         2 if vlsCal < 0.001
  ************************************************************************/
int drc_undo_threshold_compensation(drcParamBlock_t *drc, float vlsCal) {
	int i, cnt;
	float offset;
	struct drc *drcs[] = {&drc->bl.limiter, &drc->lo1drc, &drc->lo2drc, 
						&drc->mi1drc, &drc->mi2drc, 
						&drc->hi1drc, &drc->hi2drc, &drc->po1drc, &drc->po2drc};
	cnt = sizeof(drcs) / sizeof(struct drc *);

	//Check function arguments
	if (!drc) return 1;
	if (vlsCal < 0.001) return 2;
	
	/* Do these parameters have thresholds above 0? */
	for (i=0; i<cnt; i++) {
		if (getThreshold(drcs[i]) > 0) break;
	}
	if (i >= cnt) {
		/* No. All thresholds are smaller than or equal to zero. Nothing to do!  */
		return 0;
	}
	/* Yes, these parameters appear to have compensated thresholds. Let's substract vlsCal in dB  */
	offset = -20 * (float)log10(vlsCal);
	for (i=0; i<cnt; i++) {
		addThresholdOffset(drcs[i], offset);
	}
	return 0;
}

//prints filter info of all biquads in drc parameter block
void printFilters(char * hdr, struct drcParamBlock *drc) {
	struct drcBiquad *bqs[] = {&drc->bl.biquad, &drc->lo1bq, &drc->lo2bq, 
						&drc->mi1bq, &drc->mi2bq, &drc->mi3bq, &drc->mi4bq, 
						&drc->hi1bq, &drc->hi2bq, &drc->po1bq, &drc->po2bq};
	const char  *bqNames[] = { " BL", "lo1", "lo2", "mi1", "mi2", "mi3", 
							   "mi4", "hi1", "hi2", "po1", "po2" };
	const char *fltrTypes[] = { "Bypass", "1st order high pass", "1st order low pass", 
								"2nd order high pass", "2nd order low pass", 
								"High shelving", "Low shelving", "Band pass"};
	unsigned int i = 0;
	
	PRINT("%s\n", hdr);
	for (i = 0; i < sizeof(bqs) / sizeof(struct drcBiquad *); i++) {
		PRINT("%s: %5dHz Q=%8f gain=% 3fdB type=%s\n", bqNames[i], getFrequency(bqs[i]), getQfactor(bqs[i]), getGain(bqs[i]), fltrTypes[getFltrType(bqs[i])]);
	}

}

//prints threshold of all compressors in drc parameter block
void printThreshold(char * hdr, struct drcParamBlock *drc) {
	struct drc *drcs[] = {&drc->bl.limiter, &drc->lo1drc, &drc->lo2drc, 
						&drc->mi1drc, &drc->mi2drc, 
						&drc->hi1drc, &drc->hi2drc, &drc->po1drc, &drc->po2drc};
	const char  *drcNames[] = { " BL", "lo1", "lo2", "mi1", "mi2",  
							    "hi1", "hi2", "po1", "po2" };
	unsigned int i = 0;
	PRINT("%s\n", hdr);
	for (i = 0; i < sizeof(drcs) / sizeof(struct drc *); i++) {
		PRINT("%s threshold: % 3.1fdB\n", drcNames[i], getThreshold(drcs[i]));
	}

}
/**

int _tmain(int argc, _TCHAR* argv[]) {
    const char *fn = "C:\\temp\\test.drc";
	struct drcParamBlock drcIn, drcOut;
	FILE *in = fopen (fn, "rb");
	fread(&drcIn, sizeof(drcIn), 1, in);
	fclose(in);
	drcGenericToSpecific((const char *)&drcIn, (char *)&drcOut, sizeof(drcIn), 15.8, 8000);
	
	printFilters("\nOld filters", &drcIn);
	printFilters("New filters", &drcOut);
	printThreshold("\nOld thresholds", &drcIn);
	printThreshold("New thresholds", &drcOut);
	return 0;
}
**/
