#define LOG_TAG "sprdresample"
#include"sprd_resample_48kto44k.h"
#include<stdio.h>
#include <utils/Log.h>
#include <stdlib.h>
#include <string.h>
#include <audio_utils/resampler.h>
#include "dumpdata.h"
#define SRC_SAMPLE_RATE     48000
#define DEST_SAMPLE_RATE    44100
#define NR_UNIT 1920
#define SPRD_48K_TO_44100_MIN_FRAMES   480
#define SPRD_SRC_MIN_FRAMES   160

typedef struct tResample48kto44k{
    TRANSAM44K_CONTEXT_T  *pTrans44K;
    void *pTransInLeftBuffer;
    void *pTransInRightBuffer;
    void *pTransOutBuffer;
    void *pTransReadBuffer;
    void *pTransStoreBuffer;//storage the left data
    int storage_bytes;
    callback read_data;
    void *data_handle;
    int channel;

    int rate;
    int request_rate;
    struct resampler_itfe *resampler;

    int pre_readbytes;
    int frame_size;
    int before_src_upper_limit;

    int src_max_size;
    int before_src_lower_limit;
    int after_src_lower_limit;

    AUDIO_PCMDUMP_FUN dumppcmfun;
}T_TRANSFORM48K_TO_44K;


#ifndef USE_RESAMPLER_44_1_K
transform_handle SprdSrc_To_44K_Init(int rate,int request_rate,callback get_proc_data,void *data_handle, int request_channel)
{
    return NULL;
}
int SprdSrc_To_44K_DeInit(transform_handle handle)
{
    return 0;
}

static int SprdSrc_To_44K_proc(transform_handle handle,void *pInBuf , int in_bytes,int channel)
{
    return 0;
}

int SprdSrc_To_44K(transform_handle handle, void *buffer , int bytes)
{
    return 0;
}

#else
static int get_frames(int bytes, int frame_size){
    if(2==frame_size){
        return bytes>>1;
    }else if(4==frame_size){
        return bytes>>2;
    }else if(8==frame_size){
        return bytes>>3;
    }else{
        return (bytes/frame_size);
    }
}

static int get_size_with_frame(int frame, int frame_size){
    if(2==frame_size){
        return frame<<1;
    }else if(4==frame_size){
        return frame<<2;
    }else if(8==frame_size){
        return frame<<3;
    }else{
        return (frame*frame_size);
    }
}

static int re_init_44k_src(transform_handle handle,int size){
    T_TRANSFORM48K_TO_44K *pTransform=NULL;
    pTransform = (T_TRANSFORM48K_TO_44K *)handle;
    int ret=-1;

    if((pTransform->pTrans44K)&&(pTransform->src_max_size!=size))
    {
        if(pTransform->src_max_size>0){
            ret = MP344K_DeInitParam(pTransform->pTrans44K);
            if(ret < 0)
            {
                ALOGE("re_init_44k_src: MP344K_DeInitParam fail!");
            }
        }

        pTransform->src_max_size=size;
        ret = MP344K_InitParam(pTransform->pTrans44K, SRC_SAMPLE_RATE, pTransform->src_max_size);
        if(ret < 0)
        {
            ALOGE("re_init_44k_src: MP344K_InitParam fail!");
        }else{
            ALOGI("MP344K_InitParam success size:%d",pTransform->src_max_size);
        }
    }

    return 0;
}

static int realloc_src_buffer(transform_handle handle){
    T_TRANSFORM48K_TO_44K *pTransform=NULL;
    pTransform = (T_TRANSFORM48K_TO_44K *)handle;
    int ret=-1;
    {
        int min_frames=SPRD_48K_TO_44100_MIN_FRAMES;
        int bytes_tmp=0;
        if((44100==pTransform->request_rate)&&(48000==pTransform->rate)){
            min_frames=SPRD_48K_TO_44100_MIN_FRAMES;
        }else{
            min_frames=SPRD_SRC_MIN_FRAMES;
        }

        pTransform->before_src_lower_limit=pTransform->pre_readbytes*pTransform->rate/pTransform->request_rate;
        pTransform->before_src_lower_limit=(pTransform->before_src_lower_limit/min_frames)*min_frames;
        pTransform->after_src_lower_limit=pTransform->before_src_lower_limit*pTransform->request_rate/pTransform->rate;

        if(pTransform->after_src_lower_limit!=pTransform->pre_readbytes){
            pTransform->before_src_upper_limit=(pTransform->pre_readbytes*pTransform->rate+pTransform->request_rate)/pTransform->request_rate;
            pTransform->before_src_upper_limit=(pTransform->before_src_upper_limit/min_frames+1)*min_frames;
        }else{
            pTransform->before_src_upper_limit=pTransform->before_src_lower_limit;
            ALOGI("realloc_src_buffer bytes:%d (%d:%d)",
                pTransform->pre_readbytes,
                pTransform->before_src_lower_limit,
                pTransform->after_src_lower_limit);
        }
    }

    ALOGI("realloc_src_buffer before_src(%d:%d) after_src(%d:%d)",
        pTransform->before_src_lower_limit,pTransform->before_src_upper_limit,
        pTransform->after_src_lower_limit,pTransform->pre_readbytes);

    if((44100==pTransform->request_rate)&&(48000==pTransform->rate)){
        ALOGI("func:%s line:%d",__func__,__LINE__);
        if(NULL!=pTransform->pTrans44K){
            re_init_44k_src(pTransform,pTransform->before_src_upper_limit);
        }

        pTransform->pTransInLeftBuffer = (void *)realloc(pTransform->pTransInLeftBuffer,
            pTransform->before_src_upper_limit);
        if(NULL == pTransform->pTransInLeftBuffer)
            goto err;
        memset(pTransform->pTransInLeftBuffer, 0 ,pTransform->before_src_upper_limit);

        pTransform->pTransInRightBuffer = (void *)realloc(pTransform->pTransInRightBuffer,pTransform->before_src_upper_limit);
        if(NULL == pTransform->pTransInRightBuffer)
            goto err;
        memset(pTransform->pTransInRightBuffer, 0 ,pTransform->before_src_upper_limit);

        pTransform->pTransOutBuffer = (void *)realloc(pTransform->pTransOutBuffer,pTransform->before_src_upper_limit*2);
        if(NULL == pTransform->pTransOutBuffer)
            goto err;
        memset(pTransform->pTransOutBuffer, 0, pTransform->before_src_upper_limit*2);

        pTransform->pTransReadBuffer = (void *)realloc(pTransform->pTransReadBuffer,pTransform->before_src_upper_limit);
        if(NULL == pTransform->pTransReadBuffer)
            goto err;
        memset(pTransform->pTransReadBuffer, 0 ,pTransform->before_src_upper_limit);

        pTransform->pTransStoreBuffer = (short *)realloc(pTransform->pTransStoreBuffer,pTransform->before_src_upper_limit*2);//give enough
        if(NULL == pTransform->pTransStoreBuffer)
            goto err;
        memset(pTransform->pTransStoreBuffer, 0, pTransform->before_src_upper_limit*2);
    }else{
        ALOGI("func:%s line:%d",__func__,__LINE__);
        pTransform->src_max_size=pTransform->before_src_upper_limit;

        pTransform->pTransOutBuffer = (void *)realloc(pTransform->pTransOutBuffer,pTransform->before_src_upper_limit);
        if(NULL == pTransform->pTransOutBuffer)
            goto err;
        memset(pTransform->pTransOutBuffer, 0, pTransform->before_src_upper_limit);

        pTransform->pTransReadBuffer = (void  *)realloc(pTransform->pTransReadBuffer,pTransform->before_src_upper_limit);
        if(NULL == pTransform->pTransReadBuffer)
            goto err;
        memset(pTransform->pTransReadBuffer, 0 ,pTransform->before_src_upper_limit);

        pTransform->pTransStoreBuffer = (short *)realloc(pTransform->pTransStoreBuffer,pTransform->before_src_upper_limit<<1);//give enough
        if(NULL == pTransform->pTransStoreBuffer)
            goto err;
        memset(pTransform->pTransStoreBuffer, 0, pTransform->before_src_upper_limit<<1);
    }
    return 0;
err:
    ALOGE("realloc_src_buffer Failed");
    return -1;
}

transform_handle SprdSrc_To_44K_Init(
    int rate,int request_rate,
    callback get_proc_data,void *data_handle, int request_channel)
{
    int dwRc = 0;
    T_TRANSFORM48K_TO_44K *pTransform=NULL;

    pTransform = (T_TRANSFORM48K_TO_44K *) malloc(sizeof(T_TRANSFORM48K_TO_44K));
    if(NULL == pTransform)
        goto err;
    memset(pTransform, 0, sizeof(T_TRANSFORM48K_TO_44K));

    pTransform->src_max_size=0;

    if((44100==request_rate)&&(48000==rate)){
        ALOGI("SprdSrc_To_441K_Init in");
        pTransform->resampler=NULL;

        pTransform->pTrans44K = (void *)malloc(sizeof(TRANSAM44K_CONTEXT_T));
        if(NULL == pTransform->pTrans44K)
            goto err;
        memset(pTransform->pTrans44K, 0, sizeof(TRANSAM44K_CONTEXT_T));

    }else{
        dwRc = create_resampler(rate,
                               request_rate,
                               request_channel,
                               RESAMPLER_QUALITY_DEFAULT,
                               NULL, &pTransform->resampler);
        if (dwRc != 0) {
            pTransform->resampler=NULL;
            ALOGE("%s create_resampler Failed",__func__);
        }else{
            ALOGI("%s rate:%d request_rate:%d create_resampler success ",__func__,rate,request_rate);
        }
        pTransform->pTrans44K=NULL;
    }

    pTransform->rate=rate;
    pTransform->request_rate=request_rate;

    pTransform->pre_readbytes=0;
    pTransform->before_src_upper_limit=0;
    pTransform->frame_size=request_channel*2;
    pTransform->storage_bytes=0;
    pTransform->read_data  = get_proc_data;
    pTransform->data_handle = data_handle;
    pTransform->channel = request_channel;
    ALOGI("SprdSrc_To_44K_Init: dwRc: %d ",dwRc);
    return (void *)pTransform;
err:
    if(pTransform)
    {
        if(pTransform->pTransOutBuffer)
            free(pTransform->pTransOutBuffer);
        if(pTransform->pTransInLeftBuffer)
            free(pTransform->pTransInLeftBuffer);
        if(pTransform->pTransInRightBuffer)
            free(pTransform->pTransInRightBuffer);
        if(pTransform->pTransReadBuffer)
            free(pTransform->pTransReadBuffer);
        if(pTransform->pTransStoreBuffer)
            free(pTransform->pTransStoreBuffer);
        if(pTransform->pTrans44K)
            free(pTransform->pTrans44K);
        if(pTransform->resampler){
            release_resampler(pTransform->resampler);
            pTransform->resampler = NULL;
        }

        free(pTransform);
    }
    return NULL;
}


int SprdSrc_To_44K_DeInit(transform_handle handle)
{
    int dwRc = 0;
    T_TRANSFORM48K_TO_44K *pTransform=NULL;
    pTransform = (T_TRANSFORM48K_TO_44K *)handle;
    if(pTransform)
    {
        if(pTransform->pTrans44K)
        {
            dwRc = MP344K_DeInitParam(pTransform->pTrans44K);
            if(dwRc < 0)
            {
                ALOGE("Sprd_NxpTfa: SprdSrc_48K_DeInit fail!");
            }
            free(pTransform->pTrans44K);
        }
        if(pTransform->pTransOutBuffer)
            free(pTransform->pTransOutBuffer);
        if(pTransform->pTransInLeftBuffer)
            free(pTransform->pTransInLeftBuffer);
        if(pTransform->pTransInRightBuffer)
            free(pTransform->pTransInRightBuffer);
        if(pTransform->pTransReadBuffer)
            free(pTransform->pTransReadBuffer);
        if(pTransform->pTransStoreBuffer)
            free(pTransform->pTransStoreBuffer);
        if(pTransform->resampler){
            release_resampler(pTransform->resampler);
            pTransform->resampler = NULL;
        }

        free(pTransform);
    }
    return dwRc;
}

static int SprdSrc_To_44K_proc(transform_handle handle,void *pInBuf , int in_bytes,int channel)
{
    int i, count, OutSize;
    T_TRANSFORM48K_TO_44K *pTransform=NULL;
    pTransform = (T_TRANSFORM48K_TO_44K *)handle;
    TRANSAM44K_CONTEXT_T *pTrans44K = pTransform->pTrans44K;

    short *pSrcLBuf = (short *)(pTransform->pTransInLeftBuffer);
    short *pSrcRBuf = (short *)(pTransform->pTransInRightBuffer);
    short *pOutBuf = (short*)(pTransform->pTransOutBuffer);
    short *pSrcBuf = (short *)pInBuf;

       if(channel == 1)
    {
        for (i = 0; i < in_bytes/2; i++) /*compute L/R Channel*/
        {
            pSrcLBuf[i] = pSrcBuf[i];
            pSrcRBuf[i] = pSrcBuf[i];
        }
        count = MP3_Play44k(pTrans44K, pSrcLBuf, pSrcRBuf, in_bytes/2, SRC_SAMPLE_RATE, pOutBuf, pOutBuf+in_bytes/2);
        for (i = 0; i <count; i++)
        {
            pSrcBuf[i] = pOutBuf[i];
        }
        OutSize = count * 2;
    }
    else
    {
        for (i = 0; i < in_bytes/4; i++) /*compute L/R Channel*/
        {
            pSrcLBuf[i] = pSrcBuf[i*2];
            pSrcRBuf[i] = pSrcBuf[i*2+1];
        }
        count = MP3_Play44k(pTrans44K, pSrcLBuf, pSrcRBuf, in_bytes/4, SRC_SAMPLE_RATE, pOutBuf, pOutBuf+in_bytes/4);
        for ( i = 0;  i < count; i++)
        {
            pSrcBuf[i*2] = pOutBuf[i];
            pSrcBuf[i*2+1] = pOutBuf[i + in_bytes/4];
        }
        OutSize = count * 4;
    }
    ALOGV("SprdSrc_To_44K in_bytes =%d ,count =%d ", in_bytes, count);
    return OutSize;
}

int SprdSrc_To_44K(transform_handle handle, void *buffer , int bytes)
{
    int ret=0;
    int transfered_size,out_size;
    int in_unit = 0;
    T_TRANSFORM48K_TO_44K *pTransform=NULL;
    pTransform = (T_TRANSFORM48K_TO_44K *)handle;
    void *pReadBuffer = pTransform->pTransReadBuffer;
    int new_storage_size;
    int channel = pTransform->channel;
    size_t in_frames=0;
    size_t out_frames=0;
    int frame_size=pTransform->frame_size;

    if(bytes!=pTransform->pre_readbytes){
        ALOGI("SprdSrc_To_44K  realloc_src_buffer pre:%d new:%d before_src_upper_limit:%d",
            pTransform->pre_readbytes,bytes,pTransform->before_src_upper_limit);
        pTransform->pre_readbytes=bytes;
        if(realloc_src_buffer(pTransform)<0)
            return -1;
    }

    in_unit=pTransform->before_src_lower_limit;

    if(pTransform->storage_bytes < bytes)
    {
        memcpy((char *)buffer,(char *)pTransform->pTransStoreBuffer, pTransform->storage_bytes);
        transfered_size = pTransform->storage_bytes;
        pTransform->storage_bytes = 0;
        while(1)
        {

            if(bytes-transfered_size>pTransform->after_src_lower_limit){
                in_unit=pTransform->before_src_upper_limit;
            }else{
                in_unit=pTransform->before_src_lower_limit;
            }
            ALOGI("SprdSrc_To_44K in_unit:%d after_src_lower_limit:%d :%d bytes:%d",
            in_unit,pTransform->after_src_lower_limit,pTransform->before_src_upper_limit,bytes-transfered_size);
            if(in_unit>pTransform->src_max_size){
                in_unit=pTransform->src_max_size;
            }

            pReadBuffer = pTransform->pTransReadBuffer;
            ret = pTransform->read_data(pTransform->data_handle, pReadBuffer,  in_unit);
            if(ret <0)
            {
               ALOGE("get nr data error");
               return -1;
            }

            if(pTransform->resampler){
                out_frames=get_frames(bytes,frame_size);
                in_frames=get_frames(in_unit,frame_size);
                ALOGV("SprdSrc_process in_frames:%zd out_frames:%zd",in_frames,out_frames);
                pTransform->resampler->resample_from_input(pTransform->resampler,
                    (int16_t *)pReadBuffer,&in_frames,
                    (int16_t *) pTransform->pTransOutBuffer,&out_frames);
                ALOGV("SprdSrc_process out_frames:%zd frame_size:%d req:bytes:%d",
                    out_frames,frame_size,bytes);
                out_size=get_size_with_frame((int)out_frames,frame_size);
                ALOGV("read :%d out:%d",in_unit,out_size);

                if(NULL!=pTransform){
                    pTransform->dumppcmfun(pTransform->pTransOutBuffer,out_size,DUMP_MINI_RECORD_AFTER_SRC);
                }

                if( (bytes-transfered_size) > out_size)
                {
                    memcpy((char *)buffer+transfered_size, (char*)pTransform->pTransOutBuffer,out_size);
                    transfered_size+=out_size;
                }
                else
                {
                    memcpy((char *)buffer+transfered_size, (char*)pTransform->pTransOutBuffer,bytes-transfered_size );
                    //storage the bytes
                    new_storage_size = transfered_size+ out_size-bytes;//update the storage size
                    memcpy((char *)pTransform->pTransStoreBuffer, (char *)pTransform->pTransOutBuffer+(out_size - new_storage_size), new_storage_size);
                    pTransform->storage_bytes = new_storage_size;
                    ALOGI("pTransform->storage_bytes = %d",pTransform->storage_bytes);
                    break;
                }


            }else{
                out_size = SprdSrc_To_44K_proc(pTransform, pReadBuffer, in_unit, channel);
                ALOGI("SprdSrc_To_44K_proc out_size:%d input:%d",out_size,in_unit);

                if(NULL!=pTransform){
                    pTransform->dumppcmfun(pReadBuffer,out_size,DUMP_MINI_RECORD_AFTER_SRC);
                }

                if( (bytes-transfered_size) > out_size)
                {
                    memcpy((char *)buffer+transfered_size, (char*)pReadBuffer,out_size);
                    transfered_size+=out_size;
                }
                else
                {
                    memcpy((char *)buffer+transfered_size, (char*)pReadBuffer,bytes-transfered_size );
                    //storage the bytes
                    new_storage_size = transfered_size+ out_size-bytes;//update the storage size
                    memcpy((char *)pTransform->pTransStoreBuffer, (char *)pReadBuffer+(out_size - new_storage_size), new_storage_size);
                    pTransform->storage_bytes = new_storage_size;
                    ALOGV("pTransform->storage_bytes = %d",pTransform->storage_bytes);
                    break;
                }
            }
        }
    }
    else
    {
        ALOGI("SprdSrc_To_44K storage_bytes:%d req bytes:%d",
            pTransform->storage_bytes,bytes);
        memcpy((char *)buffer, (char *)pTransform->pTransStoreBuffer, bytes);
        pTransform->storage_bytes = pTransform->storage_bytes -bytes;
        memcpy((char *)pTransform->pTransStoreBuffer, (char *)pTransform->pTransStoreBuffer+bytes, pTransform->storage_bytes);
    }
    return ret;
}
#endif

void register_resample_pcmdump(transform_handle handle,void * func){
    T_TRANSFORM48K_TO_44K *pTransform=NULL;
    pTransform = (T_TRANSFORM48K_TO_44K *)handle;
    if(NULL!=pTransform){
        pTransform->dumppcmfun=func;
    }
}
