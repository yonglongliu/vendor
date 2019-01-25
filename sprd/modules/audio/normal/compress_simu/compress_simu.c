#define LOG_TAG "audio_hw_cps"
#define _GNU_SOURCE

#include "compress_simu.h"
#include<sched.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <tinyalsa/asoundlib.h>
#include <cutils/log.h>
#include <pthread.h>
#include <semaphore.h>
#include "mp3_dec_api.h"
#include <sound/asound.h>
#include "sound/compress_params.h"
#include "sound/compress_offload.h"
#include "tinycompress/tinycompress.h"
#include "cutils/list.h"
#include "ringbuffer.h"
#include <cutils/str_parms.h>
#include <cutils/properties.h>
#include <expat.h>
#include <system/thread_defs.h>
#include <cutils/sched_policy.h>
#include <sys/prctl.h>
#include "sys/resource.h"
#include <time.h>
#include <sys/time.h>
#include <linux/ioctl.h>
#include <audio_utils/resampler.h>
#include "dumpdata.h"


#if 0
#undef ALOGE
extern "C"
{
extern void  peter_log(const char *tag,const char *fmt, ...);
#define ALOGE(...)  peter_log(LOG_TAG,__VA_ARGS__);
}
#endif



#define COMPR_ERR_MAX 128
#define MP3_MAX_DATA_FRAME_LEN  (1536)  //unit by bytes
#define MP3_DEC_FRAME_LEN       (1152)  //pcm samples number

#define AUDIO_OFFLOAD_PLAYBACK_VOLUME_MAX 0x1000

enum{
    ERROR_NO_DATA = 100,
    ERROR_DECODE,
    ERROR_OTHER,
};


enum {
    STATE_OPEN =1,
    STATE_START,
    STATE_PAUSE,
    STATE_RESUME,
    STATE_STOP,
    STATE_STOP_DONE,
    STATE_DRAIN,
    STATE_DRAIN_DONE,
    STATE_PARTIAL_DRAIN,
    STATE_PARTIAL_DRAIN_DONE,
    STATE_CLOSE,
    STATE_CHANGE_DEV,
    STATE_EXIT,
};

typedef enum {
    COMPRESS_CMD_EXIT,               /* exit compress offload thread loop*/
    COMPRESS_CMD_DRAIN,              /* send a full drain request to driver */
    COMPRESS_CMD_PARTIAL_DRAIN,      /* send a partial drain request to driver */
    COMPRESS_WAIT_FOR_BUFFER,    /* wait for buffer released by driver */
    COMPERSS_CMD_PAUSE,
    COMPERSS_CMD_RESUME,
    COMPERSS_CMD_START,
    COMPERSS_CMD_STOP,
    COMPERSS_CMD_SEND_DATA,
    COMPERSS_CMD_CLOSE,
    COMPERSS_CMD_CHANGE_DEV,
}COMPRESS_CMD_T;


struct dev_set_param {
        uint32_t card;
        uint32_t device;
        struct pcm_config  *config;
        void * mixerctl;
};


struct compress_cmd {
    struct listnode node;
    COMPRESS_CMD_T cmd;
    bool is_sync;
    sem_t   sync_sem;
    int ret;
    void* param;
};


#define LARGE_PCM_BUFFER_TIME   400

#define STREAM_CACHE_MAX   2048
#define SPRD_AUD_DRIVER "/dev/audio_offloadf"


#define SPRD_AUD_IOCTL_MAGIC 	'A'

#define AUD_WAIT      			   _IO(SPRD_AUD_IOCTL_MAGIC, 1)
#define AUD_LOCK                  _IOW(SPRD_AUD_IOCTL_MAGIC, 2,int)
#define AUD_WAKEUP                _IO(SPRD_AUD_IOCTL_MAGIC, 3)




struct fade_in
{
    int16_t step;
    int32_t max_count;
};


struct  stream {
    struct ringbuffer * ring;
};


struct compress {
                int32_t aud_fd;
                int ready;
                struct compr_config *config;
                uint32_t flags;
                uint8_t error[2];

                int card;
                int device;
                struct pcm *pcm;
            	struct pcm_config pcm_cfg;
            	pthread_mutex_t  pcm_mutex;
            	pthread_mutex_t  lock;
            	pthread_t        thread;
            	int running;
            	int32_t  state;


	            pthread_mutex_t  cond_mutex;
                pthread_cond_t   cond;
                int buffer_ready;

                pthread_mutex_t  drain_mutex;
                pthread_cond_t   drain_cond;
                int drain_ok;
                int drain_waiting;

                pthread_mutex_t  partial_drain_mutex;
                pthread_cond_t   partial_drain_cond;
                int partial_drain_ok;
                int partial_drain_waiting;

                uint32_t start_time;
                uint32_t samplerate;
                uint32_t channels;
                uint32_t bitrate;

                int32_t is_first_frame;

                pthread_cond_t   cmd_cond;
                pthread_mutex_t  cmd_mutex;

                struct listnode cmd_list;

                void *mMP3DecHandle;
                int16_t mLeftBuf[MP3_DEC_FRAME_LEN];  //output pcm buffer
                int16_t mRightBuf[MP3_DEC_FRAME_LEN];
                uint8_t mMaxFrameBuf[MP3_MAX_DATA_FRAME_LEN];
                int16_t pOutputBuffer[MP3_DEC_FRAME_LEN*2];

                int16_t *pOutPcmBuffer;

                FT_MP3_ARM_DEC_Construct mMP3_ARM_DEC_Construct;
                FT_MP3_ARM_DEC_Deconstruct mMP3_ARM_DEC_Deconstruct;
                FT_MP3_ARM_DEC_InitDecoder mMP3_ARM_DEC_InitDecoder;
                FT_MP3_ARM_DEC_DecodeFrame mMP3_ARM_DEC_DecodeFrame;

                struct  stream  stream;

                uint32_t pcm_bytes_left;
                uint32_t pcm_bytes_offset;
                struct timespec ts;

                struct timespec end_ts;
                uint32_t loop_state;
                struct mixer *mixer;
                struct mixer_ctl *position_ctl;
                uint32_t prev_samples;
                uint32_t pause_position;

                struct  stream  pcm_stream;
                int32_t play_cache_pcm_first;
                struct stream  cache_pcm_stream;

                struct resampler_itfe  *resampler;
                uint32_t resample_in_samplerate;
                uint32_t resample_out_samplerate;
                uint32_t resample_channels;
                int16_t* resample_buffer;
                uint32_t resample_buffer_len;
                uint32_t resample_in_max_bytes;
                int no_src_data;
                uint32_t fade_step;

                uint32_t fade_step_2;
                uint32_t pcm_time_ms;
                long sleep_time;

                struct fade_in  fade_resume;
                struct fade_in  fade_start;

                uint32_t bytes_decoded;

                uint32_t  noirq_frames_per_msec;
                uint8_t   is_start;

                pthread_mutex_t  position_mutex;

                long left_vol;
                long right_vol;
                AUDIO_PCMDUMP_FUN dumppcmfun;
};


static int compress_signal(struct compress * compress);
static void compress_write_data_notify(struct compress *compress);
extern int pcm_avail_update(struct pcm *pcm);
extern int pcm_state(struct pcm *pcm);

//#define COMPRESS_DEBUG_INTERNAL

#ifdef COMPRESS_DEBUG_INTERNAL

static FILE * g_out_fd = NULL;

static FILE * g_out_fd2 = NULL;

int16_t test_buf[2*1024*1024] = {0};

static int out_dump_create(FILE **out_fd, const char *path)
{
    if (path == NULL) {
        ALOGE("path not assigned.");
        return -1;
    }
    *out_fd = (FILE *)fopen(path, "wb");
    if (*out_fd == NULL ) {
        ALOGE("cannot create file.");
        return -1;
    }
    ALOGI("path %s created successfully.", path);
    return 0;
}


static int out_dump_doing2( const void* buffer, size_t bytes)
{
    int ret;
    if(!g_out_fd) {
	out_dump_create(&g_out_fd,"/data/local/media/peter_track.pcm");
    }
    if (g_out_fd) {
        ret = fwrite((uint8_t *)buffer, bytes, 1, g_out_fd);
        if (ret < 0) ALOGW("%d, fwrite failed.", bytes);
    } else {
        ALOGW("out_fd is NULL, cannot write.");
    }
    return 0;
}

static int out_dump_doing3( const void* buffer, size_t bytes)
{
    int ret;
    if(!g_out_fd2) {
	out_dump_create(&g_out_fd2,"/data/local/media/peter_track2.pcm");
    }
    if (g_out_fd2) {
        ret = fwrite((uint8_t *)buffer, bytes, 1, g_out_fd2);
        if (ret < 0) ALOGW("%d, fwrite failed.", bytes);
    } else {
        ALOGW("out_fd is NULL, cannot write.");
    }
    return 0;
}


#endif




static uint32_t getNextMdBegin(uint8_t *frameBuf) {
    uint32_t header = 0;
    uint32_t result = 0;
    uint32_t offset = 0;

    if (frameBuf) {
        header = (frameBuf[0]<<24)|(frameBuf[1]<<16)|(frameBuf[2]<<8)|frameBuf[3];
        offset += 4;

        unsigned layer = (header >> 17) & 3;

        if (layer == 1) {
            //only for layer3, else next_md_begin = 0.

            if ((header & 0xFFE60000L) == 0xFFE20000L) {
                if (!(header & 0x00010000L)) {
                    offset += 2;
                    if (header & 0x00080000L) {
                        result = ((uint32_t)frameBuf[7]>>7)|((uint32_t)frameBuf[6]<<1);
                    } else {
                        result = frameBuf[6];
                    }
                } else {
                    if (header & 0x00080000L) {
                        result = ((uint32_t)frameBuf[5]>>7)|((uint32_t)frameBuf[4]<<1);
                    } else {
                        result = frameBuf[4];
                    }
                }
            }
        }
    }
    return result;
}


static uint32_t getCurFrameBitRate(uint8_t *frameBuf) {
    uint32_t header = 0;
    uint32_t bitrate = 0;

    if (frameBuf) {
        header = (frameBuf[0]<<24)|(frameBuf[1]<<16)|(frameBuf[2]<<8)|frameBuf[3];

        unsigned layer = (header >> 17) & 3;
        unsigned bitrate_index = (header >> 12) & 0x0f;
        unsigned version = (header >> 19) & 3;

        if (layer == 3) {
            // layer I
            static const int kBitrateV1[] = {
                32, 64, 96, 128, 160, 192, 224, 256,
                288, 320, 352, 384, 416, 448
            };

            static const int kBitrateV2[] = {
                32, 48, 56, 64, 80, 96, 112, 128,
                144, 160, 176, 192, 224, 256
            };

            bitrate =
                (version == 3 /* V1 */)
                ? kBitrateV1[bitrate_index - 1]
                : kBitrateV2[bitrate_index - 1];

        } else {
            // layer II or III
            static const int kBitrateV1L2[] = {
                32, 48, 56, 64, 80, 96, 112, 128,
                160, 192, 224, 256, 320, 384
            };

            static const int kBitrateV1L3[] = {
                32, 40, 48, 56, 64, 80, 96, 112,
                128, 160, 192, 224, 256, 320
            };

            static const int kBitrateV2[] = {
                8, 16, 24, 32, 40, 48, 56, 64,
                80, 96, 112, 128, 144, 160
            };

            if (version == 3 /* V1 */) {
                bitrate = (layer == 2 /* L2 */)
                          ? kBitrateV1L2[bitrate_index - 1]
                          : kBitrateV1L3[bitrate_index - 1];
            } else {
                // V2 (or 2.5)
                bitrate = kBitrateV2[bitrate_index - 1];
            }
        }
    }
    return bitrate;
}


static int set_audio_affinity(pid_t tid){
    cpu_set_t cpu_set;
    int cpu_num = 0;

    CPU_ZERO(&cpu_set);
    CPU_SET(cpu_num, &cpu_set);
    return sched_setaffinity(tid, sizeof(cpu_set), &cpu_set);
}

static bool getMPEGAudioFrameSize(
        uint32_t header, uint32_t *frame_size,
        int *out_sampling_rate, int *out_channels,
        int *out_bitrate, int *out_num_samples) {
    *frame_size = 0;

    if (out_sampling_rate) {
        *out_sampling_rate = 0;
    }

    if (out_channels) {
        *out_channels = 0;
    }

    if (out_bitrate) {
        *out_bitrate = 0;
    }

    if (out_num_samples) {
        *out_num_samples = 1152;
    }

    if ((header & 0xffe00000) != 0xffe00000) {
        return false;
    }

    unsigned version = (header >> 19) & 3;

    if (version == 0x01) {
        return false;
    }

    unsigned layer = (header >> 17) & 3;

    if (layer == 0x00) {
        return false;
    }

    unsigned protection = (header >> 16) & 1;

    unsigned bitrate_index = (header >> 12) & 0x0f;

    if (bitrate_index == 0 || bitrate_index == 0x0f) {
        // Disallow "free" bitrate.
        return false;
    }

    unsigned sampling_rate_index = (header >> 10) & 3;

    if (sampling_rate_index == 3) {
        return false;
    }

    static const int kSamplingRateV1[] = { 44100, 48000, 32000 };
    int sampling_rate = kSamplingRateV1[sampling_rate_index];
    if (version == 2 /* V2 */) {
        sampling_rate /= 2;
    } else if (version == 0 /* V2.5 */) {
        sampling_rate /= 4;
    }

    unsigned padding = (header >> 9) & 1;

    if (layer == 3) {
        // layer I

        static const int kBitrateV1[] = {
            32, 64, 96, 128, 160, 192, 224, 256,
            288, 320, 352, 384, 416, 448
        };

        static const int kBitrateV2[] = {
            32, 48, 56, 64, 80, 96, 112, 128,
            144, 160, 176, 192, 224, 256
        };

        int bitrate =
            (version == 3 /* V1 */)
                ? kBitrateV1[bitrate_index - 1]
                : kBitrateV2[bitrate_index - 1];

        if (out_bitrate) {
            *out_bitrate = bitrate;
        }

        *frame_size = (12000 * bitrate / sampling_rate + padding) * 4;

        if (out_num_samples) {
            *out_num_samples = 384;
        }
    } else {
        // layer II or III

        static const int kBitrateV1L2[] = {
            32, 48, 56, 64, 80, 96, 112, 128,
            160, 192, 224, 256, 320, 384
        };

        static const int kBitrateV1L3[] = {
            32, 40, 48, 56, 64, 80, 96, 112,
            128, 160, 192, 224, 256, 320
        };

        static const int kBitrateV2[] = {
            8, 16, 24, 32, 40, 48, 56, 64,
            80, 96, 112, 128, 144, 160
        };

        int bitrate;
        if (version == 3 /* V1 */) {
            bitrate = (layer == 2 /* L2 */)
                ? kBitrateV1L2[bitrate_index - 1]
                : kBitrateV1L3[bitrate_index - 1];

            if (out_num_samples) {
                *out_num_samples = 1152;
            }
        } else {
            // V2 (or 2.5)

            bitrate = kBitrateV2[bitrate_index - 1];
            if (out_num_samples) {
                *out_num_samples = (layer == 1 /* L3 */) ? 576 : 1152;
            }
        }

        if (out_bitrate) {
            *out_bitrate = bitrate;
        }

        if (version == 3 /* V1 */) {
            *frame_size = 144000 * bitrate / sampling_rate + padding;
        } else {
            // V2 or V2.5
            size_t tmp = (layer == 1 /* L3 */) ? 72000 : 144000;
            *frame_size = tmp * bitrate / sampling_rate + padding;
        }
    }

    if (out_sampling_rate) {
        *out_sampling_rate = sampling_rate;
    }

    if (out_channels) {
        int channel_mode = (header >> 6) & 3;

        *out_channels = (channel_mode == 3) ? 1 : 2;
    }

    return true;
}

static uint32_t U32_AT(const uint8_t *ptr) {
    return ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];
}


static uint32_t  getCurrentTimeUs()
{
   struct timeval tv;
   gettimeofday(&tv,NULL);
   return (tv.tv_sec* 1000000 + tv.tv_usec)/1000;
}



int is_compress_ready(struct compress *compress)
{
	return compress->ready;
}

int is_compress_running(struct compress *compress)
{
	return compress->running ? 1 : 0;
}

const char *compress_get_error(struct compress *compress)
{
	return compress->error;
}


int compress_set_gapless_metadata(struct compress *compress,
	struct compr_gapless_mdata *mdata)
{
        return 0;

}


static int compress_wait_event(struct compress * compress,pthread_mutex_t  *mutex)
{
    int ret = 0;
    if(compress->aud_fd) {
        pthread_mutex_unlock(mutex);
        ret = ioctl(compress->aud_fd, AUD_WAIT, NULL);
        pthread_mutex_lock(mutex);
    }
    return ret;
}

static int compress_signal(struct compress * compress)
{
    int ret = 0;
    pthread_cond_signal(&compress->cmd_cond);
    if(compress->aud_fd > 0) {
        ret = ioctl(compress->aud_fd, AUD_WAKEUP, NULL);
        if(ret) {
            ALOGE("compress_signal error %d",errno);
        }
    }
    return ret;
}


static void fade_init(struct fade_in *fade ,uint32_t max_count)
{
    fade->max_count = max_count;
}

static void fade_enable(struct fade_in *fade )
{
    fade->step = 1;
}

static void fade_disable(struct fade_in *fade )
{
    fade->step = fade->max_count;
}


static void fade_in_process(struct fade_in *fade,int16_t * buf, uint32_t bytes,uint32_t channels)
{
    int i = 0;
    uint32_t frames = 0;

    if(fade->step != fade->max_count) {
        if(channels == 2) {
            frames = bytes >> 2;
            for( i=0; i< frames; i++) {
                buf[2*i] = (int16_t)((int32_t)buf[2*i] *fade->step/fade->max_count);
                buf[2*i+1] = (int16_t)((int32_t)buf[2*i+1]*fade->step/fade->max_count);
                fade->step ++;
                if(fade->step == fade->max_count) {
                    break;
                }
            }
        }
        else if(channels == 1) {
            frames = bytes >> 1;
            for( i=0; i< frames; i++) {
                buf[i] = (int16_t)((int32_t)buf[i] *fade->step/fade->max_count);
                fade->step ++;
                if(fade->step == fade->max_count) {
                    break;
                }
            }
        }
    }
}

static void digital_gain_process(struct compress *compress,int16_t * buf, uint32_t bytes,uint32_t channels)
{
    int i = 0;
    uint32_t frames = 0;

    if((compress->left_vol!= AUDIO_OFFLOAD_PLAYBACK_VOLUME_MAX)
        || (compress->right_vol!= AUDIO_OFFLOAD_PLAYBACK_VOLUME_MAX)){
        if(channels == 2) {
            frames = bytes >> 2;
            for( i=0; i< frames; i++) {
                buf[2*i] = (int16_t)(((int32_t)buf[2*i] *compress->left_vol)>> 12);
                buf[2*i+1] = (int16_t)(((int32_t)buf[2*i+1]*compress->right_vol) >>12);
            }
        }
        else if(channels == 1) {
            frames = bytes >> 1;
            for( i=0; i< frames; i++) {
                buf[i] = (int16_t)(((int32_t)buf[i] *compress->left_vol) >> 12);
            }
        }
    }
}





static  int stream_comsume(struct  stream * stream, uint32_t bytes)
{
    if(stream->ring) {
        return ringbuffer_consume(stream->ring,bytes);
    }
    else {
        return -1;
    }
}

static int stream_copy(struct  stream * dst_stream,struct  stream * src_stream)
{
    dst_stream->ring = ringbuffer_copy(src_stream->ring);
    return 0;
}

static  int stream_write(struct  stream * stream, uint8_t *buf, uint32_t bytes, uint32_t *bytes_written)
{
    int ret = 0;
    ret = ringbuffer_write(stream->ring,buf,bytes);
    if(!ret) {
        *bytes_written = bytes;
    }
    else {
        *bytes_written = 0;
    }
    return ret;
}

static  int stream_read(struct  stream * stream, uint8_t *buf, uint32_t bytes,uint32_t * read)
{
    int ret = 0;
    ret = ringbuffer_read(stream->ring,buf,bytes);
    if(!ret) {
        *read = bytes;
    }
    else {
        *read = 0;
        ret = -1;
    }
    return ret;

}

static  int32_t stream_peek(struct  stream * stream, uint8_t *buf, uint32_t offset,uint32_t bytes)
{
    int ret = 0;
    if(stream->ring) {
        ret = ringbuffer_peek(stream->ring,buf, offset,bytes);
    }
    else {
        ret = -1;
    }
    return ret;
}

static  int stream_clean(struct  stream * stream)
{
    int ret = 0;
    if(stream->ring) {
        ret = ringbuffer_reset(stream->ring);
    }
    return ret;
}

static  uint32_t stream_total_bytes(struct  stream * stream)
{
    if(stream->ring) {
        return ringbuffer_total_write(stream->ring);
     }
    else {
        return 0;
    }
}

static  uint32_t stream_data_bytes(struct  stream * stream)
{
    if(stream->ring) {
        return ringbuffer_data_bytes(stream->ring);
    }
    else {
        return 0;
    }
}



static  int32_t  mp3_demux_frame(struct  stream  *stream,uint8_t * buf, uint32_t * frame_len,
                    uint32_t   *pNextbegin,uint32_t * pBitrate,uint32_t *pSamplerate,uint32_t *pChannel)
{
        int             ret = 0;
        uint8_t     header_buf[10];
        uint32_t    bytes_read;
        uint32_t    header = 0;
        uint32_t    frame_size;
        int             bitrate;
        int             num_samples;
        int             sample_rate;
        int             channels = 0;
        uint32_t    nextbegin;

        //    ALOGE(" compress->state %d,total_left %d",compress->state,total_left);
        ret = stream_peek(stream,header_buf,0,sizeof(header_buf));
        if(ret) {
            ALOGE("peter: stream_peek error out %d",ret);
            return -ERROR_NO_DATA;
        }
        // ALOGE("peter: stream_peek out %d",ret);
        header = U32_AT(header_buf);
        ret = getMPEGAudioFrameSize(header, &frame_size,&sample_rate, &channels,&bitrate, &num_samples);
        //ALOGE("peter:  frame_size %d, sample_rate: %d,channel: %d, bitrate : %d, num_damples: %d",frame_size,sample_rate,channels,bitrate,num_samples);

        if((ret == false) || (frame_size <= 0) || (frame_size > MP3_MAX_DATA_FRAME_LEN)) {
            ret = stream_read(stream,header_buf,1,&bytes_read);
            //out_dump_doing2(header_buf,bytes_read);
            //ALOGE("peter error framesize %d",frame_size,ret);
            return -ERROR_OTHER;
        }

        ret = stream_peek(stream,header_buf,frame_size,sizeof(header_buf));
        if(ret) {
            ALOGE("peter: stream_peek ret %d",ret);
            return -ERROR_NO_DATA;
        }
        nextbegin =getNextMdBegin(header_buf);

        ret = stream_read(stream,buf,frame_size,&bytes_read);
        if(ret) {
            ALOGE("stream_read error");
            return -ERROR_NO_DATA;;
        }
       // out_dump_doing2(buf,bytes_read);

        *frame_len = frame_size;
        *pBitrate = bitrate;
        *pSamplerate = sample_rate;
        *pChannel = channels;
        *pNextbegin = nextbegin;

        return ret;

}



static int32_t mp3_decode_process(struct compress *compress,int16_t* buf, uint32_t * bytes,uint32_t channels_req)
{
        int             i = 0;
        uint32_t    numOutBytes = 0;
        int             ret = 0;
        uint32_t    frame_size;
        int             bitrate;
        int             samplerate;
        int             channels = 0;
        uint32_t    nextbegin = 0;
        FRAME_DEC_T inputParam ;
        OUTPUT_FRAME_T outputFrame ;
        int  decoderRet = 0;
        int consecutive_err_cnt = 0;

        do {
            ret = mp3_demux_frame(&compress->stream,compress->mMaxFrameBuf, &frame_size,& nextbegin,&bitrate,&samplerate,&channels);
            if(-ERROR_OTHER == ret) {
                consecutive_err_cnt++;
            }
        } while((-ERROR_OTHER == ret) && consecutive_err_cnt < 2048);
        if(ret) {
            ALOGE("mp3_demux_frame error ret %d",ret);
            return ret;
        }

        //ALOGE("peter  ok 1 ");
        memset(&inputParam, 0, sizeof(FRAME_DEC_T));
        memset(&outputFrame, 0, sizeof(OUTPUT_FRAME_T));

        inputParam.next_begin = nextbegin;
        inputParam.bitrate = bitrate; //kbps
        inputParam.frame_buf_ptr = compress->mMaxFrameBuf;
        inputParam.frame_len = frame_size;

        outputFrame.pcm_data_l_ptr = compress->mLeftBuf;
        outputFrame.pcm_data_r_ptr =compress->mRightBuf;

        // ALOGE("peter: mNextMdBegin is %d,frame_size_dec: %d,bitrate:%d",mNextMdBegin,frame_size_dec,bitrate);

        //out_dump_doing2(inputParam.frame_buf_ptr,frame_size_dec);
        MP3_ARM_DEC_DecodeFrame(compress->mMP3DecHandle, &inputParam,&outputFrame, &decoderRet);
        //frames_dec++;
   // ALOGE("peter: decoderRet is %d,outputFrame: %d,frames_dec:%d",decoderRet,outputFrame.pcm_bytes,frames_dec);
        if(decoderRet != MP3_ARM_DEC_ERROR_NONE) { //decoder error
            ALOGE("MP3 decoder returned error %d, substituting silence", decoderRet);
            outputFrame.pcm_bytes = 1152; //samples number
        }

        if(decoderRet != MP3_ARM_DEC_ERROR_NONE) {
            ALOGE("MP3 decoder returned error %d, substituting silence", decoderRet);
            numOutBytes = outputFrame.pcm_bytes * sizeof(int16_t) *2;
            memset(buf, 0, numOutBytes);
        } else {
            for( i=0; i<outputFrame.pcm_bytes; i++) {
                if(channels_req == 1) {
                    numOutBytes = outputFrame.pcm_bytes * sizeof(int16_t) ;
                    if (2 == channels) {
                        buf[i] =(compress->mLeftBuf[i] + compress->mRightBuf[i])/2;
                    } else {
                        buf[i] = compress->mLeftBuf[i];
                    }
                }
                else if(channels_req == 2) {
                    numOutBytes = outputFrame.pcm_bytes * sizeof(int16_t) *2;
                    if (2 == channels) {
                        buf[2*i] =compress->mLeftBuf[i];
                        buf[2*i+1] =compress->mRightBuf[i];
                    } else {
                        buf[2*i] = compress->mLeftBuf[i];
                        buf[2*i+1] = compress->mLeftBuf[i];
                    }
                }
            }
        }

        *bytes = numOutBytes;

        compress->samplerate = samplerate;
        compress->channels = channels;
        compress->bitrate = bitrate;

       return 0;

}

static int32_t audio_process(struct compress *compress)
{
        int ret = 0;
        uint32_t avail = 0;
        uint32_t frame_size_dec= 0;
        uint32_t bytes_write = 0;
        uint32_t numOutBytes = 0;
        uint32_t bytes_written = 0;

        uint32_t bytes_read = 0;

        if(compress->pcm_bytes_left ==  0) {
            if(compress->play_cache_pcm_first) {
                ALOGE(" cache pcm: length:%d",stream_data_bytes(&compress->cache_pcm_stream));
                if(stream_data_bytes(&compress->cache_pcm_stream)) {
                    int32_t read_len = stream_data_bytes(&compress->cache_pcm_stream) > (1152*4)? (1152*4):stream_data_bytes(&compress->cache_pcm_stream);
                    ret = stream_read(&compress->cache_pcm_stream,compress->pOutputBuffer,read_len,&numOutBytes);
                    ALOGE(" cache pcm: length:%d,copy:%d,read_bytes:%d,ret: %d",
                    stream_data_bytes(&compress->cache_pcm_stream),read_len,numOutBytes,ret);
                    //out_dump_doing2((uint8_t*)compress->pOutputBuffer,numOutBytes);
                    if(!ret) {
                        //out_dump_doing2((uint8_t*)compress->pOutputBuffer,numOutBytes);
                    }
                }
                if((!stream_data_bytes(&compress->cache_pcm_stream)) || ret)
                {
                    compress->play_cache_pcm_first = 0;
                }
                compress->pOutPcmBuffer = compress->pOutputBuffer;
            }
            else {
                ret = mp3_decode_process( compress,compress->pOutputBuffer, &numOutBytes,compress->pcm_cfg.channels);
               // ALOGE("mp3_decode_process out:%d ,numOutBytes:%d,channels:%d",ret, numOutBytes,compress->pcm_cfg.channels);
                if(ret) {
                    numOutBytes = 0;
                    struct timespec cur_ts;
                    if(ret == -ERROR_NO_DATA) {
                        compress_write_data_notify(compress);
                        compress->no_src_data = 1;
                        if ((compress->end_ts.tv_sec == 0)&& (compress->end_ts.tv_nsec== 0)) {
                            clock_gettime(CLOCK_MONOTONIC, &compress->end_ts);
                        }
                    }
                    clock_gettime(CLOCK_MONOTONIC, &cur_ts);
                    ALOGE("peter: mp3_decode_process error %d, pcm left : %d",ret,stream_data_bytes(&compress->pcm_stream));
                    if((!stream_data_bytes(&compress->pcm_stream)) || ((cur_ts.tv_sec > (compress->end_ts.tv_sec +1)) && (cur_ts.tv_nsec >= compress->end_ts.tv_nsec))){
                        ALOGE("peter: pcm data is end and return to end playback %d",stream_data_bytes(&compress->pcm_stream));
                        return ret;
                    }
                }
                else {
                    compress->end_ts.tv_sec = 0;
                    compress->end_ts.tv_nsec = 0;
                    compress->no_src_data = 0;
                    compress->bytes_decoded += numOutBytes;

                    if((stream_data_bytes(&compress->stream) <= compress->config->fragment_size)
                           && (!compress->partial_drain_waiting)
                           && (!compress->drain_waiting)) {
                        compress_write_data_notify(compress);
                    }
                }
                if((compress->samplerate != compress->pcm_cfg.rate)&& numOutBytes) {
                    uint32_t in_frames = 0;
                    uint32_t out_frames = 0;
                    if((compress->resampler == NULL)
                        ||(compress->resample_in_samplerate != compress->samplerate)
                        || (compress->resample_out_samplerate != compress->pcm_cfg.rate)
                        || (compress->resample_channels != compress->pcm_cfg.channels)){

                        ALOGE("resampler recreate in samplerate:%d,out samplerate:%d, out channle:%d,orig:in rate:%d, orig: out rate:%d, orig channles:%d,max:%d,inbyts:%d",
                        compress->samplerate,compress->pcm_cfg.rate,compress->pcm_cfg.channels,
                        compress->resample_in_samplerate,compress->resample_out_samplerate,compress->resample_channels,compress->resample_in_max_bytes,numOutBytes);

                        if(compress->resampler) {
                            release_resampler(compress->resampler);
                            compress->resampler = NULL;
                        }
                        ret = create_resampler( compress->samplerate,
                            compress->pcm_cfg.rate,
                            compress->pcm_cfg.channels,
                            RESAMPLER_QUALITY_DEFAULT,
                            NULL,
                            &compress->resampler);
                        if (ret != 0) {
                            ALOGE("resample create failed");
                            compress->resampler = NULL;
                        }
                        else {
                            if(compress->resample_buffer) {
                                free(compress->resample_buffer);
                                compress->resample_buffer = NULL;
                            }
                            if(!compress->resample_buffer) {
                                compress->resample_buffer_len = numOutBytes*8*2;// (compress->pcm_cfg.rate*numOutBytes + compress->samplerate)/compress->samplerate + 1024;
                                compress->resample_buffer = malloc(compress->resample_buffer_len);
                                if(!compress->resample_buffer) {
                                    ALOGE("compress->resample_buffer malloc failed len:%d",compress->resample_buffer_len);
                                }
                                compress->resample_in_max_bytes = numOutBytes;
                            }
                            compress->resample_in_samplerate = compress->samplerate;
                            compress->resample_out_samplerate = compress->pcm_cfg.rate;
                            compress->resample_channels = compress->pcm_cfg.channels;
                        }
                    }

                    if(numOutBytes > compress->resample_in_max_bytes) {
                        if(compress->resample_buffer) {
                            free(compress->resample_buffer);
                            compress->resample_buffer = NULL;
                        }
                        compress->resample_buffer_len = numOutBytes*8*2;// (compress->pcm_cfg.rate*numOutBytes + compress->samplerate)/compress->samplerate + 1024;
                        compress->resample_buffer = malloc(compress->resample_buffer_len);
                        if(!compress->resample_buffer) {
                            ALOGE("compress->resample_buffer malloc failed len:%d",compress->resample_buffer_len);
                        }
                        compress->resample_in_max_bytes = numOutBytes;
                    }
                    in_frames = numOutBytes/(compress->pcm_cfg.channels<<1);
                    out_frames = compress->resample_buffer_len/(compress->pcm_cfg.channels<<1);
                    if(compress->resampler && compress->resample_buffer) {

                        compress->resampler->resample_from_input(compress->resampler,
                        (int16_t *)compress->pOutputBuffer,
                        &in_frames,
                        (int16_t *)compress->resample_buffer,
                        &out_frames);

                    }

                    numOutBytes = out_frames * (compress->pcm_cfg.channels<<1);
                    compress->pOutPcmBuffer = compress->resample_buffer;
                }
                else {
                    compress->pOutPcmBuffer = compress->pOutputBuffer;
                }
                if(ret == 0) {
                    fade_in_process(&compress->fade_start,compress->pOutPcmBuffer,numOutBytes,compress->pcm_cfg.channels);
                }
                ret = stream_write(&compress->pcm_stream,compress->pOutPcmBuffer,numOutBytes,&bytes_written);
                if(ret || (numOutBytes != bytes_written)) {
                    avail= pcm_avail_update(compress->pcm);
                    avail = pcm_frames_to_bytes(compress->pcm, avail);
                    ALOGE("audio_process:stream_write error ret %d,numOutBytes: %d,bytes_written: %d,avail: %d,data bytes:%d,bufsize:%d",
                    ret,numOutBytes,bytes_written,avail,stream_data_bytes(&compress->pcm_stream),pcm_get_buffer_size(compress->pcm));
                }
            }
             pthread_mutex_lock(&compress->pcm_mutex);
            if(compress->pcm) {
                avail= pcm_avail_update(compress->pcm);
                avail = pcm_frames_to_bytes(compress->pcm, avail);
            }
            else {
                avail = 0;
            }
             pthread_mutex_unlock(&compress->pcm_mutex);
            bytes_write = numOutBytes>avail ?avail:numOutBytes;

            compress->pcm_bytes_left = numOutBytes - bytes_write;
            compress->pcm_bytes_offset = 0;
        }
        else {
             pthread_mutex_lock(&compress->pcm_mutex);
            if(compress->pcm) {
                avail= pcm_avail_update(compress->pcm);
                avail = pcm_frames_to_bytes(compress->pcm, avail);
            }
            else {
                avail = 0;
            }
             pthread_mutex_unlock(&compress->pcm_mutex);
            bytes_write =compress->pcm_bytes_left> avail ? avail: compress->pcm_bytes_left;
            compress->pcm_bytes_left -= bytes_write;
        }     
      //  ALOGE("avail %d,bytes_write %d,compress->pcm_bytes_left %d,compress->pcm_bytes_offset:%d,compress->pcm:%x",avail,bytes_write,compress->pcm_bytes_left,compress->pcm_bytes_offset,(uint32_t)compress->pcm);

         pthread_mutex_lock(&compress->pcm_mutex);
        if(compress->pcm){
            uint32_t cur_samples = 0;
            //ret = stream_read_l(&compress->pcm_stream,test_buf,(cur_samples - compress->prev_samples)*4,&bytes_read);
            //out_dump_doing2((uint8_t*)test_buf,bytes_read);
            if(compress->pcm_time_ms < LARGE_PCM_BUFFER_TIME) {
                ret = stream_comsume(&compress->pcm_stream,bytes_write);
            }
            else {
                if(compress->position_ctl){
                    cur_samples = mixer_ctl_get_value(compress->position_ctl, 0);
                }
                ret = stream_comsume(&compress->pcm_stream,(cur_samples - compress->prev_samples)*(compress->pcm_cfg.channels<<1));
            }
            if(ret) {
                ALOGE("stream_comsume error ret : %d,cur:%d,prev: %d, bytes:%d,numOutBytes:%d,avail:%d,compress->bytes_decoded:%d,compress->no_src_data:%d",
                    ret,cur_samples,compress->prev_samples,(cur_samples - compress->prev_samples)*4,numOutBytes,avail,
                    compress->bytes_decoded,compress->no_src_data);

                if(compress->no_src_data == 1) {
                    pthread_mutex_unlock(&compress->pcm_mutex);
                    return -ERROR_NO_DATA;
                }
            }

           // ALOGE("cur_samples %d,prev_samples %d, total bytes:%d,data bytes: %d,cur_samples1:%d,",cur_samples,compress->prev_samples,stream_total_bytes(&compress->pcm_stream),stream_data_bytes(&compress->pcm_stream),(stream_total_bytes(&compress->pcm_stream) - stream_data_bytes(&compress->pcm_stream))/4);
            compress->prev_samples = cur_samples;
          // out_dump_doing2((uint8_t*)compress->pOutputBuffer +compress->pcm_bytes_offset,bytes_write);

           if(bytes_write &&compress->pcm) {
                fade_in_process(&compress->fade_resume,compress->pOutPcmBuffer +compress->pcm_bytes_offset,bytes_write,compress->pcm_cfg.channels);
              //  if(1) {//ss->pcm_cfg.rate == 16000) {
                   // out_dump_doing2((uint8_t*)compress->pOutPcmBuffer +compress->pcm_bytes_offset,bytes_write);
                //}
                if(compress->pcm_time_ms < LARGE_PCM_BUFFER_TIME) {
                    digital_gain_process(compress,(uint8_t*)compress->pOutPcmBuffer +compress->pcm_bytes_offset, bytes_write,compress->pcm_cfg.channels);
                }
                ret = pcm_mmap_write(compress->pcm,(uint8_t*)compress->pOutPcmBuffer +compress->pcm_bytes_offset,bytes_write);
                if(NULL!=compress->dumppcmfun){
                    compress->dumppcmfun((void *)(compress->pOutPcmBuffer  +compress->pcm_bytes_offset),
                        bytes_write,DUMP_MUSIC_HWL_OFFLOAD_VBC);
                }
                if(ret < 0) {
                    ALOGE("pcm_mmap_write error");
                    pcm_close(compress->pcm);
                    compress->pcm= pcm_open(compress->card, compress->device, PCM_OUT| PCM_MMAP |PCM_NOIRQ |PCM_MONOTONIC, &compress->pcm_cfg);
                    if(!pcm_is_ready(compress->pcm)) {
                        pcm_close(compress->pcm);
                        compress->pcm = NULL;
                    }
                    compress->prev_samples = 0;
                }
                if(compress->pcm_bytes_left) {
                    compress->pcm_bytes_offset += bytes_write;
                }
            }
        }
        pthread_mutex_unlock(&compress->pcm_mutex);
        return 0;
}


static long compress_get_wait_time(struct compress *compress)
{
    long wait_time = 0;
    pthread_mutex_lock(&compress->pcm_mutex);
    if(compress->pcm) {
        uint32_t avail = 0;
        avail = pcm_avail_update(compress->pcm);
        if(compress->no_src_data == 0) {
            if(avail < compress->pcm_cfg.avail_min) {
                wait_time = (compress->pcm_cfg.avail_min - avail) / compress->noirq_frames_per_msec;
                if (wait_time < 20) {
                    wait_time = 20;
                }
            }
            else {
                wait_time = 50;
            }
        }
        else {
            wait_time = (pcm_get_buffer_size(compress->pcm) - pcm_avail_update(compress->pcm))/compress->noirq_frames_per_msec;
            if(wait_time >= 150) {
                wait_time = wait_time-100;
            }
            else {
                wait_time = 50;
            }
            ALOGI("peter: nor src and wait time:%d",wait_time);
        }
    }
    else {
        wait_time = 50;
    }
    pthread_mutex_unlock(&compress->pcm_mutex);
    return (wait_time * 1000000);
}

static int compress_pcm_state_check(struct compress *compress)
{
    int state = 0;
    pthread_mutex_lock(&compress->pcm_mutex);
    if(compress->pcm) {
        state = pcm_state(compress->pcm);
       if(pcm_state(compress->pcm) != PCM_STATE_RUNNING) {
            ALOGE("error: pcm_state %d",state);
            pcm_close(compress->pcm);
            compress->pcm= pcm_open(compress->card, compress->device, PCM_OUT| PCM_MMAP |PCM_NOIRQ |PCM_MONOTONIC, &compress->pcm_cfg);
            if(!pcm_is_ready(compress->pcm)) {
                pcm_close(compress->pcm);
                compress->pcm = NULL;
            }
            compress->prev_samples = 0;
        }
    }
    else {
        compress->pcm= pcm_open(compress->card, compress->device, PCM_OUT| PCM_MMAP |PCM_NOIRQ |PCM_MONOTONIC, &compress->pcm_cfg);
        if(!pcm_is_ready(compress->pcm)) {
            pcm_close(compress->pcm);
            compress->pcm = NULL;
        }
        compress->prev_samples = 0;
    }
    pthread_mutex_unlock(&compress->pcm_mutex);
    return 0;
}

static void compress_write_data_notify(struct compress *compress)
{
    pthread_mutex_lock(&compress->cond_mutex);
    compress->buffer_ready = 1;
    pthread_cond_signal(&compress->cond);
    pthread_mutex_unlock(&compress->cond_mutex);
}


static void compress_drain_notify(struct compress *compress)
{
    pthread_mutex_lock(&compress->drain_mutex);
    compress->drain_ok = 1;
    if(compress->drain_waiting == 1) {
        pthread_cond_signal(&compress->drain_cond);
    }
    pthread_mutex_unlock(&compress->drain_mutex);

    pthread_mutex_lock(&compress->partial_drain_mutex);
    compress->partial_drain_ok = 1;
    if(compress->partial_drain_waiting == 1) {
        pthread_cond_signal(&compress->partial_drain_cond);
    }
    pthread_mutex_unlock(&compress->partial_drain_mutex);

}


static int compress_cmd_process(struct compress *compress, struct compress_cmd  *cmd)
{
    int ret = 0;
    if(cmd == NULL) {
        return 0;
    }
    if(cmd) {
    ALOGE("cmd-cmd is %d,compress->is_start:%d",cmd->cmd,compress->is_start);
    switch(cmd->cmd) {
        case COMPERSS_CMD_STOP:
            compress->loop_state = STATE_STOP;
            stream_clean(&compress->stream);
            compress->pcm_bytes_left = 0;
            compress->pcm_bytes_offset = 0;
            compress->no_src_data = 0;
            compress->bytes_decoded = 0;
            compress->is_start = 0;
            if(compress->resampler) {
                release_resampler(compress->resampler);
                compress->resampler = NULL;
            }
            stream_clean(&compress->pcm_stream);
            compress_write_data_notify(compress);
            compress_drain_notify(compress);
            pthread_mutex_lock(&compress->pcm_mutex);
            if(compress->pcm){
                pcm_close(compress->pcm);
                compress->pcm = NULL;
            }
            pthread_mutex_unlock(&compress->pcm_mutex);
            if(cmd->is_sync) {
                cmd->ret = ret;
                sem_post(&cmd->sync_sem);
            }
            else {
                free(cmd);
            }
            ret = 0;
            break;
        case COMPERSS_CMD_START:
            compress->loop_state = STATE_START;
            compress->drain_ok = 0;
            compress->partial_drain_ok = 0;
            compress->no_src_data = 0;
            compress->bytes_decoded = 0;
            compress->is_start = 1;
            compress->pcm_bytes_left = 0;
            compress->pcm_bytes_offset = 0;
            compress->prev_samples = 0;

            if(compress->mMP3DecHandle) {
                MP3_ARM_DEC_InitDecoder(compress->mMP3DecHandle);
            }
            pthread_mutex_lock(&compress->pcm_mutex);
            if(!compress->pcm) {
                compress->pcm= pcm_open(compress->card, compress->device, PCM_OUT| PCM_MMAP |PCM_NOIRQ |PCM_MONOTONIC, &compress->pcm_cfg);
                compress->prev_samples = 0;
                if(pcm_is_ready(compress->pcm)) {
                    ret = 0;
                    ALOGE("peter: pcm_open ok");
                }
                else {
                    ALOGE(" COMPERSS_CMD_START pcm open failed");
                    pcm_close(compress->pcm);
                    compress->pcm = NULL;
                }
                compress->pcm_time_ms =(uint32_t)((uint64_t)compress->pcm_cfg.period_count*compress->pcm_cfg.period_size* 1000)/compress->pcm_cfg.rate;
                ALOGE("peter:compress->pcm_time_ms is %d",compress->pcm_time_ms);
            }
            pthread_mutex_unlock(&compress->pcm_mutex);

            fade_init(&compress->fade_resume,compress->pcm_cfg.rate*100/1000);
            fade_init(&compress->fade_start,compress->pcm_cfg.rate*100/1000);
            fade_enable(&compress->fade_start);
            fade_disable(&compress->fade_resume);

            stream_clean(&compress->pcm_stream);
            compress->play_cache_pcm_first = 0;

            if(cmd->is_sync) {
                cmd->ret = ret;
                sem_post(&cmd->sync_sem);
            }
            else {
                free(cmd);
            }
            ret = 0;
            break;
        case COMPERSS_CMD_PAUSE:
            compress->loop_state = STATE_PAUSE;
            compress->is_start = 0;
            uint32_t cur_samples = 0;
            if(compress->pcm_time_ms >= LARGE_PCM_BUFFER_TIME) {
                if(compress->position_ctl) {
                    cur_samples = mixer_ctl_get_value(compress->position_ctl, 0);
                }
                ALOGE("cur_samples %d,prev_samples %d, total bytes:%d,data bytes: %d,cur_samples1:%d,",cur_samples,compress->prev_samples,stream_total_bytes(&compress->pcm_stream),stream_data_bytes(&compress->pcm_stream),(stream_total_bytes(&compress->pcm_stream) - stream_data_bytes(&compress->pcm_stream))/4);
                ret = stream_comsume(&compress->pcm_stream,(cur_samples - compress->prev_samples)*(compress->pcm_cfg.channels<<1));
                if(ret) {
                    ALOGE("stream_comsume error ret : %d",ret);
                }

                ALOGE("cur_samples %d,prev_samples %d, total bytes:%d,data bytes: %d,cur_samples1:%d,",cur_samples,compress->prev_samples,stream_total_bytes(&compress->pcm_stream),stream_data_bytes(&compress->pcm_stream),(stream_total_bytes(&compress->pcm_stream) - stream_data_bytes(&compress->pcm_stream))/4);
                ALOGE("peter: COMPERSS_CMD_PAUSE: cur_samples is %d",cur_samples);
            }
            pthread_mutex_lock(&compress->pcm_mutex);
            if(compress->pcm){
                pcm_close(compress->pcm);
                compress->pcm = NULL;
            }
            pthread_mutex_unlock(&compress->pcm_mutex);
            compress->pcm_bytes_left = 0;
            compress->pcm_bytes_offset = 0;
            compress->prev_samples = 0;

            if(cmd->is_sync) {
                cmd->ret = 0;
                sem_post(&cmd->sync_sem);
            }
            else {
                free(cmd);
            }
            ret = -1;
            break;
        case COMPERSS_CMD_RESUME:
            compress->loop_state = STATE_RESUME;
            compress->is_start = 1;
            pthread_mutex_lock(&compress->pcm_mutex);
            if(!compress->pcm) {
                compress->pcm= pcm_open(compress->card, compress->device, PCM_OUT| PCM_MMAP |PCM_NOIRQ |PCM_MONOTONIC, &compress->pcm_cfg);
                if(!pcm_is_ready(compress->pcm)) {
                    ALOGE("peter: pcm_open error:%s",pcm_get_error(compress->pcm));
                    pcm_close(compress->pcm);
                    compress->pcm = NULL;
                }
                else {
                    ret = 0;
                    ALOGE("peter: pcm_open ok");
                }
                compress->pcm_time_ms =(uint32_t)((uint64_t)compress->pcm_cfg.period_count*compress->pcm_cfg.period_size* 1000)/compress->pcm_cfg.rate;
                ALOGE("peter:compress->pcm_time_ms is %d",compress->pcm_time_ms);
            }
            pthread_mutex_unlock(&compress->pcm_mutex);

            fade_init(&compress->fade_resume,compress->pcm_cfg.rate*100/1000);
            fade_init(&compress->fade_start,compress->pcm_cfg.rate*100/1000);
            fade_disable(&compress->fade_start);
            fade_enable(&compress->fade_resume);

            if(compress->position_ctl && (compress->pcm_time_ms >= LARGE_PCM_BUFFER_TIME)) {
                cur_samples = mixer_ctl_get_value(compress->position_ctl, 0);
            }

            if(compress->cache_pcm_stream.ring) {
                ringbuffer_free(compress->cache_pcm_stream.ring);
                compress->cache_pcm_stream.ring = NULL;
            }
            stream_copy(&compress->cache_pcm_stream,&compress->pcm_stream);
            ALOGE("cur_samples %d,prev_samples %d, total bytes:%d,data bytes: %d,cur_samples1:%d,",cur_samples,compress->prev_samples,stream_total_bytes(&compress->pcm_stream),stream_data_bytes(&compress->pcm_stream),(stream_total_bytes(&compress->pcm_stream) - stream_data_bytes(&compress->pcm_stream))/4);
            compress->prev_samples = 0;
            compress->play_cache_pcm_first = 1;
            if(cmd->is_sync){
                cmd->ret = 0;
                sem_post(&cmd->sync_sem);
            }
            else {
                free(cmd);
            }
            ret = 0;
            break;
        case COMPERSS_CMD_CHANGE_DEV:
        {
            struct dev_set_param  *param = cmd->param;
            pthread_mutex_lock(&compress->position_mutex);
            compress->card = param->card;
            compress->device = param->device;
            compress->mixer = param->mixerctl;

            if(param->config) {
                memcpy(&compress->pcm_cfg, param->config, sizeof(struct pcm_config));
            }
            if(!param->config->avail_min) {
                compress->pcm_cfg.avail_min = param->config->period_size;
            }
            compress->noirq_frames_per_msec = param->config->rate / 1000;

            if(compress->pcm_stream.ring) {
                ringbuffer_free(compress->pcm_stream.ring);
                compress->pcm_stream.ring = NULL;
            }

            if(compress->cache_pcm_stream.ring) {
                ringbuffer_free(compress->cache_pcm_stream.ring);
                compress->cache_pcm_stream.ring = NULL;
            }
            compress->pcm_stream.ring = ringbuffer_init(param->config->period_count*param->config->period_size*4 + (1152*10*4));
            if(compress->pcm_stream.ring == NULL) {
                ALOGE("error:COMPERSS_CMD_CHANGE_DEV (compress->pcm_stream.ring == NULL)");
            }
            const char *mixer_ctl_name = "PCM_TOTAL_DEEPBUF";
            compress->position_ctl = mixer_get_ctl_by_name(compress->mixer, mixer_ctl_name);
            if (!compress->position_ctl) {
                ALOGE("%s: Could not get ctl for mixer cmd - %s",
                __func__, mixer_ctl_name);
            }
            pthread_mutex_lock(&compress->pcm_mutex);
            if(compress->pcm){
                ALOGE("COMPERSS_CMD_CHANGE_DEV  pcm close in");
                pcm_close(compress->pcm);
                compress->pcm = NULL;
                ALOGE("COMPERSS_CMD_CHANGE_DEV  pcm close out");
            }
            pthread_mutex_unlock(&compress->pcm_mutex);

            if(compress->resample_buffer) {
                free(compress->resample_buffer);
                compress->resample_buffer = NULL;
            }
            if(compress->resampler) {
                release_resampler(compress->resampler);
                compress->resampler = NULL;
            }

            if(cmd->is_sync) {
                cmd->ret = 0;
                sem_post(&cmd->sync_sem);
            }
            else {
                if(cmd->param) {
                    free(cmd->param);
                }
                free(cmd);
            }
            pthread_mutex_unlock(&compress->position_mutex);
            ret = 0;
        }
            break;
        case COMPRESS_CMD_DRAIN:
            compress->loop_state = STATE_DRAIN;
            if(cmd->is_sync) {
                cmd->ret = 0;
                sem_post(&cmd->sync_sem);
            }
            else {
            free(cmd);
            }
            ret = 0;
            break;
        case COMPRESS_CMD_PARTIAL_DRAIN:
            compress->loop_state = STATE_PARTIAL_DRAIN;
            if(cmd->is_sync) {
                cmd->ret = 0;
                sem_post(&cmd->sync_sem);
            }
            else {
                free(cmd);
            }
            ret = 0;
            break;
        case COMPERSS_CMD_CLOSE:
            compress->loop_state = STATE_CLOSE;
            if(cmd->is_sync) {
                cmd->ret = 0;
                sem_post(&cmd->sync_sem);
            }
            else {
                free(cmd);
            }
            ret = -1;
        break;
        case COMPERSS_CMD_SEND_DATA:
            if(cmd->is_sync) {
                cmd->ret = 0;
                sem_post(&cmd->sync_sem);
            }
            else {
                free(cmd);
            }
            if(!compress->is_start){
                ret = -1;
            }
            break;
        default:
            if(cmd->is_sync) {
                cmd->ret = 0;
                sem_post(&cmd->sync_sem);
            }
            else {
                free(cmd);
            }
            if(!compress->is_start){
                ret = -1;
            }
            break;

        }
    }
    return ret;
}


static void *thread_func(void * param)
{
    int ret = 0;
    ALOGE(" thread_func in");
    struct compress *compress  = param;
    struct compress_cmd  *cmd;
    struct listnode *item;
    pid_t tid;

    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_AUDIO);
    set_sched_policy(0, SP_FOREGROUND);
    prctl(PR_SET_NAME, (unsigned long)"Audio Offload decoder Thread", 0, 0, 0);
    if(!compress) {
            return NULL;
    }
#ifdef SET_OFFLOAD_AFFINITY
    tid = gettid();
    ret = set_audio_affinity(tid);
    if(ret < 0){
        ALOGE("set playback affinity failed for tid:%d, error:%d", tid, ret);
    } else {
        ALOGI("set playback affinity successfully for tid:%d, error:%d", tid, ret);
    }
#endif

    clock_gettime(CLOCK_MONOTONIC, &compress->ts);
    while(!((compress->state == STATE_EXIT )
            &&(compress->loop_state == STATE_CLOSE))) {
        cmd = NULL;
        pthread_mutex_lock(&compress->cmd_mutex);
        if (list_empty(&compress->cmd_list)) {
            if((compress->state == STATE_OPEN)
                || (compress->state == STATE_PAUSE)
                ||(compress->state == STATE_STOP)){
                ALOGE("%s: Waiting for signal ", __func__);
                ret = pthread_cond_wait(&compress->cmd_cond, &compress->cmd_mutex);
               // compress_wait_event(compress,&compress->cmd_mutex);
            }
            else if((compress->state == STATE_START)
                        ||(compress->state == STATE_RESUME)){
                if(compress->pcm_bytes_left || compress->no_src_data) {
                    if(stream_data_bytes(&compress->pcm_stream)) {
                        long wait_time = compress_get_wait_time(compress);
                        clock_gettime(CLOCK_MONOTONIC, &compress->ts);
                        compress->ts.tv_nsec += wait_time;
                        if(compress->ts.tv_nsec >1000000000) {
                            compress->ts.tv_nsec -= 1000000000;
                            compress->ts.tv_sec += 1;
                        }
                        //ALOGE("wait in:%ld,",wait_time);
#ifndef ANDROID4X
                        ret = pthread_cond_timedwait(&compress->cmd_cond, &compress->cmd_mutex, &compress->ts);
#else
                        ret = pthread_cond_timedwait_monotonic(&compress->cmd_cond, &compress->cmd_mutex, &compress->ts);
#endif
                        //ALOGE("wait out:%ld",ret);
                        if(ret == ETIMEDOUT){
                            compress_pcm_state_check(compress);
                        }
                    }
                    else {
                        clock_gettime(CLOCK_MONOTONIC, &compress->ts);
                        compress->ts.tv_sec += 1;
                        ALOGE("wait in: no src data,%d, wait for 1 second",compress->no_src_data);
#ifndef ANDROID4X
                        ret = pthread_cond_timedwait(&compress->cmd_cond, &compress->cmd_mutex,  &compress->ts);
#else
                        ret = pthread_cond_timedwait_monotonic(&compress->cmd_cond, &compress->cmd_mutex,  &compress->ts);
#endif
                        //ret = pthread_cond_wait(&compress->cmd_cond, &compress->cmd_mutex);
                        ALOGE("wait out:%ld",ret);
                    }
                }
                else if(compress->no_src_data) {
                }
            }
#if 0
          ALOGE("peter: ret : %d,compress->state %d,compress->pcm_bytes_left :%d,compress->loop_state:%d,,no_src_data:%d,partial_w:%d,drain_w:%d",
            ret,compress->state,compress->pcm_bytes_left,compress->loop_state,compress->no_src_data,
            compress->partial_drain_waiting,compress->drain_waiting);
#endif
        }

        if (!list_empty(&compress->cmd_list)) {
            /* get the command from the list, then process the command */
            item = list_head(&compress->cmd_list);
            cmd = node_to_item(item, struct compress_cmd, node);
            list_remove(item);
        }
        pthread_mutex_unlock(&compress->cmd_mutex);

        if(cmd) {
            ret = compress_cmd_process(compress, cmd);
            if(ret) {
                continue;
            }
        }

        if(compress->is_start)
        {
            pthread_mutex_lock(&compress->position_mutex);
            ret = audio_process(compress);
            pthread_mutex_unlock(&compress->position_mutex);
            if(ret == -ERROR_NO_DATA) {
                compress_drain_notify(compress);
            }
        }
    }
    ALOGI(" thread_func exit 1");
    while (!list_empty(&compress->cmd_list)) {
        struct compress_cmd  *cmd;
        struct listnode *item;
        /* get the command from the list, then process the command */
        item = list_head(&compress->cmd_list);
        cmd = node_to_item(item, struct compress_cmd, node);
        list_remove(item);
        if(cmd->is_sync) {
            cmd->ret = 0;
            sem_post(&cmd->sync_sem);
        }
        else {
            if(cmd->param) {
                free(cmd->param);
            }
            free(cmd);
        }
    }
    ALOGI(" thread_func exit 2");
    return NULL;
}

static int compress_send_cmd(struct compress *compress, COMPRESS_CMD_T command,void * parameter, uint32_t is_sync)
{
    int ret = 0;
    struct compress_cmd *cmd = (struct compress_cmd *)calloc(1, sizeof(struct compress_cmd));

    ALOGE("%s, cmd:%d",
        __func__, command);
    /* add this command to list, then send signal to offload thread to process the command list */
    cmd->cmd = command;
    cmd->is_sync = is_sync;
    cmd->param = parameter;
    sem_init(&cmd->sync_sem, 0, 0);
    pthread_mutex_lock(&compress->cmd_mutex);
    list_add_tail(&compress->cmd_list, &cmd->node);
    compress_signal(compress);
    pthread_mutex_unlock(&compress->cmd_mutex);
    if(is_sync) {
        if((command == COMPERSS_CMD_STOP)
            ||(command == COMPERSS_CMD_CLOSE)) {
           // stream_break(&compress->stream);
        }
        sem_wait(&cmd->sync_sem);
        ret = cmd->ret;
        if(cmd->param) {
            free(cmd->param);
        }
        free(cmd);
    }
    return ret;
}

int32_t compress_set_gain(struct compress *compress,long left_vol, long right_vol)
{
    if(!compress) {
        return -1;
    }
    ALOGE("%s in:left_vol:%d,right_vol:%d",__FUNCTION__,left_vol,right_vol);
    if(left_vol <= AUDIO_OFFLOAD_PLAYBACK_VOLUME_MAX){
        compress->left_vol = left_vol;
    }

     if(right_vol <= AUDIO_OFFLOAD_PLAYBACK_VOLUME_MAX) {
        compress->right_vol = right_vol;
    }
    return 0;
}


int32_t compress_set_dev(struct compress *compress,unsigned int card, unsigned int device,
		struct pcm_config  *config,void * mixerctl)
{
    int ret = 0;
    int need_reset = 0;
    if(!compress) {
	    ALOGE("%s error: compres is null",__FUNCTION__);
        return -1;
	}
    ALOGE("%s in",__FUNCTION__);
    pthread_mutex_lock(&compress->lock);
    if (!is_compress_ready(compress)) {
        pthread_mutex_unlock(&compress->lock);
        return -1;
    }
    if(mixerctl == NULL) {
        ALOGE("%s mixer is NULL",__FUNCTION__);
        pthread_mutex_unlock(&compress->lock);
        return -1;
    }
    if((compress->card !=  card)
            || (compress->device != device)) {
        struct dev_set_param  *param = NULL;
        param = calloc(1, sizeof(struct dev_set_param));
        if(!param) {
            ALOGE("error mallock %s",__FUNCTION__);
            goto ERROR;
        }
        param->card = card;
        param->device = device;
        param->config = config;
        param->mixerctl = mixerctl;
        ret = compress_send_cmd(compress, COMPERSS_CMD_CHANGE_DEV,param,1);
    }
    else {
        ALOGE("dev config is same");
    }

ERROR:
    pthread_mutex_unlock(&compress->lock);
    ALOGE("%s out",__FUNCTION__);
    return 0;
}




struct compress *compress_open(unsigned int card, unsigned int device,
		unsigned int flags, struct compr_config *config)
{
	struct compress *compress;
	int ret = 0;
    ALOGE("compress_open in");

	if (!config) {
		ALOGE("%s error: config is null",__FUNCTION__);
		return NULL;
	}
	if ((config->fragment_size == 0) || (config->fragments == 0)) {
		ALOGE("%s param is error",__FUNCTION__);
		return NULL;
	}

	compress = calloc(1, sizeof(struct compress));
	if (!compress) {
		ALOGE("%s param is compress NULL",__FUNCTION__);
		return NULL;
	}

    memset(compress,0, sizeof(struct compress));

    pthread_mutex_init(&compress->lock, NULL);
    pthread_cond_init(&compress->cond, NULL);
    pthread_mutex_init(&compress->cond_mutex,NULL);
    pthread_cond_init(&compress->drain_cond, NULL);
    pthread_mutex_init(&compress->drain_mutex,NULL);
    pthread_mutex_init(&compress->pcm_mutex,NULL);
    pthread_mutex_init(&compress->position_mutex,NULL);
    pthread_cond_init(&compress->partial_drain_cond, NULL);
    pthread_mutex_init(&compress->partial_drain_mutex,NULL);
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
#ifndef ANDROID4X
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
#endif
    pthread_cond_init(&compress->cmd_cond, &attr);
    pthread_mutex_init(&compress->cmd_mutex,NULL);
    list_init(&compress->cmd_list);
	compress->config = calloc(1, sizeof(struct compr_config));
	if (!compress->config)
		goto error_fail;
	memcpy(compress->config, config, sizeof(struct compr_config));
	compress->flags = flags;
	if (!((flags & COMPRESS_OUT) || (flags & COMPRESS_IN))) {
	    ALOGE("%s can't deduce device direction from given flags",__FUNCTION__);
		goto error_fail;
	}
    ret = MP3_ARM_DEC_Construct(&compress->mMP3DecHandle);
    if(ret) {
        ALOGE("MP3_ARM_DEC_Construct failed");
        goto error_fail;
    }
    compress->stream.ring = ringbuffer_init(config->fragments * config->fragment_size);
    compress->state = STATE_OPEN;
    pthread_attr_t threadattr;
    pthread_attr_init(&threadattr);
    pthread_attr_setdetachstate(&threadattr, PTHREAD_CREATE_JOINABLE);
    ret = pthread_create(&compress->thread, &threadattr,
    thread_func, (void *)compress);
	if (ret) {
		ALOGE("bt sco : duplicate thread create fail, code is %d", ret);
        goto error_fail;
    }
    pthread_attr_destroy(&threadattr);
    
    compress->aud_fd = -1;//open(SPRD_AUD_DRIVER,O_RDWR);
    if(compress->aud_fd <= 0) {
       // ALOGE("bt sco : compress->aud_fd %s open fail, code is %d", SPRD_AUD_DRIVER,ret);
      //  goto error_fail;
    }
	compress->ready = 1;
    ALOGE("compress_open out");
	return compress;

error_fail:
	if(compress->config) {
		free(compress->config);
	}
	if(compress->pcm) {
		pcm_close(compress->pcm);
	}
	if(compress->aud_fd >0) {
        close(compress->aud_fd);
    }
	free(compress);
	return NULL;
}

int compress_write(struct compress *compress, const void *buf, unsigned int size)
{
	int total = 0, ret;
	uint32_t bytes_written = 0;
	if(!compress) {
	    ALOGE("%s error: compres is null",__FUNCTION__);
        return -1;
	}
	//ALOGE("compress_write in");
	pthread_mutex_lock(&compress->lock);
	if (!(compress->flags & COMPRESS_IN)) {
		pthread_mutex_unlock(&compress->lock);
		return -1;
	}
	if (!is_compress_ready(compress)) {
	    ALOGE("compress_write out total 1 :%d",total);
		pthread_mutex_unlock(&compress->lock);
		return -1;
	}
	ret = stream_write(&compress-> stream, buf, size, &total);
	if(total) {
	   // out_dump_doing3(buf,total);
        ret = compress_send_cmd(compress, COMPERSS_CMD_SEND_DATA,NULL,0);
	}
	pthread_mutex_unlock(&compress->lock);
	//ALOGE("compress_write out total:%d",total);
	return total;
}

int compress_start(struct compress *compress)
{
	int ret = 0;
	pthread_attr_t attr;
	if(!compress) {
	    ALOGE("%s error: compres is null",__FUNCTION__);
        return -1;
	}
	pthread_mutex_lock(&compress->lock);
	if (!is_compress_ready(compress)) {
		pthread_mutex_unlock(&compress->lock);
		return -1;
	}
	if(compress->state == STATE_START) {
        pthread_mutex_unlock(&compress->lock);
        return 0;
	}
	compress->state = STATE_START;
    compress->running = 1;
	ret = compress_send_cmd(compress, COMPERSS_CMD_START,NULL,1);
	
	compress->start_time = getCurrentTimeUs();
	pthread_mutex_unlock(&compress->lock);
	return 0;
	
	
}

int compress_pause(struct compress *compress)
{
    int ret = 0;
    if(!compress) {
	    ALOGE("%s error: compres is null",__FUNCTION__);
        return -1;
	}
    ALOGE("%s in",__FUNCTION__);
    pthread_mutex_lock(&compress->lock);
    if (!is_compress_running(compress)) {
        pthread_mutex_unlock(&compress->lock);
        return 0;
    }
    compress->state = STATE_PAUSE;
	ret = compress_send_cmd(compress, COMPERSS_CMD_PAUSE,NULL,1);

	pthread_mutex_unlock(&compress->lock);
	ALOGE("%s out",__FUNCTION__);
	return 0;
}

int compress_resume(struct compress *compress)
{
    int ret = 0;
    if(!compress) {
	    ALOGE("%s error: compres is null",__FUNCTION__);
        return -1;
	}
    ALOGE("%s in",__FUNCTION__);
    pthread_mutex_lock(&compress->lock);
    if(compress->state != STATE_PAUSE) {
        pthread_mutex_unlock(&compress->lock);
        return 0;
    }
    compress->state = STATE_RESUME;
    if (!is_compress_running(compress)) {
        pthread_mutex_unlock(&compress->lock);
        return 0;
    }
   ret = compress_send_cmd(compress, COMPERSS_CMD_RESUME,NULL,1);

    pthread_mutex_unlock(&compress->lock);
    ALOGE("%s out",__FUNCTION__);
    return 0;
}

int compress_resume_async(struct compress *compress,int sync)
{
    int ret = 0;
    if(!compress) {
	    ALOGE("%s error: compres is null",__FUNCTION__);
        return -1;
	}
    ALOGE("%s in",__FUNCTION__);
    pthread_mutex_lock(&compress->lock);
    compress->state = STATE_RESUME;
    if (!is_compress_running(compress)) {
        pthread_mutex_unlock(&compress->lock);
        return 0;
    }
    ret = compress_send_cmd(compress, COMPERSS_CMD_RESUME,NULL,0);

    pthread_mutex_unlock(&compress->lock);
    ALOGE("%s out",__FUNCTION__);
    return 0;
}



int compress_drain(struct compress *compress)
{
    int ret = 0;
    if(!compress) {
	    ALOGE("%s error: compres is null",__FUNCTION__);
        return -1;
	}
    ALOGE("%s in",__FUNCTION__);
    pthread_mutex_lock(&compress->lock);
	if (!is_compress_running(compress)) {
	    pthread_mutex_unlock(&compress->lock);
		return -1;
	}
	ret = compress_send_cmd(compress, COMPRESS_CMD_DRAIN,NULL,1);
	pthread_mutex_unlock(&compress->lock);
	ALOGE("%s out half",__FUNCTION__);
	pthread_mutex_lock(&compress->drain_mutex);
	if(!compress->drain_ok){
	    compress->drain_waiting = 1;
	    
        ALOGE("%s in 1",__FUNCTION__);
        pthread_cond_wait(&compress->drain_cond, &compress->drain_mutex);
        ALOGE("%s out 1",__FUNCTION__);
        compress->drain_waiting = 0;
	}
	compress->drain_ok = 0;
	pthread_mutex_unlock(&compress->drain_mutex);
    
    ALOGE("%s out",__FUNCTION__);
	return 0;
}

int compress_partial_drain(struct compress *compress)
{
    int ret = 0;
    if(!compress) {
	    ALOGE("%s error: compres is null",__FUNCTION__);
        return -1;
	}
    ALOGE("%s in",__FUNCTION__);
    pthread_mutex_lock(&compress->lock);
	if (!is_compress_running(compress)){
	    pthread_mutex_unlock(&compress->lock);	
		return 0;
    }
    ret = compress_send_cmd(compress, COMPRESS_CMD_PARTIAL_DRAIN,NULL,1);
    pthread_mutex_unlock(&compress->lock);
    pthread_mutex_lock(&compress->partial_drain_mutex);
    if(!compress->partial_drain_ok){
        ALOGE("%s in 1",__FUNCTION__);
        compress->partial_drain_waiting = 1;
        pthread_cond_wait(&compress->partial_drain_cond, &compress->partial_drain_mutex);
        ALOGE("%s out 1",__FUNCTION__);
        compress->partial_drain_waiting = 0;
	}
	compress->partial_drain_ok = 0;
	pthread_mutex_unlock(&compress->partial_drain_mutex);
    ALOGE("%s out",__FUNCTION__);
	return 0;
}

int compress_get_tstamp(struct compress *compress,
			unsigned long *samples, unsigned int *sampling_rate)
{
	uint32_t avail = 0;
    uint32_t total = 0;
    if(!compress) {
	    ALOGE("%s error: compres is null",__FUNCTION__);
        return -1;
	}
	//ALOGE("peter: compress_get_tstamp in :%l, %d",*samples,*sampling_rate);
    pthread_mutex_lock(&compress->lock);
	if (!is_compress_ready(compress)){
	    pthread_mutex_unlock(&compress->lock);
		return -1;
    }

    pthread_mutex_lock(&compress->position_mutex);
    pthread_mutex_lock(&compress->pcm_mutex);
    if(compress->pcm) {
        avail= pcm_avail_update(compress->pcm);
        avail = pcm_frames_to_bytes(compress->pcm, avail);
        total = pcm_get_buffer_size(compress->pcm);
        total = pcm_frames_to_bytes(compress->pcm, total);
      
    	if(compress->samplerate == compress->pcm_cfg.rate) {
    	    *samples = pcm_bytes_to_frames(compress->pcm,compress->bytes_decoded - (compress->pcm_bytes_left + total - avail));
    	    
    	}
    	else {
    	    *samples =pcm_bytes_to_frames(compress->pcm,compress->bytes_decoded) - (pcm_bytes_to_frames(compress->pcm,(compress->pcm_bytes_left + total - avail))* compress->samplerate)/compress->pcm_cfg.rate;
    	   // *samples = ((uint64_t)(stream_total_bytes(stream)+ compress->base_position )* compress->samplerate +(compress->pcm_cfg.rate>>1))/compress->pcm_cfg.rate;//(uint32_t)((uint64_t)(cur_time  -  compress->start_time) * compress->samplerate*2/1000);
    	}

    	//ALOGE("peter_time: *samples: %d, compress->bytes_decoded:%d,compress->pcm_bytes_left:%d,total:%d,avail: %d,compress->pcm:%d",*samples,compress->bytes_decoded,compress->pcm_bytes_left,total,avail,(uint32_t)compress->pcm);
	}
	else {
        *samples =(compress->bytes_decoded)/(compress->pcm_cfg.channels<<1)- ((stream_data_bytes(&compress->pcm_stream)/(compress->pcm_cfg.channels<<1))* compress->samplerate +(compress->pcm_cfg.rate>>1))/compress->pcm_cfg.rate;
	}
	pthread_mutex_unlock(&compress->pcm_mutex);
	pthread_mutex_unlock(&compress->position_mutex); 

	*sampling_rate = compress->samplerate;
	if(compress->samplerate == 0) {
        *sampling_rate = 44100;
	}
    pthread_mutex_unlock(&compress->lock);
	//ALOGE("peter: compress_get_tstamp  out :%zu, %d,cur_samples :%d",*samples,*sampling_rate,cur_samples);
	return 0;
}

int compress_next_track(struct compress *compress)
{
    if(!compress) {
	    ALOGE("%s error: compres is null",__FUNCTION__);
        return -1;
	}
    ALOGE("%s in",__FUNCTION__);
    pthread_mutex_lock(&compress->lock);
	if (!is_compress_running(compress)) {
	    pthread_mutex_unlock(&compress->lock);
		return -1;
    }
    pthread_mutex_unlock(&compress->lock);
    ALOGE("%s out",__FUNCTION__);
	return 0;
}

int compress_wait(struct compress *compress, int timeout_ms)
{
     ALOGE("%s in",__FUNCTION__);
     
    if(!compress) {
	    ALOGE("%s error: compres is null",__FUNCTION__);
        return -1;
	}
	pthread_mutex_lock(&compress->cond_mutex);
	if(!compress->buffer_ready) {
        ALOGE("%s in 1",__FUNCTION__);
        pthread_cond_wait(&compress->cond, &compress->cond_mutex);
        ALOGE("%s out 1",__FUNCTION__);
	}
	compress->buffer_ready = 0;
	pthread_mutex_unlock(&compress->cond_mutex);
	 ALOGE("%s out",__FUNCTION__);
	//sem_wait(&compress->sem_wait);
	return 0;
}

int compress_stop(struct compress *compress)
{
    if(!compress) {
	    ALOGE("%s error: compres is null",__FUNCTION__);
        return -1;
	}
    ALOGE("compress_stop in");
    pthread_mutex_lock(&compress->lock);
	if (!is_compress_running(compress)){
        pthread_mutex_unlock(&compress->lock);
        return 0;
	}
    compress->running = 0;
	compress->state = STATE_STOP ;
    compress_send_cmd(compress, COMPERSS_CMD_STOP,NULL,1);

	pthread_mutex_unlock(&compress->lock);
	ALOGE("compress_stop out");
	return 0;
}

void compress_nonblock(struct compress *compress, int nonblock)
{
	
}

void compress_close(struct compress *compress)
{
    int ret = 0;
    if(!compress) {
	    ALOGE("%s error: compres is null",__FUNCTION__);
        return ;
	}
    ALOGE("compress_close in");
    do{
        ret = pthread_mutex_trylock(&compress->lock);
        if(ret) {
            ALOGE("compress_close but is busy");
            pthread_mutex_lock(&compress->cond_mutex);
            compress->buffer_ready = 1;
            pthread_cond_signal(&compress->cond);
            pthread_mutex_unlock(&compress->cond_mutex);
            usleep(20000);
        }
    }while(ret);

    compress->running = 0;
    compress->ready = 0;

    compress->state = STATE_EXIT;
    compress_send_cmd(compress, COMPERSS_CMD_CLOSE,NULL,1);

    if(compress->thread) {
        pthread_join(compress->thread, (void **) NULL);
    }
    if(compress->mMP3DecHandle) {
        MP3_ARM_DEC_Deconstruct(&compress->mMP3DecHandle);
        compress->mMP3DecHandle = NULL;
    }

    if(compress->pcm_stream.ring) {
        ringbuffer_free(compress->pcm_stream.ring);
        compress->pcm_stream.ring = NULL;
    }

    if(compress->stream.ring) {
        ringbuffer_free(compress->stream.ring);
        compress->stream.ring = NULL;
    }

    if(compress->cache_pcm_stream.ring) {
        ringbuffer_free(compress->cache_pcm_stream.ring);
        compress->cache_pcm_stream.ring = NULL;
    }

	if(compress->pcm){
        pcm_close(compress->pcm);
        compress->pcm = NULL;
    }

    if(compress->aud_fd >0) {
        close(compress->aud_fd);
    }

    if(compress->resample_buffer) {
        free(compress->resample_buffer);
    }

    if(compress->resampler) {
        release_resampler(compress->resampler);
    }

    pthread_mutex_destroy(&compress->lock);

	pthread_cond_destroy(&compress->cond);
    pthread_mutex_destroy(&compress->cond_mutex);
    
    pthread_cond_destroy(&compress->cmd_cond);
    pthread_mutex_destroy(&compress->cmd_mutex);
    
    pthread_mutex_destroy(&compress->pcm_mutex);
    
    pthread_mutex_destroy(&compress->position_mutex);
    
    pthread_cond_destroy(&compress->drain_cond);
	pthread_mutex_destroy(&compress->drain_mutex);
	
	pthread_cond_destroy(&compress->partial_drain_cond);
	pthread_mutex_destroy(&compress->partial_drain_mutex);

	free(compress->config);
	free(compress);
	ALOGE("compress_close out");
}


void register_offload_pcmdump(struct compress *compress,void *fun){
    compress->dumppcmfun=fun;
}