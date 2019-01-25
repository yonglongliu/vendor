#ifdef AUDIO_TEST_INTERFACE_V2
#include "endpoint_test.h"
#include <audio_utils/resampler.h>
#include <sys/select.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "ring_buffer.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>


#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

#include <expat.h>

#include <tinyalsa/asoundlib.h>
#define MIN_BUFFER_TIMS_MS  20
#define DEFAULT_FRAME_SIZE 320
#define AUDIO_EXT_CONTROL_PIPE_MAX_BUFFER_SIZE  1024
#define LOG_TAG "ENDPOINT_TEST"

#define MMI_DEFAULT_PCM_FILE  "/data/local/media/aploopback.pcm"

#define TEST_BUFFER_COUNT 16

static int32_t  mono2stereo(int16_t *out, int16_t * in, uint32_t in_samples) ;
static const unsigned char s_16k_pcmdata_mono[] = {
    0x92,0x02,0xcb,0x0b,0xd0,0x14,0x1d,0x1d,0xfc,0x24,0x17,0x2c,0x4a,0x32,0x69,0x37,
    0x92,0x3b,0x4e,0x3e,0x22,0x40,0x56,0x40,0x92,0x3f,0x12,0x3d,0x88,0x39,0x10,0x35,
    0xf0,0x2e,0x51,0x28,0xce,0x20,0x7f,0x18,0xd5,0x0f,0xda,0x06,0xdf,0xfd,0xa4,0xf4,
    0xa2,0xeb,0x39,0xe3,0x57,0xdb,0x3d,0xd4,0x1f,0xce,0xe2,0xc8,0xb1,0xc4,0xc0,0xc1,
    0xec,0xbf,0xc1,0xbf,0xa4,0xc0,0xf2,0xc2,0x18,0xc6,0xc2,0xca,0xc8,0xd0,0x36,0xd7,
    0xbb,0xde,0xe6,0xe6,0xa5,0xef,0xa6,0xf8,
};

static int get_pcm_config(struct audio_test_point *sink,
    struct pcm_config* config,struct test_endpoint_param *param){
    struct tiny_stream_out * stream_out=NULL;
    stream_out=(struct tiny_stream_out *)(sink->stream);

    if(param->type==INPUT_ENDPOINT_TEST){
        config->rate=param->pcm_rate;
        config->channels=param->pcm_channels;
    }else{
        config->rate=stream_out->config.rate;
        config->channels=stream_out->config.channels;
    }
    ALOGI("get_pcm_config rate:%d channle:%d",config->rate,config->channels);
    return 0;
}

static int32_t stereo2mono(int16_t *out, int16_t * in, uint32_t in_samples) {
    int i = 0;
    int out_samples =  in_samples >> 1;
    for(i = 0 ; i< out_samples ; i++) {
        out[i] =(in[2*i+1] + in[2*i]) /2;
    }
    return out_samples;
}

static int endpoint_calculation_ring_buffer_size(int size){
    int i=31;
    while(i){
        if((size & (1<<i))!=0)
            break;
        else
            i--;
    }

    if(i<=0)
        return 0;
    else
        return 1<<(i+1);
}


static int deinit_resampler(struct endpoint_resampler_ctl *resampler_ctl)
{
    ALOGI("deinit_resampler:%p",resampler_ctl->resampler);
    if (resampler_ctl->resampler) {
        release_resampler(resampler_ctl->resampler);
        resampler_ctl->resampler = NULL;
    }

    if(resampler_ctl->resampler_buffer!=NULL){
        free(resampler_ctl->resampler_buffer);
        resampler_ctl->resampler_buffer=NULL;
        resampler_ctl->resampler_buffer_size=0;
    }

    if(resampler_ctl->conversion_buffer!=NULL){
        free(resampler_ctl->conversion_buffer);
        resampler_ctl->conversion_buffer=NULL;
        resampler_ctl->conversion_buffer_size=0;
    }
    return 0;
}

static int init_resampler(struct endpoint_resampler_ctl *resampler_ctl)
{
    resampler_ctl->resampler = NULL;
    resampler_ctl->resampler_buffer=NULL;
    resampler_ctl->resampler_buffer_size=0;
    resampler_ctl->conversion_buffer=NULL;
    resampler_ctl->conversion_buffer_size=0;
    return 0;
}


static int pcm_buffer_format_change(
    void *input_buffer ,
    int input_buffer_size,
    struct endpoint_resampler_ctl *resampler_ctl){

    int frame_size=4;

    if ( resampler_ctl->channels!=resampler_ctl->request_channles){
            int num_read_buff_bytes=0;

            num_read_buff_bytes=input_buffer_size*resampler_ctl->request_channles/resampler_ctl->channels;
            if (num_read_buff_bytes != resampler_ctl->conversion_buffer_size) {
                if (num_read_buff_bytes > resampler_ctl->conversion_buffer_size) {
                    resampler_ctl->conversion_buffer_size=num_read_buff_bytes;
                    ALOGI("pcm_buffer_format_change LINE:%d %p %d",__LINE__,
                        resampler_ctl->conversion_buffer,resampler_ctl->conversion_buffer_size);
                    resampler_ctl->conversion_buffer = realloc(resampler_ctl->conversion_buffer, resampler_ctl->conversion_buffer_size);
                }
            }

            if((resampler_ctl->request_channles == 1) && (resampler_ctl->channels == 2)) {
                stereo2mono(resampler_ctl->conversion_buffer,input_buffer,input_buffer_size/2);
            }else if((resampler_ctl->request_channles == 2) && (resampler_ctl->channels == 1)) {
                mono2stereo(resampler_ctl->conversion_buffer,input_buffer,input_buffer_size/2);
            }
    }

    if ( resampler_ctl->rate!=resampler_ctl->request_rate){
        int resampler_inputbuffer_size=0;
        void *resampler_inputbuffer=NULL;
        int new_resampler_buffer_size=0;
        int in_frames=0;
        int out_frames=0;

        if((resampler_ctl->resampler_buffer==NULL)&&(NULL==resampler_ctl->resampler)){
            int ret=0;
            ret = create_resampler(resampler_ctl->request_rate,
                                   resampler_ctl->rate,
                                   resampler_ctl->request_channles,
                                   RESAMPLER_QUALITY_DEFAULT,
                                   NULL, &resampler_ctl->resampler);
            if (ret != 0) {
                ret = -EINVAL;
                ALOGE("%s line:%d create_resampler Failed",__func__,__LINE__);
                return ret;
            }
        }

        if(resampler_ctl->conversion_buffer!=NULL){
            resampler_inputbuffer=resampler_ctl->conversion_buffer;
            resampler_inputbuffer_size=resampler_ctl->conversion_buffer_size;
        }else{
            resampler_inputbuffer=input_buffer;
            resampler_inputbuffer_size=input_buffer_size;
        }
        new_resampler_buffer_size=2*resampler_inputbuffer_size*resampler_ctl->request_rate/resampler_ctl->rate;

        if (new_resampler_buffer_size > resampler_ctl->resampler_buffer_size) {
            resampler_ctl->resampler_buffer_size=new_resampler_buffer_size;
            ALOGI("pcm_buffer_format_change LINE:%d %p %d",__LINE__,
                resampler_ctl->resampler_buffer,resampler_ctl->resampler_buffer_size);
            resampler_ctl->resampler_buffer = realloc(resampler_ctl->resampler_buffer, resampler_ctl->resampler_buffer_size);
        }

        in_frames=resampler_inputbuffer_size/frame_size;
        out_frames=resampler_ctl->resampler_buffer_size/frame_size;
        ALOGI("pcm_buffer_format_change LINE:%d in_frames:%d out_frames:%d",
        __LINE__,in_frames,out_frames);
        resampler_ctl->resampler->resample_from_input(resampler_ctl->resampler,
                (int16_t *)resampler_inputbuffer,&in_frames,
                (int16_t *) resampler_ctl->resampler_buffer,&out_frames);
        ALOGI("pcm_buffer_format_change LINE:%d in_frames:%d out_frames:%d",
        __LINE__,in_frames,out_frames);
        return out_frames*frame_size;
    }else{
        if(resampler_ctl->resampler_buffer!=NULL){
            free(resampler_ctl->resampler_buffer);
            resampler_ctl->resampler_buffer=NULL;
        }
        resampler_ctl->resampler_buffer_size=0;
   }
    return resampler_ctl->conversion_buffer_size;
}

static int sink_to_write(struct audio_test_point *sink,void *buffer, int size){
    struct tiny_stream_out * stream_out=NULL;
    int num_read=0;

    stream_out=(struct tiny_stream_out *)(sink->stream);
    num_read=ring_buffer_get(sink->ring_buf, (void *)buffer, size);
    if(num_read > 0){
        int ret=-1;
        if(NULL!=stream_out){
            ret = out_write(sink->stream, buffer,num_read);
        }else{
            usleep(MIN_BUFFER_TIMS_MS*1000);
        }

        {
            int available_size=ring_buffer_len(sink->ring_buf);
            if(available_size<size){
                usleep(MIN_BUFFER_TIMS_MS*1000);
            }
        }
    }else{
        usleep(MIN_BUFFER_TIMS_MS*1000);
    }
    return num_read;
}

static int source_in_read(struct audio_test_point *source,struct test_endpoint_param *param, void *buffer, int size){
    struct tiny_stream_in * stream_in=NULL;
    stream_in=(struct tiny_stream_in *)(source->stream);

    int num_read=0;
    if(param->fd>0){
        num_read=read(param->fd,buffer,size);
        if(num_read<0){
                    ALOGE("source_in_read,errno:%u,%s num_read:%d",
              errno, strerror(errno),num_read);
                    ALOGE("audio_source_thread read:%d failed size:%d",param->fd,size);

            lseek(param->fd,0,SEEK_SET);
            num_read=read(param->fd,buffer,size);
            if((num_read>size)||(num_read<0)){
                ALOGE("source_in_read read failed size:%d",num_read);
                num_read=0;
                usleep(MIN_BUFFER_TIMS_MS*1000);
                return -1;
            }
        }

        if((source->resampler_ctl.channels!=source->resampler_ctl.request_channles)
            ||(source->resampler_ctl.rate!=source->resampler_ctl.request_rate)){
            num_read=pcm_buffer_format_change(buffer,num_read,&source->resampler_ctl);
            if(source->resampler_ctl.resampler_buffer!=NULL){
                producer_proc(source->ring_buf, (unsigned char *)source->resampler_ctl.resampler_buffer, (unsigned int)num_read);
            }else{
                producer_proc(source->ring_buf, (unsigned char *)source->resampler_ctl.conversion_buffer, (unsigned int)num_read);
            }
        }else{
            producer_proc(source->ring_buf, (unsigned char *)buffer, (unsigned int)num_read);
        }
    }else if ((param->data!=NULL) &&(param->data_size>0)){
      producer_proc(source->ring_buf, (unsigned char *)param->data, (unsigned int)param->data_size);
    }else if(source->stream!=NULL) {
        num_read=in_read(source->stream,buffer,size);
        LOG_V("audio_source_thread read:0x%x req:0x%x",num_read,size);
        if (num_read){
            producer_proc(source->ring_buf, (unsigned char *)buffer, (unsigned int)size);
        }else{
            ALOGW("audio_source_thread: no data read num_read:%d size:%d",num_read,size);
            usleep(MIN_BUFFER_TIMS_MS*1000);
        }
    }else{
        ALOGE("%s %d",__func__,__LINE__);
        usleep(50*MIN_BUFFER_TIMS_MS*1000);
    }
    return num_read;
}

static void endpoint_thread_exit(struct audio_test_point *point){
    deinit_resampler(&point->resampler_ctl);
    if(point->param.fd>0){
        close(point->param.fd);
        point->param.fd=-1;
    }

    if(point->param.data!=NULL){
        free(point->param.data);
        point->param.data=NULL;
    }
}

static void *audio_source_thread(void *args){
    struct audio_test_point *source=(struct audio_test_point *)args;
    struct test_endpoint_param *param=(struct test_endpoint_param *)&source->param;
    int ret=0;

    char *buffer= (char *)malloc(source->period_size);
    if (NULL==buffer) {
        ALOGE("Unable to allocate %d bytes\n", source->period_size);
        return NULL;
    }

    pthread_mutex_lock(&(source->lock));

    ALOGI("audio_source_thread param:%d %d %p period_size:%d",
        param->fd,param->data_size,source->stream,source->period_size);

    while(source->is_exit==false){
        pthread_mutex_unlock(&(source->lock));
        ret=source_in_read(source,&source->param, buffer, source->period_size);
        if((ret< source->period_size)&&(source->param.fd>0)){
            lseek(param->fd,0,SEEK_SET);
        }
        pthread_mutex_lock(&(source->lock));
    }

    if(NULL!=buffer){
        free(buffer);
    }

    if(source->stream!=NULL){
        in_standby(source->stream);
    }
    endpoint_thread_exit(source);

    pthread_mutex_unlock(&(source->lock));

    ALOGI("audio_source_thread exit");
    return args;
}

static void *audio_sink_thread(void *args){
    struct audio_test_point *sink=(struct audio_test_point *)args;

    char *buffer=NULL;
    buffer= (char *)malloc(sink->period_size);
    if (NULL==buffer) {
        ALOGE("Unable to allocate %d bytes\n", sink->period_size);
        return NULL;
    }

    ALOGI("audio_sink_thread stream:%p period_size:%d", sink->stream,sink->period_size);
    ALOGI("audio_sink_thread ring_buf:%p sink:%p",sink->ring_buf,sink);

    pthread_mutex_lock(&(sink->lock));
    while(sink->is_exit==false){
        pthread_mutex_unlock(&(sink->lock));
        sink_to_write(sink,buffer,sink->period_size);
        pthread_mutex_lock(&(sink->lock));
    }

    if(NULL!=buffer){
        free(buffer);
        buffer=NULL;
    }

    if(sink->stream!=NULL){
        out_standby(sink->stream);
    }

    endpoint_thread_exit(sink);
    pthread_mutex_unlock(&(sink->lock));
    ALOGI("audio_sink_thread exit");
    return args;
}

static void open_input_endpoint(void * dev,
    struct audio_test_point *input,
    struct test_endpoint_param *param){

    struct tiny_audio_device * adev=(struct tiny_audio_device *)dev;
    struct audio_config config;
    struct audio_stream_in *stream_in=NULL;

    config.sample_rate=param->pcm_rate;
    if(param->pcm_channels==2){
        config.channel_mask=AUDIO_CHANNEL_IN_STEREO;
    }else{
        config.channel_mask=AUDIO_CHANNEL_IN_MONO;
    }
    config.format=AUDIO_FORMAT_PCM_16_BIT;

    if(config.sample_rate==0){
            config.sample_rate=44100;
    }

    ALOGI("open_input_endpoint rate:%d stream:%p in_devices:0x%x"
        ,config.sample_rate,input->stream,adev->in_devices);
    if(NULL==input->stream){
        adev_open_input_stream(dev,0,adev->in_devices,&config,&stream_in,AUDIO_INPUT_FLAG_NONE,NULL,AUDIO_SOURCE_MIC);
        input->stream=stream_in;
    }
    if(NULL!=input->stream){
        in_standby(input->stream);
    }else{
        ALOGE("%s line:%d",__func__,__LINE__);
    }
}

static void  init_test_bufffer(struct audio_test_point *point,struct test_endpoint_param *param,bool is_sink){
    int delay_size=0;

    if(NULL==point->stream){
        point->period_size=0;
        point->ring_buf=NULL;
        return;
    }else{
        if(true==is_sink){
            struct tiny_stream_out * stream_out=(struct tiny_stream_out *)point->stream;
            struct audio_stream_out * audio_stream=NULL;
            audio_stream=(struct audio_stream_out *)(&stream_out->stream);
            point->period_size=audio_stream->common.get_buffer_size(&audio_stream->common);
            delay_size=param->delay*stream_out->config.rate*stream_out->config.channels*2/1000;
        }else{
            struct tiny_stream_in * stream_in=(struct tiny_stream_in *)point->stream;
            struct audio_stream_in *audio_stream=NULL;
            audio_stream=(struct audio_stream_in *)(&stream_in->stream);
            point->period_size=audio_stream->common.get_buffer_size(&audio_stream->common);
            delay_size=param->delay*stream_in->config.rate*stream_in->config.channels*2/1000;
        }
    }

    {
        int ring_buffer_size=point->period_size*TEST_BUFFER_COUNT;
        if((unsigned int)ring_buffer_size<param->data_size){
            ring_buffer_size=param->data_size*TEST_BUFFER_COUNT;
            ALOGW("init_test_bufffer data_size:%d period_size:%d",param->data_size,point->period_size);
        }
        ring_buffer_size=endpoint_calculation_ring_buffer_size(ring_buffer_size);
        point->ring_buf=ring_buffer_init(ring_buffer_size,(delay_size/DEFAULT_FRAME_SIZE+1)*DEFAULT_FRAME_SIZE);
        if(NULL==point->ring_buf){
            point->period_size=0;
        }
    }
}

static void dump_point(struct audio_test_point *point){
    if(point==NULL){
        ALOGE("%s line:%d point:%p",__func__,__LINE__,point);
        return;
    }

    ALOGI("point:%p is_exit:%d stream:%d period_size:%d",
        point,point->is_exit,point->stream,point->period_size);

    if(point->ring_buf!=NULL){
        ALOGI("point:%p ring_buf:%p %p ring_buf  size:%d in:%d out:%d",
            point,point->ring_buf,point->ring_buf->size,
            point->ring_buf->in,
            point->ring_buf->out);
    }

    ALOGI("point:%p param: pcm_rate:%d pcm_channels:%d fd:%d data:%p data_size:%d type:%d delay:%d test_step:%d",
        point,
        point->param.pcm_rate,
        point->param.pcm_channels,
        point->param.fd,
        point->param.data,
        point->param.data_size,
        point->param.type,
        point->param.delay,
        point->param.test_step
    );

}

static int free_endpoint(struct audio_test_point *point){

    if(point==NULL){
        ALOGE("%s line:%d point:%p",__func__,__LINE__,point);
        return -1;
    }

    point->is_exit=true;
    usleep(MIN_BUFFER_TIMS_MS*1000);

    if(point->ring_buf!=NULL){
        ring_buffer_free(point->ring_buf);
       point->ring_buf=NULL;
    }
    free(point);
    return 0;
}

static struct audio_test_point *create_source(void *dev,struct test_endpoint_param *param,
    int request_channles, int request_rate){

    struct audio_test_point *source=NULL;
    struct tiny_audio_device * adev=(struct tiny_audio_device *)dev;
    int i=0;

    source=malloc(sizeof(struct audio_test_point));
    if(NULL==source){
         ALOGE("create_source malloc  failed");
         return NULL;
    }

    memset(source,0,sizeof(struct audio_test_point));
    source->stream=NULL;
    source->param.type=param->type;
    source->param.data=NULL;
    source->param.data_size=0;
    source->param.fd=-1;
    init_resampler(&source->resampler_ctl);

    if (pthread_mutex_init(&(source->lock), NULL) != 0) {
        ALOGE("Failed pthread_mutex_init loop->in.lock,errno:%u,%s",
              errno, strerror(errno));
        return NULL;
    }

    source->is_exit=false;

    if(OUTPUT_ENDPOINT_TEST==param->type){
        if((param->fd<=0)&&(param->data!=NULL)&&(param->data_size>0)){
             if((param->pcm_channels!=request_channles)
                 ||(param->pcm_rate!=request_rate)){
                    int out_size=0;
                    char *new_data=NULL;
                    int data_size_count=0;
                    struct endpoint_resampler_ctl resampler_ctl;
                    int new_data_size=2*MIN_BUFFER_TIMS_MS*param->pcm_channels*param->pcm_rate/1000;
                    data_size_count=(new_data_size/param->data_size)+1;
                    new_data_size=data_size_count*param->data_size;
                    new_data=(char *)malloc(new_data_size);
                   if(NULL==new_data){
                        ALOGE("create_source malloc %dbytes failed",new_data_size);
                        if(NULL!=source){
                            free(source);
                        }
                        return NULL;
                   }
                   ALOGI("create_source data size:%d new_size:%d",param->data_size,new_data_size);
                   memset(&resampler_ctl,0,sizeof(struct endpoint_resampler_ctl));
                   for(i=0;i<data_size_count;i++){
                       memcpy(new_data+i*param->data_size,param->data,param->data_size);
                   }
                   param->data_size=new_data_size;
                   free(param->data);
                   param->data=new_data;

                  resampler_ctl.channels=param->pcm_channels;
                  resampler_ctl.request_channles=request_channles;
                  resampler_ctl.rate=param->pcm_rate;
                  resampler_ctl.request_rate=request_rate;

                  ALOGI("line:%d resampler_ctl:%d %d %d %d",
                     __LINE__,resampler_ctl.channels,resampler_ctl.request_channles,resampler_ctl.rate,resampler_ctl.request_rate);
                 out_size=pcm_buffer_format_change(param->data,param->data_size,&resampler_ctl);
                 source->param.data_size=out_size;
                 ALOGI("LINE:%d out_size:%d data_size:%d",__LINE__,out_size,param->data_size);
                 param->data_size=out_size;
                 param->pcm_channels=request_channles;
                 param->pcm_rate=request_rate;
                 free(param->data);
                 if(resampler_ctl.resampler_buffer!=NULL){
                     param->data=(void *)resampler_ctl.resampler_buffer;
                     resampler_ctl.resampler_buffer=NULL;
                     resampler_ctl.resampler_buffer_size=0;
                 }else{
                     param->data=(void *)resampler_ctl.conversion_buffer;
                     resampler_ctl.conversion_buffer=NULL;
                     resampler_ctl.conversion_buffer_size=0;
                 }
                 deinit_resampler(&resampler_ctl);
             }
        }
    }else{
        open_input_endpoint(dev,source,param);
         if(INPUT_ENDPOINT_TEST==param->type){
             init_test_bufffer(source,param,false);
         }
    }
    ALOGI("create_source fd:%d",source->param.fd);
    return source;
}

struct audio_test_point *create_sink(void *dev,void * stream,
    struct test_endpoint_param *param){
    struct tiny_audio_device * adev=(struct tiny_audio_device *)dev;

    struct tiny_stream_out * stream_out=NULL;
    struct audio_stream_out * audio_stream=NULL;

    struct audio_test_point *sink=NULL;
    int delay_size=0;
    sink=malloc(sizeof(struct audio_test_point));
    memset(sink,0,sizeof(struct audio_test_point));
    sink->param.data=NULL;
    sink->param.data_size=0;
    sink->param.fd=-1;
    sink->param.type=param->type;
    sink->resampler_ctl.resampler=NULL;
    init_resampler(&sink->resampler_ctl);
    stream_out=(struct tiny_stream_out *)(stream);
    sink->stream=stream;

    if(NULL!=stream_out){
        if(false==stream_out->standby){
            ALOGE("%s line:%d %p",__func__,__LINE__,stream_out);
            goto error;
        }
        stream_out->devices=adev->out_devices;
    }

    if (pthread_mutex_init(&(sink->lock), NULL) != 0) {
        ALOGE("Failed pthread_mutex_init loop->in.lock,errno:%u,%s",
              errno, strerror(errno));
        goto error;

    }

    if(INPUT_ENDPOINT_TEST!=param->type){
        init_test_bufffer(sink,param,true);
        sink->param.fd=param->fd;
    }
    ALOGI("create_sink fd:%d",sink->param.fd);

    sink->is_exit=false;
    return sink;

error:

    if(sink!=NULL){
        free(sink);
        sink=NULL;
    }
    return sink;
}

static int source_to_sink(struct audio_test_point *source,
    struct audio_test_point *sink,struct test_endpoint_param *param){
    struct tiny_stream_in * stream_in=NULL;
    struct tiny_stream_out * stream_out=NULL;
    stream_in=(struct tiny_stream_in *)(source->stream);
    stream_out=(struct tiny_stream_out *)(sink->stream);

    ALOGI("source_to_sink:%p %p param type:%d fd:%d"
        ,source,sink,param->type,param->fd);
    switch(param->type){
        case INPUT_ENDPOINT_TEST:
                stream_in->requested_channels=param->pcm_channels;
                stream_in->requested_rate=param->pcm_rate;
                sink->period_size=source->period_size;
                sink->ring_buf=source->ring_buf;
                sink->param.fd=param->fd;
            break;
        case OUTPUT_ENDPOINT_TEST:
                source->resampler_ctl.request_channles=stream_out->config.channels;
                source->resampler_ctl.request_rate=stream_out->config.rate;
                source->resampler_ctl.channels=param->pcm_channels;
                source->resampler_ctl.rate=param->pcm_rate;

                if((param->data_size!=0)&&(param->data!=NULL)){
                    source->param.data=param->data;
                    source->param.data_size=param->data_size;
                    param->data=NULL;
                    param->data_size=0;
                }

                source->param.fd=param->fd;
                source->period_size=sink->period_size;
                source->ring_buf=sink->ring_buf;
                if(source->stream!=NULL){
                    source->stream=NULL;
                }
            break;
        case INPUT_OUTPUT_ENDPOINT_LOOP_TEST:
                stream_in->requested_channels=stream_out->config.channels;
                stream_in->requested_rate=stream_out->config.rate;
                source->period_size=sink->period_size;
                source->ring_buf=sink->ring_buf;
                source->param.fd=-1;
            break;
        default:
            stream_in->requested_channels=0;
            stream_in->requested_rate=0;
            source->period_size=0;
            sink->period_size;
            ALOGE("source_to_sink unknow test case");
            break;
    }

    ALOGI("source_to_sink: fd:%d source:%d",sink->param.fd,source->param.fd);
    dump_point(sink);
    dump_point(source);

    if(pthread_create(&sink->thread, NULL, audio_sink_thread, (void *)sink)) {
        ALOGE("audio_sink_thread creating tx thread failed !!!!");
        return -2;
    }

    if(pthread_create(&source->thread, NULL, audio_source_thread, (void *)source)){
        ALOGE("audio_source_thread creating rx thread failed !!!!");
        return -3;
    }
    return 0;
}

static int read_record_data(struct audio_test_point *sink,int size){
    char *buffer=NULL;
    char *buffer_tmp=NULL;
    struct test_endpoint_param *param=&sink->param;
    int ret=0;
    int total_read=0;
    int default_fd=-1;
    int write_fd=-1;

    if((NULL==sink)||(size<=0)){
        ALOGE("%s line:%d sink:%p size:%d",__func__,__LINE__,sink,size);
        return -1;
    }

    buffer=malloc(size);
    if(NULL==buffer){
        ALOGE("%s line:%d malloc %d bytes failed",__func__,__LINE__,size);
        return -1;
    }

    pthread_mutex_lock(&sink->lock);
    total_read=size;
    buffer_tmp=buffer;
    do{
        ret=ring_buffer_get(sink->ring_buf, (void *)buffer_tmp, total_read);
        total_read-=ret;
        buffer_tmp+=ret;
        usleep(10*1000);
        ALOGI("read_record_data total_read:%d < size:%d",total_read,size);
    }while(total_read);
    pthread_mutex_unlock(&sink->lock);

    if(param->fd<=0){
        ALOGI("read_record_data start open:%s",MMI_DEFAULT_PCM_FILE);
        default_fd=open(MMI_DEFAULT_PCM_FILE, O_WRONLY);
        if(default_fd< 0) {
            ALOGE("read_record_data open:%s failed",MMI_DEFAULT_PCM_FILE);
            free(buffer);
            return -1;
        }
        write_fd=default_fd;
    }else{
        ALOGI("%s line:%d",__func__,__LINE__);
        write_fd=param->fd;
    }

    {
        unsigned char *data_tmp=buffer;
        int to_write=size;

        while(to_write) {
            ret = write(write_fd,data_tmp,to_write);
            if(ret <= 0) {
                usleep(10000);
                continue;
            }
            if(ret < to_write) {
                usleep(10000);
            }
            to_write -= ret;
            data_tmp += ret;
        }
    }
    free(buffer);

    if(default_fd> 0) {
        close(default_fd);
        default_fd=-1;
    }

    if(param->fd> 0) {
        close(param->fd);
        param->fd=-1;
    }

    return ret;
}

static int stop_endpoint_test(struct endpoint_test_control *ctl){
    struct audio_test_point *source=ctl->source;
    struct audio_test_point *sink=ctl->sink;
    void *res;

    if((source==NULL)||(sink==NULL)){
        ALOGE("%s line:%d source:%p sink:%p",__func__,__LINE__,source,sink);
        return -1;
    }

    /* wait source thread exit */
    pthread_mutex_lock(&source->lock);
    if(source->is_exit==true){
        ALOGE("%s line:%d source not running",__func__,__LINE__);
        pthread_mutex_unlock(&source->lock);
    }else{
        source->is_exit=true;
        pthread_mutex_unlock(&source->lock);
        pthread_join(source->thread,  &res);
        ALOGI("endpoint_test_stop %d",__LINE__);
    }

    /* wait sink thread exit */
    pthread_mutex_lock(&sink->lock);
    if(sink->is_exit==true){
        ALOGE("%s line:%d sink not running",__func__,__LINE__);
        pthread_mutex_unlock(&sink->lock);
    }else{
        sink->is_exit=true;
        pthread_mutex_unlock(&sink->lock);
        pthread_join(sink->thread,  &res);
        ALOGI("endpoint_test_stop %d",__LINE__);
    }

    /* free test buffer */
    pthread_mutex_lock(&sink->lock);
    if(sink->ring_buf!=NULL){
        ring_buffer_free(sink->ring_buf);
        sink->ring_buf=NULL;
    }
    pthread_mutex_unlock(&sink->lock);

    pthread_mutex_destroy(&(source->lock));
    pthread_mutex_destroy(&(sink->lock));

    free(ctl->sink);
    free(ctl->source);
    ctl->sink=NULL;
    ctl->source=NULL;
    ALOGI("endpoint_test_stop exit");
    return 0;
}

static int start_endpoint_test(void * dev,struct endpoint_test_control *ctl,
    void *stream,struct test_endpoint_param *param){
    struct pcm_config config;

    if((ctl->sink!=NULL)||(ctl->source!=NULL)){
        ALOGE("%s line:%d source:%p sink:%p",__func__,__LINE__,ctl->source,ctl->sink);
        return -1;
    }

    ctl->sink=create_sink(dev,stream,param);
    if(ctl->sink==NULL){
        goto error;
    }

    get_pcm_config(ctl->sink,&config,param);

    ctl->source=create_source(dev,param,config.channels,config.rate);
    if(ctl->source==NULL){
        goto error;
    }

    source_to_sink(ctl->source,ctl->sink,param);
    return 0;

error:

    if(ctl->source!=NULL){
        free_endpoint(ctl->source);
    }

    if(ctl->sink!=NULL){
        free_endpoint(ctl->sink);
    }
    return -1;
}

int endpoint_test_process(void * dev,struct endpoint_control *ctl,
    struct test_endpoint_param *param){

    switch(param->type){
        case OUTPUT_ENDPOINT_TEST:
            if(param->test_step==0){
                ALOGI("%s line:%d",__func__,__LINE__);
                stop_endpoint_test(&ctl->ouput_test);
              }else{
                ALOGI("%s line:%d",__func__,__LINE__);
                start_endpoint_test(dev,&ctl->ouput_test,ctl->primary_ouputstream,param);
            }
            break;

        case INPUT_ENDPOINT_TEST:
            if(param->test_step==0){
                ALOGI("%s line:%d",__func__,__LINE__);
                stop_endpoint_test(&ctl->input_test);
            }else if(param->test_step==1){
                start_endpoint_test(dev,&ctl->input_test,NULL,param);
                ALOGI("%s line:%d sink:%p",__func__,__LINE__,ctl->input_test.sink);
            }else{
                ALOGI("%s line:%d sink:%p",__func__,__LINE__,ctl->input_test.sink);
                read_record_data(ctl->input_test.sink , param->data_size);
            }
            break;

        case INPUT_OUTPUT_ENDPOINT_LOOP_TEST:
            if(param->test_step==0){
                ALOGI("%s line:%d",__func__,__LINE__);
                stop_endpoint_test(&ctl->loop_test);
              }else{
                ALOGI("%s line:%d",__func__,__LINE__);
                start_endpoint_test(dev,&ctl->loop_test,ctl->primary_ouputstream,param);
            }
            break;

        default:
            ALOGI("%s line:%d",__func__,__LINE__);
            break;
    }
    return 0;
}

int audio_endpoint_test(void * dev,struct str_parms *parms,char * val){
    int channel=0;
    int devices=0;
    char value[AUDIO_EXT_CONTROL_PIPE_MAX_BUFFER_SIZE];
    int ret = -1;
    int val_int = 0;
    char *data=NULL;

    struct tiny_audio_device * adev=(struct tiny_audio_device *)dev;
    struct test_endpoint_param *test_param=malloc(sizeof(struct test_endpoint_param));
    if(NULL==test_param){
        ALOGE("%s line:%d",__func__,__LINE__);
        return -1;
    }
    memset(test_param,0,sizeof(struct test_endpoint_param));
    test_param->type=-1;
    test_param->data=NULL;
    test_param->data_size=0;
    test_param->fd=-1;

    ret = str_parms_get_str(parms,"endpoint_test", value, sizeof(value));
    if(ret >= 0){
        test_param->type= strtoul(value,NULL,0);
    }

    ret = str_parms_get_str(parms,"endpoint_teststep", value, sizeof(value));
    if(ret >= 0){
        test_param->test_step= strtoul(value,NULL,0);
        if(0==test_param->test_step){
            goto out;
        }
    }

    ret = str_parms_get_str(parms,"endpoint_delay", value, sizeof(value));
    if(ret >= 0){
        test_param->delay= strtoul(value,NULL,0);
    }

    ret = str_parms_get_str(parms,"pcm_channels", value, sizeof(value));
    if(ret >= 0){
        test_param->pcm_channels= strtoul(value,NULL,0);
        LOG_D("%s %d pcm_channels:%d",__func__,__LINE__,test_param->pcm_channels);
    }

    ret = str_parms_get_str(parms,"pcm_rate", value, sizeof(value));
    if(ret >= 0){
        test_param->pcm_rate = strtoul(value,NULL,0);
        LOG_D("%s %d pcm_rate:%d",__func__,__LINE__,test_param->pcm_rate);
    }

    ret = str_parms_get_str(parms,"pcm_datasize", value, sizeof(value));
    if(ret >= 0){
        test_param->data_size= strtoul(value,NULL,0);

        if(test_param->data_size>0){
            test_param->data=(char *)malloc(test_param->data_size);
            if(NULL == test_param->data){
                LOG_E("audio_endpoint_test malloc %d bytes failed",test_param->data_size);
                test_param->data_size=0;
            }
        }
    }

    ret = str_parms_get_str(parms,"pcm_data", value, sizeof(value));
    if(ret >= 0){
        if(NULL==test_param->data){
            LOG_E("pcm_data ERR");
        }else{
            unsigned int size =(unsigned int)string_to_hex(test_param->data,value,test_param->data_size);
            if(test_param->data_size!=size){
                LOG_E("pcm_data data_size ERR:%d %d",size,test_param->data_size);
            }
        }
    }else{
        ret = str_parms_get_str(parms,"pcm_file", value, sizeof(value));
        if(ret >= 0){
            if(test_param->type==OUTPUT_ENDPOINT_TEST){
                test_param->fd=open(value, O_RDONLY);
                ALOGI("%s line:%d fd:%d pcm file:%s",
                    __func__,__LINE__,test_param->fd,value);
                if(test_param->fd<0){
                    ALOGW("audio_endpoint_test open pcm file %s failed errno=%d errr msg:%s",
                        value,errno,strerror(errno));
                }
            }else if(test_param->type==INPUT_ENDPOINT_TEST){
                test_param->fd=open(value, O_WRONLY | O_CREAT |O_TRUNC ,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                ALOGI("%s line:%d fd:%d",__func__,__LINE__,test_param->fd);
                if(test_param->fd<0){
                    ALOGW("audio_endpoint_test create pcm file %s failed errno=%d errr msg:%s",
                        value,errno,strerror(errno));
                }
            }
        }
    }

    if(test_param->type==OUTPUT_ENDPOINT_TEST){
        if(test_param->pcm_rate==0){
            test_param->pcm_rate=44100;
        }

        if(test_param->pcm_channels==0){
            test_param->pcm_channels=2;
        }

        if(((test_param->data==NULL)||(test_param->data_size==0))
            &&( test_param->fd<0)){
                if(test_param->data!=NULL){
                    free(test_param->data);
                }
                test_param->data_size=sizeof(s_16k_pcmdata_mono);
                test_param->data=(char *)malloc(test_param->data_size);
                if(NULL == test_param->data){
                    LOG_E("audio_endpoint_test malloc failed");
                    test_param->data_size=0;
                }

                ALOGI("use default pcm data for playback:%p size:%d",test_param->data,test_param->data_size);
                memcpy(test_param->data,&s_16k_pcmdata_mono,sizeof(s_16k_pcmdata_mono));
                test_param->pcm_channels=1;
                test_param->pcm_rate=44100;
        }
    }else if(test_param->type==INPUT_ENDPOINT_TEST){
        if(test_param->pcm_rate==0){
            test_param->pcm_rate=16000;
        }

        if(test_param->pcm_channels==0){
            test_param->pcm_channels=1;
        }
    }

out:
    ALOGI("test_param file:%d pcm_rate:%d pcm_channels:%d data_size:%d",
        test_param->fd,test_param->pcm_rate,test_param->pcm_channels,test_param->data_size);
    ret= endpoint_test_process(dev,&adev->test_ctl,test_param);

    if(test_param->data!=NULL){
        free(test_param->data);
        test_param->data=NULL;
    }

    free(test_param);
    return ret;
}

int audio_endpoint_test_init(void * dev,void *out_stream){
    struct tiny_audio_device * adev=(struct tiny_audio_device *)dev;
    struct endpoint_control *test_ctl=(struct endpoint_control *)&adev->test_ctl;

    memset(test_ctl,0,sizeof(struct endpoint_control));

    test_ctl->primary_ouputstream=out_stream;
    test_ctl->ouput_test.source=NULL;
    test_ctl->ouput_test.sink=NULL;

    test_ctl->input_test.source=NULL;
    test_ctl->input_test.sink=NULL;

    test_ctl->loop_test.source=NULL;
    test_ctl->loop_test.sink=NULL;
    ALOGI("%s %d %p",__func__,__LINE__, test_ctl->primary_ouputstream);
    return 0;
}
#endif
