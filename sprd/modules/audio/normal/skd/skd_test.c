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

#define LOG_TAG "skd_test"
static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
        size_t bytes);
static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
        size_t bytes);
static int adev_open_input_stream(struct audio_hw_device *dev,
        audio_io_handle_t handle,
        audio_devices_t devices,
        struct audio_config *config,
        struct audio_stream_in **stream_in,
        audio_input_flags_t flags,
        const char *address __unused,
        audio_source_t source);
static int in_standby(struct audio_stream *stream);
static int out_standby(struct audio_stream *stream);

int audio_skd_test(void * dev,struct str_parms *parms,int type);

enum aud_skd_test_m {
    TEST_AUDIO_SKD_IDLE =0,
    TEST_AUDIO_SKD_OUT_DEVICES,
    TEST_AUDIO_SKD_IN_DEVICES,
    TEST_AUDIO_SKD_IN_OUT_LOOP,
    TEST_AUDIO_SKD_OUT_IN_LOOP,
};

#define DEFAULT_OUT_SAMPLING_RATE 44100
/* constraint imposed by VBC: all period sizes must be multiples of 160 */
#define VBC_BASE_FRAME_COUNT 160
/* number of base blocks in a short period (low latency) */
#define SHORT_PERIOD_MULTIPLIER 8 /* 29 ms */
/* number of frames per short period (low latency) */
#define SHORT_PERIOD_SIZE (VBC_BASE_FRAME_COUNT * SHORT_PERIOD_MULTIPLIER)
#define CAPTURE_PERIOD_COUNT 3
#define PLAYBACK_SHORT_PERIOD_COUNT   3      /*4*/

static  struct pcm_config _default_config = {
    .channels = 2,
    .rate = DEFAULT_OUT_SAMPLING_RATE,
    .period_size = SHORT_PERIOD_SIZE,
    .period_count = PLAYBACK_SHORT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = SHORT_PERIOD_SIZE,
    .avail_min = SHORT_PERIOD_SIZE,
};

#define TEST_AUDIO_LOOP_MIN_DATA_MS 500
//#define DSPLOOP_FRAME_SIZE 488
#define DSPLOOP_FRAME_SIZE 512

static int _calculation_ring_buffer_size(int ms,int size){
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

static unsigned int producer_proc(struct ring_buffer *ring_buf,unsigned char * buf,unsigned int size)
{
    int to_write=size;
    int ret=0;
    unsigned char *tmp=NULL;
    tmp=buf;
    int try_count=0;
    while(to_write) {
        ret = ring_buffer_put(ring_buf, (void *)tmp, to_write);
        if(ret <= 0) {
            usleep(10000);
            try_count++;

            if(try_count>50){
                break;
            }
            continue;
        }
        if(ret < to_write) {
            usleep(10000);
            try_count++;
        }

        to_write -= ret;
        tmp += ret;
        try_count=0;
    }
    return size;
}

static void *skd_loop_rx_thread(void *args){
    struct loop_ctl_t *in=(struct loop_ctl_t *)args;

    struct tiny_stream_in *in_stream=in->stream;
    struct audio_stream_in * audio_stream=(struct audio_stream_in *)(&in_stream->stream);

    char *buffer=NULL;
    int num_read = 0;
    int size=0;
    int bytes_read=0;

    pthread_mutex_lock(&(in->lock));
    size=audio_stream->common.get_buffer_size(&audio_stream->common);
    buffer = (char *)malloc(size);
    if (!buffer) {
        ALOGE("Unable to allocate %d bytes\n", size);
        goto ERR;
    }

    sem_wait(&in->sem);
    in->state=true;

    in_stream->pop_mute=false;
    in_stream->pop_mute_bytes=0;

    ALOGI("skd_loop_rx_thread in device:%x size:%x",in_stream->device,size);
    while(in->is_exit==false){
        pthread_mutex_unlock(&(in->lock));
        num_read=in_read(in->stream,buffer,size);
        LOG_V("skd_loop_rx_thread read:0x%x req:0x%x",num_read,size);
        if (num_read){
            bytes_read += size;
            producer_proc(in->ring_buf, (unsigned char *)buffer, (unsigned int)size);
            LOG_V("skd_loop_rx_thread capture 0x%x total:0x%x",size,bytes_read);
        }else{
            ALOGW("skd_loop_rx_thread: no data read num_read:%d",num_read);
            usleep(20*1000);
        }
        pthread_mutex_lock(&(in->lock));
    }

    in->state=false;

    if(NULL!=buffer){
        free(buffer);
        buffer=NULL;
    }

    pthread_mutex_unlock(&(in->lock));
    ALOGI("skd_loop_rx_thread exit success");
    return NULL;

ERR:

    if(NULL!=buffer){
        free(buffer);
        buffer=NULL;
    }

    in_standby(in->stream);
    in->state=false;
    pthread_mutex_unlock(&(in->lock));

    ALOGE("skd_loop_rx_thread exit err");
    return NULL;
}

static void *skd_loop_tx_thread(void *args){
    struct loop_ctl_t *out=(struct loop_ctl_t *)args;
    struct tiny_stream_out *out_stream=out->stream;
    struct audio_stream_out * audio_stream=(struct audio_stream_out *)(&out_stream->stream);
    char *buffer=NULL;
    int num_read = 0;
    int size=0;
    int ret=-1;
    int write_count=0;
    int no_data_count=0;

    pthread_mutex_lock(&(out->lock));

    size=audio_stream->common.get_buffer_size(&audio_stream->common);

    buffer = (char *)malloc(size);
    if (!buffer) {
        ALOGE("skd_loop_tx_thread Unable to allocate %d bytes\n", size);
        goto ERR;
    }

    sem_wait(&out->sem);
    out->state=true;
    ALOGI("skd_loop_tx_thread out devices:%x size:%d",out_stream->devices,size);
    while(out->is_exit==false){
        num_read=ring_buffer_get(out->ring_buf, (void *)buffer, size);
        LOG_V("skd_loop_tx_thread read:0x%x req:0x%x",num_read,size);
        if(num_read > 0){
            no_data_count=0;
            pthread_mutex_unlock(&(out->lock));
            ret = out_write(out->stream, buffer,num_read);
            pthread_mutex_lock(&(out->lock));

        }else{
            usleep(20*1000);
            no_data_count++;
            ALOGI("skd_loop_tx_thread no data read:%d",no_data_count);
            if(out->is_exit==true){
                break;
            }else{
                if(no_data_count>=100){
                    break;
                }
            }
        }
    }

    if(NULL!=buffer){
        free(buffer);
        buffer=NULL;
    }
    out->state=false;
    pthread_mutex_unlock(&(out->lock));
    ALOGI("skd_loop_tx_thread exit success");
    return NULL;

ERR:
    if(NULL!=buffer){
        free(buffer);
        buffer=NULL;
    }

    out_standby(out->stream);
    out->state=false;
    pthread_mutex_unlock(&(out->lock));
    ALOGE("skd_loop_tx_thread exit err");
    return NULL;
}

static int audio_skd_loop_standby(struct tiny_audio_device *adev,struct skd_loop_t *loop)
{
    struct loop_ctl_t *ctl=NULL;
    void *res;
    int wait_count=30;
    struct loop_ctl_t *in=(struct loop_ctl_t *)&(loop->in);
    struct loop_ctl_t *out=(struct loop_ctl_t *)&(loop->out);

    ALOGI("audio_skd_loop_standby enter");

    if(TEST_AUDIO_SKD_IDLE==loop->state){
        ALOGW("audio_skd_loop_stop failed, loop is not work");
        return -1;
    }

    /********************************************/
    pthread_mutex_lock(&in->lock);
    in->is_exit=true;
    pthread_mutex_unlock(&in->lock);

    pthread_mutex_lock(&out->lock);
    out->is_exit=true;
    pthread_mutex_unlock(&out->lock);


    /********************************************/
    pthread_mutex_unlock(&adev->lock);
    pthread_join(loop->in.thread,  &res);

    ALOGI("audio_skd_loop_standby %d",__LINE__);
    pthread_join(loop->out.thread, &res);

    ALOGI("audio_skd_loop_standby %d",__LINE__);
    pthread_mutex_lock(&adev->lock);
    /********************************************/
    pthread_mutex_unlock(&adev->lock);
    in_standby(loop->in.stream);
    out_standby(loop->out.stream);
    pthread_mutex_lock(&adev->lock);

    /********************************************/
    pthread_mutex_lock(&out->lock);
    if(out->ring_buf!=NULL){
        ring_buffer_free(loop->out.ring_buf);
        out->ring_buf=NULL;
    }
    pthread_mutex_unlock(&out->lock);

    pthread_mutex_lock(&in->lock);
    in->ring_buf=NULL;
    pthread_mutex_unlock(&in->lock);

    /********************************************/
    loop->state=TEST_AUDIO_SKD_IDLE;

    pthread_mutex_destroy(&(loop->out.lock));
    pthread_mutex_destroy(&(loop->in.lock));
    ALOGI("audio_skd_loop_standby exit");

    return 0;
}

static void skd_open_input_stream(void * dev,struct skd_loop_t * skd_loop){
    struct tiny_audio_device * adev=(struct tiny_audio_device *)dev;
    struct audio_config config;
    struct audio_stream_in *stream_in=NULL;

    config.sample_rate=pcm_config_mm_fast.rate;
    config.channel_mask=AUDIO_CHANNEL_IN_STEREO;
    config.format=AUDIO_FORMAT_PCM_16_BIT;

    ALOGI("skd_open_input_stream rate:%d",config.sample_rate);
    pthread_mutex_unlock(&adev->lock);
    if(NULL==skd_loop->in.stream){
        adev_open_input_stream(dev,0,adev->in_devices,&config,&stream_in,AUDIO_INPUT_FLAG_NONE,NULL,AUDIO_SOURCE_MIC);
        skd_loop->in.stream=stream_in;
        in_standby(skd_loop->in.stream);
    }
    pthread_mutex_lock(&adev->lock);

    ALOGI("skd_open_input_stream stream in:%p out:%p",skd_loop->in.stream,skd_loop->out.stream);
}

static int audio_skd_loop_start(struct tiny_audio_device *adev,struct skd_loop_t *loop,int type){
    struct ring_buffer *ring_buf=NULL;
    struct pcm_config * in_config=NULL;
    int size=0;
    int min_buffer_size=0;
    int skd_zero_size=0;
    int delay_size=0;
    struct skd_loop_t * skd_loop=(struct skd_loop_t *)&(adev->skd_loop);
    struct audio_stream_out * audio_stream=NULL;

    struct tiny_stream_in * stream_in=NULL;
    struct tiny_stream_out * stream_out=NULL;

    skd_open_input_stream(adev,loop);

    stream_in=(struct tiny_stream_in *)skd_loop->in.stream;
    stream_out=(struct tiny_stream_out *)skd_loop->out.stream;

    audio_stream=(struct audio_stream_out *)(&stream_out->stream);
    size=audio_stream->common.get_buffer_size(&audio_stream->common)*16;

    if(TEST_AUDIO_SKD_IDLE!=loop->state){
        pthread_mutex_unlock(&adev->lock);
        ALOGW("audio_skd_loop_start failed, loop is working now");
        return -1;
    }

    if((NULL==stream_out)
        ||(NULL==stream_in)){
        ALOGE("audio_skd_loop_start stream error in:%p out:%p",stream_in,stream_out);
        return -2;
    }

    if(false==stream_out->standby){
        ALOGE("audio_skd_loop_start stream status error out:%d",stream_out->standby);
        //return -3;
    }

    memcpy(&stream_out->config,&pcm_config_mm_fast,sizeof(pcm_config_mm_fast));

    stream_in->device=adev->in_devices;
    stream_out->devices=adev->out_devices;
    stream_in->requested_channels=stream_out->config.channels;
    stream_in->requested_rate=stream_out->config.rate;

    in_config=&stream_out->config;

    size=_calculation_ring_buffer_size(loop->loop_delay*16,size);

    /* skd process frame buffer size is 122 word(244bytes),
       the delay buffer size must be a multiple of skd frame buffer */
    delay_size=loop->loop_delay*in_config->rate*in_config->channels*2/1000;
    skd_zero_size=delay_size/DSPLOOP_FRAME_SIZE;
    skd_zero_size+=1;
    skd_zero_size*=DSPLOOP_FRAME_SIZE;

    ring_buf=ring_buffer_init(size,skd_zero_size);
    if(NULL==ring_buf){
        goto error;
    }

    if (pthread_mutex_init(&(loop->in.lock), NULL) != 0) {
        ALOGE("Failed pthread_mutex_init loop->in.lock,errno:%u,%s",
              errno, strerror(errno));
        goto error;
    }

    if (pthread_mutex_init(&(loop->out.lock), NULL) != 0) {
        ALOGE("Failed pthread_mutex_init loop->out.lock,errno:%u,%s",
              errno, strerror(errno));
        goto error;
    }

    loop->in.ring_buf=ring_buf;
    loop->out.ring_buf=ring_buf;

    loop->in.state=true;
    loop->out.state=true;

    loop->in.is_exit=false;
    loop->out.is_exit=false;

    sem_init(&loop->in.sem, 0, 1);
    sem_init(&loop->out.sem, 0, 1);

    if(TEST_AUDIO_SKD_IN_OUT_LOOP==type){
        if(pthread_create(&loop->out.thread, NULL, skd_loop_tx_thread, (void *)&(loop->out))) {
            ALOGE("skd_loop_tx_thread creating tx thread failed !!!!");
            goto error;
        }

        if(pthread_create(&loop->in.thread, NULL, skd_loop_rx_thread, (void *)&(loop->in))){
            ALOGE("skd_loop_rx_thread creating rx thread failed !!!!");
            goto error;
        }
    }

    loop->state=type;
    ALOGI("audio_skd_loop_start sucess size:%d",size);
    return 0;

error:
    if(NULL!=ring_buf){
        ring_buffer_free(ring_buf);
    }

    pthread_mutex_destroy(&(loop->out.lock));
    pthread_mutex_destroy(&(loop->in.lock));

    loop->state=TEST_AUDIO_SKD_IDLE;

    pthread_mutex_unlock(&adev->lock);
    out_standby(loop->out.stream);
    in_standby(loop->in.stream);
    pthread_mutex_lock(&adev->lock);

    ALOGI("audio_skd_loop_start failed");
    return -1;
}

void skd_init(void * dev,void *out_stream){
    struct tiny_audio_device * adev=(struct tiny_audio_device *)dev;
    struct skd_loop_t * skd_loop=(struct skd_loop_t *)&(adev->skd_loop);
    skd_loop->out.stream=out_stream;
}

int audio_skd_test(void * dev,struct str_parms *parms,int type){
    int ret=0;
    struct tiny_audio_device * adev=(struct tiny_audio_device *)dev;
    pthread_mutex_lock(&adev->lock);
    if(type){
        ret=audio_skd_loop_start(adev,&adev->skd_loop,type);
    }else{
        ret=audio_skd_loop_standby(adev,&adev->skd_loop);
    }
    pthread_mutex_unlock(&adev->lock);
    return ret;
}
