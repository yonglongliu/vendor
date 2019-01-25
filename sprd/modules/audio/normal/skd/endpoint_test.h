#ifndef ENDPINT_TEST_H
#define ENDPINT_TEST_H
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

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

enum aud_endpoint_test_type{
    ENDPOINT_TEST_IDLE =0,
    INPUT_ENDPOINT_TEST,
    OUTPUT_ENDPOINT_TEST,
    INPUT_OUTPUT_ENDPOINT_LOOP_TEST,
};

struct test_endpoint_param{
    int pcm_rate;
    int pcm_channels;
    int fd;
    void *data;
    unsigned int data_size;
    int type;
    int delay;
    int test_step;
};

struct endpoint_resampler_ctl{
   struct resampler_itfe *resampler;
   char *resampler_buffer;
   int resampler_buffer_size;
    char *conversion_buffer;
    int conversion_buffer_size;
    int rate;
    int channels;
    int request_rate;
    int request_channles;
};

struct audio_test_point{
    pthread_t thread;
    bool is_exit;
    pthread_mutex_t lock;
    void *stream;
    struct ring_buffer *ring_buf;
    struct test_endpoint_param param;
    struct endpoint_resampler_ctl resampler_ctl;
    int period_size;
};

struct endpoint_test_control{
    struct audio_test_point *source;
    struct audio_test_point *sink;
};

struct endpoint_control{
   void *primary_ouputstream;
    struct endpoint_test_control ouput_test;
    struct endpoint_test_control input_test;
    struct endpoint_test_control loop_test;
};

int audio_endpoint_test(void * adev,struct str_parms *parms,char * val);
int audio_endpoint_test_init(void * dev,void *out_stream);
#define AUDIO_EXT_CONTROL_PIPE_MAX_BUFFER_SIZE  1024
#endif
