
#ifndef _MP3_ENC_ENCODER_H
#define _MP3_ENC_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct MP3_ENC_INPUT
{
        int32  sample_rate;      // sample rate of encoding music
        int32  bit_rate;         // bit rate of encoding music
        int16  ch_count;         // channel count of encoding music
        int16  MS_sign;          // MS sign
//        int16  IS_sign;          // IS sign
        int16  VBR_sign;         // VBR or CBR 
        int32  cut_off;  
       
} MP3_ENC_INPUT_T;
    
typedef struct MP3_ENC_OUTPUT
{
    int32  *stream_buf_ptr;      // mp3 encoded stream
    int16   stream_len;          // encoded mp3 stream length    
} MP3_ENC_OUTPUT_T;





/*
   memory constrct
       return value:  mp3_enc_mem_ptr         
*/
int16 MP3_ENC_MemoryAlloc(MP3_ENC_DATA_T **mp3_enc_mem_ptr);
/*
    description:
       input:   
           data_ptr: input pcm data and the data buffer is continuous, the size is 2304
           mp3_enc_buffer_ptr: the assistant memory for coding process
           in_parameter_ptr:  configuration parameter
*/
void MP3_ENC_Init(MP3_ENC_INPUT_T *in_parameter_ptr,
                  MP3_ENC_DATA_T  *mp3_enc_buffer_ptr);
/*
    description:
       input:   
           data_ptr: input pcm data and the data buffer is continuous, the size is 2304
           mp3_enc_buffer_ptr: the assistant memory for coding process
       output data:
           output_data_t_ptr: output data and parameter
*/
int16 MP3_ENC_EncoderProcess(int16            *data_ptr,
                             int16            data_len,
                            MP3_ENC_DATA_T             *mp3_enc_buffer_ptr);
/*
    description:
       input: 
           mp3_enc_buffer_ptr: the assistant memory for coding process
       output data:
           output_data_t_ptr: output data and parameter
*/
void MP3_ENC_ExitEncoder(MP3_ENC_DATA_T *mp3_enc_buffer_ptr);


/*
   memory deconstrct
*/
int16 MP3_ENC_MemoryFree(MP3_ENC_DATA_T **mp3_enc_mem_ptr);


typedef int16 (*FT_MP3_ENC_MemoryAlloc)(MP3_ENC_DATA_T **mp3_enc_mem_ptr);
typedef void (*FT_MP3_ENC_InitDecoder)(MP3_ENC_INPUT_T *in_parameter_ptr,
                  MP3_ENC_DATA_T  *mp3_enc_buffer_ptr);
typedef int16 (*FT_MP3_ENC_EncoderProcess)(int16            *data_ptr,
                             int16            data_len,
                            MP3_ENC_DATA_T             *mp3_enc_buffer_ptr);
typedef void (*FT_MP3_ENC_ExitEncoder)(MP3_ENC_DATA_T *mp3_enc_buffer_ptr);
typedef int16 (*FT_MP3_ENC_MemoryFree)(MP3_ENC_DATA_T **mp3_enc_mem_ptr);


#ifdef __cplusplus
}
#endif
#endif
