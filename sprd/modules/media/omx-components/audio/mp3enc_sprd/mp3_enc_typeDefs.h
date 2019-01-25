
#ifndef _MP3_ENC_TYPEDEFS_H
#define _MP3_ENC_TYPEDEFS_H

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////
#include <string.h>
/////////////////////////////////////////////


#ifdef WIN32
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include "math.h"
#endif

#ifdef WIN32
#define LIT_END    //  if define LIT_END, work as little endian mode else as big endian mode
#endif

#define LIT_END

//#define win32
#define QUANT32
#define POOL_LEN 720

#define MP3_ENC_IS_START_L 18   //must bigger than 11
#define MP3_ENC_IS_START_S 30   //must bigger than 18
#define MP3_ENC_IS_START_M 29   //must bigger than 17

#define MP3_ENC_SUB_BAND_NUM 18
#define MP3_ENC_SUB_BAND_LEN 32
#define MP3_ENC_GR_NUMBER    MP3_ENC_SUB_BAND_NUM * MP3_ENC_SUB_BAND_LEN
#define MP3_DATA_FIXED       12
#define MP3_HAN_WIN_SIZE     512
#define MP3_ENC_SHORT_TYPE   2
#define MP3_ENC_REAL_BITS14 14
#define MP3_ENC_TABLE_BITS6 6
#define MP3_ENC_INTERP_BITS1 (MP3_ENC_REAL_BITS14 - MP3_ENC_TABLE_BITS6)

#define MP3_ENC_MEMSET(MP3_data, MP3_n, MP3_m)       memset(MP3_data, MP3_n, MP3_m)
#define MP3_ENC_MEMCPY(MP3_des, MP3_src, MP3_m)      memcpy(MP3_des, MP3_src, MP3_m)
#define MP3_ENC_MALLOC(aac_m)						 malloc(aac_m)
#define MP3_ENC_FREE(file_ptr)						 free(file_ptr)
    
#define MP3_ENC_ASSERT(x)                          
#define MP3_ENC_NULL                                 0
#define MP3_ENC_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MP3_ENC_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MP3_ENC_ABS(a)     ((a >= 0)   ? (a) : (-a))
#define ABS(x) if(x<0)x=-x
#define MAX_LIMIT(x,y) if(x>y)x=y
#define MIN_LIMIT(x,y) if(x<y)x=y
//#define MP3_ENC_PRINTF                               //printf
//#define MP3_ENC_FPRINTF                              //fprintf
#define inv32(x) ((x&0xff)<<24)+((x&0xff00)<<8)+((x&0xff0000)>>8)+((x&0xff000000)>>24)
#define inv16(x) ((x&0xff)<<8)+((x&0xff00)>>8)

#define ENG_PREC 12

typedef struct  
{
	int32 frames;		//
    int32 main_data_end;
    int32 mask_offset;
    int32 MK15;

    int32 bit_rate;		//not useful
    int32 sample_rate;
    int32 avg_bit_rate;
    int32 writen;
    int32 frame_bits;
    int32 pool_bits;
    int32 head_bits;
    int32 available;
    int32 avg_bits;
    int32 cost_smooth_bits;
    int32 avai_smooth_bits;
    int32 pe_smooth[2];
    int32 max_smooth;
    int32 pe_total;


    int32 sum_PE  [2][2];
    int32 gr_bits [2][2];
    int32 def_bits[2][2];
    int32 cost;

    int32 ancillary_bit;
    int32 cut_off;

    int16 byteN   [15];
    int16 padding [15];

    int16 pre_flag[2][2];

    int16 ID;
    int16 grs;
    int16 bit_rate_idx;//not useful
    int16 bit_rate_fix;//not useful

	int16 pad;
    int16 sample_idx;
    int16 channels;
    int16 mode;
    int16 mode_extension;


    int32 *rate_tab;
/*
    int16 is_ctrl;
    int16 is_process;
    int16 is_used;
    int16 is_start[2];
    int16 is_bin[2];
*/
	int16 use_bit_pool;
    int16 ms_ctrl;
    int16 ms_process;
    int16 ms_used;
    int16 comWin;

    int32 part23_len         [2][2];
    int16 big_val            [2][2];
        int16 big_sfb        [2][2];    //编码辅助，big_val对应的缩放因子带
        int16 count1         [2][2];    //编码辅助，01数据的长度
    int16 global_gain        [2][2];
    int16 scalefac_compress  [2][2];
    int16 scalefac_scale     [2][2];

    int16 block_split        [2][2];
    int16 count1_select      [2][2];
    int16 subblock_gain      [2][2][3];
    int32 gr_max             [2][3];

    int16 region_len         [2][2][2];
    int16 region_select      [2][2][3];
    int16 region_bin         [2][2][3];
    
    int16 scfsi[2][4];
    int16 scfsi_chn[2];

    int32 block_type    [2][2];
    int32 switch_type   [2][2];

    int16 contious      [2][2];
}MP3_ENC_HEAD;

typedef struct  
{
    int16 *sfbwL;//22
    int16 sfbwS[39];//39
	
    int16 isfbwL[22];//22
    int16 isfbwS[39];//39
	
    int16 *mask_fix_L;
    int16 *mask_fix_S;
	
}MP3_ENC_TAB;



typedef struct  
{
    int32 spec          [4608];
    //int32 spec_MS       [2304];
    int32 *spec_MS;     //reuse spec+2304
    int16 *spec_q ;     //reuse spec+2304
    int16 *spec_s ;     //reuse spec+2304+1152
    int32 eng           [312] ;   //eng of each half subband 
    int16 SF            [312] ;
    int16 *max_idx      ;   //max idx of each subband reuse SF+156
    int32 bit_alloc     [156] ;
    int32 old_mask      [88]  ;
    
    int16 max_eng_sfb    [2][2];   //max sfb idx,eng based
    int16 max_sfb_offset [2][2];   //
    int16 max_spec_sfb   [2][2];   //max sfb idx,amplitude based    
    int16 max_spec_offset[2][2];   //

    int16 scale[2];
    int16 *sfbw         [2][2];
    int16 *isfbw        [2][2];
        int16 *mask_fix [2][2];
    int16 K             [2][2];
    int16 max_bin       [2][2];
    int32 *sf2amp;
}MP3_ENC_SPEC;

typedef struct  
{
    int32 ana_buf[1024];
    int32 ana_idx[2];

    int32 sub_buf[4608];
    int32 sub_idx[2];


    //inverse transform buffer
    int32 syn_buf[1024];
    int32 syn_idx[2];

    int32 lap_buf[1152];
    int32 rec_buf[2304];

    int32 window[2];
}MP3_ENC_TRAN;

typedef struct 
{    /*
    int32 *cur_bit_stream;
#ifdef LIT_END
    // LITTLE ENDIAN 
    int16 bit_left;
    int16 word_used;
#else
    // BIG ENDIAN 
    int16 word_used;
    int16 bit_left;
#endif
    int16 frame_stop;

    
    int16 buf_idx;
    int16 frm_len;
    int16 data_empty;

    int32 buf_pp[2][POOL_LEN];
    int32 *buf_ptr;
    int32 *out_stream_ptr;
*/
    uint32 buf[POOL_LEN];
    //int32 *buf_ptr;
    int32 bit_left;
    int32 byte_used;
    int32 frame_stop;

	uint8 *stream_out;
	int32 *stream_len;
    //FILE *bit_stream;
    
} MP3_ENC_BIT_POOL_T;
typedef struct 
{
  int8 riff[4];    /* ASCII "RIFF"  */
  int32 chunksize;  /* int32 int for file size */
  int8 wave[4];    /* ASCII "WAVE" */
  int8 fmt[4];     /* ASCII "fmt " */
  int32 reserve;  /* fmt chunk size 16 */

  int16 fmttag;    /* 1 = wave format PCM */
  int16 channels;   

  int32 samplerate;
  int32 buffersize; /* rate * channels * sample size / 8  */

  int16 BlockAlign; /* channels * sample size / 8  */
  int16 samplesize;  /* bits per sample of one channel. */

  int8 data[4];     /* ASCII "data"  */
  int32 datasize;   /* data size  channels * samples * (sample size / 8)  */
  int32 samples;	
}WAVE_HEAD;


typedef struct MP3_ENC_DATA_T
{
    MP3_ENC_TAB    enc_tab;
    MP3_ENC_HEAD   enc_head; 
    MP3_ENC_SPEC   enc_spec;
	MP3_ENC_TRAN   enc_tran;
    MP3_ENC_BIT_POOL_T bit_pool_buf;
    
    int32 data_buffer[663];//[512+1152+4608+2304+156];
    
} MP3_ENC_DATA_T;

#ifdef __cplusplus
}
#endif
#endif
