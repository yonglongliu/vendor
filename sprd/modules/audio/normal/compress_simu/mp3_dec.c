#define LOG_TAG "audio_hw_mp3"

#include <stdlib.h>
#include "mp3_dec_api.h"
#include <cutils/log.h>


#include <cutils/properties.h>
#include <dlfcn.h>

static FT_MP3_ARM_DEC_Construct mMP3_ARM_DEC_Construct;
static FT_MP3_ARM_DEC_Deconstruct mMP3_ARM_DEC_Deconstruct;
static FT_MP3_ARM_DEC_InitDecoder mMP3_ARM_DEC_InitDecoder;
static FT_MP3_ARM_DEC_DecodeFrame mMP3_ARM_DEC_DecodeFrame;
static void* mLibHandle;


int MP3_ARM_DEC_init()
{
    char * libName = "libomx_mp3dec_sprd.so";
    if(mLibHandle) {
        dlclose(mLibHandle);
    }
    mLibHandle = dlopen("libomx_mp3dec_sprd.so", RTLD_NOW);
    if(mLibHandle == NULL) {
        ALOGE("openDecoder, can't open lib: %s",libName);
        return -1;
    }

    mMP3_ARM_DEC_Construct = (FT_MP3_ARM_DEC_Construct)dlsym(mLibHandle, "MP3_ARM_DEC_Construct");
    if(mMP3_ARM_DEC_Construct == NULL) {
        ALOGE("Can't find MP3_ARM_DEC_Construct in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP3_ARM_DEC_Deconstruct = (FT_MP3_ARM_DEC_Deconstruct)dlsym(mLibHandle, "MP3_ARM_DEC_Deconstruct");
    if(mMP3_ARM_DEC_Deconstruct == NULL) {
        ALOGE("Can't find MP3_ARM_DEC_Deconstruct in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP3_ARM_DEC_InitDecoder = (FT_MP3_ARM_DEC_InitDecoder)dlsym(mLibHandle, "MP3_ARM_DEC_InitDecoder");
    if(mMP3_ARM_DEC_InitDecoder == NULL) {
        ALOGE("Can't find MP3_ARM_DEC_InitDecoder in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    mMP3_ARM_DEC_DecodeFrame = (FT_MP3_ARM_DEC_DecodeFrame)dlsym(mLibHandle, "MP3_ARM_DEC_DecodeFrame");
    if(mMP3_ARM_DEC_DecodeFrame == NULL) {
        ALOGE("Can't find MP3_ARM_DEC_DecodeFrame in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return -1;
    }

    return 0;
}

int MP3_ARM_DEC_release()
{
    if(mLibHandle) {
        dlclose(mLibHandle);
    }
    mMP3_ARM_DEC_Construct = NULL;
    mMP3_ARM_DEC_Deconstruct = NULL;
    mMP3_ARM_DEC_InitDecoder = NULL;
    mMP3_ARM_DEC_DecodeFrame = NULL;
    return 0;
}


int MP3_ARM_DEC_Construct(void **h_decoder_ptr)
{
    if(mMP3_ARM_DEC_Construct) {
        return mMP3_ARM_DEC_Construct(h_decoder_ptr);
    }
    else {
        *h_decoder_ptr = NULL;
        return -1;
    }
}
int MP3_ARM_DEC_Deconstruct(void const **h_decoder_ptr){
    if(mMP3_ARM_DEC_Deconstruct) {
        return mMP3_ARM_DEC_Deconstruct(h_decoder_ptr);
    }
    else {
        return -1;
    }
}
void MP3_ARM_DEC_InitDecoder(void * param){
    if(mMP3_ARM_DEC_InitDecoder) {
        mMP3_ARM_DEC_InitDecoder(param);
    }
    else {
        return;
    }
}
void MP3_ARM_DEC_DecodeFrame(void * param,
                             FRAME_DEC_T *frame_dec_buf_ptr,    // [Input]
                             OUTPUT_FRAME_T *output_frame_ptr,  // [Output]
                             uint32_t *decode_result            // [Output]
                             )
{
    if(mMP3_ARM_DEC_DecodeFrame) {
        mMP3_ARM_DEC_DecodeFrame(param,frame_dec_buf_ptr,output_frame_ptr,decode_result);
    }
    else {
        *decode_result = -1;
    }
    return;
}

