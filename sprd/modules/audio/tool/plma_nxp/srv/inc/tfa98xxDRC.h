/*
 * tfa98xxDRC.h
 *
 *  Created on: Nov 11, 2013
 *      Author: EL and shankar
 */

#ifndef TFADRC_H_
#define TFADRC_H_

#ifdef __cplusplus
extern "C" {
#endif
	
typedef struct {
	unsigned char byte[3];
} int24_t;

struct drcBiquad {
	int24_t freq;		// center frequency
	int24_t Q;			// Q factor
	int24_t gain;		// gain in dB
	int24_t type;		// filter type (= enum dspBqFiltType_t)
};

struct drc {
	int24_t enabled;	// drc enabled
	int24_t sidechain;	// side chain usage
	int24_t kneetype;	// knee type enum
	int24_t env;		// envelope
	int24_t attack;		// attack time in ms
	int24_t release;	// release time in ms;
	int24_t thresDb;	// threshold in dB;
	int24_t ratio;
	int24_t makeupGain;	// make up gain in dB
};

struct drcBandLimited {
	int24_t enabled;			// band limted drc enabled (0 = off)
	struct drcBiquad biquad;	// biquad for band 
	struct drc limiter;			// band limiter
};		

typedef struct drcParamBlock {
	int24_t drcOn;			// drc module enabled. 0 == off
	struct drcBiquad hi1bq;	// high band biquads
	struct drcBiquad hi2bq;
	struct drcBiquad mi1bq;	// mid band biqauds
	struct drcBiquad mi2bq;
	struct drcBiquad mi3bq;
	struct drcBiquad mi4bq;
	struct drcBiquad lo1bq;	// low band biquads
	struct drcBiquad lo2bq;
	struct drcBiquad po1bq;	// post biquads
	struct drcBiquad po2bq;
	struct drc hi1drc;		// high compressors
	struct drc hi2drc;
	struct drc mi1drc;		// mid compressors
	struct drc mi2drc;
	struct drc lo1drc;		// low compressors
	struct drc lo2drc;
	struct drc po1drc;		// post compressors
	struct drc po2drc;
	struct drcBandLimited bl;// band limited compressor
} drcParamBlock_t;

enum dspBqFiltType{
	bypass,
	firstOrderHP,
	firstOrderLP,
	secondOrderHP,
	secondOrderLP,
	trebleShelv,
	bassShelv,
	bandPass
};

//Scale factors (from CF)
#define DSP_BQ_Q_EXP (6)
#define DSP_BQ_K_EXP (8)
#define EXP2LIN_20LOG10_IN_EXP (8)

#define SQRT2_2 ((float) 0.70710678118654757)
#define SCALE_FACTOR(S) (1 << (23 - S))

int drc_generic_to_specific(char *parambuf,   
				int sz,			
				float vlsCal, 
				int sr);

int drc_undo_threshold_compensation(
	            drcParamBlock_t *drc, 
				float vlsCal);

void printFilters(char * hdr, struct drcParamBlock *drc);
void printThreshold(char * hdr, struct drcParamBlock *drc);

#ifdef __cplusplus
}
#endif

#endif /* TFADRC_H_ */
