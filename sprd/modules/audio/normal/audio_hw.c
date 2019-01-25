/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "audio_hw_primary"
#define LOG_NDEBUG 0
volatile int log_level = 3;
#define LOG_V(...)  ALOGV_IF(log_level >= 5,__VA_ARGS__);
#define LOG_D(...)  ALOGD_IF(log_level >= 4,__VA_ARGS__);
#define LOG_I(...)  ALOGI_IF(log_level >= 3,__VA_ARGS__);
#define LOG_W(...)  ALOGW_IF(log_level >= 2,__VA_ARGS__);
#define LOG_E(...)  ALOGE_IF(log_level >= 1,__VA_ARGS__);

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
#include <audio_utils/resampler.h>
#include "audio_pga.h"
#include "vb_effect_if.h"
#include "vb_pga.h"

#include "eng_audio.h"
#include "aud_proc.h"
#include "vb_control_parameters.h"
#include "string_exchange_bin.h"
#include <hardware_legacy/power.h>

#include "dumpdata.h"
#include "fmrecordhal_api.h"

#ifdef AUDIO_MUX_PCM
#include "audio_mux_pcm.h"
#endif

#ifdef NXP_SMART_PA
#include "Sprd_NxpTfa.h"
#include "Sprd_Spk_Fm.h"
#include "sprdFmTrack.h"
#endif
#include"record_nr_api.h"
#include"sprd_resample_48kto44k.h"
#include "AudioCustom_MmiApi.h"
#include "sprd_cp_control.h"

#include "smart_amp.h"
#include "audio_offload.h"
#ifdef AUDIO_TEST_INTERFACE_V2
#include "endpoint_test.h"
#endif
//#define XRUN_DEBUG
//#define VOIP_DEBUG

#ifdef XRUN_DEBUG
#define XRUN_TRACE  ALOGW
#else
#define XRUN_TRACE
#endif
#define BLUE_TRACE  ALOGW

#ifdef VOIP_DEBUG
#define VOIP_TRACE  ALOGW
#else
#define VOIP_TRACE
#endif

/**
  * container_of - cast a member of a structure out to the containing structure
  * @ptr:    the pointer to the member.
  * @type:   the type of the container struct this is embedded in.
  * @member: the name of the member within the struct.
  *
  */
#define container_of(ptr, type, member) ({      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

//#define AUDIO_DUMP
#define AUDIO_DUMP_EX

#define AUDIO_OUT_FILE_PATH  "data/local/media/audio_out.pcm"


//make sure this device is not used by android
#define SPRD_AUDIO_IN_DUALMIC_VOICE  0x81000000 //in:0x80000000

#define PRIVATE_NAME_LEN 60

#define CTL_TRACE(exp) ALOGV(#exp" is %s", ((exp) != NULL) ? "successful" : "failure")

#define PRIVATE_MIC_BIAS                  "mic bias"
#define PRIVATE_VBC_CONTROL              "vb control"
#define PRIVATE_VBC_EQ_SWITCH            "eq switch"
#define PRIVATE_VBC_EQ_UPDATE            "eq update"
#define PRIVATE_VBC_EQ_PROFILE            "eq profile"
#define PRIVATE_INTERNAL_PA              "internal PA"
#define PRIVATE_INTERNAL_HP_PA              "internal HP PA"
#define PRIVATE_TFA_FW_LOAD                 "TFA Firmware Load"

#define PRIVATE_INTERNAL_HP_PA_DELAY		"internal HP PA Delay"

#define FM_DIGITAL_SUPPORT_PROPERTY  "ro.digital.fm.support"

#define PRIVATE_VBC_DA_EQ_SWITCH            "da eq switch"
#define PRIVATE_VBC_AD01_EQ_SWITCH            "ad01 eq switch"
#define PRIVATE_VBC_AD23_EQ_SWITCH            "ad23 eq switch"

#define PRIVATE_VBC_DA_EQ_PROFILE            "da eq profile"
#define PRIVATE_VBC_AD01_EQ_PROFILE            "ad01 eq profile"
#define PRIVATE_VBC_AD23_EQ_PROFILE            "ad23 eq profile"
#define PRIVATE_SPEAKER_MUTE            "spk mute"
#define PRIVATE_SPEAKER2_MUTE           "spk2 mute"
#define PRIVATE_EARPIECE_MUTE           "earpiece mute"
#define PRIVATE_HEADPHONE_MUTE           "hp mute"
#define PRIVATE_AUD_LOOP_VBC  "Aud Loop in VBC Switch"
#define PRIVATE_AUD1_LOOP_VBC  "Aud1 Loop in VBC Switch"
#define PRIVATE_AUD_CODEC_INFO	"Aud Codec Info"
#define PRIVATE_VIRT_OUTPUT_SWITCH   "Virt Output Switch"

/*FM da0/da1 mux*/
#define PRIVATE_FM_DA0_MUX  "fm da0 mux"
#define PRIVATE_FM_DA1_MUX  "fm da1 mux"

/* Line-in FM mute */
#define PRIVATE_LINEIN_MUTE "linein mute"
/* vbc switch to dfm or codec */
#define PRIVATE_VBC_AD01IIS_TO_DFM "vbc_ad01iis_to_dfm"
/* ALSA cards for sprd */
#define CARD_SPRDPHONE "sprdphone"
#define CARD_VAUDIO    "VIRTUAL AUDIO"
#define CARD_VAUDIO_W  "VIRTUAL AUDIO W"
#define CARD_VAUDIO_LTE  "saudiolte"
#define CARD_SCO    "saudiovoip"
#define CARD_BT_SCO    "all-i2s"


#define VBC_REG_FILE "/proc/asound/sprdphone/vbc"
#define SPRD_CODEC_REG_FILE "/proc/asound/sprdphone/sprd-codec"
#define AUDIO_DMA_REG_FILE "/proc/asound/sprdphone/sprd-dmaengine"

#define DUMP_BUFFER_MAX_SIZE 256
#define DUMP_AUDIO_PROPERTY     "media.audio.dump"
#define REG_SPLIT "\n"

#define AUDIO_DUMP_WRITE_BUFFER(fd,buffer,size)     (fd> 0) ? write(fd, buffer, size): wirte_buffer_to_log(buffer,size)
#define AUDIO_DUMP_WRITE_STR(fd,buffer)     (fd> 0) ? write(fd, buffer, strlen(buffer)): ALOGI("%s\n",buffer)
void wirte_buffer_to_log(void *buffer,int size);
void dump_all_audio_reg(int fd,int dump_flag);
extern void register_offload_pcmdump(struct compress *compress,void *fun);

typedef enum {
    ADEV_DUMP_DEFAULT,
    ADEV_DUMP_TINYMIX,
    ADEV_DUMP_VBC_REG,
    ADEV_DUMP_CODEC_REG,
    ADEV_DUMP_DMA_REG,
    ADEV_DUMP_PCM_ERROR_CODEC_REG,
    ADEV_DUMP_PCM_ERROR_VBC_REG,
    ADEV_DUMP_PCM_ERROR_DMA_REG,
}audio_dump_switch_e;


/* ALSA ports for sprd */
#define PORT_MM 0
#define PORT_DEPP_BUFFER 5
#define PORT_MODEM 1
#define PORT_FM 4
#ifdef VBC_NOT_SUPPORT_AD23
#define PORT_MM_C 0
#else
#define PORT_MM_C 2
#endif

/* constraint imposed by VBC: all period sizes must be multiples of 160 */
#define VBC_BASE_FRAME_COUNT 160
/* number of base blocks in a short period (low latency) */
#define SHORT_PERIOD_MULTIPLIER 6//8 /* 29 ms */
/* number of frames per short period (low latency) */
#define SHORT_PERIOD_SIZE (VBC_BASE_FRAME_COUNT * SHORT_PERIOD_MULTIPLIER)
/* number of short periods in a long period (low power) */
#define CAPTURE_PERIOD_SIZE (VBC_BASE_FRAME_COUNT * 12)
#ifdef _LPA_IRAM
#define LONG_PERIOD_MULTIPLIER 3 /* 87 ms */
#else
#define LONG_PERIOD_MULTIPLIER 3  /* 87  ms */
#endif
/* number of frames per long period (low power) */
#define LONG_PERIOD_SIZE (SHORT_PERIOD_SIZE * LONG_PERIOD_MULTIPLIER)
/* number of periods for low power playback */
#define PLAYBACK_LONG_PERIOD_COUNT 2
/* number of pseudo periods for low latency playback */
#define PLAYBACK_SHORT_PERIOD_COUNT  4// 3      /*4*/
/* number of periods for capture */
#define CAPTURE_PERIOD_COUNT 4
/* minimum sleep time in out_write() when write threshold is not reached */
#define MIN_WRITE_SLEEP_US 5000

#define RESAMPLER_BUFFER_FRAMES (SHORT_PERIOD_SIZE * 2)
#define RESAMPLER_BUFFER_SIZE (4 * RESAMPLER_BUFFER_FRAMES)


#define DEFAULT_OUT_SAMPLING_RATE 44100

#define DEFAULT_IN_SAMPLING_RATE  8000
#ifdef AUDIO_OLD_MODEM
#define DEFAULT_FM_SRC_SAMPLING_RATE 32000
#else
#define DEFAULT_FM_SRC_SAMPLING_RATE 48000
#endif
/* sampling rate when using MM low power port */
#define MM_LOW_POWER_SAMPLING_RATE 44100
/* sampling rate when using MM full power port */
#define MM_FULL_POWER_SAMPLING_RATE 48000
/* sampling rate when using VX port for narrow band */
#define VX_NB_SAMPLING_RATE 8000
/* sampling rate when using VX port for wide band */
#define VX_WB_SAMPLING_RATE 16000

#define I2S_TYPE_EXT_CODEC 0
#define I2S_TYPE_BTSCO_NB VX_NB_SAMPLING_RATE
#define I2S_TYPE_BTSCO_WB VX_WB_SAMPLING_RATE

#define RECORD_POP_MIN_TIME_CAM    800   // ms
#define RECORD_POP_MIN_TIME_MIC    200   // ms

#define BT_SCO_UPLINK_IS_STARTED        (1 << 0)
#define BT_SCO_DOWNLINK_IS_EXIST        (1 << 1)
#define BT_SCO_DOWNLINK_OPEN_FAIL       (1 << 8)
#define AUDFIFO "/data/local/media/audiopara_tuning"

#define VOIP_PIPE_NAME_MAX    16

#define DEVICE_CTL_ON_MAX_COUNT  128
#define MAX_STOP_THRESHOLD ((unsigned int)-1)/2-1

#define  AUDIO_HAL_WAKE_LOCK_NAME "audio-hal"

enum {
    AUD_FM_TYPE_DIGITAL = 0,
    /* This line-in is reserved for samsung's line-in FM. */
    AUD_FM_TYPE_LINEIN,
    AUD_FM_TYPE_LINEIN_VBC,
    /* Every level of FM volume has 3 gains(ADC PGA, VBC DG and
     * Speaker/Headphone PGA) to set.
     */
    AUD_FM_TYPE_LINEIN_3GAINS,
    AUD_FM_TYPE_MAX,
};

struct pcm_config pcm_config_mm = {
    .channels = 2,
    .rate = DEFAULT_OUT_SAMPLING_RATE,
    .period_size = SHORT_PERIOD_SIZE,
    .period_count = PLAYBACK_SHORT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = SHORT_PERIOD_SIZE * PLAYBACK_SHORT_PERIOD_COUNT,
    .avail_min = SHORT_PERIOD_SIZE,
    .stop_threshold = -1,
};
#if 1
struct pcm_config pcm_config_mm_fast = {
    .channels = 2,
    .rate = DEFAULT_OUT_SAMPLING_RATE,
    .period_size = SHORT_PERIOD_SIZE,
    .period_count = PLAYBACK_SHORT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = SHORT_PERIOD_SIZE * PLAYBACK_SHORT_PERIOD_COUNT,
    .avail_min = SHORT_PERIOD_SIZE,
    .stop_threshold = -1,
};
#else
struct pcm_config pcm_config_mm_fast = {
    .channels = 2,
    .rate = DEFAULT_OUT_SAMPLING_RATE,
    .period_size = SHORT_PERIOD_SIZE,
    .period_count = PLAYBACK_SHORT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = SHORT_PERIOD_SIZE/2,
    .avail_min = SHORT_PERIOD_SIZE/2,
};
#endif

struct pcm_config pcm_config_mm_deepbuf = {
    .channels = 2,
    .rate = DEFAULT_OUT_SAMPLING_RATE,
    .period_size = 0x21c0/2,
    .period_count = 10,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = 0x21c0/2,
    .avail_min = 0x21c0/2,
};


/*google fm solution get pcm data from FM module
then playback it use music mode*/
struct pcm_config fm_record_config = {
    .channels = 2,
    .rate = 44100,
    .period_size = (160*4*2),
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
};

#ifdef NXP_SMART_PA
struct NxpTpa_Info NxpTfa9890_Info = {
    "all-i2s",
    1,
    PCM_OUT|PCM_MMAP|PCM_NOIRQ,
    {
    .channels = 2,
    .rate = 48000,
    .period_size = 1280,
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = 1280,
    .avail_min = 1280*4/2,
    },
};
struct NxpTpa_Info NxpTfa9890_Call_Info = {
    "all-i2s",
    2,
    PCM_OUT,
    {
    .channels = 2,
    .rate = 16000,
    .period_size = 1280,
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = 1280,
    .stop_threshold = -1, /* To avoid a pcm_start error. */
    .avail_min = 1280*4/2,
    },
};
#endif

struct pcm_config pcm_config_mm_ul = {
    .channels = 2,
    .rate = MM_FULL_POWER_SAMPLING_RATE,
    .period_size = CAPTURE_PERIOD_SIZE,
    .period_count = CAPTURE_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_vx = {
    .channels = 2,
    .rate = VX_NB_SAMPLING_RATE,
    .period_size = VBC_BASE_FRAME_COUNT,
    .period_count = 2,
    .format = PCM_FORMAT_S16_LE,
    .stop_threshold = MAX_STOP_THRESHOLD,
};

struct pcm_config pcm_config_vrec_vx = {
    .channels = 1,
    .rate = VX_NB_SAMPLING_RATE,
    .period_size = 320,
    .period_count = 8,
    .format = PCM_FORMAT_S16_LE,
    .stop_threshold = MAX_STOP_THRESHOLD,
};
struct pcm_config pcm_config_record_incall = {
    .channels = 1,
    .rate = VX_NB_SAMPLING_RATE,
    .period_size = 320,
    .period_count = 8,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_vx_voip = {
    .channels = 2,
#ifndef AUDIO_OLD_MODEM
    .rate = VX_WB_SAMPLING_RATE,
#else
    .rate = VX_NB_SAMPLING_RATE,
#endif
    .period_size = VBC_BASE_FRAME_COUNT,
    .period_count = 2,
    .format = PCM_FORMAT_S16_LE,
    .stop_threshold = MAX_STOP_THRESHOLD,
};
struct pcm_config pcm_config_vrec_vx_voip = {
    .channels = 1,
#ifndef AUDIO_OLD_MODEM
    .rate = VX_WB_SAMPLING_RATE,
#else
    .rate = VX_NB_SAMPLING_RATE,
#endif
    .period_size = 320,
    .period_count = 8,
    .format = PCM_FORMAT_S16_LE,
    .stop_threshold = MAX_STOP_THRESHOLD,
};

#define VOIP_CAPTURE_STREAM     0x1
#define VOIP_PLAYBACK_STREAM    0x2



struct pcm_config pcm_config_vplayback = {
    .channels = 2,
    .rate = VX_NB_SAMPLING_RATE,
    .period_size = 320,
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_scoplayback = {
    .channels = 1,
#ifndef AUDIO_OLD_MODEM
    .rate = VX_WB_SAMPLING_RATE,
    .period_size = 320,
#else
    .rate = VX_NB_SAMPLING_RATE,
    .period_size = 160,
#endif
    .period_count = 5,
    .format = PCM_FORMAT_S16_LE,
};


struct pcm_config pcm_config_btscoplayback_nb = {
    .channels = 1,
    .rate = VX_NB_SAMPLING_RATE,
    .period_size = 160,
    .period_count = 5,
    .format = PCM_FORMAT_S16_LE,
    .stop_threshold = -1,
};
struct pcm_config pcm_config_btscoplayback_wb = {
    .channels = 2,
    .rate = VX_WB_SAMPLING_RATE,
    .period_size = 320,
    .period_count = 5,
    .format = PCM_FORMAT_S16_LE,
    .stop_threshold = -1,
};

struct pcm_config pcm_config_scocapture = {
    .channels = 1,
#ifndef AUDIO_OLD_MODEM
    .rate = VX_WB_SAMPLING_RATE,
#else
    .rate = VX_NB_SAMPLING_RATE,
#endif
    .period_size = 640,
    .period_count = 8,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_btscocapture_nb = {
    .channels = 1,
    .rate = VX_NB_SAMPLING_RATE,
    .period_size = 640,
    .period_count = 8,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_btscocapture_wb = {
    .channels = 2,
    .rate = VX_WB_SAMPLING_RATE,
    .period_size = 640,
    .period_count = 8,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_scocapture_i2s = {
    .channels = 2,
    .rate = DEFAULT_FM_SRC_SAMPLING_RATE,
    .period_size = 640,
    .period_count = 8,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_fm_dl = {
    .channels = 2,
    .rate = DEFAULT_OUT_SAMPLING_RATE,
    .period_size = LONG_PERIOD_SIZE,
    .period_count = PLAYBACK_LONG_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .stop_threshold = MAX_STOP_THRESHOLD,
};
struct pcm_config pcm_config_fm_ul = {
    .channels = 2,
    .rate = DEFAULT_FM_SRC_SAMPLING_RATE,
    .period_size = CAPTURE_PERIOD_SIZE,
    .period_count = CAPTURE_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .stop_threshold = MAX_STOP_THRESHOLD,
};


#define MIN(x, y) ((x) > (y) ? (y) : (x))

struct route_setting
{
    char *ctl_name;
    int intval;
    char *strval;
};

struct tiny_dev_cfg {
    int mask;

    struct route_setting *on;
    unsigned int on_len;

    struct route_setting *off;
    unsigned int off_len;
};

struct tiny_private_ctl {
    struct mixer_ctl *mic_bias_switch;
    struct mixer_ctl *vbc_switch;
    struct mixer_ctl *vbc_eq_switch;
    struct mixer_ctl *vbc_eq_update;
    struct mixer_ctl *vbc_eq_profile_select;
    struct mixer_ctl *internal_pa;
    struct mixer_ctl *internal_hp_pa;
    struct mixer_ctl *vbc_da_eq_switch;
    struct mixer_ctl *vbc_ad01_eq_switch;
    struct mixer_ctl *vbc_ad23_eq_switch;
    struct mixer_ctl *vbc_da_eq_profile_select;
    struct mixer_ctl *vbc_ad01_eq_profile_select;
    struct mixer_ctl *vbc_ad23_eq_profile_select;
    struct mixer_ctl *speaker_mute;
    struct mixer_ctl *speaker2_mute;
    struct mixer_ctl *earpiece_mute;
    struct mixer_ctl *headphone_mute;
    struct mixer_ctl *fm_loop_vbc;
    struct mixer_ctl *ad1_fm_loop_vbc;
    struct mixer_ctl *internal_hp_pa_delay;
    struct mixer_ctl *aud_codec_info;
    struct mixer_ctl *fm_da0_mux;
    struct mixer_ctl *fm_da1_mux;
    struct mixer_ctl *linein_mute;
    struct mixer_ctl *vbc_ad01iis_to_dfm;
#ifdef NXP_SMART_PA
    struct mixer_ctl *tfa_fw_load;
#endif
};

struct stream_routing_manager {
    pthread_t        routing_switch_thread;
    bool             is_exit;
    sem_t           device_switch_sem;
};

struct bt_sco_thread_manager {
    bool             thread_is_exit;
    pthread_t        dup_thread;
    pthread_mutex_t  dup_mutex;
    pthread_mutex_t  cond_mutex;
    pthread_cond_t   cond;
    sem_t            dup_sem;
    volatile bool    dup_count;
    volatile bool    dup_need_start;
};

#define VOIP_PIPE_NAME_MAX    16
#define MAX_AT_CMD_LENGTH   32
#define MAX_AT_CMD_TYPE  9
/*
typedef struct{
   char  s_at_cmd_route[MAX_CMD_LENGTH];
   char  s_at_cmd_voluem[MAX_CMD_LENGTH];
   char  s_at_cmd_micmute[MAX_CMD_LENGTH];
   char  s_at_cmd_downlinkmute[MAX_CMD_LENGTH];
   char  s_at_cmd_audioloop[MAX_CMD_LENGTH];
   char  s_at_cmd_usecase[MAX_CMD_LENGTH];
   char  s_at_cmd_extra_volume[MAX_CMD_LENGTH];
   char  s_at_cmd_bt_sample[MAX_CMD_LENGTH];
}T_AT_CMD;
*/
//this struct describe  timer for real call end;
typedef struct
{
    timer_t timer_id;
    bool created;
} voip_timer_t;

typedef struct {
    unsigned short  adc_pga_gain_l;
    unsigned short  adc_pga_gain_r;
    uint32_t        pa_config;
    uint32_t        fm_pa_config;
    uint32_t        voice_pa_config;
    uint32_t        hp_pa_config;
    uint32_t        hp_pa_delay_config;
    uint32_t        fm_hp_pa_config;
    uint32_t        voice_hp_pa_config;
    uint32_t        fm_pga_gain_l;
    uint32_t        fm_pga_gain_r;
    uint32_t        dac_pga_gain_l;
    uint32_t        dac_pga_gain_r;
    uint32_t        cg_pga_gain_l;
    uint32_t        cg_pga_gain_r;
    uint32_t        fm_cg_pga_gain_l;
    uint32_t        fm_cg_pga_gain_r;
    uint32_t        out_devices;
    uint32_t        in_devices;
    uint32_t        mode;
}pga_gain_nv_t;

typedef struct{
   char  at_cmd[MAX_AT_CMD_TYPE][MAX_AT_CMD_LENGTH];
   uint32_t   at_cmd_priority[MAX_AT_CMD_TYPE];
   uint32_t   at_cmd_dirty;
   int    routeDev;
}T_AT_CMD;

#ifdef AUDIO_DEBUG
typedef struct{
    void* cache_buffer;
    size_t buffer_length;
    size_t write_flag;
    size_t total_length;
    bool more_one;
    long sampleRate;
    long channels;
    int wav_fd;
    FILE* dump_fd;
} out_dump_t;

typedef struct{
    bool dump_to_cache;
    bool dump_as_wav;
    bool dump_voip;
    bool dump_bt_sco;
    bool dump_vaudio;
    bool dump_music;
    bool dump_in_read;
    bool dump_in_read_afterprocess;
    bool dump_in_read_aftersrc;
    bool dump_in_read_vbc;
    bool dump_in_read_afternr;

    out_dump_t* out_voip;
    out_dump_t* out_bt_sco;
    out_dump_t* out_vaudio;
    out_dump_t* out_music;
    out_dump_t* in_read;
    out_dump_t* in_read_afterprocess;
    out_dump_t* in_read_aftersrc;
    out_dump_t* in_read_vbc;
    out_dump_t* in_read_afternr;

}dump_info_t;

struct audiotest_config
{
    struct pcm_config config;
    unsigned char * file;
    int fd;
    void * function;
    int card;
};

struct in_test_t
{
    struct audiotest_config config;
    pthread_t thread;
    int state;
    sem_t   sem;
    bool isloop;
    struct dev_test_t *dev_test;
    int len;
};

struct out_test_t
{
    struct audiotest_config config;
    pthread_t thread;
    int state;
    sem_t   sem;
    bool isloop;
    struct dev_test_t *dev_test;
    short *out ; // audio source
    short  len ;
    int    frequency ;
};

struct loop_test_t
{
    struct audiotest_config config;
    pthread_t thread;
    int state;
    sem_t   sem;
    struct tiny_audio_device *adev;
};

enum aud_dev_test_m {
    TEST_AUDIO_IDLE =0,
    TEST_AUDIO_OUT_DEVICES,
    TEST_AUDIO_IN_DEVICES,
    TEST_AUDIO_IN_OUT_LOOP,
    TEST_AUDIO_OUT_IN_LOOP,
    TEST_AUDIO_CP_LOOP,
};
struct dev_test_t {
    struct in_test_t in_test;
    struct out_test_t  out_test;
    struct loop_test_t  loop_test;
    int state;
    struct tiny_audio_device *adev;
    int test_mode;
};

struct ext_control_mgr {
    int fd;
    bool isExit;
    pthread_t thread_id;
};

typedef struct{
    dump_info_t* dump_info;
    struct dev_test_t dev_test;
    struct ext_control_mgr entryMgr;
}ext_contrl_t;
#endif

struct loop_ctl_t
{
    pthread_t thread;
    sem_t   sem;
    bool is_exit;
    pthread_mutex_t lock;
    struct ring_buffer *ring_buf;
    void *stream;
    int state;
};

struct skd_loop_t {
    struct loop_ctl_t in;
    struct loop_ctl_t out;
    int state;
    int loop_delay;
};

typedef struct
{
    bool is_entry_exit;
    pthread_t thread_id;
    int fake_fd;
}vb_ctl_modem_monitor_mgr;

typedef struct
{
    int fd;
    pthread_t thread_id;
}audio_tunning_mgr;

#ifdef NXP_SMART_PA
enum {
    SCENE_MUSIC,
    SCENE_CALL,
};

struct ext_codec_config {
    int scene;
    struct route_setting *routes;
    unsigned int num_routes;
};

struct ext_codec_info {
    char *name;
    struct ext_codec_config *configs;
    unsigned int num_configs;
    bool i2s_shared; /* shares the i2s dai with bt sco? */
};
#endif

enum RECORD_NR_MASK {
    RECORD_48000_NR =0,
    RECORD_44100_NR =1,
    RECORD_16000_NR =2,
    RECORD_8000_NR =3,
    RECORD_UNSUPPORT_RATE_NR =4,
};

struct tiny_audio_device {
    struct audio_hw_device hw_device;
    struct listnode active_out_list;
    pthread_mutex_t lock;       /* see note below on mutex acquisition order */
    struct mixer *mixer;
    struct mixer *cp_mixer[AP_TYPE];
    audio_mode_t mode;
    int out_devices;
    int in_devices;
    int prev_out_devices;
    int prev_in_devices;
    int routeDev;
    volatile int cur_vbpipe_fd;  /*current vb pipe id, if all pipes is closed this is -1.*/
    cp_type_t  cp_type;
    struct pcm *pcm_modem_dl;
    struct pcm *pcm_modem_ul;
    struct pcm *pcm_fm_dl;
    volatile int call_start;
    volatile int call_connected;
    volatile int call_prestop;
    volatile int vbc_2arm;
    pthread_mutex_t vbc_lock;/*for multiple vb pipe.*/
    float voice_volume;
    struct tiny_stream_in *active_input;
    struct tiny_stream_out *active_output;
    struct tiny_stream_out *active_deepbuffer_output;
    struct tiny_stream_out *active_offload_output;
    
    bool deepbuffer_enable;
    bool mic_mute;
    bool bluetooth_nrec;
    int  bluetooth_type;
    bool low_power;
    bool realCall; //for forbid voip

    struct tiny_dev_cfg *dev_cfgs;
    unsigned int num_dev_cfgs;

    struct tiny_dev_cfg *dev_linein_cfgs;
    unsigned int num_dev_linein_cfgs;

struct tiny_private_ctl private_ctl;
    struct audio_pga *pga;
    pga_gain_nv_t *pga_gain_nv;
    bool eq_available;

    audio_modem_t *cp;
    i2s_bt_t *i2s_btcall_info;
    AUDIO_TOTAL_T *audio_para;
    audio_tunning_mgr tunningMgr;

    volatile int bt_sco_state;
    struct bt_sco_thread_manager bt_sco_manager;

    struct stream_routing_manager  routing_mgr;
    struct stream_routing_manager  voice_command_mgr;
#ifdef AUDIO_DEBUG
    ext_contrl_t* ext_contrl;
#endif
    int voip_state;
    int voip_start;
#ifdef AUDIO_VOIP_VERSION_2
    int voip_playback_start;
#endif
    bool master_mute;
    bool cache_mute;
    int fm_volume;
    bool fm_mute;
    bool fm_open;
    bool fm_record;
    bool fm_uldl;/*flag google fm solution*/
    int fm_type;
    bool force_fm_mute;
    pthread_mutex_t mute_lock;
    int requested_channel_cnt;
    int  input_source;
    T_AT_CMD  *at_cmd_vectors;
    voip_timer_t voip_timer; //for forbid voip
    pthread_mutex_t               device_lock;
    pthread_mutex_t               vbc_dlulock;
    int  device_force_set;
    char* cp_nbio_pipe;
    struct mixer *i2s_mixer;
    int i2sfm_flag;
    int hfp_start;
#ifdef NXP_SMART_PA
    bool smart_pa_exist; /* Is there a smart pa chip actually? */
    fm_handle fmRes; //for smartpa fm
    nxp_pa_handle callHandle;//for smartpa call
    unsigned int internal_codec_rate;
    /* Info about external codec(such as nxp smart pa) */
    struct ext_codec_info *ext_codec;
#endif
    int aud_proc_init;
    int nr_mask;
    bool enable_other_rate_nr;

    struct skd_loop_t skd_loop;//used for skd test

    AUDIO_MMI_HANDLE mmiHandle; // custom mmi handle;
    struct cp_control_res * record_tone_info;
    SMART_AMP_MGR *pSmartAmpMgr;
    AUDIO_OUTPUT_DESC_T audio_outputs_state;
    struct mixer_ctl *offload_volume_ctl;
    int32_t is_offload_running;
    struct mixer_ctl *vbc_access;
    int mixer_route;
    void * misc_ctl;
    vb_ctl_modem_monitor_mgr modem_monitor_mgr;
#ifdef AUDIO_TEST_INTERFACE_V2
    struct endpoint_control test_ctl;
#endif
    int dump_flag;
    bool is_debug_btsco;
    bool record_bypass;
};

struct tiny_stream_out {
    struct audio_stream_out stream;

    pthread_mutex_t lock;       /* see note below on mutex acquisition order */
    struct pcm_config config;
    struct pcm_config cur_config;
    unsigned int sample_rate;
    audio_channel_mask_t channel_mask;
    audio_format_t format;
    struct pcm *pcm;
    struct pcm *pcm_vplayback;
    struct pcm *pcm_voip;
    struct pcm *pcm_voip_mixer;
    struct pcm *pcm_bt_sco;
    struct compr_config compress_config;
    int is_voip;
    int is_voip_mixer;
    int is_bt_sco;
    struct resampler_itfe  *resampler_vplayback;
    struct resampler_itfe  *resampler_sco;
    struct resampler_itfe  *resampler_bt_sco;
    struct resampler_itfe *resampler;
    char *buffer;
    char * buffer_vplayback;
    char * buffer_voip;
    char * buffer_bt_sco;
    struct compress *compress;
    struct compr_gapless_mdata gapless_mdata;
    stream_callback_t audio_offload_callback;
    void *audio_offload_cookie;
    AUDIO_HW_APP_T audio_app_type;         /* 0:primary; 1:offload */
    AUDIO_OFFLOAD_STATE_T audio_offload_state;    /* AUDIO_OFFLOAD_STOPED; PLAYING; PAUSED */
    unsigned int offload_format;
    unsigned int offload_samplerate;
    bool is_offload_compress_started;      /* flag indicates compress_start state */
    bool is_offload_need_set_metadata;     /* flag indicates to set the metadata to driver */
    bool is_audio_offload_thread_blocked;  /* flag indicates processing the command */
    int is_offload_nonblocking;            /* flag indicates to use non-blocking write */
    pthread_cond_t audio_offload_cond;
    pthread_t audio_offload_thread;
    struct listnode audio_offload_cmd_list;
    int standby;
    struct tiny_audio_device *dev;
    unsigned int devices;
    int write_threshold;
    bool low_power;
    FILE * out_dump_fd;
    audio_output_flags_t flags;
    uint64_t written;
#ifdef NXP_SMART_PA
    int nxp_pa;
    nxp_pa_handle handle_pa;
#endif
    SMART_AMP_CTL *pSmartAmpCtl;
    int smartAmpBatteryDelta;
    int smartAmpCacheBatteryVol;
    int smartAmpLoopCnt;
    struct mixer_ctl *offload_volume_ctl;
    long left_volume;
    long right_volume;
    struct listnode node;
    int is_paused_by_hal;
};

#define MAX_PREPROCESSORS 3 /* maximum one AGC + one NS + one AEC per input stream */

struct tiny_stream_in {
    struct audio_stream_in stream;

    pthread_mutex_t lock;       /* see note below on mutex acquisition order */
    struct pcm_config config;
    struct pcm *pcm;
    struct pcm * mux_pcm;
#ifdef NXP_SMART_PA
    FMPcmHandler FmRecHandle; //for fm record
#endif
    FMPcmHandler fmUlDlHandle; /*for google fm solution*/
    int is_voip;
    int is_bt_sco;
    int device;
    struct resampler_itfe *resampler;
    struct resampler_buffer_provider buf_provider;
    int16_t *buffer;
    size_t frames_in;
    unsigned int requested_rate;
    unsigned int requested_channels;
    int standby;
    audio_source_t source;
    int num_preprocessors;
    int16_t *proc_buf;
    int16_t *bt_sco_buf;
    size_t proc_buf_size;
    size_t need_size;
    size_t proc_frames_in;
    int16_t *ref_buf;
    size_t ref_buf_size;
    size_t ref_frames_in;
    int read_status;
    bool pop_mute;
    int pop_mute_bytes;
    struct tiny_audio_device *dev;
    int active_rec_proc;
    bool is_fm;
    void *pFMBuffer;
    record_nr_handle rec_nr_handle;
    transform_handle Tras48To44;
    int64_t frames_read; /* total frames read, not cleared when entering standby */

    int conversion_buffer_size;
    char *conversion_buffer;
    bool normal_c_open;
};

struct config_parse_state {
    struct tiny_audio_device *adev;
    struct tiny_dev_cfg *dev;
    bool on;

    struct route_setting *path;
    unsigned int path_len;

    char private_name[PRIVATE_NAME_LEN];
};

typedef struct {
    int mask;
    const char *name;
}dev_names_para_t;

static const dev_names_para_t dev_names_linein[] = {
    { AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_FM_SPEAKER, "speaker" },
    { AUDIO_DEVICE_OUT_WIRED_HEADSET | AUDIO_DEVICE_OUT_WIRED_HEADPHONE |AUDIO_DEVICE_OUT_FM_HEADSET,
        "headphone" },
    { AUDIO_DEVICE_OUT_EARPIECE, "earpiece" },
    /* ANLG for voice call via linein*/
    { AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET | AUDIO_DEVICE_OUT_ALL_FM, "line" },
    { AUDIO_DEVICE_OUT_FM_HEADSET, "line-headphone" },
    { AUDIO_DEVICE_OUT_FM_SPEAKER, "line-speaker" },

    { AUDIO_DEVICE_IN_COMMUNICATION, "comms" },
    { AUDIO_DEVICE_IN_AMBIENT, "ambient" },
    { AUDIO_DEVICE_IN_BUILTIN_MIC, "builtin-mic" },
    { AUDIO_DEVICE_IN_WIRED_HEADSET, "headset-in" },
    { AUDIO_DEVICE_IN_AUX_DIGITAL, "digital" },
    { AUDIO_DEVICE_IN_BACK_MIC, "back-mic" },
    { SPRD_AUDIO_IN_DUALMIC_VOICE, "dual-mic-voice" },
    //{ "linein-capture"},
};
static const dev_names_para_t dev_names_digitalfm[] = {
    { AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_FM_SPEAKER, "speaker" },
    { AUDIO_DEVICE_OUT_WIRED_HEADSET | AUDIO_DEVICE_OUT_WIRED_HEADPHONE |AUDIO_DEVICE_OUT_FM_HEADSET,
        "headphone" },
    { AUDIO_DEVICE_OUT_EARPIECE, "earpiece" },
    /* ANLG for voice call via linein*/
    { AUDIO_DEVICE_OUT_FM, "digital-fm" },


    { AUDIO_DEVICE_IN_COMMUNICATION, "comms" },
    { AUDIO_DEVICE_IN_AMBIENT, "ambient" },
    { AUDIO_DEVICE_IN_BUILTIN_MIC, "builtin-mic" },
    { AUDIO_DEVICE_IN_WIRED_HEADSET, "headset-in" },
    { AUDIO_DEVICE_IN_AUX_DIGITAL, "digital" },
    { AUDIO_DEVICE_IN_BACK_MIC, "back-mic" },
    { SPRD_AUDIO_IN_DUALMIC_VOICE, "dual-mic-voice" },
    //{ AUDIO_DEVICE_IN_LINE_IN, "line"},
    //{ "linein-capture"},
};
#define FM_VOLUME_MAX 16
//the default value about fm volume, it will be reload in audio_para
int fm_headset_volume_tbl[FM_VOLUME_MAX] = {
    99,47,45,43,41,39,37,35,33,31,29,27,25,23,21,20};

int fm_speaker_volume_tbl[FM_VOLUME_MAX] = {
    99,47,45,43,41,39,37,35,33,31,29,27,25,23,21,20};


static dev_names_para_t *dev_names = NULL;

/* this enum is for use-case type processed in cp side */
typedef enum {
    AUDIO_CP_USECASE_VOICE  = 0,
    AUDIO_CP_USECASE_VT,
    AUDIO_CP_USECASE_VOIP_1,
    AUDIO_CP_USECASE_VOIP_2,
    AUDIO_CP_USECASE_VOIP_3,
    AUDIO_CP_USECASE_VOIP_4,
    AUDIO_CP_USECASE_MAX,
} audio_cp_usecase_t;

enum{
    AUDIO_MODE_HEADSET = 0,
    AUDIO_MODE_HEADFREE = 1,
    AUDIO_MODE_HANDSET = 2,
    AUDIO_MODE_HANDSFREE = 3,
    AUDIO_MODE_ISCHANDSFREE = 4,
    AUDIO_MODE_ISCHEADFREE = 5,
};

typedef struct {
    int16_t   nr_switch;
    int16_t   modu1_switch;
    int16_t   modu2_switch;
    int16_t   min_psne;
    int16_t   max_psne;
    int16_t   nr_dgain;
    int16_t   ns_limit;
    int16_t   ns_factor;
} NR_CONTROL_PARAM_T;

/*
 * card define
 */
 /*
 * s_tinycard  is used to playback/capture of pcm data when vbc is controlled by ap part.
 *             for example
 *                        a.  playback in non-call.
 *                        b.  capture in non-call.
 *                        c.  voip in ap part.
 */
static int s_tinycard = -1;
 /*
 * s_vaudio  used in td mode
 * s_vaudio_w used in w mode
 *           is used to playback/capture of pcm data when vbc is controlled by cp part.
 *                 for example: playback/capture in call.
 */
static int s_vaudio = -1;
static int s_vaudio_w = -1;
static int s_vaudio_lte = -1;

 /*
 * s_voip  is used to voip call in cp part when vbc is controlled by cp dsp.
 */
static int s_voip= -1;

 /*
 * s_bt_sco  is used to playback/capture of pcm data when bt device of single channel is running.
 */
static int s_bt_sco = -1;

static AUDIO_TOTAL_T *audio_para_ptr = NULL;
static struct tiny_audio_device *s_adev= NULL;
static pthread_mutex_t adev_init_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned int audio_device_ref_count;

static dump_data_info_t dump_info;

/**
 * NOTE: when multiple mutexes have to be acquired, always respect the following order:
 *        hw device > in stream > out stream
 */
extern void * aud_misc_ctl_open();
extern void  aud_misc_ctl_close(void * misc);
extern int aud_misc_ctl_force_lowpower(void * misc);
extern int aud_misc_ctl_cancel_lowpower(void * misc);
extern int32_t compress_set_gain(struct compress *compress,long left_vol, long right_vol);
extern int MP3_ARM_DEC_init();
extern int MP3_ARM_DEC_release();
static ssize_t read_pcm_data(void *stream, void* buffer, size_t bytes);
extern int get_snd_card_number(const char *card_name);
extern void tinymix_list_controls(struct mixer *mixer,char * buf, unsigned int  *bytes);
extern void tinymix_detail_control(struct mixer *mixer, const char *control,
                                   int print_all,char * buf, unsigned int  *bytes);
extern void tinymix_set_value(struct mixer *mixer, const char *control,
                              char **values, unsigned int num_values);
extern void set_virt_output_switch(struct tiny_audio_device *adev, bool on);

int set_call_route(struct tiny_audio_device *adev, int device, int on);
char * adev_get_cp_card_name(struct tiny_audio_device *adev);
static void select_devices_signal(struct tiny_audio_device *adev);
static void select_devices_signal_asyn(struct tiny_audio_device *adev);
static int out_device_disable(struct tiny_audio_device *adev,int out_dev);
static void do_select_devices(struct tiny_audio_device *adev);
static int set_route_by_array(struct mixer *mixer, struct route_setting *route,unsigned int len);
#ifdef NXP_SMART_PA
static int set_ext_codec(struct tiny_audio_device *adev, int scene);
#endif
static int adev_set_voice_volume(struct audio_hw_device *dev, float volume);
static int adev_set_master_mute(struct audio_hw_device *dev, bool mute);
static int set_codec_mute(struct tiny_audio_device *adev,bool mute, bool in_call);
static int do_input_standby(struct tiny_stream_in *in);
static int do_output_standby(struct tiny_stream_out *out);
static void force_all_standby(struct tiny_audio_device *adev);
static int set_i2s_parameter(struct mixer *i2s_mixer,int i2s_type);
static int audiopara_tuning_manager_close(audio_tunning_mgr *pMgr);
static bool not_support_ad23(void);
static void vbc_ad01iis_select(struct tiny_audio_device *adev,
    bool is_to_dfm);

static struct route_setting * get_route_setting (
        struct tiny_audio_device *adev,
        int devices,
        int on);

static int get_route_depth (
        struct tiny_audio_device *adev,
        int devices,
        int on);

static int init_rec_process(int rec_mode, int sample_rate);
static int aud_rec_do_process(void * buffer,size_t bytes,int channels,void * tmp_buffer,size_t tmp_buffer_bytes);

static void *stream_routing_thread_entry(void * adev);
static int stream_routing_manager_create(struct tiny_audio_device *adev);
static void stream_routing_manager_close(struct tiny_audio_device *adev);

static void *voice_command_thread_entry(void * adev);
static int voice_command_manager_create(struct tiny_audio_device *adev);
static void voice_command_manager_close(struct tiny_audio_device *adev);

static int audio_bt_sco_duplicate_start(struct tiny_audio_device *adev, bool enable);
static int audiopara_get_compensate_phoneinfo(void* pmsg);
static void codec_lowpower_open(struct tiny_audio_device *adev,bool on);
/*
 * NOTE: audio stream(playback, capture) dump just for debug.
 */
static int out_dump_create(FILE **out_fd, const char *path);
static int out_dump_doing(FILE *out_fd, const void* buffer, size_t bytes);
static int out_dump_release(FILE **fd);
static void adev_config_end(void *data, const XML_Char *name);
static void adev_config_parse_private(struct config_parse_state *s, const XML_Char *name);
static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs);
static void do_select_devices(struct tiny_audio_device *adev);
static void aud_vb_effect_stop(struct tiny_audio_device *adev);
static void aud_vb_effect_start(struct tiny_audio_device *adev);
static void dump_tinymix_infor(int fd,int dump_flag,struct mixer *mixer);
static int adev_dump(const audio_hw_device_t *device, int fd);
static int set_fm_gain(struct tiny_audio_device *adev);

#ifdef ENABLE_DEVICES_CTL_ON
static int set_route_by_array_v2(struct mixer *mixer, struct route_setting *route,
        unsigned int len, struct mixer_ctl **ctl_on,int ctl_on_count,bool on_off);
#endif
int adev_record_tone_start(struct tiny_audio_device *adev, char* file_name); 
extern void vb_effect_setpara(AUDIO_TOTAL_T *para);
extern void vb_da_effect_config_mixer_ctl(struct mixer_ctl *da_profile_select);
extern void vb_ad_effect_config_mixer_ctl(struct mixer_ctl *ad01_profile_select, 
        struct mixer_ctl *ad23_profile_select);
extern void set_audio_mode_num(int mode);

#ifdef VB_CONTROL_PARAMETER_V2
#include "vb_control_parameters_v2.c"
#else
#include "vb_control_parameters.c"
#endif
#include "at_commands_generic.c"
#include "mmi_audio_loop.c"
#include "audio_offload.c"

#ifdef AUDIO_DEBUG
#include "ext_control.c"
#include "skd/skd_test.c"
#ifdef AUDIO_TEST_INTERFACE_V2
#include "skd/endpoint_test.c"
#endif
#endif
#define LOG_TAG "audio_hw_primary"


/* Converts audio_format to pcm_format.
 * Parameters:
 *  format  the audio_format_t to convert
 *
 * Logs a fatal error if format is not a valid convertible audio_format_t.
 */
static inline enum pcm_format pcm_format_from_audio_format(audio_format_t format)
{
    switch (format) {
#if HAVE_BIG_ENDIAN
    case AUDIO_FORMAT_PCM_16_BIT:
        return PCM_FORMAT_S16_BE;
    case AUDIO_FORMAT_PCM_24_BIT_PACKED:
        return PCM_FORMAT_S24_3BE;
    case AUDIO_FORMAT_PCM_32_BIT:
        return PCM_FORMAT_S32_BE;
    case AUDIO_FORMAT_PCM_8_24_BIT:
        return PCM_FORMAT_S24_BE;
#else
    case AUDIO_FORMAT_PCM_16_BIT:
        return PCM_FORMAT_S16_LE;
    case AUDIO_FORMAT_PCM_24_BIT_PACKED:
        return PCM_FORMAT_S24_3LE;
    case AUDIO_FORMAT_PCM_32_BIT:
        return PCM_FORMAT_S32_LE;
    case AUDIO_FORMAT_PCM_8_24_BIT:
        return PCM_FORMAT_S24_LE;
#endif
    case AUDIO_FORMAT_PCM_FLOAT:  /* there is no equivalent for float */
    default:
        LOG_ALWAYS_FATAL("pcm_format_from_audio_format: invalid audio format %#x", format);
        return 0;
    }
}

/* Converts pcm_format to audio_format.
 * Parameters:
 *  format  the pcm_format to convert
 *
 * Logs a fatal error if format is not a valid convertible pcm_format.
 */
static inline audio_format_t audio_format_from_pcm_format(enum pcm_format format)
{
    switch (format) {
#if HAVE_BIG_ENDIAN
    case PCM_FORMAT_S16_BE:
        return AUDIO_FORMAT_PCM_16_BIT;
    case PCM_FORMAT_S24_3BE:
        return AUDIO_FORMAT_PCM_24_BIT_PACKED;
    case PCM_FORMAT_S24_BE:
        return AUDIO_FORMAT_PCM_8_24_BIT;
    case PCM_FORMAT_S32_BE:
        return AUDIO_FORMAT_PCM_32_BIT;
#else
    case PCM_FORMAT_S16_LE:
        return AUDIO_FORMAT_PCM_16_BIT;
    case PCM_FORMAT_S24_3LE:
        return AUDIO_FORMAT_PCM_24_BIT_PACKED;
    case PCM_FORMAT_S24_LE:
        return AUDIO_FORMAT_PCM_8_24_BIT;
    case PCM_FORMAT_S32_LE:
        return AUDIO_FORMAT_PCM_32_BIT;
#endif
    default:
        LOG_ALWAYS_FATAL("audio_format_from_pcm_format: invalid pcm format %#x", format);
        return 0;
    }
}

static long getCurrentTimeUs()
{
   struct timeval tv;
   gettimeofday(&tv,NULL);
   return tv.tv_sec* 1000000 + tv.tv_usec;
}

/*  copy left channel to right channel*/
static int left2right(int16_t *buffer, uint32_t samples){
    int i = samples/2;
    while(i--){
        buffer[2*i+1]=buffer[2*i];
    }
    return 0;
}

static void get_partial_wakeLock() {
    int ret = acquire_wake_lock(PARTIAL_WAKE_LOCK, AUDIO_HAL_WAKE_LOCK_NAME);
    ALOGI("get_partial_wakeLock ret is %d.", ret);
}

static void release_wakeLock() {
    int ret = release_wake_lock(AUDIO_HAL_WAKE_LOCK_NAME);
    ALOGI("release_wakeLock ret is %d.", ret);
}

bool i2s_switch_sel(struct tiny_audio_device *adev, cp_type_t type)
{
    struct mixer *mixer=adev->mixer;
    audio_modem_t  *modem = adev->cp;
    int ret=-1;
    int i;
    struct route_setting **i2s_route=NULL;
    struct route_setting *i2s_route1=NULL;

    LOG_D("i2s_switch_sel: type %d",type);

    switch(type){
        case CP_CSFB:
            i2s_route=modem->i2s_switch_btcall;
            break;
        case FM_IIS:
            i2s_route=modem->i2s_switch_fm;
            break;
        case AP_TYPE:
            i2s_route=modem->i2s_switch_ap;
            break;
        case PUBCP:
            i2s_route=modem->i2s_switch_pubcp;
            break;
#ifdef NXP_SMART_PA
        case EXT_CODEC_AP:
            i2s_route1=modem->i2s_switch_ext_codec_ap;
            break;
        case EXT_CODEC_CP:
            i2s_route1=modem->i2s_switch_ext_codec_cp;
            break;
#endif
        default:
            LOG_D("i2s_switch_sel:default %d",type);
            break;
    }

    for (i = 0; i < I2S_CTL_INF; i++){
        if (NULL != i2s_route && NULL != i2s_route[i]) {
            struct mixer_ctl *ctl = mixer_get_ctl_by_name(mixer, i2s_route[i]->ctl_name);
            if (NULL != ctl) {
                if (i2s_route[i]->strval) {
                    ret = mixer_ctl_set_enum_by_string(ctl, i2s_route[i]->strval);
                    if (ret != 0) {
                        ALOGE("Failed to set '%s' to '%s'\n",
                                i2s_route[i]->ctl_name, i2s_route[i]->strval);
                    } else {
                        ALOGI("Set '%s' to '%s'\n",
                                i2s_route[i]->ctl_name, i2s_route[i]->strval);
                    }
                } else {
                    int j=0;
                    /* This ensures multiple (i.e. stereo) values are set jointly */
                    for (j=0; j < mixer_ctl_get_num_values(ctl); j++) {
                        ret = mixer_ctl_set_value(ctl, j, i2s_route[i]->intval);
                        if (ret != 0) {
                            ALOGE("Failed to set '%s'.%d to %d\n",
                                    i2s_route[i]->ctl_name, j, i2s_route[i]->intval);
                        } else {
                            ALOGV("Set '%s'.%d to %d\n",
                                    i2s_route[i]->ctl_name, j, i2s_route[i]->intval);
                        }
                    }
                }
            }
        }
    }

    if(NULL!=i2s_route1){
            struct mixer_ctl *ctl=mixer_get_ctl_by_name(mixer, i2s_route1->ctl_name);
            if(NULL!=ctl){
                if (i2s_route1->strval) {
                    ret = mixer_ctl_set_enum_by_string(ctl, i2s_route1->strval);
                    if (ret != 0) {
                        ALOGE("Failed to set '%s' to '%s'\n",
                                i2s_route1->ctl_name, i2s_route1->strval);
                    } else {
                        ALOGI("Set '%s' to '%s'\n",
                                i2s_route1->ctl_name, i2s_route1->strval);
                    }
                } else {
                    int j=0;
                    /* This ensures multiple (i.e. stereo) values are set jointly */
                    for (j=0; j < mixer_ctl_get_num_values(ctl); j++) {
                        ret = mixer_ctl_set_value(ctl, j, i2s_route1->intval);
                        if (ret != 0) {
                            ALOGE("Failed to set '%s'.%d to %d\n",
                                    i2s_route1->ctl_name, j, i2s_route1->intval);
                        } else {
                            ALOGV("Set '%s'.%d to %d\n",
                                    i2s_route1->ctl_name, j, i2s_route1->intval);
                        }
                    }
                }
            }
    }

    return 0;
}

int i2s_pin_mux_sel(struct tiny_audio_device *adev, int type)
{
    int count = 0;
    audio_modem_t  *modem;
    i2s_bt_t *i2s_btcall_info;
    uint8_t *ctl_on = "1";
    uint8_t *ctl_off = "0";
    uint8_t cur_state[2] = {0};
    uint8_t *ctl_str = "1";
    char cp_name[CP_NAME_MAX_LEN];

    ALOGI("i2s_pin_mux_sel in type is %d",type);

    modem = adev->cp;
    i2s_btcall_info = adev->i2s_btcall_info;
    vbc_ctrl_pipe_para_t	*vbc_ctrl_pipe_info = modem->vbc_ctrl_pipe_info;

    i2s_ctl_t * i2s_ctl_info =  NULL;

    ctrl_node *p_ctlr_node = NULL;

    if ((NULL!=modem->i2s_fm) && (type == FM_IIS)) {
        if (modem->i2s_fm->ctrl_file_fd  <= 0) {
            modem->i2s_fm->ctrl_file_fd = open(modem->i2s_fm->ctrl_path,O_RDWR | O_SYNC);
        }
        count = write(modem->i2s_fm->ctrl_file_fd,"1",1);
    } else if (NULL != i2s_btcall_info->i2s_ctl_info) {
        if (type != AP_TYPE) {
            for (count = 0 ; count < modem->num ; count++) {
                ALOGW("i2s_pin_mux_sel in type i--11-----%s----%d ----" , vbc_ctrl_pipe_info->s_vbc_ctrl_pipe_name ,vbc_ctrl_pipe_info->cp_type);
                if (vbc_ctrl_pipe_info->cp_type == type) {
                    break;
                }
                vbc_ctrl_pipe_info++ ;
            }
            ALOGW("-----in  cpu_index count  is %d ", count);
            if( count == modem->num){
                ALOGE("i2s_pin_mux_sel ERROR return");
                return -1;
            }
            i2s_btcall_info->cpu_index = vbc_ctrl_pipe_info->cpu_index ;
        } else {
            i2s_btcall_info->cpu_index = AP_TYPE;
        }

        if (type >= CP_MAX) {
            i2s_btcall_info->cpu_index = AP_TYPE;
        }

        i2s_ctl_info =  i2s_btcall_info->i2s_ctl_info +i2s_btcall_info->cpu_index;
        ALOGW("-----in  cpu_index  is %d ", i2s_btcall_info->cpu_index);
        p_ctlr_node = i2s_ctl_info->p_ctlr_node_head ;
        ALOGW("i2s_pin_mux_sel in type i----------i2s_ctl_info->is_switch = %d " ,i2s_ctl_info->is_switch );
        if ((adev->out_devices & AUDIO_DEVICE_OUT_ALL_SCO)||((adev->in_devices & ~AUDIO_DEVICE_BIT_IN) & AUDIO_DEVICE_IN_ALL_SCO)) {
            if (i2s_ctl_info->is_switch) {
                ALOGW("i2s_pin_mux_sel in type i-----------3");
                while (p_ctlr_node) {
                    if (p_ctlr_node->ctrl_file_fd <= 0) {
                        p_ctlr_node->ctrl_file_fd = open( p_ctlr_node->ctrl_path,O_RDWR | O_SYNC);
                    }
                    //itoa(p_ctlr_node->ctrl_value, ctl_on ,10);
                    if (p_ctlr_node->ctrl_file_fd > 0) {
                        ALOGW("-----in  i2s_pin_mux_sel in type is %s write  %s ",  p_ctlr_node->ctrl_path  ,p_ctlr_node->ctrl_value );
                        count = write(p_ctlr_node->ctrl_file_fd,p_ctlr_node->ctrl_value,1);
                    }
                    p_ctlr_node = p_ctlr_node->next;
                }
            }
        }
    } else {
	i2s_switch_sel(adev, type);
    }

    return 0;
}


/*for NXP SmartPA*/
int i2s_pin_mux_sel_new(struct tiny_audio_device *adev, FUN_TYPE fun_type, cp_type_t CpuType)
{
    int count = 0;
    int max_num = 0;
    int ctrl_index = 0;

    audio_modem_t  *modem = NULL;
    i2s_bt_t *i2s_btcall_info =NULL;
    modem = adev->cp;
    i2s_btcall_info = adev->i2s_btcall_info;

    i2s_ctl_t * i2s_ctl_info =  NULL;
    ctrl_node *p_ctlr_node = NULL;

    if (CpuType >= CP_MAX && fun_type != EXT_CODEC) {
        return -1;
    }

    if (BTCALL == fun_type) {
        if (NULL!=i2s_btcall_info->i2s_ctl_info) {
            i2s_ctl_info =  i2s_btcall_info->i2s_ctl_info;
            max_num = i2s_ctl_info->max_num;
            ALOGW("i2s_btcall_info->i2s_ctl_info->max_num = %d",max_num);
            for (ctrl_index = 0;ctrl_index < max_num;ctrl_index ++) {
                i2s_ctl_info =  i2s_btcall_info->i2s_ctl_info +ctrl_index;
                if (i2s_ctl_info->cpu_index == CpuType) {
                    break;
                }
            }
            if (ctrl_index == max_num) {
                ALOGE("i2s_btcall_info, cpu_index could not match");
                return -1;
            }
            p_ctlr_node = i2s_ctl_info->p_ctlr_node_head ;
            if (i2s_ctl_info->is_switch) {
                ALOGW("i2s_pin_mux_sel_new in type i-----------3");
                while (p_ctlr_node) {
                    if (p_ctlr_node->ctrl_file_fd <= 0) {
                        p_ctlr_node->ctrl_file_fd = open( p_ctlr_node->ctrl_path,O_RDWR | O_SYNC);
                    }
                    if (p_ctlr_node->ctrl_file_fd > 0) {
                        ALOGW("-----in  i2s_pin_mux_sel_new in type is %s write  %s ",  p_ctlr_node->ctrl_path  ,p_ctlr_node->ctrl_value );
                        count = write(p_ctlr_node->ctrl_file_fd,p_ctlr_node->ctrl_value,1);
                    }
                    p_ctlr_node = p_ctlr_node->next;
                }
            }
        }else{
            return i2s_switch_sel(adev,CpuType);
        }
    } else if (EXT_CODEC == fun_type) {
        return i2s_switch_sel(adev, CpuType);
    }
    return true;
}

static void mp3_low_power_check(struct tiny_audio_device *adev)
{
    LOG_D("mp3_low_power_check:low_power:%d,is_offload_running:%d:active_input:%x,active_output:%x,deep:%x,misc_ctl:%x",
            adev->low_power,adev->is_offload_running,(uint32_t)adev->active_input,(uint32_t)adev->active_output,
            (uint32_t)adev->active_deepbuffer_output,(uint32_t)adev->misc_ctl);
    if(adev->low_power == true) {
        if((adev->is_offload_running == 1)
            && (adev->active_input == NULL)
            && (adev->active_output == NULL)
            && (adev->active_deepbuffer_output == NULL)) {
            if(adev->misc_ctl) {
                LOG_I("mp3_low_power_check set for low power");
                aud_misc_ctl_force_lowpower(adev->misc_ctl);
            }
        }
    }
    else {
        if(adev->misc_ctl) {
            aud_misc_ctl_cancel_lowpower(adev->misc_ctl);
        }
    }
}

static void out_codec_mute_check(struct tiny_audio_device *adev)
{
    LOG_D("out_codec_mute_check:fm_o:%d,fm_vol:%d,is_off_run:%d,active off:%x,actiev:%x,call_start:%d,deep:%x",
        adev->fm_open,adev->fm_volume,adev->is_offload_running,
        (uint32_t)adev->active_offload_output,
        (uint32_t)adev->active_output,adev->call_start,
        (uint32_t)adev->active_deepbuffer_output);

    if((((adev->fm_open && adev->fm_volume == 0) && (adev->is_offload_running == 0))
            || ((adev->fm_open == 0)&&(adev->is_offload_running == 1)
                            &&(adev->active_offload_output)
                            &&(adev->active_offload_output->left_volume == 0)
                            &&(adev->active_offload_output->right_volume == 0))
             ||(((adev->fm_open && adev->fm_volume == 0)
                            &&(adev->is_offload_running == 1)
                            &&(adev->active_offload_output)
                            &&(adev->active_offload_output->left_volume == 0)
                            &&(adev->active_offload_output->right_volume == 0))))
            && (adev->active_output == NULL)
            && (adev->call_start == 0)
            && (adev->active_deepbuffer_output == NULL)){
            pthread_mutex_lock(&adev->mute_lock);
            if(!adev->master_mute) {
                adev->master_mute = true;
                ALOGI("%s out_codec_mute_check master_mute", __func__);
                select_devices_signal(adev);
            }
            pthread_mutex_unlock(&adev->mute_lock);
        }
}


static cp_type_t get_cur_cp_type( struct tiny_audio_device *adev )
{
    return adev->cp_type;
}

static  int  pcm_mixer(int16_t  *buffer, uint32_t samples)
{
    int i=0;
    int16_t * tmp_buf=buffer;
    for(i=0;i<(samples/2);i++){
        tmp_buf[i]=(buffer[2*i+1]+buffer[2*i])/2;
    }
    return 0;
}

static int do_pcm_mixer(int16_t *in_buffer,uint32_t in_size, int16_t *out_buffer)
{
    int i=0;
    int16_t *temp_buf = (int16_t *)in_buffer;
    for(i=0;i<(in_size/4);i++){
        out_buffer[i]=temp_buf[2*i+1];
    }
    return 0;
}

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

static int out_dump_doing(FILE *out_fd, const void* buffer, size_t bytes)
{
    int ret;
    if (out_fd) {
        ret = fwrite((uint8_t *)buffer, bytes, 1, out_fd);
        if (ret < 0) ALOGW("%d, fwrite failed.", bytes);
    } else {
        ALOGW("out_fd is NULL, cannot write.");
    }
    return 0;
}

static int out_dump_release(FILE **fd)
{
    if(*fd > 0) {
	    fclose(*fd);
	    *fd = NULL;
    }
    return 0;
}


int set_call_route(struct tiny_audio_device *adev, int device, int on)
{
    struct route_setting *cur_setting;
    int cur_depth = 0;

#ifdef VB_CONTROL_PARAMETER_V2
    cur_setting = get_linein_route_setting(adev, device, on);
    cur_depth = get_linein_route_depth(adev, device, on);
#else
    cur_setting = get_route_setting(adev, device, on);
    cur_depth = get_route_depth(adev, device, on);
#endif
    if (adev->mixer && cur_setting)
        set_route_by_array(adev->mixer, cur_setting, cur_depth);

    return 0;
}

void audio_pcm_dump(void *buffer,int size,dump_switch_e dump_switch_info){
#ifdef AUDIO_DUMP_EX
    dump_info.buf = buffer;
    dump_info.buf_len = size;
    dump_info.dump_switch_info = dump_switch_info;
    dump_data(dump_info);
#endif
}

static struct route_setting * get_route_setting(
        struct tiny_audio_device *adev,
        int devices,
        int on)
{
    unsigned int i = 0;
    for (i=0; i<adev->num_dev_cfgs; i++) {
        if ((devices & AUDIO_DEVICE_BIT_IN) && (adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)) {
            if ((devices & ~AUDIO_DEVICE_BIT_IN) & adev->dev_cfgs[i].mask) {
                if (on)
                    return adev->dev_cfgs[i].on;
                else
                    return adev->dev_cfgs[i].off;
            }
        } else if (!(devices & AUDIO_DEVICE_BIT_IN) && !(adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)){
            if (devices & adev->dev_cfgs[i].mask) {
                if (on)
                    return adev->dev_cfgs[i].on;
                else
                    return adev->dev_cfgs[i].off;
            }
        }
    }
    ALOGW("[get_route_setting], warning: devices(0x%08x) NOT found.", devices);
    return NULL;
}

static int get_route_depth (
        struct tiny_audio_device *adev,
        int devices,
        int on)
{
    unsigned int i = 0;

    for (i=0; i<adev->num_dev_cfgs; i++) {
        if ((devices & AUDIO_DEVICE_BIT_IN) && (adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)) {
            if ((devices & ~AUDIO_DEVICE_BIT_IN) & adev->dev_cfgs[i].mask) {
                if (on)
                    return adev->dev_cfgs[i].on_len;
                else
                    return adev->dev_cfgs[i].off_len;
            }
        } else if (!(devices & AUDIO_DEVICE_BIT_IN) && !(adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)){
            if (devices & adev->dev_cfgs[i].mask) {
                if (on)
                    return adev->dev_cfgs[i].on_len;
                else
                    return adev->dev_cfgs[i].off_len;
            }
        }
    }
    ALOGW("[get_route_setting], warning: devices(0x%08x) NOT found.", devices);
    return 0;
}

/* The enable flag when 0 makes the assumption that enums are disabled by
 * "Off" and integers/booleans by 0 */
static int set_route_by_array(struct mixer *mixer, struct route_setting *route,
        unsigned int len)
{
    struct mixer_ctl *ctl;
    unsigned int i, j, ret;

    /* Go through the route array and set each value */
    for (i = 0; i < len; i++) {
        ctl = mixer_get_ctl_by_name(mixer, route[i].ctl_name);
        if (!ctl) {
            ALOGE("Unknown control '%s'\n", route[i].ctl_name);
            continue;
        }

        if (route[i].strval) {
            ret = mixer_ctl_set_enum_by_string(ctl, route[i].strval);
            if (ret != 0) {
                ALOGE("Failed to set '%s' to '%s'\n",
                        route[i].ctl_name, route[i].strval);
            } else {
                ALOGI("Set '%s' to '%s'\n",
                        route[i].ctl_name, route[i].strval);
            }
        } else {
            /* This ensures multiple (i.e. stereo) values are set jointly */
            for (j = 0; j < mixer_ctl_get_num_values(ctl); j++) {
                ret = mixer_ctl_set_value(ctl, j, route[i].intval);
                if (ret != 0) {
                    ALOGE("Failed to set '%s'.%d to %d\n",
                            route[i].ctl_name, j, route[i].intval);
                } else {
                    ALOGV("Set '%s'.%d to %d\n",
                            route[i].ctl_name, j, route[i].intval);
                }
            }
        }
    }

    return 0;
}

#ifdef ENABLE_DEVICES_CTL_ON
static int set_route_by_array_v2(struct mixer *mixer, struct route_setting *route,
        unsigned int len, struct mixer_ctl **ctl_on,int ctl_on_count,bool on_off)
{
    struct mixer_ctl *ctl;
    unsigned int i, j, ret;
    unsigned int ctl_count=ctl_on_count;

    /* Go through the route array and set each value */
    for (i = 0; i < len; i++) {
        ctl = mixer_get_ctl_by_name(mixer, route[i].ctl_name);
        if (!ctl) {
            ALOGE("Unknown control '%s'\n", route[i].ctl_name);
            continue;
        }

        if(on_off==true){
            if(ctl_count>=DEVICE_CTL_ON_MAX_COUNT-1){
                ALOGI("set_route_by_array_v2 ctl_count error:%d",ctl_count);
            }else{
                ctl_on[ctl_count++]=ctl;
            }
        }else{
            for(j=0;j<ctl_on_count;j++){
                if(ctl_on[j]==ctl){
                    ctl=NULL;
                    break;
                }
            }
        }

        if(NULL==ctl){
            ALOGI("set_route_by_array_v2 %s do not set off ",route[i].ctl_name);
            continue;
        }

        if (route[i].strval) {
            ret = mixer_ctl_set_enum_by_string(ctl, route[i].strval);
            if (ret != 0) {
                ALOGE("Failed to set '%s' to '%s'\n",
                        route[i].ctl_name, route[i].strval);
            } else {
                ALOGI("Set '%s' to '%s'\n",
                        route[i].ctl_name, route[i].strval);
            }
        } else {
            /* This ensures multiple (i.e. stereo) values are set jointly */
            for (j = 0; j < mixer_ctl_get_num_values(ctl); j++) {
                ret = mixer_ctl_set_value(ctl, j, route[i].intval);
                if (ret != 0) {
                    ALOGE("Failed to set '%s'.%d to %d\n",
                            route[i].ctl_name, j, route[i].intval);
                } else {
                    ALOGV("Set '%s'.%d to %d\n",
                            route[i].ctl_name, j, route[i].intval);
                }
            }
        }
    }
    return ctl_count;
}
#endif

#ifdef NXP_SMART_PA
static int set_ext_codec(struct tiny_audio_device *adev, int scene)
{
    struct ext_codec_info *ext_codec = adev->ext_codec;
    unsigned int num_configs;
    struct ext_codec_config *configs;
    int i;

    ALOGV("Set ext codec for scene: %d.", scene);

    if (!ext_codec) {
        ALOGE("ext codec is NULL when set ext codec.");
        return -1;
    }

    num_configs = ext_codec->num_configs;
    configs = ext_codec->configs;

    for (i = 0; i < num_configs; i++) {
        configs += i;
        if (scene == configs->scene)
            break;
    }
    if (i == num_configs) {
        ALOGE("No ext codec configs refer to scene: %d!", scene);
        return -1;
    }

    return set_route_by_array(adev->i2s_mixer, configs->routes, configs->num_routes);
}
#endif

static void do_select_devices_static(struct tiny_audio_device *adev)
{
    unsigned int i;
    int ret;
    int cur_out = 0;
    int cur_in = 0;
    int pre_out = 0;
    int pre_in = 0;
    int force_set = 0;
    bool fm_iis_connection = false;
#ifdef NXP_SMART_PA
    int fm_close = 0;
    int fm_start  = 0;
#endif
    int vol = 1;
    int deviceMode = 0;
#ifdef ENABLE_DEVICES_CTL_ON
    struct mixer_ctl *ctl_on[DEVICE_CTL_ON_MAX_COUNT]={NULL};
    int ctl_on_count=0;
#endif

    ALOGV("do_select_devices_static E");
    if(adev->voip_state) {
        ALOGI("do_select_devices  in %x,but voip is on so send at to cp in",adev->out_devices);
        ret = at_cmd_route(adev);  //send at command to cp
        ALOGI("do_select_devices in %x,but voip is on so send at to cp out ret is %d",adev->out_devices,
ret);
        if (ret < 0) {
            ALOGE("do_seletc devices at_cmd_route error(%d) ",ret);
        }
        return;
    }
    if(adev->call_start == 1){
       goto out;
    }

    cur_in = adev->in_devices;
    cur_out = adev->out_devices;
    pre_in = adev->prev_in_devices;
    pre_out = adev->prev_out_devices;
    force_set = adev->device_force_set;
    adev->device_force_set = 0;

    if (adev->fm_open) {
        if (adev->fm_type != AUD_FM_TYPE_LINEIN_3GAINS) {
            cur_out |= AUDIO_DEVICE_OUT_FM;
        } else {
            if (cur_out == AUDIO_DEVICE_OUT_SPEAKER)
                cur_out |= AUDIO_DEVICE_OUT_FM_SPEAKER;
            else if (cur_out == AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
                     cur_out == AUDIO_DEVICE_OUT_WIRED_HEADSET)
                cur_out |= AUDIO_DEVICE_OUT_FM_HEADSET;

            vol = adev->fm_volume;
        }
    } else {
        if (adev->fm_type != AUD_FM_TYPE_LINEIN_3GAINS)
            cur_out &= ~AUDIO_DEVICE_OUT_FM;
        else
            cur_out &= ~(AUDIO_DEVICE_OUT_FM_SPEAKER | AUDIO_DEVICE_OUT_FM_HEADSET);
    }

    adev->prev_out_devices = cur_out;
    adev->prev_in_devices = cur_in;


    if (pre_out == cur_out
            &&  pre_in == cur_in && (!force_set)) {
        ALOGI("Not to change devices: OUT=0x%08x, IN=0x%08x",
                pre_out, pre_in);
       goto out;
    }
    ALOGI("Changing out_devices: from (0x%08x) to (0x%08x)",
            pre_out, cur_out);
    ALOGI("Changing in_devices: from (0x%08x) to (0x%08x)",
            pre_in, cur_in);

    if (adev->pSmartAmpMgr->isEnable) {
        struct tiny_stream_out *pOut = NULL;
        if (adev->deepbuffer_enable) {
            pOut = adev->active_deepbuffer_output;
        } else {
            pOut = adev->active_output;
        }
        if (pOut) {
            deviceMode = GetAudio_mode_number_from_device(adev, cur_out, cur_in);
            if (AUDIO_MODE_ISCHEADFREE == deviceMode) {
                select_smart_amp_config(pOut->pSmartAmpCtl, HEADFREE);
            }else if (AUDIO_MODE_ISCHANDSFREE == deviceMode) {
                select_smart_amp_config(pOut->pSmartAmpCtl, HANDSFREE);
            }
        } else {
            ALOGE("select_smart_amp_config Out Is NULL");
        }
    }
    /* note: should not has return statement betwen
       * vbc_access enable and disabe.
       */
    if (adev->vbc_access)
        mixer_ctl_set_value(adev->vbc_access, 0, 1);

    ret = GetAudio_PaConfig_by_devices(adev,adev->pga_gain_nv,cur_out,cur_in);
    if(ret < 0){
       goto out;
    }
    SetAudio_PaConfig_by_devices(adev,adev->pga_gain_nv);

    if(adev->eq_available)
        vb_effect_sync_devices(cur_out, cur_in);

    /* Turn on new devices first so we don't glitch due to powerdown... */
    for (i = 0; i < adev->num_dev_cfgs; i++) {
	/* separate INPUT/OUTPUT case for some common bit used. */
        if ((cur_out & adev->dev_cfgs[i].mask)
	    && !(adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)) {
        pthread_mutex_lock(&adev->lock);
        if (AUDIO_DEVICE_OUT_FM == adev->dev_cfgs[i].mask && adev->pcm_fm_dl == NULL) {
            pthread_mutex_unlock(&adev->lock);
            ALOGE("%s:open FM device",__func__);
            pthread_mutex_lock(&adev->lock);
            out_codec_mute_check(adev);
            ALOGI("open FM port %s", __func__);
            adev->pcm_fm_dl= pcm_open(s_tinycard, PORT_FM, PCM_OUT, &pcm_config_fm_dl);
            if (!pcm_is_ready(adev->pcm_fm_dl)) {
                ALOGE("%s:AUDIO_DEVICE_OUT_FM cannot open pcm_fm_dl : %s", __func__,pcm_get_error(adev->pcm_fm_dl));
                pcm_close(adev->pcm_fm_dl);
                adev->pcm_fm_dl= NULL;
                pthread_mutex_unlock(&adev->lock);
            } else {
              pthread_mutex_unlock(&adev->lock);
              if( 0 != pcm_start(adev->pcm_fm_dl)){
                  ALOGE("%s:pcm_start pcm_fm_dl start unsucessfully: %s", __func__,pcm_get_error(adev->pcm_fm_dl));
              }
              pthread_mutex_lock(&adev->lock);
              if(adev->fm_volume == 0)
              {
                  if (adev->private_ctl.fm_da0_mux)
                      mixer_ctl_set_value(adev->private_ctl.fm_da0_mux, 0, 0);
                  if (adev->private_ctl.fm_da1_mux)
                      mixer_ctl_set_value(adev->private_ctl.fm_da1_mux, 0, 0);
              }
              pthread_mutex_unlock(&adev->lock);
            }
        }else{
            pthread_mutex_unlock(&adev->lock);

        }

#ifdef ENABLE_DEVICES_CTL_ON
        ret=set_route_by_array_v2(adev->mixer, adev->dev_cfgs[i].on,
                adev->dev_cfgs[i].on_len,ctl_on,ctl_on_count,true);
        if (ret>0) {
            ctl_on_count=ret;
        }
#else
        set_route_by_array(adev->mixer, adev->dev_cfgs[i].on, adev->dev_cfgs[i].on_len);
#endif

        //first open fm path and then switch iis
        if (cur_out & AUDIO_DEVICE_OUT_FM && !fm_iis_connection) {
#ifdef AUDIO_OLD_MODEM
            //7731 no need to change i2s
#else
                i2s_pin_mux_sel(adev, FM_IIS);
#endif
                fm_iis_connection = true;
            }
        }

        if (((cur_in & ~AUDIO_DEVICE_BIT_IN) & adev->dev_cfgs[i].mask)
            && (adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)) {
            /* force close main-mic ADCL when channel count is one for power issue */
            if ((AUDIO_DEVICE_IN_BUILTIN_MIC == ( cur_in & adev->dev_cfgs[i].mask))
                 && (adev->requested_channel_cnt == 1)) {
                    if ((NULL != adev->cp) && (1 == adev->cp->mic_default_channel)) {

#ifdef ENABLE_DEVICES_CTL_ON
                        ret=set_route_by_array_v2(adev->mixer, adev->dev_cfgs[i].on,
                            adev->dev_cfgs[i].on_len,ctl_on,ctl_on_count,true);
                        if(ret>0){
                            ctl_on_count=ret;
                        }
#else
                        set_route_by_array(adev->mixer, adev->dev_cfgs[i].on, adev->dev_cfgs[i].on_len);
#endif

                    } else {
                        /* force close main-mic ADCL when channel count is one for power issue */
                        //close_adc_channel(adev->mixer, true, false, false);
                        struct mixer_ctl *ctl;
                        ctl = mixer_get_ctl_by_name(adev->mixer,"ADCR Mixer MainMICADCR Switch");
                        mixer_ctl_set_value(ctl, 0, 1);
                        ctl = mixer_get_ctl_by_name(adev->mixer,"Mic Function");
                        mixer_ctl_set_value(ctl, 0, 1);
                    }
            } else {
#ifdef ENABLE_DEVICES_CTL_ON
                        ret=set_route_by_array_v2(adev->mixer, adev->dev_cfgs[i].on,
                            adev->dev_cfgs[i].on_len,ctl_on,ctl_on_count,true);
                        if(ret>0){
                            ctl_on_count=ret;
                        }
#else
                        set_route_by_array(adev->mixer, adev->dev_cfgs[i].on,
                            adev->dev_cfgs[i].on_len);
#endif
            }
        }
    }

#ifdef NXP_SMART_PA
    ALOGV("sprdfm audio_hw : static cur_out is %x, %d",cur_out,cur_out & AUDIO_DEVICE_OUT_SPEAKER);
    if (adev->fm_open && (cur_out & AUDIO_DEVICE_OUT_SPEAKER)) {
       ALOGV("sprdfm audio_hw : static start");
       fm_start = 1;
    }
    else
       fm_close = 1;
#endif
    /* ...then disable old ones. */
    for (i = 0; i < adev->num_dev_cfgs; i++) {
        if (!(cur_out & adev->dev_cfgs[i].mask)
            && !(adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)) {
#ifdef ENABLE_DEVICES_CTL_ON
                ret=set_route_by_array_v2(adev->mixer, adev->dev_cfgs[i].off,
                        adev->dev_cfgs[i].off_len,ctl_on,ctl_on_count,false);
#else
                set_route_by_array(adev->mixer, adev->dev_cfgs[i].off,
                        adev->dev_cfgs[i].off_len);
#endif
            pthread_mutex_lock(&adev->lock);
        if(AUDIO_DEVICE_OUT_FM == adev->dev_cfgs[i].mask && adev->pcm_fm_dl != NULL)
        {
            ALOGE("%s:close FM device",__func__);

            if(adev->pcm_fm_dl != NULL){/*recheck is to avoid pcm_fm_dl has been
            freed in setting mode in vbc_ctrl_thread_linein_routine*/
                pcm_close(adev->pcm_fm_dl);
                adev->pcm_fm_dl= NULL;
                fm_iis_connection = false;
            }
            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_lock(&adev->mute_lock);
            if(adev->master_mute){
                if(adev->cache_mute != adev->master_mute) {
                    ALOGV("close FM so we set codec to mute by master_mute");
                    adev->cache_mute = true;
                    codec_lowpower_open(adev,true);
                }
            }
            pthread_mutex_unlock(&adev->mute_lock);
            //reset fm volume
            adev->fm_volume = -1;
        }else{
              pthread_mutex_unlock(&adev->lock);
        }
        }

        if (!((cur_in & ~AUDIO_DEVICE_BIT_IN) & adev->dev_cfgs[i].mask)
	    && (adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)) {
#ifdef ENABLE_DEVICES_CTL_ON
                ret=set_route_by_array_v2(adev->mixer, adev->dev_cfgs[i].off,
                        adev->dev_cfgs[i].off_len,ctl_on,ctl_on_count,false);
#else
                set_route_by_array(adev->mixer, adev->dev_cfgs[i].off,
                        adev->dev_cfgs[i].off_len);
#endif
        }
    }

    if (adev->fm_type == AUD_FM_TYPE_LINEIN_3GAINS && adev->fm_open == false)
        adev->fm_volume = -1;

    /* update EQ profile*/
    if(adev->eq_available) {
        int mode = GetAudio_mode_number_from_device(adev, cur_out, cur_in);
        vb_effect_profile_apply(mode);
    }

    SetAudio_gain_route(adev,vol,cur_out,cur_in);
out:
#ifdef NXP_SMART_PA
    if (adev->smart_pa_exist && (fm_close || fm_start)) {
        ALOGV("sprdfm:  static NxpTfa_Fm_Stop start");
        adev->device_force_set =1;
        select_devices_signal(adev);
    }
#endif
    if (adev->vbc_access)
        mixer_ctl_set_value(adev->vbc_access, 0, 0);
    ALOGV("do_select_devices_static X");
}

#define OUT_KCONTROL_NUM 4
static void out_dac_switch(struct tiny_audio_device *adev,bool on){
    int index = 0;
    char *daclname = "DACL Switch";
    char *dacrname = "DACR Switch";
    struct mixer_ctl *dacl= NULL;
    struct mixer_ctl *dacr= NULL;
    int mixernum = OUT_KCONTROL_NUM;
    struct mixer_ctl *ctl[OUT_KCONTROL_NUM] = {0};
    int old[OUT_KCONTROL_NUM] = {0};
    char *mixername[OUT_KCONTROL_NUM] = {
        "HPLCGL Switch",
        "HPRCGR Switch",
        "DACLSPKL Enable",
        "DACRSPKL Enable",
    };
    int waitms = 40;
    //get the mixer ctrl and save the mixer state.
    for(index = 0;index < mixernum;index++){
        ctl[index] = mixer_get_ctl_by_name(adev->mixer,mixername[index]);
        if(!ctl[index]){
            ALOGE("Unknown control '%s'\n", mixername[index]);
            return;
        }
        old[index] = mixer_ctl_get_value(ctl[index], 0);
    }

    //close the mixer for pop noise
    for(index = 0;index < mixernum;index++){
        if(old[index] == 1){
            ALOGE("%s turn off'%s'\n", __func__,mixername[index]);
          mixer_ctl_set_value(ctl[index], 0, 0);
        }
    }

    dacl = mixer_get_ctl_by_name(adev->mixer,daclname);
    if (!dacl){
            ALOGE("Unknown control '%s'\n", daclname);
            return;
    }
    mixer_ctl_set_value(dacl, 0, on);
    dacr = mixer_get_ctl_by_name(adev->mixer,dacrname);
    if (!dacr){
        ALOGE("Unknown control '%s'\n", dacrname);
        return;
    }
    mixer_ctl_set_value(dacr, 0, on);

    //restore the mixer pre state
    for(index = 0;index < mixernum;index++){
        if(old[index] == 1){
          ALOGE("%s turn on '%s'\n", __func__,mixername[index]);
          mixer_ctl_set_value(ctl[index], 0, 1);
        }
    }
}
static void codec_lowpower_open(struct tiny_audio_device *adev,bool on){
    ALOGI("%s :%s",__func__,on?"on":"off");
    if(on){
        set_codec_mute(adev,true,false);
        out_dac_switch(adev,0);
    } else {
        out_dac_switch(adev,1);
        set_codec_mute(adev,false,false);
    }
}

static void codec_lowpower_open_for_call(struct tiny_audio_device *adev,bool on){
    ALOGI("%s :%s",__func__,on?"on":"off");
    if(on){
        set_codec_mute(adev,true,true);
        out_dac_switch(adev,0);
    } else {
        out_dac_switch(adev,1);
        set_codec_mute(adev,false,true);
    }
}

static int out_device_disable(struct tiny_audio_device *adev,int out_dev)
{
    int i = 0;
    for (i = 0; i < adev->num_dev_cfgs; i++) {
        if ((out_dev & adev->dev_cfgs[i].mask)
            && !(adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)) {
            set_route_by_array(adev->mixer, adev->dev_cfgs[i].off,
            adev->dev_cfgs[i].off_len);
            if(AUDIO_DEVICE_OUT_FM == adev->dev_cfgs[i].mask && adev->pcm_fm_dl != NULL) {
                ALOGE("%s:close FM device",__func__);
                pcm_close(adev->pcm_fm_dl);
                adev->pcm_fm_dl= NULL;
            }
        }
    }
    return 0;
}

static int set_fm_gain(struct tiny_audio_device *adev)
{
    int rc = 0;
    int fm_gain = 0;
    int flagVaild = 0;
    if (NULL == adev) {
        ALOGE("%s argin is NULL!", __FUNCTION__);
        return -1;
    }
    if ((0 > adev->fm_volume) || (FM_VOLUME_MAX <= adev->fm_volume)) {
        ALOGE("%s fm_volume is error!", __FUNCTION__, adev->fm_volume);
        return -1;
    }
    int i = adev->fm_volume;
    if (adev->out_devices & AUDIO_DEVICE_OUT_SPEAKER) {
        fm_gain |= fm_speaker_volume_tbl[i];
        fm_gain |= fm_speaker_volume_tbl[i]<<16;
        flagVaild = 1;
    } else if ((adev->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) ||
                (adev->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE)){
        fm_gain |= fm_headset_volume_tbl[i];
        fm_gain |= fm_headset_volume_tbl[i]<<16;
        flagVaild = 1;
    }
    if (flagVaild != 0 && false == adev->force_fm_mute) {
        rc = SetAudio_gain_fmradio(adev,fm_gain);
    }
    return rc;
}

static void do_select_devices(struct tiny_audio_device *adev)
{
    unsigned int i;
    int ret;
    int cur_out = 0;
    int cur_in = 0;
    int pre_out = 0;
    int pre_in = 0;
    bool voip_route_ops = false;
    int  force_set = 0;
    bool fm_iis_connection = false;
    bool codec_mute = false;
    bool codec_mute_set = false;
    int fm_close = 0;
    int fm_start = 0;
    int vol = 1;
    int deviceMode = 0;
    int bypass_outdevice = 0;
    int bypass_indevice = 0;

#ifdef ENABLE_DEVICES_CTL_ON
    struct mixer_ctl * ctl_on[DEVICE_CTL_ON_MAX_COUNT]={NULL};
    int ctl_on_count=0;
#endif

    ALOGV("do_select_devices E");
    pthread_mutex_lock(&adev->lock);
    if(adev->voip_state) {
        ALOGI("do_select_devices  in %x,but voip is on so send at to cp in",adev->out_devices);
        voip_route_ops = true;
    }
   pthread_mutex_unlock(&adev->lock);
    if(voip_route_ops){
      ret = at_cmd_route(adev);  //send at command to cp
      return;
    }
    ALOGI("cache_mute=%d ,mastermute=%d", adev->cache_mute, adev->master_mute);
    pthread_mutex_lock(&adev->device_lock);
    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&adev->mute_lock);
    if (adev->cache_mute == adev->master_mute) {
        ALOGI("Not to change mute: %d", adev->cache_mute);
    } else {
        /* mute codec PA */
        if(adev->master_mute && (adev->pcm_fm_dl != NULL &&
            adev->fm_volume != 0)){
            ALOGD("%s,fm is open so do not mute codec",__func__);
        }else{
            codec_mute_set = true;
            codec_mute = adev->master_mute;
            adev->cache_mute = adev->master_mute;
        }
    }
    pthread_mutex_unlock(&adev->mute_lock);
    if(adev->call_start == 1){
        pthread_mutex_unlock(&adev->lock);
        pthread_mutex_unlock(&adev->device_lock);
        if(codec_mute_set){
            codec_lowpower_open_for_call(adev, codec_mute);
        }
        return;
    }

    set_virt_output_switch(adev,true);

    cur_in = adev->in_devices;
    cur_out = adev->out_devices;
    pre_in = adev->prev_in_devices;
    pre_out = adev->prev_out_devices;
    force_set = adev->device_force_set;
    adev->device_force_set = 0;

    // get mmi test device when mmi is enable;
    if (MMI_TEST_START == AudioMMIIsEnable(adev->mmiHandle)) {
        AudioMMIGetDevice(adev->mmiHandle, &cur_out, &cur_in);
    }

    if (adev->fm_open) {
        if (adev->fm_type != AUD_FM_TYPE_LINEIN_3GAINS) {
            cur_out |= AUDIO_DEVICE_OUT_FM;
        } else {
            if (cur_out == AUDIO_DEVICE_OUT_SPEAKER)
                cur_out |= AUDIO_DEVICE_OUT_FM_SPEAKER;
            else if (cur_out == AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
                     cur_out == AUDIO_DEVICE_OUT_WIRED_HEADSET)
                cur_out |= AUDIO_DEVICE_OUT_FM_HEADSET;

            vol = adev->fm_volume;
        }
    } else {
        if (adev->fm_type != AUD_FM_TYPE_LINEIN_3GAINS)
            cur_out &= ~AUDIO_DEVICE_OUT_FM;
        else
            cur_out &= ~(AUDIO_DEVICE_OUT_FM_SPEAKER | AUDIO_DEVICE_OUT_FM_HEADSET);
    }

    if(cur_out && (!(cur_out & (~AUDIO_DEVICE_OUT_ALL_SCO)))) {
        if((adev->active_output || adev->active_deepbuffer_output )
            && (!(adev->bt_sco_state & BT_SCO_DOWNLINK_IS_EXIST))) {
            ALOGE("yaye  need_bypass_output!%s, adev->active_output:%p,  adev->active_deepbuffer_output:%p, bt_sco_state:%x",
                __func__, adev->active_output, adev->active_deepbuffer_output, adev->bt_sco_state);
            bypass_outdevice = 1;
        }
    }

    if(cur_in && (!(cur_in & (~AUDIO_DEVICE_IN_ALL_SCO)))) {
        if((adev->active_input)
            && (!(adev->bt_sco_state & BT_SCO_UPLINK_IS_STARTED))) {
            ALOGE("yaye  need_bypass_input!adev->active_input:%p, bt_sco_state:%x",
                __func__, adev->active_input, adev->bt_sco_state);
            bypass_indevice = 1;
        }
    }

    if(!bypass_outdevice){
        adev->prev_out_devices = cur_out;
    }
    if(!bypass_indevice){
    adev->prev_in_devices = cur_in;
    }
    pthread_mutex_unlock(&adev->lock);

    if(codec_mute_set){
                codec_lowpower_open(adev, codec_mute);
    }

    if (pre_out == cur_out
            &&  pre_in == cur_in && (!force_set) ) {
        ALOGI("Not to change devices: OUT=0x%08x, IN=0x%08x",
                pre_out, pre_in);
        goto out;
    }

    ALOGI("Changing out_devices: from (0x%08x) to (0x%08x), Changing in_devices: from (0x%08x) to (0x%08x)",
            pre_out, cur_out, pre_in, cur_in);

    if (adev->pSmartAmpMgr->isEnable) {
        struct tiny_stream_out *pOut = NULL;
        if (adev->deepbuffer_enable) {
            pOut = adev->active_deepbuffer_output;
        } else {
            pOut = adev->active_output;
        }
        if (pOut) {
            deviceMode = GetAudio_mode_number_from_device(adev, cur_out, cur_in);
            if (AUDIO_MODE_ISCHEADFREE == deviceMode) {
                select_smart_amp_config(pOut->pSmartAmpCtl, HEADFREE);
            }else if (AUDIO_MODE_ISCHANDSFREE == deviceMode) {
                select_smart_amp_config(pOut->pSmartAmpCtl, HANDSFREE);
            }
        } else {
            ALOGE("select_smart_amp_config Out Is NULL");
        }
    }
    /* note: should not has return statement betwen
       * vbc_access enable and disabe.
       */
    if (adev->vbc_access)
        mixer_ctl_set_value(adev->vbc_access, 0, 1);
    ret = GetAudio_PaConfig_by_devices(adev,adev->pga_gain_nv,cur_out,cur_in);
    if(ret < 0){
       goto out;
    }
    SetAudio_PaConfig_by_devices(adev,adev->pga_gain_nv);

    if(adev->eq_available)
        vb_effect_sync_devices(cur_out, cur_in);

    /* Turn on new devices first so we don't glitch due to powerdown... */
    for (i = 0; i < adev->num_dev_cfgs; i++) {
	/* separate INPUT/OUTPUT case for some common bit used. */
        if ((cur_out & adev->dev_cfgs[i].mask)
	    && !(adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)) {
            if (AUDIO_DEVICE_OUT_FM == adev->dev_cfgs[i].mask && adev->pcm_fm_dl == NULL) {
                ALOGE("%s:open FM device",__func__);
                pthread_mutex_lock(&adev->lock);
                //force_all_standby(adev);
                ALOGI("open FM port %s", __func__);
                adev->pcm_fm_dl= pcm_open(s_tinycard, PORT_FM, PCM_OUT, &pcm_config_fm_dl);
                if (!pcm_is_ready(adev->pcm_fm_dl)) {
                    ALOGE("%s:cannot open pcm_fm_dl : %s", __func__,pcm_get_error(adev->pcm_fm_dl));
                    pcm_close(adev->pcm_fm_dl);
                    adev->pcm_fm_dl= NULL;
                    pthread_mutex_unlock(&adev->lock);
                } else {
                    pthread_mutex_unlock(&adev->lock);
                  if( 0 != pcm_start(adev->pcm_fm_dl)){
                      ALOGE("%s:pcm_start pcm_fm_dl start unsucessfully: %s", __func__,pcm_get_error(adev->pcm_fm_dl));
                  }
                  pthread_mutex_lock(&adev->mute_lock);
                  if(adev->master_mute){
                      ALOGV("open FM and set codec unmute");
                      adev->cache_mute = false;
                      codec_lowpower_open(adev,false);
                  }
                  pthread_mutex_unlock(&adev->mute_lock);
                }
            }

#ifdef ENABLE_DEVICES_CTL_ON
                ret=set_route_by_array_v2(adev->mixer, adev->dev_cfgs[i].on,
                        adev->dev_cfgs[i].on_len,ctl_on,ctl_on_count,true);
                if(ret>0){
                    ctl_on_count=ret;
                }
#else
            set_route_by_array(adev->mixer, adev->dev_cfgs[i].on,
                    adev->dev_cfgs[i].on_len);
#endif
            //first open fm path and then switch iis
            if(cur_out & AUDIO_DEVICE_OUT_FM && !fm_iis_connection){
#ifdef AUDIO_OLD_MODEM
            //7731 no need to change i2s
#else
                i2s_pin_mux_sel(adev, FM_IIS);
#endif
                fm_iis_connection = true;
		        ALOGE("do_select_devices, fm_volume: %d",adev->fm_volume);
                if(adev->fm_volume !=0){
                    if (pre_out != cur_out) {
#ifdef FM_VERSION_IS_GOOGLE
                        if (adev->private_ctl.fm_da0_mux)
                            mixer_ctl_set_value(adev->private_ctl.fm_da0_mux, 0, 0);
                        if (adev->private_ctl.fm_da1_mux)
                            mixer_ctl_set_value(adev->private_ctl.fm_da1_mux, 0, 0);
#else
                        if (false == adev->force_fm_mute) {
                            if (adev->private_ctl.fm_da0_mux)
                                mixer_ctl_set_value(adev->private_ctl.fm_da0_mux, 0, 1);
                            if (adev->private_ctl.fm_da1_mux)
                                mixer_ctl_set_value(adev->private_ctl.fm_da1_mux, 0, 1);
                        } else {
                            ALOGI("FM state is force_fm_mute, so can't un mute");
                        }
#endif
                    } else {
                        ALOGI("FM out device isn't change, so don't enable fm da mixer!");
                    }
                } else {
                        if (adev->private_ctl.fm_da0_mux)
                            mixer_ctl_set_value(adev->private_ctl.fm_da0_mux, 0, 0);
                        if (adev->private_ctl.fm_da1_mux)
                            mixer_ctl_set_value(adev->private_ctl.fm_da1_mux, 0, 0);
                }
            }
        }

        if (((cur_in & ~AUDIO_DEVICE_BIT_IN) & adev->dev_cfgs[i].mask)
            && (adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)) {
            /* force close main-mic ADCL when channel count is one for power issue */
            if ((AUDIO_DEVICE_IN_BUILTIN_MIC == ( cur_in & adev->dev_cfgs[i].mask))
                 && (adev->requested_channel_cnt == 1)) {
                if ((NULL!=adev->cp) && (1 == adev->cp->mic_default_channel)) {
#ifdef ENABLE_DEVICES_CTL_ON
                    ret=set_route_by_array_v2(adev->mixer, adev->dev_cfgs[i].on,
                            adev->dev_cfgs[i].on_len,ctl_on,ctl_on_count,true);
                    if(ret>0){
                        ctl_on_count=ret;
                    }
#else
                    set_route_by_array(adev->mixer, adev->dev_cfgs[i].on, adev->dev_cfgs[i].on_len);
#endif
                } else {
                    /* force close main-mic ADCL when channel count is one for power issue */
                    //close_adc_channel(adev->mixer, true, false, false);
                    struct mixer_ctl *ctl;
                    ctl = mixer_get_ctl_by_name(adev->mixer,"ADCR Mixer MainMICADCR Switch");
                    mixer_ctl_set_value(ctl, 0, 1);
                    ctl = mixer_get_ctl_by_name(adev->mixer,"Mic Function");
                    mixer_ctl_set_value(ctl, 0, 1);
                }
            } else {
#ifdef ENABLE_DEVICES_CTL_ON
              ret=set_route_by_array_v2(adev->mixer, adev->dev_cfgs[i].on,
                    adev->dev_cfgs[i].on_len,ctl_on,ctl_on_count,true);
              if(ret>0){
                  ctl_on_count=ret;
              }
#else
              set_route_by_array(adev->mixer, adev->dev_cfgs[i].on, adev->dev_cfgs[i].on_len);
#endif

            }
        }
    }

#ifdef NXP_SMART_PA
    if (adev->fm_open && (cur_out & AUDIO_DEVICE_OUT_SPEAKER)) {
        fm_start = 1;
    }
    else
       fm_close = 1;
#endif

    /* ...then disable old ones. */
    for (i = 0; i < adev->num_dev_cfgs; i++) {
        if (!(cur_out & adev->dev_cfgs[i].mask)
            && !(adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)) {
            if(bypass_outdevice == 0) {
#ifdef ENABLE_DEVICES_CTL_ON
                set_route_by_array_v2(adev->mixer, adev->dev_cfgs[i].off,
                        adev->dev_cfgs[i].off_len,ctl_on,ctl_on_count,false);
#else
                set_route_by_array(adev->mixer, adev->dev_cfgs[i].off,
                        adev->dev_cfgs[i].off_len);
#endif
            }
            if(AUDIO_DEVICE_OUT_FM == adev->dev_cfgs[i].mask && adev->pcm_fm_dl != NULL)
            {
                ALOGE("%s:close FM device",__func__);
                pthread_mutex_lock(&adev->lock);
                if(adev->pcm_fm_dl != NULL){/*recheck is to avoid pcm_fm_dl has been
                freed in setting mode in vbc_ctrl_thread_linein_routine*/
                    pcm_close(adev->pcm_fm_dl);
                    adev->pcm_fm_dl= NULL;
                    fm_iis_connection = false;
                }
                pthread_mutex_lock(&adev->mute_lock);
                if(adev->master_mute){
                    if(adev->cache_mute != adev->master_mute) {
                        ALOGV("close FM so we set codec to mute by master_mute");
                        adev->cache_mute = true;
                        codec_lowpower_open(adev,true);
                    }
                }
                pthread_mutex_unlock(&adev->mute_lock);
                //reset fm volume
                adev->fm_volume = -1;
                pthread_mutex_unlock(&adev->lock);
            }
        }

        if (!((cur_in & ~AUDIO_DEVICE_BIT_IN) & adev->dev_cfgs[i].mask)
            && (adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)) {
            if(bypass_outdevice == 0) {
#ifdef ENABLE_DEVICES_CTL_ON
                set_route_by_array_v2(adev->mixer, adev->dev_cfgs[i].off,
                        adev->dev_cfgs[i].off_len,ctl_on,ctl_on_count,false);
#else
                set_route_by_array(adev->mixer, adev->dev_cfgs[i].off,
                        adev->dev_cfgs[i].off_len);
#endif
            }
        }
    }
    /* update EQ profile*/
    /* only force set or output will set eq */
    if(adev->eq_available && ((pre_out != cur_out) || force_set )){
        int mode = GetAudio_mode_number_from_device(adev, cur_out, cur_in);
        vb_effect_profile_apply(mode);
    }
    SetAudio_gain_route(adev,vol,cur_out,cur_in);
out:
    pthread_mutex_unlock(&adev->device_lock);
#ifdef NXP_SMART_PA
    if (adev->smart_pa_exist) {
        if(fm_start)
            ret = NxpTfa_Fm_Start(adev->fmRes);
        else if(fm_close)
            ret = NxpTfa_Fm_Stop(adev->fmRes);
    }
#endif
    if (adev->vbc_access)
        mixer_ctl_set_value(adev->vbc_access, 0, 0);

    ALOGV("do_select_devices X");
}

static void select_devices_signal(struct tiny_audio_device *adev)
{
    ALOGE("select_devices_signal starting... adev->out_devices 0x%x adev->in_devices 0x%x",adev->out_devices,adev->in_devices);
        sem_post(&adev->routing_mgr.device_switch_sem);
    ALOGI("select_devices_signal finished.");
}

static void select_devices_signal_asyn(struct tiny_audio_device *adev)
{
    ALOGE("select_devices_signal_asyn starting... adev->out_devices 0x%x adev->in_devices 0x%x",adev->out_devices,adev->in_devices);
    sem_post(&adev->routing_mgr.device_switch_sem);
    ALOGI("select_devices_signal_asyn finished.");
}


static int start_call(struct tiny_audio_device *adev)
{
    ALOGE("Opening modem PCMs");
    return 0;
}

static void end_call(struct tiny_audio_device *adev)
{
    ALOGE("Closing modem PCMs");
}

static void set_eq_filter(struct tiny_audio_device *adev)
{

}

static void set_input_volumes(struct tiny_audio_device *adev, int main_mic_on,
        int headset_mic_on, int sub_mic_on)
{

}

static void set_output_volumes(struct tiny_audio_device *adev, bool tty_volume)
{
}

void force_all_standby(struct tiny_audio_device *adev)
{
    struct tiny_stream_in *in;
    struct tiny_stream_out *out;

    audio_offload_pause(adev);

    if (adev->active_output) {
        out = adev->active_output;
        pthread_mutex_lock(&out->lock);
        do_output_standby(out);
        pthread_mutex_unlock(&out->lock);
    }

    if (adev->active_deepbuffer_output) {
        out = adev->active_deepbuffer_output;
        pthread_mutex_lock(&out->lock);
        do_output_standby(out);
        pthread_mutex_unlock(&out->lock);
    }

    if (adev->active_input) {
        in = adev->active_input;
        pthread_mutex_lock(&in->lock);
        do_input_standby(in);
        pthread_mutex_unlock(&in->lock);
    }

}

static bool is_output_active(struct tiny_audio_device *adev){
    if ((adev->active_deepbuffer_output) || (adev->active_output)||adev->is_offload_running){
        return true;
    }else{
        return false;
    }
}

static void force_standby_for_voip(struct tiny_audio_device *adev)
{
    struct tiny_stream_in *in;
    struct tiny_stream_out *out;

    if(((adev->voip_state & VOIP_CAPTURE_STREAM) == 0)
        || ((adev->voip_state & VOIP_PLAYBACK_STREAM) == 0)) {
         audio_offload_pause(adev); 
    }

    if (adev->active_output && (!(adev->voip_state & VOIP_PLAYBACK_STREAM))) {
       out = adev->active_output;
       pthread_mutex_lock(&out->lock);
       do_output_standby(out);
       pthread_mutex_unlock(&out->lock);
    }

    if (adev->active_input && ( !(adev->voip_state & VOIP_CAPTURE_STREAM))) {
       in = adev->active_input;
       pthread_mutex_lock(&in->lock);
       do_input_standby(in);
       pthread_mutex_unlock(&in->lock);
    }

}


static void select_mode(struct tiny_audio_device *adev)
{
    if (adev->mode == AUDIO_MODE_IN_CALL) {
        ALOGE("Entering IN_CALL state, %s first call...out_devices:0x%x mode:%d ", adev->call_start ? "not":"is",adev->out_devices,adev->mode);
    } else {
        ALOGE("Leaving IN_CALL state, call_start=%d, mode=%d out_devices:0x%x ",
                adev->call_start, adev->mode,adev->out_devices);
    }
}

static int start_vaudio_output_stream(struct tiny_stream_out *out)
{
    unsigned int card = -1;
    unsigned int port = PORT_MM;
    int ret=0;
    struct tiny_audio_device *adev = NULL;
    cp_type_t cp_type;
#ifdef AUDIO_OLD_MODEM
    card = s_vaudio;
#endif
    out->cur_config = pcm_config_vplayback;
    out->buffer_vplayback = malloc(RESAMPLER_BUFFER_SIZE);
    if(!out->buffer_vplayback){
        ALOGE("start_vaudio_output_stream: alloc fail, size: %d", RESAMPLER_BUFFER_SIZE);
        goto error;
    }
    else {
        memset(out->buffer_vplayback, 0, RESAMPLER_BUFFER_SIZE);
    }

    cp_type = get_cur_cp_type(out->dev);
    if(cp_type == CP_TG) {
	    s_vaudio = get_snd_card_number(CARD_VAUDIO);
        card = s_vaudio;
    }
    else if (cp_type == CP_W) {
	    s_vaudio_w = get_snd_card_number(CARD_VAUDIO_W);
        card = s_vaudio_w;
    }
    else if (cp_type == CP_CSFB) {
        s_vaudio_lte = get_snd_card_number(CARD_VAUDIO_LTE);
        card = s_vaudio_lte;
    }
    if(out->flags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER)
        port = 1;
    else
        port = 0;

    ALOGI("start vaudio port :%d",port);
    BLUE_TRACE("start vaudio_output_stream cp_type is %d ,card is %d",cp_type, card);
#ifdef AUDIO_MUX_PCM
	out->pcm_vplayback= mux_pcm_open(SND_CARD_VOICE_TG, port, PCM_OUT| PCM_MMAP |PCM_NOIRQ, &out->cur_config);
#else
    #ifdef SUPPORT_64BIT_HAL
    out->pcm_vplayback= pcm_open(card, port, PCM_OUT, &out->cur_config);
    #else
    out->pcm_vplayback= pcm_open(card, port, PCM_OUT| PCM_MMAP |PCM_NOIRQ |PCM_MONOTONIC, &out->cur_config);
    #endif
#endif
    if (!pcm_is_ready(out->pcm_vplayback)) {
        ALOGE("%s:cannot open pcm_vplayback : %s", __func__,pcm_get_error(out->pcm_vplayback));
        goto error;
    }
    else {
        ret = create_resampler( DEFAULT_OUT_SAMPLING_RATE,
                out->cur_config.rate,
                out->config.channels,
                RESAMPLER_QUALITY_DEFAULT,
                NULL,
                &out->resampler_vplayback);
        if (ret != 0) {
            goto error;
        }
    }
    return 0;

error:
    adev =( struct tiny_audio_device *)out->dev;
    if(NULL!=adev){
        dump_all_audio_reg(-1,adev->dump_flag);
    }
    if(out->buffer_vplayback){
        free(out->buffer_vplayback);
        out->buffer_vplayback=NULL;
    }
    if(out->pcm_vplayback){
        ALOGE("%s: pcm_vplayback open error: %s", __func__, pcm_get_error(out->pcm_vplayback));
#ifdef AUDIO_MUX_PCM

#else
        pcm_close(out->pcm_vplayback);
#endif
        out->pcm_vplayback=NULL;
        ALOGE("start_vaudio_output_stream: out\n");
    }
    return -1;
}



static int open_voip_codec_pcm(struct tiny_audio_device *adev)
{
    struct pcm_config pcm_config_dl = pcm_config_vx_voip;
    struct pcm_config pcm_config_ul = pcm_config_vrec_vx_voip;

    ALOGD("voip:%s in adev->voip_state is %x", __func__, adev->voip_state);
       if(adev->pcm_fm_dl) {
           out_device_disable(adev,AUDIO_DEVICE_OUT_FM);
       }

#ifdef NXP_SMART_PA
    if (adev->smart_pa_exist)
        adev->internal_codec_rate = pcm_config_dl.rate;
#endif

        pthread_mutex_lock(&adev->vbc_dlulock);
       if(!adev->pcm_modem_dl) {
           adev->pcm_modem_dl= pcm_open(s_tinycard, PORT_MODEM, PCM_OUT, &pcm_config_dl);
           if (!pcm_is_ready(adev->pcm_modem_dl)) {
              ALOGE("%s:cannot open pcm_modem_dl : %s", __func__, pcm_get_error(adev->pcm_modem_dl));
              dump_all_audio_reg(-1,adev->dump_flag);
               pcm_close(adev->pcm_modem_dl);
               adev->pcm_modem_dl = NULL;
		ALOGE("voip:open voip_codec_pcm dl fail");
              pthread_mutex_unlock(&adev->vbc_dlulock);
		return -1;
           }
       }
    ALOGD("voip:open codec pcm in 2");
       if(!adev->pcm_modem_ul) {
           adev->pcm_modem_ul= pcm_open(s_tinycard, PORT_MODEM, PCM_IN, &pcm_config_ul);
           if (!pcm_is_ready(adev->pcm_modem_ul)) {
               ALOGE("%s:cannot open pcm_modem_ul : %s", __func__, pcm_get_error(adev->pcm_modem_ul));
               dump_all_audio_reg(-1,adev->dump_flag);
              pcm_close(adev->pcm_modem_ul);
               adev->pcm_modem_ul = NULL;
		ALOGE("voip:open voip_codec_pcm ul fail");
              pthread_mutex_unlock(&adev->vbc_dlulock);
		return -1;
           }
       }

	if( 0 != pcm_start(adev->pcm_modem_dl)) {
		 ALOGE("voip:pcm_start pcm_modem_dl start unsucessfully");
	}
	if( 0 != pcm_start(adev->pcm_modem_ul)) {
	    ALOGE("voip:pcm_start pcm_modem_ul start unsucessfully");
	}
    pthread_mutex_unlock(&adev->vbc_dlulock);

    ALOGD("voip:open codec pcm out 2");

    ALOGE("voip:open voip_codec_pcm out");
    return 0;
}

static int  close_voip_codec_pcm(struct tiny_audio_device *adev)
{
    ALOGD("voip:close voip codec pcm in adev->voip_state is %x",adev->voip_state);
    if(!adev->voip_state) {
       pthread_mutex_lock(&adev->vbc_dlulock);
       if(adev->pcm_modem_ul) {
           pcm_close(adev->pcm_modem_ul);
           adev->pcm_modem_ul = NULL;
       }
       if(adev->pcm_modem_dl) {
           pcm_close(adev->pcm_modem_dl);
           adev->pcm_modem_dl = NULL;
       }
	     pthread_mutex_unlock(&adev->vbc_dlulock);

       if(adev->fm_open){
           adev->device_force_set = 1;
           select_devices_signal_asyn(adev);
       }
       audio_offload_resume(adev);
    }
    ALOGD("voip:close voip codec pcm out");
    return 0;
}




#ifdef AUDIO_MUX_PCM
static int start_mux_output_stream(struct tiny_stream_out *out)
{
    unsigned int card = 0;
    unsigned int port = PORT_MM;
    int ret=0;
    card = s_vaudio;
    out->cur_config = pcm_config_vplayback;
    out->buffer_vplayback = malloc(RESAMPLER_BUFFER_SIZE);
    if(!out->buffer_vplayback){
        ALOGE("start_mux_output_stream: alloc fail, size: %d", RESAMPLER_BUFFER_SIZE);
        goto error;
    }
    else {
        memset(out->buffer_vplayback, 0, RESAMPLER_BUFFER_SIZE);
    }
    out->pcm_vplayback= mux_pcm_open(card, port, PCM_OUT, &out->cur_config);
    if (!pcm_is_ready(out->pcm_vplayback)) {
        ALOGE("%s:mux_pcm_open pcm is not ready!!!", __func__);
        goto error;
    }
    else {
        ret = create_resampler( DEFAULT_OUT_SAMPLING_RATE,
                out->cur_config.rate,
                out->config.channels,
                RESAMPLER_QUALITY_DEFAULT,
                NULL,
                &out->resampler_vplayback);
        if (ret != 0) {
            goto error;
        }
    }
    return 0;

error:
    if(out->buffer_vplayback){
        free(out->buffer_vplayback);
        out->buffer_vplayback=NULL;
    }
    if(out->pcm_vplayback){
        ALOGE("start_vaudio_output_stream error: %s", pcm_get_error(out->pcm_vplayback));
        mux_pcm_close(out->pcm_vplayback);
        out->pcm_vplayback=NULL;
        ALOGE("start_vaudio_output_stream: out\n");
    }
    return -1;
}
#endif
static int start_sco_output_stream(struct tiny_stream_out *out)
{
    unsigned int card = -1;
    unsigned int port = PORT_MM;
	struct tiny_audio_device *adev = out->dev;

    int ret=0;
    BLUE_TRACE(" start_sco_output_stream in");
    s_voip = get_snd_card_number(CARD_SCO);
    card = s_voip;
    out->buffer_voip = malloc(RESAMPLER_BUFFER_SIZE);
    if(!out->buffer_voip){
        ALOGE("start_sco_output_stream: alloc fail, size: %d", RESAMPLER_BUFFER_SIZE);
        goto error;
    }
    else {
        memset(out->buffer_voip, 0, RESAMPLER_BUFFER_SIZE);
    }

    ALOGD("start_sco_output_stream ok 1 ");
    open_voip_codec_pcm(adev);
    out->cur_config = pcm_config_scoplayback;

#ifdef AUDIO_MUX_PCM
    out->pcm_voip = mux_pcm_open(SND_CARD_VOIP_TG, port, PCM_OUT| PCM_MMAP |PCM_NOIRQ, &out->cur_config);
#else
    #ifdef SUPPORT_64BIT_HAL
    out->pcm_voip = pcm_open(card, port, PCM_OUT, &out->cur_config);
    #else
    out->pcm_voip = pcm_open(card, port, PCM_OUT| PCM_MMAP |PCM_NOIRQ |PCM_MONOTONIC, &out->cur_config);
    #endif
#endif
    ALOGD("start_sco_output_stream ok 2");


    if (!pcm_is_ready(out->pcm_voip)) {
        ALOGE("%s:cannot open pcm_voip : %s", __func__,pcm_get_error(out->pcm_voip));
        goto error;
    }
    else {
        ret = create_resampler( DEFAULT_OUT_SAMPLING_RATE,
                out->cur_config.rate,
                out->config.channels,
                RESAMPLER_QUALITY_DEFAULT,
                NULL,
                &out->resampler_sco);
        if (ret != 0) {
            goto error;
        }
    }

     audio_offload_resume(adev); 

    ALOGE("start_sco_output_stream ok");
    return 0;

error:
    ALOGE("start_sco_output_stream error ");
    dump_all_audio_reg(-1,adev->dump_flag);
    if(out->buffer_voip){
        free(out->buffer_voip);
        out->buffer_voip=NULL;
    }
    if(out->pcm_voip){
        ALOGE("start_sco_output_stream error: %s", pcm_get_error(out->pcm_voip));
#ifdef AUDIO_MUX_PCM
	mux_pcm_close(out->pcm_voip);
#else
        pcm_close(out->pcm_voip);
#endif
        out->pcm_voip=NULL;
        ALOGE("start_sco_output_stream: out\n");
    }
    return -1;
}

static int start_bt_sco_output_stream(struct tiny_stream_out *out)
{
    unsigned int card = 0;
    unsigned int port = PORT_MM;
    struct tiny_audio_device *adev = out->dev;
    int ret=0;
    BLUE_TRACE(" start_bt_sco_output_stream in");
    card = s_bt_sco;
    out->buffer_bt_sco = malloc(RESAMPLER_BUFFER_SIZE);
    if(!out->buffer_bt_sco){
        ALOGE("start_bt_sco_output_stream: alloc fail, size: %d", RESAMPLER_BUFFER_SIZE);
        goto error;
    }
    else {
        memset(out->buffer_bt_sco, 0, RESAMPLER_BUFFER_SIZE);
    }

    /* if bt sco capture stream has already been started, we just close bt sco capture
       stream. we will call start_input_stream in the next in_read func to start bt sco
       capture stream again.
       here we must get in->lock, because in_read maybe call pcm_read in the same time.
    */
    ALOGE("bt sco : %s before", __func__);
    if(adev->bt_sco_state & BT_SCO_UPLINK_IS_STARTED) {
        struct tiny_stream_in *bt_sco_in = adev->active_input;
        if(bt_sco_in) {
            ALOGE("bt sco : %s do_input_standby", __func__);
            pthread_mutex_lock(&bt_sco_in->lock);
            do_input_standby(bt_sco_in);
            pthread_mutex_unlock(&bt_sco_in->lock);
        }
    }
    adev->bt_sco_state |= BT_SCO_DOWNLINK_IS_EXIST;

    ALOGE("bt sco : %s after", __func__);
    if(adev->bluetooth_type == VX_WB_SAMPLING_RATE)
        out->cur_config = pcm_config_btscoplayback_wb;
    else
        out->cur_config = pcm_config_btscoplayback_nb;

    ALOGI("%s set i2s para samplerate:%d", __func__, adev->bluetooth_type);
    set_i2s_parameter(adev->i2s_mixer, adev->bluetooth_type);

#ifdef USE_PCM_IRQ_MODE
    out->pcm_bt_sco = pcm_open(card, port, PCM_OUT | PCM_MONOTONIC, &out->cur_config);
#else
    out->pcm_bt_sco = pcm_open(card, port, PCM_OUT| PCM_MMAP |PCM_NOIRQ | PCM_MONOTONIC, &out->cur_config);
#endif

    if (!pcm_is_ready(out->pcm_bt_sco)) {
        ALOGE("%s:cannot open pcm_bt_sco : %s", __func__,pcm_get_error(out->pcm_bt_sco));
        goto error;
    }
    else {
        ret = create_resampler( DEFAULT_OUT_SAMPLING_RATE,
                out->cur_config.rate,
                out->config.channels,
                RESAMPLER_QUALITY_DEFAULT,
                NULL,
                &out->resampler_bt_sco);
        if (ret != 0) {
            goto error;
        }
    }

    ALOGE("start_bt_sco_output_stream error ok");
    return 0;

error:
    ALOGE("start_sco_output_stream error ");
    dump_all_audio_reg(-1,adev->dump_flag);
    if(out->buffer_bt_sco){
        free(out->buffer_bt_sco);
        out->buffer_bt_sco=NULL;
    }
    if(out->pcm_bt_sco){
        ALOGE("start_sco_output_stream error: %s", pcm_get_error(out->pcm_bt_sco));
        pcm_close(out->pcm_bt_sco);
        out->pcm_bt_sco=NULL;
        ALOGE("start_sco_output_stream: out\n");
    }
    return -1;
}

static int start_sco_mixer_stream(struct tiny_stream_out *out)
{
    unsigned int card = -1;
    unsigned int port = 1;
    struct tiny_audio_device *adev = out->dev;
    int ret=0;

    ALOGI("start_sco_mixer_stream in");
    s_voip = get_snd_card_number(CARD_SCO);
    card = s_voip;
    out->buffer_voip = malloc(RESAMPLER_BUFFER_SIZE);
    if(!out->buffer_voip){
        ALOGE("start_sco_mixer_stream: alloc fail, size: %d", RESAMPLER_BUFFER_SIZE);
        goto error;
    }
    else {
        memset(out->buffer_voip, 0, RESAMPLER_BUFFER_SIZE);
    }

    out->cur_config = pcm_config_scoplayback;
#ifdef AUDIO_MUX_PCM
    out->pcm_voip_mixer = mux_pcm_open(SND_CARD_VOIP_TG, port, PCM_OUT| PCM_MMAP |PCM_NOIRQ, &out->cur_config);
#else
    #ifdef SUPPORT_64BIT_HAL
    out->pcm_voip_mixer = pcm_open(card, port, PCM_OUT, &out->cur_config);
    #else
    out->pcm_voip_mixer = pcm_open(card, port, PCM_OUT| PCM_MMAP |PCM_NOIRQ |PCM_MONOTONIC, &out->cur_config);
    #endif
#endif

    if (!pcm_is_ready(out->pcm_voip_mixer)) {
        ALOGE("%s:cannot open pcm_voip_mixer : %s", __func__,pcm_get_error(out->pcm_voip));
        goto error;
    }
    else {
        ret = create_resampler( DEFAULT_OUT_SAMPLING_RATE,
                out->cur_config.rate,
                out->config.channels,
                RESAMPLER_QUALITY_DEFAULT,
                NULL,
                &out->resampler_sco);
        if (ret != 0) {
            goto error;
        }
    }

    ALOGI("start_sco_mixer_stream out");
    return 0;

error:
    ALOGE("start_sco_mixer_stream error ");
    if(out->buffer_voip){
        free(out->buffer_voip);
        out->buffer_voip=NULL;
    }
    if(out->pcm_voip_mixer){
        ALOGE("start_sco_mixer_stream error: %s", pcm_get_error(out->pcm_voip_mixer));
#ifdef AUDIO_MUX_PCM
	mux_pcm_close(out->pcm_voip_mixer);
#else
        pcm_close(out->pcm_voip_mixer);
#endif
        out->pcm_voip_mixer=NULL;
        ALOGE("start_sco_mixer_stream: out");
    }
    return -1;
}


/* must be called with hw device and output stream mutexes locked */
static int start_output_stream(struct tiny_stream_out *out)
{
    struct tiny_audio_device *adev = out->dev;
    unsigned int card = 0;
    unsigned int port = PORT_MM;
    int ret=0;

    if (adev->deepbuffer_enable) {
        if (AUDIO_OUTPUT_FLAG_DEEP_BUFFER&out->flags) {
            if(NULL==adev->active_deepbuffer_output){
                out->devices=adev->out_devices;
            }
            adev->active_deepbuffer_output = out;
            out->is_bt_sco = false;
            out->is_voip = false;
        } else {
            if(out->audio_app_type != AUDIO_HW_APP_OFFLOAD) {
                if(NULL==adev->active_output){
                    out->devices=adev->out_devices;
                }
                adev->active_output = out;
            }
        }
    } else {
        if(out->audio_app_type != AUDIO_HW_APP_OFFLOAD) {
            adev->active_output = out;
        }
    }

    ALOGD("start output stream mode:%x devices:%x call_start:%d, call_connected:%d, is_voip:%d, voip_state:%x, is_bt_sco:%d",
        adev->mode, adev->out_devices, adev->call_start, adev->call_connected, out->is_voip,adev->voip_state, out->is_bt_sco);
    pthread_mutex_lock(&adev->mute_lock);
    if(adev->master_mute){
        adev->master_mute = false;
    }
    pthread_mutex_unlock(&adev->mute_lock);

    if (!adev->call_start && adev->voip_state == 0) {
        /* FIXME: only works if only one output can be active at a time*/
        adev->out_devices &= (~AUDIO_DEVICE_OUT_ALL);
        adev->out_devices |= out->devices;
        if(adev->out_devices & AUDIO_DEVICE_OUT_ALL_SCO) {
            i2s_pin_mux_sel(adev,AP_TYPE);
        }
        adev->prev_out_devices = ~adev->out_devices;
        select_devices_signal(adev);
    }
    else if (adev->voip_state) {
        at_cmd_cp_usecase_type(AUDIO_CP_USECASE_VOIP_1);  /* set the usecase type to cp side */
        if((adev->out_devices &AUDIO_DEVICE_OUT_ALL) != out->devices) {
            adev->out_devices &= (~AUDIO_DEVICE_OUT_ALL);
            adev->out_devices |= out->devices;
        }
#ifndef AUDIO_OLD_MODEM
        if(adev->out_devices & AUDIO_DEVICE_OUT_ALL_SCO) {
            if(adev->cp_type == CP_TG)
                i2s_pin_mux_sel(adev,1);
            else if(adev->cp_type == CP_W)
                i2s_pin_mux_sel(adev,0);
            else if( adev->cp_type ==  CP_CSFB)
                i2s_pin_mux_sel(adev,CP_CSFB);
        }
#endif
	adev->prev_out_devices = ~adev->out_devices;
        select_devices_signal(adev);
    }
    /* default to low power: will be corrected in out_write if necessary before first write to
     * tinyalsa.
     */
    if(out->is_voip_mixer){
        ret = start_sco_mixer_stream(out);
        if(ret){
            return ret;
        }
    }else if(out->is_voip){
        ret=start_sco_output_stream(out);
        if(ret){
            return ret;
        }
    }
    else if(out->is_bt_sco){
        ret=start_bt_sco_output_stream(out);
        if(ret){
            return ret;
        }
    }
    else if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        ret = audio_start_compress_output(out);
        out_codec_mute_check(adev);
        register_offload_pcmdump(out->compress,audio_pcm_dump);
        return ret;
    }
    else if(adev->call_connected && ( !out->pcm_vplayback)) {
        set_cp_mixer_type(adev->cp_mixer[adev->cp_type], adev->mixer_route);

        ret=start_vaudio_output_stream(out);
        if(ret){
            return ret;
        }
    }
    else {
        BLUE_TRACE("open s_tinycard in");
        card = s_tinycard;
        if(out->flags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER) {
            out->cur_config = pcm_config_mm_deepbuf;
            port = PORT_DEPP_BUFFER;
        }
        else {
            out->cur_config = pcm_config_mm_fast;
        }
        out->low_power = 0;
#ifdef USE_PCM_IRQ_MODE
        out->pcm = pcm_open(card, port, PCM_OUT | PCM_MONOTONIC, &out->cur_config);
#else
        out->pcm = pcm_open(card, port, PCM_OUT | PCM_MMAP | PCM_NOIRQ | PCM_MONOTONIC, &out->cur_config);
#endif
        if (!pcm_is_ready(out->pcm)) {
            ALOGE("%s: cannot open pcm: %s", __func__, pcm_get_error(out->pcm));
            dump_all_audio_reg(-1,adev->dump_flag);
            pcm_close(out->pcm);
            out->pcm = NULL;

            if(adev->deepbuffer_enable){
                if(AUDIO_OUTPUT_FLAG_DEEP_BUFFER&out->flags){
                    adev->active_deepbuffer_output = NULL;
                }else{
                    adev->active_output = NULL;
                }
            }else{
                adev->active_output = NULL;
            }

            return -ENOMEM;
        }

        if (out->flags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER) {
            out->low_power = adev->low_power;
            if (out->low_power) {
                    int avail_min_ms=0;
                    out->write_threshold =pcm_config_mm_deepbuf.period_size*pcm_config_mm_deepbuf.period_count;
                    out->cur_config.avail_min = ( out->write_threshold *3 ) /4;
                    avail_min_ms=out->cur_config.avail_min* 1000 /(out->cur_config.rate*4);
                    ALOGW("out->write_threshold=%d, config.avail_min=%d %dms, start_threshold=%d",
                            out->write_threshold,out->cur_config.avail_min, avail_min_ms,out->cur_config.start_threshold);
            } else {
                    int avail_min_ms=0;
                    out->write_threshold = pcm_config_mm_deepbuf.start_threshold;
                    out->cur_config.avail_min = out->write_threshold;
                    avail_min_ms=out->cur_config.avail_min* 1000 /(out->cur_config.rate*4);
                    ALOGW("out->write_threshold=%d, config.avail_min=%d %dms, start_threshold=%d",
                            out->write_threshold,out->cur_config.avail_min, avail_min_ms,out->cur_config.start_threshold);
            }
            pcm_set_avail_min(out->pcm, out->cur_config.avail_min);
        }
        BLUE_TRACE("open s_tinycard successfully");
    }

    out->resampler->reset(out->resampler);
#ifdef AUDIO_DUMP
    out_dump_create(&out->out_dump_fd, AUDIO_OUT_FILE_PATH);
#endif

    out_codec_mute_check(adev);
    return 0;
}

static int check_input_parameters(uint32_t sample_rate, int format, int channel_count)
{
    if (format != AUDIO_FORMAT_PCM_16_BIT)
        return -EINVAL;

    if ((channel_count < 1) || (channel_count > 2))
        return -EINVAL;

    switch(sample_rate) {
        case 8000:
        case 11025:
        case 16000:
        case 22050:
        case 24000:
        case 32000:
        case 44100:
        case 48000:
            break;
        default:
            return -EINVAL;
    }

    return 0;
}

static size_t get_input_buffer_size(uint32_t sample_rate, int format, int channel_count)
{
    size_t size;
    size_t device_rate;

    if (check_input_parameters(sample_rate, format, channel_count) != 0)
        return 0;

    /* take resampling into account and return the closest majoring
       multiple of 16 frames, as audioflinger expects audio buffers to
       be a multiple of 16 frames */
    size = (pcm_config_mm_ul.period_size * sample_rate) / pcm_config_mm_ul.rate;
    size = ((size + 15) / 16) * 16;

    return size * channel_count * sizeof(short);
}

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD){
        return out->offload_samplerate;
    }
    return out->sample_rate;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    return 0;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD)
        return out->compress_config.fragment_size;
    /* take resampling into account and return the closest majoring
       multiple of 16 frames, as audioflinger expects audio buffers to
       be a multiple of 16 frames */
    size_t size = (out->config.period_size * DEFAULT_OUT_SAMPLING_RATE) / out->config.rate;
    size = ((size + 15) / 16) * 16;
    BLUE_TRACE("[TH] size=%d, frame_size=%d", size, audio_stream_frame_size(stream));
    return size * audio_stream_frame_size(stream);
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    return (audio_channel_mask_t)out->channel_mask;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD){
        return out->offload_format;
    }
    return out->format;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    return 0;
}

/* must be called with hw device locked */
static int do_voip_mixer_standby(struct tiny_audio_device *adev)
{
    struct tiny_stream_out *out = NULL;
    audio_offload_pause(adev);
    if(adev->active_deepbuffer_output && adev->active_deepbuffer_output->is_voip_mixer) {
        out = adev->active_deepbuffer_output;
        pthread_mutex_lock(&out->lock);
        if(out->is_voip_mixer) {
            pcm_close(out->pcm_voip_mixer);
            out->pcm_voip_mixer =NULL;
            out->is_voip_mixer = 0;
        }
        if(out->buffer_voip) {
            free(out->buffer_voip);
            out->buffer_voip = 0;
        }
        if(out->resampler_sco) {
            release_resampler(out->resampler_sco);
            out->resampler_sco= 0;
        }
        out->standby = 1;
        pthread_mutex_unlock(&out->lock);
        adev->active_deepbuffer_output = NULL;
    }
    return 0;
}



/* must be called with hw device and output stream mutexes locked */
static int do_output_standby(struct tiny_stream_out *out)
{
    struct tiny_audio_device *adev = out->dev;
    ALOGV("do_output_standby in %d",out->audio_app_type);
    if (!out->standby) {
        if (out->pcm) {
            pcm_close(out->pcm);
            out->pcm = NULL;
        }
        BLUE_TRACE("do_output_standby.mode:%d ",adev->mode);

        if(out->pcm_voip && out->is_voip) {
                if(!(adev->voip_state &(~VOIP_PLAYBACK_STREAM))) {
                    ALOGI("do_voip_mixer_standby in out_standby");
                    do_voip_mixer_standby(adev);
                }
        }

        if (adev->deepbuffer_enable) {
            if(AUDIO_OUTPUT_FLAG_DEEP_BUFFER&out->flags){
                if(out->is_voip_mixer && out->pcm_voip_mixer) {
                    ALOGI("do_voip_mixer_standby in out_standby byself");
                    pcm_close(out->pcm_voip_mixer);
                    out->pcm_voip_mixer = NULL;
                }
                out->is_voip_mixer = 0;
                adev->active_deepbuffer_output = NULL;
            }else{
                adev->active_output = NULL;
            }
        } else {
            adev->active_output = NULL;
        }

        if(out->pcm_voip) {
#ifdef AUDIO_MUX_PCM
	mux_pcm_close(out->pcm_voip);
#else
            pcm_close(out->pcm_voip);
#endif
            out->pcm_voip = NULL;
        }
        if(out->buffer_voip) {
            free(out->buffer_voip);
            out->buffer_voip = 0;
        }
        if(out->resampler_sco) {
            release_resampler(out->resampler_sco);
            out->resampler_sco= 0;
        }
        if(out->is_voip) {
            adev->voip_state &= (~VOIP_PLAYBACK_STREAM);
            if(!adev->voip_state) {
                if(!adev->voip_state) {
                    close_voip_codec_pcm(adev);
                }
            }
            out->is_voip = false;
        }
        if(out->pcm_bt_sco) {
            /* if bt sco capture stream is started now, we must stop bt sco capture
               stream first when we close bt sco playback stream.
               we will call start_input_stream in the next in_read func to start bt
               sco capture stream again.
            */
            ALOGE("bt sco : %s before", __func__);
            if(adev->bt_sco_state & BT_SCO_UPLINK_IS_STARTED) {
                struct tiny_stream_in *bt_sco_in = adev->active_input;
                if(bt_sco_in) {
                    ALOGE("bt sco : %s do_input_standby", __func__);
                    pthread_mutex_lock(&bt_sco_in->lock);
                    do_input_standby(bt_sco_in);
                    pthread_mutex_unlock(&bt_sco_in->lock);
                }
            }
            ALOGE("bt sco : %s after", __func__);

            pcm_close(out->pcm_bt_sco);
            out->pcm_bt_sco = NULL;

            ALOGE("bt sco : %s downlink is not exist", __func__);
            adev->bt_sco_state &= (~BT_SCO_DOWNLINK_IS_EXIST);
        }
        if(out->buffer_bt_sco) {
            free(out->buffer_bt_sco);
            out->buffer_bt_sco = 0;
        }
        if(out->resampler_bt_sco) {
            release_resampler(out->resampler_bt_sco);
            out->resampler_bt_sco= 0;
        }
        if(out->is_bt_sco) {
            out->is_bt_sco=false;
        }
        if(out->pcm_vplayback) {
#ifdef AUDIO_MUX_PCM
            mux_pcm_close(out->pcm_vplayback);
#else
            pcm_close(out->pcm_vplayback);
#endif

            out->pcm_vplayback = NULL;
            if(out->buffer_vplayback) {
                free(out->buffer_vplayback);
                out->buffer_vplayback = 0;
            }
            if(out->resampler_vplayback) {
                release_resampler(out->resampler_vplayback);
                out->resampler_vplayback = 0;
            }
        }

#ifdef AUDIO_DUMP
        out_dump_release(&out->out_dump_fd);
#endif

        if (out->pSmartAmpCtl && adev->pSmartAmpMgr->isEnable &&
            adev->pSmartAmpMgr->isBoostDevice) {
            smart_amp_set_boost(out->pSmartAmpCtl, BOOST_BYPASS);
        }
        out->standby = 1;
    }

#ifdef NXP_SMART_PA
    if(adev->smart_pa_exist && out->nxp_pa) {
        out->nxp_pa = false;
        if(out->handle_pa) {
            NxpTfa_Close(out->handle_pa);
            out->handle_pa = NULL;
        }
    }
#endif

    if(adev->deepbuffer_enable){
        if(AUDIO_OUTPUT_FLAG_DEEP_BUFFER&out->flags){
            adev->active_deepbuffer_output = NULL;
        }else{
            adev->active_output = NULL;
        }
    }else{
        adev->active_output = NULL;
    }
    out_codec_mute_check(adev);
    mp3_low_power_check(adev);
    ALOGV("do_output_standby in out");
    return 0;
}

static int out_standby(struct audio_stream *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    int status;

    pthread_mutex_lock(&out->dev->lock);
    pthread_mutex_lock(&out->lock);
    status = do_output_standby(out);
    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);
    return status;
}

static int do_offload_out_standby(struct tiny_stream_out *out)
{
    struct tiny_audio_device *adev = out->dev;
     if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        audio_stop_compress_output(out, 0);
        out->gapless_mdata.encoder_delay = 0;
        out->gapless_mdata.encoder_padding = 0;
        if (out->compress != NULL) {
            ALOGE("audio_offload compress_close");
            compress_close(out->compress);
            out->compress = NULL;
        }
        if(adev->active_offload_output == out){
            adev->audio_outputs_state &= ~ AUDIO_OUTPUT_DESC_OFFLOAD;
            adev->active_offload_output = NULL;
        }
        out->standby = 1;
    }else {
        if(adev->active_offload_output == out){
            adev->audio_outputs_state &= ~ AUDIO_OUTPUT_DESC_PRIMARY;
        }
    }
    return 0;
}

static int offload_out_standby(struct audio_stream *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    ALOGE("%s in", __func__);
    pthread_mutex_lock(&out->lock);
    if((out->audio_app_type == AUDIO_HW_APP_OFFLOAD) && (adev->active_offload_output == out)) {
        audio_offload_mute(out);
    }
    pthread_mutex_unlock(&out->lock);
    pthread_mutex_lock(&out->dev->lock);
    pthread_mutex_lock(&out->lock);
    do_offload_out_standby(out);
    out_codec_mute_check(adev);
    mp3_low_power_check(adev);
    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);
    ALOGE("%s out", __func__);
    return 0;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;

    char buffer[DUMP_BUFFER_MAX_SIZE]={0};

    if(out->standby){
        snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
            "\noutput dump ---->\n"
            "flags:%x  "
            "audio_app_type:%d standby\n",
            out->flags,
            out->audio_app_type);
        AUDIO_DUMP_WRITE_STR(fd,buffer);
    }else{
        snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
            "\noutput dump ---->\n "
            "flags:%x  "
            "audio_app_type:%d  \n"
            "pcm:pcm:%p "
            "pcm_vplayback::%p "
            "pcm_voip::%p "
            "pcm_voip_mixer::%p "
            "pcm_bt_sco::%p\n",
            out->flags,
            out->audio_app_type,
            out->pcm,
            out->pcm_vplayback,
            out->pcm_voip,
            out->pcm_voip_mixer,
            out->pcm_bt_sco);
        AUDIO_DUMP_WRITE_STR(fd,buffer);
        memset(buffer,0,sizeof(buffer));

        snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
            "point:%p %p %p %p %p %p %p %p\n",
            out->resampler_vplayback,
            out->resampler_sco,
            out->resampler_bt_sco,
            out->resampler,
            out->buffer,
            out->buffer_vplayback,
            out->buffer_voip,
            out->buffer_bt_sco);
        AUDIO_DUMP_WRITE_STR(fd,buffer);
        memset(buffer,0,sizeof(buffer));

        snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
            "config:%d %d %d %d 0x%x 0x%x 0x%x\n",
            out->config.channels,
            out->config.rate,
            out->config.period_size,
            out->config.period_count,
            out->config.start_threshold,
            out->config.avail_min,
            out->config.stop_threshold);
        AUDIO_DUMP_WRITE_STR(fd,buffer);
        memset(buffer,0,sizeof(buffer));

        snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
            "config:%d %d %d %d 0x%x 0x%x 0x%x\n",
            out->cur_config.channels,
            out->cur_config.rate,
            out->cur_config.period_size,
            out->cur_config.period_count,
            out->cur_config.start_threshold,
            out->cur_config.avail_min,
            out->cur_config.stop_threshold);
        AUDIO_DUMP_WRITE_STR(fd,buffer);
        memset(buffer,0,sizeof(buffer));

        snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
            "is_voip:%d "
            "is_voip_mixer:%d "
            "is_bt_sco:%d "
            "devices:0x%x "
            "write_threshold:%d "
            "low_power:%d "
            "written:%ld \n",
            out->is_voip,
            out->is_voip_mixer,
            out->is_bt_sco,
            out->devices,
            out->write_threshold,
            out->low_power,
            out->written);
        AUDIO_DUMP_WRITE_STR(fd,buffer);
        memset(buffer,0,sizeof(buffer));

        if(AUDIO_HW_APP_OFFLOAD==out->audio_app_type){
            snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
                "offload:state:%d "
                "forma::%d "
                "samplerate::%d "
                "channel_mask::%d "
                "compress_started:%d "
                "set_metadata:%d "
                "thread_blocked;:%d "
                "nonblocking:%d"
                "volume_ctl:%p\n",
                out->audio_offload_state,
                out->offload_format,
                out->offload_samplerate,
                out->channel_mask,
                out->is_offload_compress_started,
                out->is_offload_need_set_metadata,
                out->is_audio_offload_thread_blocked,
                out->is_offload_nonblocking,
                out->offload_volume_ctl);
            AUDIO_DUMP_WRITE_STR(fd,buffer);
            memset(buffer,0,sizeof(buffer));
        }

        if (out->pSmartAmpCtl && adev->pSmartAmpMgr->isEnable &&
            adev->pSmartAmpMgr->isBoostDevice) {
            smart_amp_dump(fd,out->pSmartAmpCtl);
        }
    }

    snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),"<----output dump\n");
    AUDIO_DUMP_WRITE_STR(fd,buffer);
    ALOGI("adev_dump exit");

    return 0;
}

static int out_devices_check_unlock(struct audio_stream *stream,audio_devices_t device){
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    struct listnode *item;
    struct listnode *item2;

    if(!list_empty(&adev->active_out_list)){
        list_for_each_safe(item, item2,&adev->active_out_list){
            out = node_to_item(item, struct tiny_stream_out, node);
            if(out!=stream){
                out->devices=device;
            }
        }
    }
    return 0;
}


static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    struct tiny_stream_in *in;
    struct str_parms *parms;
    char *str;
    char value[32];
    int ret, val = 0;
    static int cur_mode = 0;
    int status = 0;

    BLUE_TRACE("[out_set_parameters], kvpairs=%s devices:0x%x mode:%d ", kvpairs,adev->out_devices,adev->mode);
    /*set Empty Parameter then return 0*/
    if (0 == strncmp(kvpairs, "", sizeof(char))) {
        return status;
    }

    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        ALOGV("[out_set_parameters],after str_parms_get_str,val(0x%x) ",val);
        pthread_mutex_lock(&adev->lock);
        pthread_mutex_lock(&out->lock);
		  if (((adev->out_devices & AUDIO_DEVICE_OUT_ALL) != val) && ((val != 0)) //val=0 will cause XRUN. So ignore the "val=0"expect for closing FM path.
                  || ((AUDIO_MODE_IN_CALL == adev->mode) && (val != 0))
                  ||((out->devices!=val)&&(val != 0))
                  || (adev->voip_start && (val != 0))) {
            adev->out_devices &= ~AUDIO_DEVICE_OUT_ALL;
            adev->out_devices |= val;
            out->devices = val;
            ALOGW("out_set_parameters want to set devices:0x%x old_mode:%d new_mode:%d call_start:%d ",adev->out_devices,cur_mode,adev->mode,adev->call_start);
            out_devices_check_unlock(out,val);
            if(1 == adev->call_start) {
                if(adev->out_devices & AUDIO_DEVICE_OUT_ALL_SCO) {
                    if(adev->cp_type == CP_TG)
                        i2s_pin_mux_sel(adev,1);
                    else if(adev->cp_type == CP_W)
                        i2s_pin_mux_sel(adev,0);
                    else if( adev->cp_type ==  CP_CSFB)
                        i2s_pin_mux_sel(adev,CP_CSFB);
                }
            }
 #ifndef AUDIO_OLD_MODEM
            else if(adev->voip_start){
                if(adev->out_devices & AUDIO_DEVICE_OUT_ALL_SCO) {
                    if(adev->cp_type == CP_TG)
                        i2s_pin_mux_sel(adev,1);
                    else if(adev->cp_type == CP_W)
                        i2s_pin_mux_sel(adev,0);
                    else if( adev->cp_type ==  CP_CSFB)
                        i2s_pin_mux_sel(adev,CP_CSFB);
                }
            }
 #endif
            else {
                if(adev->out_devices & AUDIO_DEVICE_OUT_ALL_SCO) {
                    i2s_pin_mux_sel(adev,AP_TYPE);
                }
            }

            cur_mode = adev->mode;
            if((!adev->call_start)&&(!adev->voip_state))
                select_devices_signal(adev);
            if((AUDIO_MODE_IN_CALL == adev->mode) || adev->voip_start) {
                ret = at_cmd_route(adev);  //send at command to cp
            }
            pthread_mutex_unlock(&out->lock);
            pthread_mutex_unlock(&adev->lock);
          } else {
            if(val != 0) {
                out_devices_check_unlock(out,val);
            }
              pthread_mutex_unlock(&out->lock);
            pthread_mutex_unlock(&adev->lock);
            ALOGW("the same devices(0x%x) with val(0x%x) val is zero...",adev->out_devices,val);
        }
        ret = 0 ;
    }
    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        audio_get_compress_metadata(out, parms);
    }
    ALOGV("out_set_parameters out...call_start:%d",adev->call_start);
    str_parms_destroy(parms);
    return status;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    uint32_t latency_time ;
    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD)
        return 100;
    latency_time = (out->config.period_size * out->config.period_count * 1000) /
           out->config.rate;
    if(out->flags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER){
        latency_time = out->config.start_threshold * 1000 / out->config.rate ;
    }

    return latency_time;
}

static int out_set_volume(struct audio_stream_out *stream, float left,
        float right)
{
    long volume[2];
    int need_unmute = 0;
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    pthread_mutex_lock(&out->lock);
    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD){

        const char *mixer_ctl_name = "VBC DAC23 MIXERDG";
        struct tiny_audio_device *adev = out->dev;
        if(!out->offload_volume_ctl) {
            struct mixer_ctl *ctl;

            ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
            if (!ctl) {
                pthread_mutex_unlock(&out->lock);
                ALOGE("%s: Could not get ctl for mixer cmd - %s",
                      __func__, mixer_ctl_name);
                return -EINVAL;
            }
            out->offload_volume_ctl = ctl;
        }
        volume[0] = (long)(left * AUDIO_OFFLOAD_PLAYBACK_VOLUME_MAX);
        volume[1] = (long)(right * AUDIO_OFFLOAD_PLAYBACK_VOLUME_MAX);
        if((out->left_volume == 0) && (out->right_volume == 0)
            && (volume[0] || volume[1])) {
            need_unmute = 1;
        }
        out->left_volume = volume[0];
        out->right_volume = volume[1];
        mixer_ctl_set_array(out->offload_volume_ctl, volume, sizeof(volume)/sizeof(volume[0]));
        compress_set_gain(out->compress,out->left_volume, out->right_volume);
        ALOGD("%s, offload left volume [%d] ,right volume [%d] ",
                __func__, volume[0], volume[1]);
        pthread_mutex_unlock(&out->lock);
        if((volume[0] == 0) && (volume[1] == 0)) {
            pthread_mutex_lock(&adev->device_lock);
            pthread_mutex_lock(&adev->lock);
            out_codec_mute_check(adev);
            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_unlock(&adev->device_lock);
        }
        else {
            if(need_unmute) {
                pthread_mutex_lock(&adev->device_lock);
                pthread_mutex_lock(&adev->lock);
                pthread_mutex_lock(&adev->mute_lock);
                if(adev->master_mute == 1) {
                    adev->master_mute = 0;
                    select_devices_signal(adev);
                }
                pthread_mutex_unlock(&adev->mute_lock);
                pthread_mutex_unlock(&adev->lock);
                pthread_mutex_unlock(&adev->device_lock);
            }
        }
        return 0;
    }
    pthread_mutex_unlock(&out->lock);

    return -ENOSYS;
}

static bool out_bypass_data(struct tiny_stream_out *out, uint32_t frame_size, uint32_t sample_rate, size_t bytes)
{
    /*
       1. There is some time between call_start and call_connected, we should throw away some data here.
       2. During in  AUDIO_MODE_IN_CALL and not in call_start, we should throw away some data in BT device.
       3. If mediaserver crash, we should throw away some pcm data after restarting mediaserver.
       4. After call thread gets stop_call cmd, but hasn't get lock.
       */

    struct tiny_audio_device *adev = out->dev;

    if(cp_mixer_is_enable(adev->record_tone_info)){
        ALOGD("YAYE: out_bypass_data mixer_to_cp_uplink!!!");
        pthread_mutex_unlock(&adev->lock);
        pthread_mutex_unlock(&out->lock);
        usleep((int64_t)bytes * 1000000 / frame_size / sample_rate);
        return true;
    }

    if (0
#ifdef AUDIO_DEBUG
            ||(TEST_AUDIO_IN_OUT_LOOP==adev->ext_contrl->dev_test.test_mode)
            ||(TEST_AUDIO_CP_LOOP==adev->ext_contrl->dev_test.test_mode)
#endif
            || (adev->call_start && (!adev->call_connected))
            || adev->call_prestop
            || ((adev->fm_type == AUD_FM_TYPE_LINEIN_3GAINS) && (adev->fm_volume != -1))) {
        MY_TRACE("out_write throw away data call_start(%d) mode(%d) devices(0x%x) call_connected(%d) vbc_2arm(%d) call_prestop(%d)...",adev->call_start,adev->mode,adev->out_devices,adev->call_connected,adev->vbc_2arm,adev->call_prestop);
#ifdef AUDIO_DEBUG
            if((TEST_AUDIO_IN_OUT_LOOP==adev->ext_contrl->dev_test.test_mode)
                ||(TEST_AUDIO_CP_LOOP==adev->ext_contrl->dev_test.test_mode)){
                ALOGE("out_bypass_data TEST_AUDIO_IN_OUT_LOOP");
            }
#endif
        pthread_mutex_unlock(&adev->lock);
        pthread_mutex_unlock(&out->lock);
        usleep((int64_t)bytes * 1000000 / frame_size / sample_rate);
        return true;
    }
    else {
        return false;
    }
}

static ssize_t out_write_vaudio(struct tiny_stream_out *out, const void* buffer,
        size_t bytes)
{
    void *buf;
    int ret = 0;
    size_t frame_size = 0;
    size_t in_frames = 0;
    size_t out_frames =0;
    struct tiny_audio_device *adev = out->dev;

    frame_size = audio_stream_frame_size(&out->stream.common);
    in_frames = bytes / frame_size;
    out_frames = RESAMPLER_BUFFER_SIZE / frame_size;
    LOG_D("out_write_vaudio in");
    if(out->pcm_vplayback) {
        BLUE_TRACE("out_write_vaudio in bytes is %d",bytes);
        out->resampler_vplayback->resample_from_input(out->resampler_vplayback,
                (int16_t *)buffer,
                &in_frames,
                (int16_t *)out->buffer_vplayback,
                &out_frames);
        buf = out->buffer_vplayback;

		int i=0;
		int16_t * buf_p = buffer;
		/*for(i=0;i<5;i++)
		{
			VOIP_TRACE("voip:out_write_vaudio is %d,%d,%d,%d,%d,%d,%d,%d,%d,%d",*(buf_p+0),*(buf_p+1),*(buf_p+2),*(buf_p+3),*(buf_p+4),*(buf_p+5),*(buf_p+6),*(buf_p+7),*(buf_p+8),*(buf_p+9));
		}*/
#ifdef AUDIO_MUX_PCM
        ret = mux_pcm_write(out->pcm_vplayback, (void *)buf, out_frames*frame_size);
#else
        #ifdef SUPPORT_64BIT_HAL
        ret = pcm_write(out->pcm_vplayback, (void *)buf, out_frames*frame_size);
        #else
        ret = pcm_mmap_write(out->pcm_vplayback, (void *)buf, out_frames*frame_size);
        #endif
#endif
        LOG_D("out_write_vaudio out out frames  is %d",out_frames);

        audio_pcm_dump(buf,out_frames * frame_size,DUMP_MUSIC_HWL_MIX_VAUDIO);
#ifdef AUDIO_DEBUG
        if(adev->ext_contrl->dump_info->dump_vaudio){
            do_dump(adev->ext_contrl->dump_info,DUMP_MINIVAUDIO,
                            (void*)buf,out_frames*frame_size);
        }
#endif
    }
    else
        usleep(out_frames*1000*1000/out->cur_config.rate);

    return ret;
}

static ssize_t out_write_sco_mixer(struct tiny_stream_out *out, const void* buffer,
        size_t bytes)
{
    void *buf;
    int ret = 0;
    size_t frame_size = 0;
    size_t in_frames = 0;
    size_t out_frames =0;
    struct tiny_audio_device *adev = out->dev;


    frame_size = audio_stream_frame_size((const struct audio_stream *)(&out->stream.common));
    in_frames = bytes / frame_size;
    out_frames = RESAMPLER_BUFFER_SIZE / frame_size;

    if(out->pcm_voip_mixer) {
        out->resampler_sco->resample_from_input(out->resampler_sco,
                (int16_t *)buffer,
                &in_frames,
                (int16_t *)out->buffer_voip,
                &out_frames);
        buf = out->buffer_voip;
        if(frame_size == 4){
            pcm_mixer(buf, out_frames*(frame_size/2));
        }
#ifdef AUDIO_MUX_PCM
	ret = mux_pcm_write(out->pcm_voip_mixer, (void *)buf, out_frames*frame_size/2);
#else
        #ifdef SUPPORT_64BIT_HAL
        ret = pcm_write(out->pcm_voip_mixer, (void *)buf, out_frames*frame_size/2);
        #else
        ret = pcm_mmap_write(out->pcm_voip_mixer, (void *)buf, out_frames*frame_size/2);
        #endif
#endif
        if(ret < 0) {
            ALOGE("out_write_sco: pcm_mmap_write error: ret %d", ret);
        }
    }
    else
        usleep(out_frames*1000*1000/out->cur_config.rate);

    LOG_V("voip:out_write_sco_mixer out bytes is %d,frame_size %d, in_frames %d, out_frames %d,out->pcm_voip_mixer %x",
                bytes, frame_size,in_frames, out_frames,out->pcm_voip_mixer);
    return ret;
}

static ssize_t out_write_sco(struct tiny_stream_out *out, const void* buffer,
        size_t bytes)
{
    void *buf;
    //void *buffer1 = NULL;
    int ret = 0;
    size_t frame_size = 0;
    size_t in_frames = 0;
    size_t out_frames =0;
    struct tiny_audio_device *adev = out->dev;


    frame_size = audio_stream_frame_size((const struct audio_stream *)(&out->stream.common));
    in_frames = bytes / frame_size;
    out_frames = RESAMPLER_BUFFER_SIZE / frame_size;
    LOG_D("out_write_sco in bytes is %d,frame_size %d, in_frames %d, out_frames %d,out->pcm_voip %x", bytes, frame_size,in_frames, out_frames,out->pcm_voip);
    if(out->pcm_voip) {
        out->resampler_sco->resample_from_input(out->resampler_sco,
                (int16_t *)buffer,
                &in_frames,
                (int16_t *)out->buffer_voip,
                &out_frames);
        buf = out->buffer_voip;
        if(frame_size == 4){
            pcm_mixer(buf, out_frames*(frame_size/2));

        }
#ifdef AUDIO_MUX_PCM
        ret = mux_pcm_write(out->pcm_voip, (void *)buf, out_frames*frame_size/2);
#else
        #ifdef SUPPORT_64BIT_HAL
        ret = pcm_write(out->pcm_voip, (void *)buf, out_frames*frame_size/2);
        #else
        ret = pcm_mmap_write(out->pcm_voip, (void *)buf, out_frames*frame_size/2);
        #endif
#endif
        if(ret < 0) {
            ALOGE("out_write_sco: pcm_mmap_write error: ret %d", ret);
        }
        else {
        audio_pcm_dump(buf,out_frames * frame_size/2,DUMP_MUSIC_HWL_VOIP_WRITE);
#ifdef AUDIO_DEBUG
        if(adev->ext_contrl->dump_info->dump_voip){
            do_dump(adev->ext_contrl->dump_info,DUMP_MINIVOIP,
                            (void*)buf,out_frames*frame_size/2);
        }
#endif
        }
    }
    else
        usleep(out_frames*1000*1000/out->cur_config.rate);

    LOG_D("voip:out_write_sco out bytes is %d,frame_size %d, in_frames %d, out_frames %d,out->pcm_voip %x", bytes, frame_size,in_frames, out_frames,out->pcm_voip);
    return ret;
}

static ssize_t out_write_bt_sco(struct tiny_stream_out *out, const void* buffer,
        size_t bytes)
{
    void *buf;
    int ret = -1;
    size_t frame_size = 0;
    size_t in_frames = 0;
    size_t out_frames =0;
    size_t out_bytes = 0;
    struct tiny_audio_device *adev = out->dev;

    frame_size = audio_stream_frame_size(&out->stream.common);
    in_frames = bytes / frame_size;
    out_frames = RESAMPLER_BUFFER_SIZE / frame_size;
    LOG_E("out_write_bt_sco in bytes is %d,frame_size %d, in_frames %d, out_frames %d,out->pcm_voip %x", bytes, frame_size,in_frames, out_frames,out->pcm_voip);
    if(out->pcm_bt_sco) {
        out->resampler_bt_sco->resample_from_input(out->resampler_bt_sco,
                (int16_t *)buffer,
                &in_frames,
                (int16_t *)out->buffer_bt_sco,
                &out_frames);
        buf = out->buffer_bt_sco;
        if(adev->bluetooth_type == VX_NB_SAMPLING_RATE)
            pcm_mixer(buf, out_frames*(frame_size/2));
        out_bytes = pcm_frames_to_bytes(out->pcm_bt_sco , out_frames);
#ifdef AUDIO_DUMP
       out_dump_doing(out->out_dump_fd, (void *)buf, out_bytes);
#endif
#ifdef AUDIO_DEBUG
       if(adev->ext_contrl->dump_info->dump_bt_sco){
            do_dump(adev->ext_contrl->dump_info,DUMP_MINI_BT_SCO,
                            (void*) buf, out_bytes);
       }
#endif
        //BLUE_TRACE("voip:out_write_bt_sco");
        LOG_D("out_write_bt_sco  bt_type:%d, out_frames:%d, out_byte:%d", adev->bluetooth_type, out_frames, out_bytes);

#ifdef USE_PCM_IRQ_MODE
        ret = pcm_write(out->pcm_bt_sco, (void *)buf, out_bytes);
#else
        ret = pcm_mmap_write(out->pcm_bt_sco, (void *)buf, out_bytes);
#endif

        audio_pcm_dump(buf,out_bytes,DUMP_MUSIC_HWL_BTSCO_WRITE);
#ifdef AUDIO_DEBUG
            if(adev->ext_contrl->dump_info->dump_voip){
                do_dump(adev->ext_contrl->dump_info,DUMP_MUSIC_HWL_BTSCO_WRITE,
                                (void*)buf,out_frames*frame_size/2);
            }
#endif
    }
    else{

        usleep(out_frames*1000*1000/out->cur_config.rate);
        return ret;
    }

    LOG_D("voip:out_write_bt_sco out bytes is %d,frame_size %d, in_frames %d, out_frames %d,out->pcm_voip %x", bytes, frame_size,in_frames, out_frames,out->pcm_voip);
    return ret;
}


static int out_get_presentation_position(const struct audio_stream_out *stream,
                                   uint64_t *frames, struct timespec *timestamp)
{
    if( stream == NULL || frames == NULL || timestamp == NULL){
        return -EINVAL;
    }

    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct pcm *pcm = NULL;
    int ret = -1;
    unsigned long dsp_frames;
    pthread_mutex_lock(&out->lock);
    if (out->pcm_voip){
        pcm = out->pcm_voip;
    } else if (out->pcm_bt_sco){
        pcm = out->pcm_bt_sco;
    } else if (out->pcm_vplayback){
        pcm = out->pcm_vplayback;
    } else {
        pcm = out->pcm;
    }

    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        if (out->compress != NULL) {
                compress_get_tstamp(out->compress, &dsp_frames,
                                    &out->offload_samplerate);
                *frames = dsp_frames;
                ret = 0;
                clock_gettime(CLOCK_MONOTONIC, timestamp);
        }
    } else {
        if (pcm) {
            size_t avail;
            if (pcm_get_htimestamp(pcm, &avail, timestamp) == 0) {
                //ALOGE("out_get_presentation_position out frames %llu timestamp.tv_sec %llu timestamp.tv_nsec %llu",*frames,timestamp->tv_sec,timestamp->tv_nsec);
                size_t kernel_buffer_size = out->cur_config.period_size * out->cur_config.period_count;
                size_t kernel_frames_before_resample = kernel_buffer_size - avail;
                int64_t signed_frames = out->written;
                if((out->cur_config.rate != out->config.rate) && (out->cur_config.rate != 0))
                    kernel_frames_before_resample = kernel_frames_before_resample*out->config.rate/out->cur_config.rate;

                signed_frames -= kernel_frames_before_resample;

                if (signed_frames >= 0) {
                    *frames = signed_frames;
                    ret = 0;
                }
            }
        }
    }
    pthread_mutex_unlock(&out->lock);
    //ALOGE("out_get_presentation_position ret %d",ret);
    return ret;
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
        size_t bytes)
{
    if( stream == NULL || buffer == NULL){
        return -EINVAL;
    }

    int ret = 0;
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    audio_modem_t *modem = adev->cp;
    size_t frame_size = 0;
    size_t in_frames = 0;
    size_t out_frames =0;
    struct tiny_stream_in *in;
    bool low_power;
    int kernel_frames;
    void *buf;
    int smart_amp_buf_size = 0;
    void *smart_amp_buf;
    static long time1=0, time2=0, deltatime=0, deltatime2=0;
    static long time3=0, time4=0, deltatime3=0,write_index=0, writebytes=0;

    /* acquiring hw device mutex systematically is useful if a low priority thread is waiting
     * on the output stream mutex - e.g. executing select_mode() while holding the hw device
     * mutex
     */
    LOG_V("out_write1: out->flags %d out->devices %x,start: out->is_voip is %d, out->is_voip_mixer is %d adev->voip_state is %d,adev->voip_start is %d",
                out->flags, out->devices,out->is_voip,out->is_voip_mixer,adev->voip_state,adev->voip_start);
    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&out->lock);
    if (0==out->standby) {//in playing
        if(modem->debug_info.enable) {
            time3 = getCurrentTimeUs();
            if(time4>time3) {
                deltatime3 = time4-time3;
                if(deltatime3>modem->debug_info.lastthis_outwritetime_gate) {
                    ALOGD("out_write lastthisoutwritedebuginfo time: %d(gate, us), %ld(us), %d(bytes).",
                        modem->debug_info.lastthis_outwritetime_gate,
                        deltatime3, bytes);
                }
            }
        }
    }

    out->written += bytes / (out->config.channels * sizeof(short));
    if (out_bypass_data(out,audio_stream_frame_size(&stream->common),out_get_sample_rate(&stream->common),bytes)) {
        return bytes;
    }


#ifdef VOIP_DSP_PROCESS
#ifndef AUDIO_OLD_MODEM
    if ((adev->voip_start == 1) &&(!adev->call_start)&&(!voip_is_forbid(adev)))
#else
    if (((adev->voip_start == 1) && (!(out->devices & AUDIO_DEVICE_OUT_ALL_SCO)))&&(!adev->call_start)&&(!voip_is_forbid(adev)))
#endif
    {
#ifdef AUDIO_VOIP_VERSION_2
        if((!out->is_voip )&&(0==(AUDIO_OUTPUT_FLAG_DEEP_BUFFER&out->flags)) && (out->audio_app_type != AUDIO_HW_APP_OFFLOAD)){
#else
        if(!out->is_voip ) {
#endif
            ALOGD("sco:out_write start and do standby");
            adev->voip_state |= VOIP_PLAYBACK_STREAM;
            force_standby_for_voip(adev);
            ALOGI("out->is_voip is %d",out->is_voip);
            do_output_standby(out);
            out->is_voip=true;
        }
    }
    else {
        if(out->is_voip) {
            ALOGD("sco:out_write stop and do standby");
            do_output_standby(out);
        }
        if(voip_is_forbid(adev)){
            LOG_V("out_write:voip is forbid");
        }
    }
#endif
    if((AUDIO_OUTPUT_FLAG_DEEP_BUFFER&out->flags) && adev->voip_state) {
        if(!out->is_voip_mixer) {
            ALOGI("start voip mixer and standby dev->voip_state: %d", adev->voip_state);
            do_output_standby(out);
            out->is_voip_mixer = 1;
        }
    }else {
        if(out->is_voip_mixer) {
            ALOGI("do_output_standby voip_mixer");
            do_output_standby(out);
        }
    }
#ifndef AUDIO_OLD_MODEM
    if((out->devices & AUDIO_DEVICE_OUT_ALL_SCO) && (!out->is_voip )&&(!adev->call_start ) && (out->audio_app_type != AUDIO_HW_APP_OFFLOAD)) {
#else
    if((out->devices & AUDIO_DEVICE_OUT_ALL_SCO)&&(!adev->call_start ) && (out->audio_app_type != AUDIO_HW_APP_OFFLOAD)) {
#endif
        if(!out->is_bt_sco) {
            ALOGI("bt_sco:out_write_start and do standby");
            do_output_standby(out);
            out->is_bt_sco=true;
        }
    }
    else {
        if(out->is_bt_sco) {
           ALOGI("bt_sco:out_write_stop and do standby");
            do_output_standby(out);
        }
    }

    LOG_V("into out_write 2: start:out->devices %x,out->is_voip is %d, voip_state is %d,adev->voip_start is %d",out->devices,out->is_voip,adev->voip_state,adev->voip_start);
    if((!(out->is_voip |out->is_voip_mixer)) && adev->voip_state &&(out->audio_app_type != AUDIO_HW_APP_OFFLOAD)) {
        ALOGE("out_write: drop data and sleep,out->is_voip is %d, adev->voip_state is %d,adev->voip_start is %d",out->is_voip,adev->voip_state,adev->voip_start);
        pthread_mutex_unlock(&out->lock);
        pthread_mutex_unlock(&adev->lock);
	usleep(100000);
        return bytes;
    }
#ifdef NXP_SMART_PA
    if (adev->smart_pa_exist) {
        if((!(out->is_bt_sco||out->is_voip || adev->call_start || (out->audio_app_type == AUDIO_HW_APP_OFFLOAD)))
            && (out->devices & AUDIO_DEVICE_OUT_SPEAKER)) {
            if(!out->nxp_pa) {
                if(out->devices == AUDIO_DEVICE_OUT_SPEAKER) {
                    do_output_standby(out);
                }
                if (adev->ext_codec->i2s_shared)
                    set_i2s_parameter(adev->i2s_mixer, I2S_TYPE_EXT_CODEC);
                i2s_pin_mux_sel_new(adev, EXT_CODEC, EXT_CODEC_AP);
                out->handle_pa = NxpTfa_Open(&NxpTfa9890_Info, 1, 0);
                if(out->handle_pa) {
                    out->nxp_pa = true;
                    set_ext_codec(adev, SCENE_MUSIC);
                    adev->active_output = out;
                }
            }
        } else {
            if((out->nxp_pa == true) &&(!(out->devices & AUDIO_DEVICE_OUT_SPEAKER))) {
                out->nxp_pa = false;
                if(out->handle_pa) {
                    NxpTfa_Close(out->handle_pa);
                    out->handle_pa = NULL;
                }
            }
        }
    }
#endif
#ifdef NXP_SMART_PA
    if (out->standby &&((out->nxp_pa == false) || ((out->nxp_pa == true)&&(out->devices&(~ AUDIO_DEVICE_OUT_SPEAKER))))){
#else
    if (out->standby) {
#endif
        out->standby = 0;
        if(modem->debug_info.enable) {
            write_index=0;
            writebytes=0;
        }
        ret = start_output_stream(out);
        if (ret != 0) {
            pthread_mutex_unlock(&adev->lock);
            goto exit;
        }
    }
    low_power = adev->low_power && !adev->active_input;
    pthread_mutex_unlock(&adev->lock);
    LOG_V("into out_write 3: start: out->is_bt_sco is %d, out->devices is %x,out->is_voip is %d, voip_state is %d,adev->voip_start is %d",out->is_bt_sco,out->devices,out->is_voip,adev->voip_state,adev->voip_start);

#ifdef NXP_SMART_PA
    if(adev->smart_pa_exist && out->nxp_pa == true) {
        ret = NxpTfa_Write(out->handle_pa,buffer,bytes);
    }
#endif
    if(out->is_voip_mixer) {
        ret=out_write_sco_mixer(out,buffer,bytes);
    }
    else if(out->is_voip){
        //BLUE_TRACE("sco playback out_write call_start(%d) call_connected(%d) ...in....",adev->call_start,adev->call_connected);
        ret=out_write_sco(out,buffer,bytes);
    }
    else if(out->is_bt_sco){
        //BLUE_TRACE("out_write_bt_sco call_start(%d) call_connected(%d) ...in....",adev->call_start,adev->call_connected);
        ret=out_write_bt_sco(out,buffer,bytes);
    }
    else if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        ALOGD("%s, compress card",__func__);
        ret = out_write_compress(out,buffer,bytes);
        pthread_mutex_unlock(&out->lock);
        return ret;
    }
    else if (adev->call_connected) {
        ret=out_write_vaudio(out,buffer,bytes);
    }
#ifdef NXP_SMART_PA
    else if(!adev->smart_pa_exist || out->devices & (~AUDIO_DEVICE_OUT_SPEAKER)) {
#else
    else {
#endif
        frame_size = audio_stream_frame_size((const struct audio_stream *)(&out->stream.common));
        in_frames = bytes / frame_size;
        out_frames = RESAMPLER_BUFFER_SIZE / frame_size;

        if(out->flags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER){
            if (low_power != out->low_power){
                if (low_power) {
                    int avail_min_ms=0;
                    out->write_threshold =pcm_config_mm_deepbuf.period_size*pcm_config_mm_deepbuf.period_count;
                    out->config.avail_min = ( out->write_threshold *3 ) /4;
                    avail_min_ms=out->config.avail_min* 1000 /(out->config.rate*4);
                    ALOGW("out->write_threshold=%d, config.avail_min=%d %dms, start_threshold=%d",
                            out->write_threshold,out->config.avail_min, avail_min_ms,out->config.start_threshold);
                } else {
                    int avail_min_ms=0;
                    //out->write_threshold = (pcm_config_mm_deepbuf.period_size*pcm_config_mm_deepbuf.period_count * 1)/12;
                    out->write_threshold = pcm_config_mm_deepbuf.start_threshold;
                    out->config.avail_min = out->write_threshold;
                    avail_min_ms=out->config.avail_min* 1000 /(out->config.rate*4);
                    ALOGW("out->write_threshold=%d, config.avail_min=%d %dms, start_threshold=%d",
                            out->write_threshold,out->config.avail_min, avail_min_ms,out->config.start_threshold);
                }
                pcm_set_avail_min(out->pcm, out->config.avail_min);
                out->low_power = low_power;
            }

            do{
                struct timespec time_stamp;

                if (pcm_get_htimestamp(out->pcm, (unsigned int *)&kernel_frames, &time_stamp) < 0)
                    break;

                kernel_frames = pcm_get_buffer_size(out->pcm) - kernel_frames;
                if(kernel_frames > out->write_threshold){
                    unsigned long time = (unsigned long)(((int64_t)(kernel_frames - out->write_threshold) * 1000000) /DEFAULT_OUT_SAMPLING_RATE);
                    if (time < MIN_WRITE_SLEEP_US){
                        time = MIN_WRITE_SLEEP_US;
                    }
                    time1 = getCurrentTimeUs();
                    usleep(time);
                    time2 = getCurrentTimeUs();
                }
            }while(kernel_frames > out->write_threshold);
        }else{
            if ((low_power != out->low_power) /*&& (out->flags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER)*/) {
                if (low_power) {
                    out->config.avail_min = (out->config.period_size*out->config.period_count * 1) / 2;
                    ALOGW("low_power out->write_threshold=%d, config.avail_min=%d, start_threshold=%d",
                            out->write_threshold,out->config.avail_min, out->config.start_threshold);
                } else {
                    out->config.avail_min = SHORT_PERIOD_SIZE;
                    ALOGW("out->write_threshold=%d, config.avail_min=%d, start_threshold=%d",
                            out->write_threshold,out->config.avail_min, out->config.start_threshold);
                }
                pcm_set_avail_min(out->pcm, out->config.avail_min);
                out->low_power = low_power;
            }
        }

        /* only use resampler if required */
        if (out->config.rate != DEFAULT_OUT_SAMPLING_RATE) {
            out->resampler->resample_from_input(out->resampler,
                    (int16_t *)buffer,
                    &in_frames,
                    (int16_t *)out->buffer,
                    &out_frames);
            buf = out->buffer;
        } else {
            out_frames = in_frames;
            buf = (void *)buffer;
        }
        XRUN_TRACE("in_frames=%d, out_frames=%d", in_frames, out_frames);
        XRUN_TRACE("out->write_threshold=%d, config.avail_min=%d, start_threshold=%d",
                out->write_threshold,out->config.avail_min, out->config.start_threshold);
        if(modem->debug_info.enable) {
            time1 = getCurrentTimeUs();
        }

        if (out->pSmartAmpCtl && adev->pSmartAmpMgr->isEnable && out->devices & AUDIO_DEVICE_OUT_SPEAKER) {
            if (adev->pSmartAmpMgr->isBoostDevice) {
                ret = smart_amp_set_boost(out->pSmartAmpCtl, BOOST_5V);
                if (ret < 0 ) {
                    ALOGE("smart_amp_set_boost is fail!");
                }
            } else {
                    if (0 >= out->smartAmpLoopCnt) {
                    long int batteryVoltage = 0;
                    ret = smart_amp_get_battery_voltage(&batteryVoltage, BATTERY_VOLTAGE_PATH);
                    if (ret == 0) {
                        if (abs(out->smartAmpCacheBatteryVol - batteryVoltage) >= out->smartAmpBatteryDelta) {
                            ret = smart_amp_set_battery_voltage_uv(out->pSmartAmpCtl, batteryVoltage);
                            if (0 == ret) {
                                out->smartAmpCacheBatteryVol = batteryVoltage;
                            } else {
                                ALOGE("smart_amp_set_battery_voltage_uv is fail!");
                            }
                        }
                    } else {
                        ALOGE("smart_amp_get_battery_voltage is fail!");
                    }
                    out->smartAmpLoopCnt = SMART_AMP_LOOP_CNT;
                } else {
                    out->smartAmpLoopCnt--;
                }
            }
            ret = smart_amp_process((void *)buf, out_frames * frame_size, &smart_amp_buf, &smart_amp_buf_size, out->pSmartAmpCtl);
            if (0 == ret) {
                if (smart_amp_buf || smart_amp_buf_size) {
#ifdef USE_PCM_IRQ_MODE
                    ret = pcm_write(out->pcm, (void *)smart_amp_buf, smart_amp_buf_size);//smart amp music playback
#else
                    ret = pcm_mmap_write(out->pcm, (void *)smart_amp_buf, smart_amp_buf_size);// smart amp music playback
#endif
                } else {
                    ret = out_frames * frame_size;
                }
            } else {
                ALOGE("smart_amp_process is fail!");
            }
        } else {
            if (out->pSmartAmpCtl && adev->pSmartAmpMgr->isEnable &&
                !(out->devices & AUDIO_DEVICE_OUT_SPEAKER)) {
                if (adev->pSmartAmpMgr->isBoostDevice) {
                    // don't speaker to set bypass state when smartamp is boost mode
                    ret = smart_amp_set_boost(out->pSmartAmpCtl, BOOST_BYPASS);
                    if (ret < 0 ) {
                        ALOGE("smart_amp_set_boost is fail!");
                    }
                }
            }
#ifdef USE_PCM_IRQ_MODE
            ret = pcm_write(out->pcm, (void *)buf, out_frames * frame_size);//music playback
#else
            ret = pcm_mmap_write(out->pcm, (void *)buf, out_frames * frame_size);//music playback
#endif
        }
        if(modem->debug_info.enable) {
            time2 = getCurrentTimeUs();
            deltatime2 = ((time1>time2)?(time1-time2):(time2-time1));
        }

#ifdef AUDIO_DUMP
    out_dump_doing(out->out_dump_fd, (void *)buf, out_frames * frame_size);
#endif
#ifdef AUDIO_DEBUG
    if(adev->ext_contrl->dump_info->dump_music){
         do_dump(adev->ext_contrl->dump_info,DUMP_MINIPRIMARY,
                         (void*)buf,out_frames * frame_size);
    }
#endif
        if(out->flags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER){
            audio_pcm_dump(buf,out_frames * frame_size,DUMP_MUSIC_HWL_DEEPBUFFER_BEFOORE_VBC);
        }else{
            audio_pcm_dump(buf,out_frames * frame_size,DUMP_MUSIC_HWL_BEFOORE_VBC);
        }
    }
    if(modem->debug_info.enable) {
        int unnormal = 0;
        time4 = getCurrentTimeUs();

        write_index++;
        writebytes += bytes;

        if(deltatime3>modem->debug_info.lastthis_outwritetime_gate) {
            unnormal = 1;
        }

        if(deltatime2>modem->debug_info.pcmwritetime_gate) {
            unnormal = 1;
        }

        if(deltatime > modem->debug_info.sleeptime_gate) {
            unnormal = 1;
        }
        if(unnormal) {
            ALOGD("out_write debug %ld(index), %d(bytes), %ld(total bytes), lastthis:%ld, %d(gate), pcmwrite:%ld, %d(gate), sleep:%ld, %d(gate).",
                        write_index, bytes, writebytes,
                        deltatime3, modem->debug_info.lastthis_outwritetime_gate,
                        deltatime2, modem->debug_info.pcmwritetime_gate,
                        deltatime, modem->debug_info.sleeptime_gate);
        }
    }

exit:
    if (ret < 0) {
        dump_all_audio_reg(-1,adev->dump_flag);
        if(out->pcm_voip)
            ALOGW("warning:%d, (%s)", ret, pcm_get_error(out->pcm_voip));
        if (out->pcm)
            ALOGW("warning:%d, (%s)", ret, pcm_get_error(out->pcm));
        else if (out->pcm_vplayback)
            ALOGW("vwarning:%d, (%s)", ret, pcm_get_error(out->pcm_vplayback));

        if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD){
            audio_offload_mute(out);
        }
    pthread_mutex_unlock(&out->lock);
    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&out->lock);
        if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD){
            do_offload_out_standby(out);
        }
        else {
            do_output_standby(out);
        }
#ifdef NXP_SMART_PA
        if(adev->smart_pa_exist && out->nxp_pa) {
            out->nxp_pa = false;
            if(out->handle_pa) {
                NxpTfa_Close(out->handle_pa);
                out->handle_pa = NULL;
            }
        }
#endif
	pthread_mutex_unlock(&adev->lock);
        usleep(10000);
    }
    pthread_mutex_unlock(&out->lock);
    
    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD){
        return ret;
    }
    else {
        return bytes;
    }
}

static int out_get_render_position(const struct audio_stream_out *stream,
        uint32_t *dsp_frames)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    if (dsp_frames == NULL)
        return -1;

    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        pthread_mutex_lock(&out->lock);
        if (out->compress != NULL) {
            unsigned long frames = 0;
            compress_get_tstamp(out->compress, &frames,
                    &out->offload_samplerate);
            *dsp_frames = (uint32_t)frames;
            //ALOGV("%s: rendered frames %d, offload_samplerate %d ",
            //       __func__, *dsp_frames, out->offload_samplerate);
        } else {
            *dsp_frames = 0;
        }
        pthread_mutex_unlock(&out->lock);
        return 0;
    }

    return -1;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    return 0;
}

/** audio_stream_in implementation **/

static int get_next_buffer(struct resampler_buffer_provider *buffer_provider,
        struct resampler_buffer* buffer);


static void release_buffer(struct resampler_buffer_provider *buffer_provider,
        struct resampler_buffer* buffer);


static int in_deinit_resampler(struct tiny_stream_in *in)
{
    if (in->resampler) {
        release_resampler(in->resampler);
        in->resampler = NULL;
    }

    if(in->buffer) {
        free(in->buffer);
        in->buffer = NULL;
    }
    return 0;
}


static int in_init_resampler(struct tiny_stream_in *in)
{
    int ret=0;
    int size=0;
    in->buf_provider.get_next_buffer = get_next_buffer;
    in->buf_provider.release_buffer = release_buffer;
    if(in->config.channels == 2) {
        size = in->config.period_size * audio_stream_frame_size(&in->stream.common);
    }
    else {
        size = in->config.period_size * audio_stream_frame_size(&in->stream.common)*2;
    }
    in->buffer = malloc(size);
    if (!in->buffer) {
        ALOGE("in_init_resampler: alloc fail, size: %d", size);
        ret = -ENOMEM;
        goto err;
    }
    else {
        memset(in->buffer, 0, size);
    }

    ret = create_resampler(in->config.rate,
            in->requested_rate,
            in->config.channels,
            RESAMPLER_QUALITY_DEFAULT,
            &in->buf_provider,
            &in->resampler);
    if (ret != 0) {
        ret = -EINVAL;
        goto err;
    }

    return ret;

err:
    in_deinit_resampler(in);
    return ret;
}

/* audio_bt_sco_dup_thread_func is just to handle bt sco capture stream.
   The thread will write zero data to bt_sco_card if bt sco playback is
   not started. The thread will stop to write zero data to bt_sco_card
   if bt sco playback is started.
*/
static void *audio_bt_sco_dup_thread_func(void * param)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)param;
    struct pcm *bt_sco_playback = NULL;
    int ret = 0;
    void *buf = NULL;
    struct pcm_config  pcm_config_bt_sco;

    buf = malloc(SHORT_PERIOD_SIZE);
    if(!buf) {
        ALOGE("bt sco : alloc buffer for bt sco output fail");
        goto exit;
    } else {
        memset(buf, 0, SHORT_PERIOD_SIZE);
    }

    while(!adev->bt_sco_manager.thread_is_exit) {
        pthread_mutex_lock(&adev->bt_sco_manager.cond_mutex);
        if(adev->bt_sco_manager.dup_need_start) {
            pthread_mutex_unlock(&adev->bt_sco_manager.cond_mutex);
            if(bt_sco_playback == NULL) {
                ALOGE("bt sco : duplicate downlink card opening");
                if(adev->bluetooth_type == VX_WB_SAMPLING_RATE)
                    pcm_config_bt_sco = pcm_config_btscoplayback_wb;
                else
                    pcm_config_bt_sco = pcm_config_btscoplayback_nb;
                #ifdef USE_PCM_IRQ_MODE
                bt_sco_playback = pcm_open(s_bt_sco, PORT_MM, PCM_OUT |PCM_MONOTONIC, &pcm_config_bt_sco);
                #else
                bt_sco_playback = pcm_open(s_bt_sco, PORT_MM, PCM_OUT| PCM_MMAP |PCM_NOIRQ |PCM_MONOTONIC, &pcm_config_bt_sco);
                #endif
                if(!pcm_is_ready(bt_sco_playback)) {
                    ALOGE("%s:cannot open bt_sco_playback : %s", __func__,pcm_get_error(bt_sco_playback));
                    pcm_close(bt_sco_playback);
                    bt_sco_playback = NULL;
                    adev->bt_sco_state |= BT_SCO_DOWNLINK_OPEN_FAIL;
                } else {
                    adev->bt_sco_state &= (~BT_SCO_DOWNLINK_OPEN_FAIL);
                }

                ALOGE("bt sco : duplicate open downlink card signal");
                sem_post(&adev->bt_sco_manager.dup_sem);
            }

            ALOGV("bt sco : duplicate thread write");
            if(bt_sco_playback) {
            #ifdef USE_PCM_IRQ_MODE
               ret = pcm_write(bt_sco_playback, buf, SHORT_PERIOD_SIZE / 2);
            #else
               ret = pcm_mmap_write(bt_sco_playback, buf, SHORT_PERIOD_SIZE / 2);
            #endif
                ALOGV("bt sco : duplicate thread write ret is %d", ret);
            }
        } else {
            if(bt_sco_playback) {
                pcm_close(bt_sco_playback);
                bt_sco_playback = NULL;

                ALOGE("bt sco : duplicate close downlink card signal");
                sem_post(&adev->bt_sco_manager.dup_sem);
            }

            ALOGE("bt sco : duplicate thread wait for start");
            pthread_cond_wait(&adev->bt_sco_manager.cond, &adev->bt_sco_manager.cond_mutex);
            pthread_mutex_unlock(&adev->bt_sco_manager.cond_mutex);

            ALOGE("bt sco : duplicate thread is started now ...");
        }
    }

exit:
    ALOGE("bt sco : duplicate thread exit");
    if(bt_sco_playback) {
        pcm_close(bt_sco_playback);
    }
    if(buf) {
        free(buf);
    }
    return NULL;
}

/* if bt sco playback stream is not started and bt sco capture stream is started, we will
   start duplicate_thread to write zero data to bt_sco_card.
   we will stop duplicate_thread to write zero data to bt_sco_card if bt sco playback stream
   is started or bt sco capture stream is stoped.
*/
static int audio_bt_sco_duplicate_start(struct tiny_audio_device *adev, bool enable)
{
    int ret = 0;
    ALOGE("bt sco : %s duplicate thread %s", __func__, (enable ? "start": "stop"));
    pthread_mutex_lock(&adev->bt_sco_manager.dup_mutex);
    if(enable != adev->bt_sco_manager.dup_count) {
        adev->bt_sco_manager.dup_count = enable;

        pthread_mutex_lock(&adev->bt_sco_manager.cond_mutex);
        adev->bt_sco_manager.dup_need_start = enable;
        pthread_cond_signal(&adev->bt_sco_manager.cond);
        pthread_mutex_unlock(&adev->bt_sco_manager.cond_mutex);

        ALOGE("bt sco : %s %s before wait", __func__, (enable ? "start": "stop"));
        sem_wait(&adev->bt_sco_manager.dup_sem);
        ALOGE("bt sco : %s %s after wait", __func__, (enable ? "start": "stop"));
    }
    if(enable && (adev->bt_sco_state & BT_SCO_DOWNLINK_OPEN_FAIL)) {
        adev->bt_sco_manager.dup_count = !enable;
        adev->bt_sco_manager.dup_need_start = !enable;
        ret = -1;
    } else {
        ret = 0;
    }
    pthread_mutex_unlock(&adev->bt_sco_manager.dup_mutex);
    return ret;
}

static int audio_bt_sco_thread_create(struct tiny_audio_device *adev)
{
    int ret = 0;
    pthread_attr_t attr;

    /* create a thread to bt sco playback.*/
    pthread_attr_init(&attr);
    memset(&adev->bt_sco_manager, 0, sizeof(struct bt_sco_thread_manager));
    adev->bt_sco_manager.thread_is_exit = false;
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    ret = pthread_create(&adev->bt_sco_manager.dup_thread, &attr,
            audio_bt_sco_dup_thread_func, (void *)adev);
    if (ret) {
        ALOGE("bt sco : duplicate thread create fail, code is %d", ret);
    }
    pthread_attr_destroy(&attr);

    /* initialize mutex and condition variable objects */
    pthread_mutex_init(&adev->bt_sco_manager.dup_mutex, NULL);
    pthread_mutex_init(&adev->bt_sco_manager.cond_mutex, NULL);
    pthread_cond_init(&adev->bt_sco_manager.cond, NULL);
    sem_init(&adev->bt_sco_manager.dup_sem, 0, 0);

    return ret;
}

static void audio_bt_sco_thread_destory(struct tiny_audio_device *adev)
{
    int ret = 0;

    adev->bt_sco_manager.thread_is_exit = true;
    ALOGE("bt sco : duplicate thread destory before");
    pthread_mutex_lock(&adev->bt_sco_manager.cond_mutex);
    pthread_cond_signal(&adev->bt_sco_manager.cond);
    pthread_mutex_unlock(&adev->bt_sco_manager.cond_mutex);
    ALOGI("adev_close singnal singnal");
    ret = pthread_join(adev->bt_sco_manager.dup_thread, NULL);
    ALOGE("bt sco : duplicate thread destory ret is %d", ret);
    adev->bt_sco_manager.dup_thread = 0;

    pthread_mutex_destroy(&adev->bt_sco_manager.dup_mutex);
    pthread_mutex_destroy(&adev->bt_sco_manager.cond_mutex);
    pthread_cond_destroy(&adev->bt_sco_manager.cond);
    sem_destroy(&adev->bt_sco_manager.dup_sem);
}

static int check_support_nr(struct tiny_audio_device *adev,int rate){
    bool nr_param_enable=true;
    int nr_mask=RECORD_UNSUPPORT_RATE_NR;

    NR_CONTROL_PARAM_T *pNvInfo_handsfree = (NR_CONTROL_PARAM_T *)
        (&(audio_para_ptr[GetAudio_InMode_number_from_device(AUDIO_DEVICE_IN_BUILTIN_MIC)].audio_enha_eq.nrArray[0]));
    NR_CONTROL_PARAM_T *pNvInfo_headset = (NR_CONTROL_PARAM_T *)
        (&(audio_para_ptr[GetAudio_InMode_number_from_device(AUDIO_DEVICE_IN_WIRED_HEADSET)].audio_enha_eq.nrArray[0]));
    ALOGI("check_support_nr nr param switch:0x%x 0x%x nr_config:0x%x 0x%x rate:%d",
        pNvInfo_handsfree->nr_switch,pNvInfo_headset->nr_switch,
        adev->nr_mask,adev->enable_other_rate_nr,rate);

    if((pNvInfo_handsfree->nr_switch)||(pNvInfo_headset->nr_switch)){
        nr_param_enable=true;
    }else{
        nr_param_enable=false;
    }

    switch(rate){
        case 48000:
            nr_mask=RECORD_48000_NR;
            break;
        case 44100:
            nr_mask=RECORD_44100_NR;
            break;
        case 16000:
            nr_mask=RECORD_16000_NR;
            break;
        case 8000:
            nr_mask=RECORD_8000_NR;
            break;
        default:
            ALOGI("check_support_nr rate:%d nr:%d",
                rate,adev->enable_other_rate_nr);
            return adev->enable_other_rate_nr;
    }

    if((adev->nr_mask&(1<<nr_mask))&&(true==nr_param_enable)){
        return true;
    }else{
        return false;
    }
}

/* must be called with hw device and input stream mutexes locked */
static int start_input_stream(struct tiny_stream_in *in)
{
    int ret = 0;
    struct tiny_audio_device *adev = in->dev;
    struct pcm_config  old_config = in->config;
    struct mixer *in_mixer= NULL;
    struct mixer_ctl *ctl1;
    adev->active_input = in;
    ALOGW("start_input_stream in mode:%x devices:%x call_start:%d, is_voip:%d, is_bt_sco:%d, adev->fm_open %d adev->fm_record %d",adev->mode,adev->in_devices,adev->call_start, in->is_voip, in->is_bt_sco, adev->fm_open , adev->fm_record);
    if (!adev->call_start) {
        adev->in_devices &= ~AUDIO_DEVICE_IN_ALL;
        adev->in_devices |= in->device;
        if((in->device & ~ AUDIO_DEVICE_BIT_IN) & AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET) {
#ifndef AUDIO_OLD_MODEM
            if(!adev->voip_start) {
                i2s_pin_mux_sel(adev,AP_TYPE);
            }else {
                if(adev->cp_type == CP_TG)
                    i2s_pin_mux_sel(adev,1);
                else if(adev->cp_type == CP_W)
                    i2s_pin_mux_sel(adev,0);
                else if( adev->cp_type ==  CP_CSFB)
                    i2s_pin_mux_sel(adev,CP_CSFB);
            }
#else
            i2s_pin_mux_sel(adev,AP_TYPE);
#endif
        }
        adev->prev_in_devices = ~adev->in_devices;
#ifdef FM_VERSION_IS_GOOGLE
        if ((in->device & ~AUDIO_DEVICE_BIT_IN) & AUDIO_DEVICE_IN_FM_TUNER) {
            adev->fm_open = true;
            adev->fm_record = true;
            adev->fm_uldl = true;
            in->is_fm = true;

            if (1 == in->requested_channels) {
                int dwSize = get_input_buffer_size(in->requested_rate, AUDIO_FORMAT_PCM_16_BIT,
                                                  in->requested_channels);
                if (dwSize <= 0) {
                    ALOGE("adev_open_input_stream get_buffer_size is fail, size:%d %d %d %d ",
                        dwSize, in->requested_rate, in->config.format, in->requested_channels);
                    ret =  -ENOMEM;
                    goto err;
                }
                in->pFMBuffer = malloc(dwSize * 2);
                if (NULL == in->pFMBuffer) {
                    ALOGE("adev_open_input_stream Malloc FMBuffer is fail, pFMBuffer = 0x:%x", in->pFMBuffer);
                    ret =  -ENOMEM;
                    goto err;
                }
            }
        }
#endif
        select_devices_signal(adev);
    }

    /* this assumes routing is done previously */

    if(in->is_voip) {
        //BLUE_TRACE("start sco input stream in");
		BLUE_TRACE("voip:start sco input stream in in->requested_channels %d,in->config.channels %d",in->requested_channels,in->config.channels);
        open_voip_codec_pcm(adev);
        BLUE_TRACE("voip:start sco input stream in 4");

        in->config = pcm_config_scocapture;
        if(in->config.channels  != in->requested_channels) {
            //in->config.channels = in->requested_channels;
        }
        in->active_rec_proc = 0;
        BLUE_TRACE("in voip:opencard");
#ifdef AUDIO_MUX_PCM
	in->mux_pcm = mux_pcm_open(SND_CARD_VOIP_TG,PORT_MM,PCM_IN,&in->config );
	 if (!pcm_is_ready(in->mux_pcm)) {
        ALOGE(" in->mux_pcm = mux_pcm_open open error");
            goto err;
        }
#else
        in->pcm = pcm_open(s_voip,PORT_MM,PCM_IN,&in->config );
        if (!pcm_is_ready(in->pcm)) {
            ALOGE("%s:cannot open in->pcm : %s", __func__,pcm_get_error(in->pcm));
            goto err;
        }
#endif

#ifndef VOIP_DSP_PROCESS
        if(!adev->aud_proc_init) {
            adev->aud_proc_init = 1;
            in->active_rec_proc = init_rec_process(GetAudio_InMode_number_from_device(adev->in_devices), in->requested_rate );
            ALOGI("record process sco module created is %s.", in->active_rec_proc ? "successful" : "failed");
        }
#endif
    }
    else if(in->is_bt_sco || adev->i2sfm_flag) {
        int bt_read_size = 2*in->need_size;
        if (!in->bt_sco_buf) {
            in->bt_sco_buf = malloc(bt_read_size);
	    if (!in->bt_sco_buf) {
                ALOGE("start_input_stream: bt_sco_buf alloc failed");
                goto err;
            } else {
                memset(in->bt_sco_buf, 0, bt_read_size);
            }
        }
        BLUE_TRACE("voip:start bt sco input stream in");
        if(adev->i2sfm_flag)
            in->config = pcm_config_scocapture_i2s;
        else if (adev->bluetooth_type == VX_WB_SAMPLING_RATE)
            in->config = pcm_config_btscocapture_wb;
        else if(adev->bluetooth_type == VX_NB_SAMPLING_RATE)
            in->config = pcm_config_btscocapture_nb;

        in->active_rec_proc = 0;
        ret = set_i2s_parameter(adev->i2s_mixer, adev->bluetooth_type);
        if(ret){
            goto err;
        }
        BLUE_TRACE("in  bt_sco:opencard");
        in->pcm = pcm_open(s_bt_sco,PORT_MM,PCM_IN,&in->config );
        if (!pcm_is_ready(in->pcm)) {
            ALOGE("%s:cannot open in->pcm : %s", __func__,pcm_get_error(in->pcm));
            goto err;
        }
        if(!adev->aud_proc_init) {
            adev->aud_proc_init = 1;
            in->active_rec_proc = init_rec_process(GetAudio_InMode_number_from_device(adev->in_devices), in->requested_rate );
            ALOGI("record process sco module created is %s.", in->active_rec_proc ? "successful" : "failed");
        }
        if(in->requested_rate != in->config.rate) {
            ALOGE("bt sco input : in->requested_rate is %d, in->config.rate is %d",in->requested_rate, in->config.rate);
            ret= in_init_resampler(in);
            ALOGE("bt sco input : in_init_resampler ret is %d",ret);
            if(ret){
                goto err;
            }
        }

        /* we will start duplicate_thread to write zero data to bt_sco_card if bt sco playback stream is not started.
           we will let start_input_stream return error if duplicate_thread open bt_sco_card fail.
        */
        adev->bt_sco_state |= BT_SCO_UPLINK_IS_STARTED;

        ALOGE("bt sco : %s before", __func__);
        if(!(adev->bt_sco_state & BT_SCO_DOWNLINK_IS_EXIST)) {
            if(audio_bt_sco_duplicate_start(adev, true)) {
                ALOGE("bt sco : %s start duplicate fail");
                goto err;
            }
        }
        ALOGE("bt sco : %s after", __func__);
    }
    else if(adev->call_start) {
        int card=-1;
        cp_type_t cp_type = CP_MAX;
        in->active_rec_proc = 0;
        in->config = pcm_config_record_incall;
        if(in->config.channels  != in->requested_channels) {
            ALOGE("%s, voice-call input : in->requested_channels is %d, in->config.channels is %d",
                    __func__,in->requested_channels, in->config.channels);
            //in->config.channels = in->requested_channels;
        }

        cp_type = get_cur_cp_type(in->dev);
        if(cp_type == CP_TG) {
            s_vaudio = get_snd_card_number(CARD_VAUDIO);
            card = s_vaudio;
        } else if(cp_type == CP_W) {
            s_vaudio_w = get_snd_card_number(CARD_VAUDIO_W);
            card = s_vaudio_w;
        } else if( adev->cp_type == CP_CSFB) {
            s_vaudio_lte = get_snd_card_number(CARD_VAUDIO_LTE);
            card = s_vaudio_lte;
        }

        in_mixer = adev->cp_mixer[adev->cp_type];
        ctl1 = mixer_get_ctl_by_name(in_mixer,"PCM CAPTURE Route");
        if(in->source == AUDIO_SOURCE_VOICE_UPLINK) {
            mixer_ctl_set_value(ctl1, 0, 2);
            LOG_I("start_input_stream set_mixer ctl:%x source is AUDIO_SOURCE_VOICE_UPLINK", ctl1);
        } else if(in->source == AUDIO_SOURCE_VOICE_DOWNLINK) {
            mixer_ctl_set_value(ctl1, 0, 1);
            LOG_I("start_input_stream set_mixer ctl:%x source is AUDIO_SOURCE_VOICE_DOWNLINK", ctl1);
        }

#ifdef AUDIO_MUX_PCM
        in->mux_pcm = mux_pcm_open(SND_CARD_VOICE_TG,PORT_MM,PCM_IN,&in->config);
        if (!pcm_is_ready(in->mux_pcm)) {
            ALOGE("voice-call rec cannot mux_pcm_open mux_pcm driver: %s", pcm_get_error(in->mux_pcm));
           goto err;
        }
#else
        in->pcm = pcm_open(card,PORT_MM,PCM_IN,&in->config);
        if (!pcm_is_ready(in->pcm)) {
            ALOGE("%s: voice-call rec cannot open pcm_in driver: %s", __func__, pcm_get_error(in->pcm));
            goto err;
        }
#endif

    }
    else {

#ifdef FM_VERSION_IS_GOOGLE
        if(adev->fm_open && adev->fm_record && in->is_fm){
#else
        if(adev->fm_open && adev->fm_record) {
#endif
            in->config = pcm_config_fm_ul;
            if(in->config.channels != in->requested_channels) {
              in->config.channels = in->requested_channels;
            }
#ifdef NXP_SMART_PA
            if (adev->smart_pa_exist) {
                in->config.rate = 44100;
                in->FmRecHandle = fm_pcm_open(fm_record_config.rate, fm_record_config.channels, 1280*4, 2);
                if(!in->FmRecHandle)
                {
                        ALOGE("Sprdfm: %s,open pcm fail, code is %s", __func__, strerror(errno));
                        goto err;
                }
            } else
#endif
            {
                if (adev->fm_uldl) { /*Android M FM Solution*/
                    //in->config.rate = 44100;
                    in->config.rate = fm_record_config.rate;
                    in->fmUlDlHandle = fm_pcm_open(fm_record_config.rate, fm_record_config.channels, 1280*4, 2);
                    if(!in->fmUlDlHandle) {
                        ALOGE("Sprdfm: %s,open pcm fail, code is %s", __func__, strerror(errno));
                        goto err;
                    }
                }else{
                   in->pcm = pcm_open(s_tinycard, PORT_MM, PCM_IN, &in->config);
                   if(!pcm_is_ready(in->pcm)) {
                     dump_all_audio_reg(-1,adev->dump_flag);
                     pcm_close(in->pcm);
                     ALOGE("fm rec cannot open pcm_in driver : %s,samplerate:%d", pcm_get_error(in->pcm),in->config.rate);
                     in->pcm = NULL;
                   }
                   if(NULL == in->pcm){
                       in->config =  pcm_config_mm_ul;
                       in->config.rate = MM_LOW_POWER_SAMPLING_RATE;
                       if(in->config.channels != in->requested_channels) {
                         in->config.channels = in->requested_channels;
                       }
                       ALOGE("fm rec try to open pcm_in driver again using samplerate:%d",in->config.rate);
                       in->pcm = pcm_open(s_tinycard, PORT_MM, PCM_IN, &in->config);
                       if(!pcm_is_ready(in->pcm)) {
                         ALOGE("%s: pcm_open AUDIO_DEVICE_OUT_FM 1 cannot open pcm: %s", __func__, pcm_get_error(in->pcm));
                         goto err;
                       }
                   }
                }
                /* fm capture opened success */
                ALOGI("fm record opened");
            }
        } else {
#if defined(VBC_NOT_SUPPORT_AD23) && defined(NXP_SMART_PA)
            if((adev->fm_open)&&(adev->smart_pa_exist)){
                ALOGI("Fm is activited, normal record wait stop fm start");
                NxpTfa_Fm_Pause(adev->fmRes,true);
                ALOGI("Fm is activited, normal record wait stop fm end");
            }
#endif

          bool is_support_nr=check_support_nr(adev,in->requested_rate);
          in->config = pcm_config_mm_ul;
          if(in->config.channels != in->requested_channels) {
              in->config.channels = in->requested_channels;
            }

            if(in->config.rate != in->requested_rate)
            {
                if(true==is_support_nr){
                    ALOGI("enable nr process rate:%d requested_rate:%d",
                        in->config.rate ,in->requested_rate );
                }else{
                    in->config.rate = in->requested_rate;
                }
            }

            if(in->requested_rate != pcm_config_mm_ul.rate) {
                in->config.period_size  = ((((pcm_config_mm_ul.period_size * pcm_config_mm_ul.period_count*1000)/pcm_config_mm_ul.rate) * in->config.rate ) /(1000 * in->config.period_count))/160 * 160;
            }
            ALOGE("start_input_stream pcm_open_0");
            vbc_ad01iis_select(adev, false);
            in->pcm = pcm_open(s_tinycard, PORT_MM_C, PCM_IN, &in->config);
            if(!pcm_is_ready(in->pcm)) {
              ALOGE("%s: pcm_open 0 cannot open pcm: %s", __func__, pcm_get_error(in->pcm));
              dump_all_audio_reg(-1,adev->dump_flag);
               if(in->pcm) {
                    pcm_close(in->pcm);
                    in->pcm = NULL;
                }
                in->config.rate = pcm_config_mm_ul.rate;
                if(in->requested_rate != pcm_config_mm_ul.rate) {
                    in->config.period_size  = ((((pcm_config_mm_ul.period_size * pcm_config_mm_ul.period_count*1000)/pcm_config_mm_ul.rate) * in->config.rate ) /(1000 * in->config.period_count))/160 * 160;
                }
                ALOGE("start_input_stream pcm_open_1");
                in->pcm = pcm_open(s_tinycard, PORT_MM_C, PCM_IN, &in->config);
                if(!pcm_is_ready(in->pcm)) {
                    ALOGE("%s: pcm_open 1 cannot open pcm: %s", __func__, pcm_get_error(in->pcm));
                    goto err;
                }
            }
            /* normal capture opend success */
            in->normal_c_open = true;
#ifdef RECORD_NR
            if((is_support_nr==true) &&(in->config.rate != in->requested_rate)&& (!adev->aud_proc_init) ){
                int16 *pNvInfo = &(audio_para_ptr[GetAudio_InMode_number_from_device(adev->in_devices)].audio_enha_eq.nrArray[0]);
                if (NULL == pNvInfo) {
                    ALOGE("pNvInfo is NULL ! %s");
                    goto err;
                }
                in->active_rec_proc = init_rec_process(GetAudio_InMode_number_from_device(adev->in_devices),in->requested_rate);
                in->rec_nr_handle = AudioRecordNr_Init(pNvInfo, read_pcm_data, (void *)in, in->requested_channels);
                if(NULL ==in->rec_nr_handle) {
                    ALOGE("init_audio_record_nr failed");
                    goto err;
                }
                in->Tras48To44=SprdSrc_To_44K_Init(in->config.rate,in->requested_rate,
                    AudioRecordNr_Proc, (void *)in->rec_nr_handle, in->requested_channels);
                if(NULL == in->Tras48To44 ) {
                    ALOGE("SprdSrc_To_441K_Init");
                    goto err;
                }
                register_resample_pcmdump(in->Tras48To44,audio_pcm_dump);
                adev->aud_proc_init = 1;
            }
            else if((in->config.rate == 48000)&&(in->requested_rate == 48000) && (!adev->aud_proc_init)){
                int16 *pNvInfo = &(audio_para_ptr[GetAudio_InMode_number_from_device(adev->in_devices)].audio_enha_eq.nrArray[0]);
                if (NULL == pNvInfo)
                {
                    ALOGE("pNvInfo is NULL ! %s");
                    goto err;
                }
                in->active_rec_proc = init_rec_process(GetAudio_InMode_number_from_device(adev->in_devices),in->requested_rate);
                in->rec_nr_handle = AudioRecordNr_Init(pNvInfo, read_pcm_data, (void *)in, in->requested_channels);
                if(NULL ==in->rec_nr_handle)
                {
                    ALOGE("init_audio_record_nr failed");
                    goto err;
                }
                adev->aud_proc_init = 1;
            }
            else if (!adev->aud_proc_init) {
                /* start to process pcm data captured, such as noise suppression.*/
                in->active_rec_proc = init_rec_process(GetAudio_InMode_number_from_device(adev->in_devices),in->requested_rate);
                adev->aud_proc_init = 1;
                ALOGI("record process module created is %s.", in->active_rec_proc ? "successful" : "failed");
            }
#else
        /* start to process pcm data captured, such as noise suppression.*/
            if(!adev->aud_proc_init) {
                adev->aud_proc_init = 1;
                in->active_rec_proc = init_rec_process(GetAudio_InMode_number_from_device(adev->in_devices),in->requested_rate);
                ALOGI("record process module created is %s.", in->active_rec_proc ? "successful" : "failed");
            }
#endif
        }
    }

    if((in->requested_rate != in->config.rate) && (NULL == in->Tras48To44)) {
        ALOGE(": in->requested_rate is %d, in->config.rate is %d",in->requested_rate, in->config.rate);
        ret= in_init_resampler(in);
        ALOGE(": in_init_resampler ret is %d",ret);
        if(ret) {
            goto err;
        }
    }

    ALOGE("start_input,channels=%d,peroid_size=%d, peroid_count=%d,rate=%d",
            in->config.channels, in->config.period_size,
            in->config.period_count, in->config.rate);

    /* if no supported sample rate is available, use the resampler */
    if (in->resampler) {
        in->resampler->reset(in->resampler);
        in->frames_in = 0;
    }

    {
        int size = 0;
        int buf_size = 0;
        size = in->config.period_size;
        size = ((size + 15) / 16) * 16;
        buf_size =  size * 2 * sizeof(short);
        if(in->proc_buf_size < buf_size){
	    if(in->proc_buf)
		free(in->proc_buf);
            in->proc_buf = malloc(buf_size);
            if(!in->proc_buf) {
                goto err;
            }
            in->proc_buf_size = buf_size;
        }
    }

    ALOGE("start input stream out");
    return 0;

err:
    in->config = old_config;
    dump_all_audio_reg(-1,adev->dump_flag);
    if(in->pcm) {
        pcm_close(in->pcm);
        /* if close normal capture or fm capture then switch to fm */
        if (true == in->normal_c_open) {
            ALOGI("normal capture closed");
            vbc_ad01iis_select(adev, true);
            in->normal_c_open = false;
        }
        ALOGE("normal rec cannot open pcm_in driver: %s", pcm_get_error(in->pcm));
        in->pcm = NULL;
    }

     if(in->mux_pcm) {
     		ALOGE("normal rec cannot open pcm_in driver: %s", pcm_get_error(in->pcm));
#ifdef AUDIO_MUX_PCM
		mux_pcm_close(in->mux_pcm);
#endif
		 in->mux_pcm = NULL;
    	}
#ifdef NXP_SMART_PA
    if(adev->smart_pa_exist && in->FmRecHandle) {
        ALOGE("fm_pcm_open pcm_in driver ");
        fm_pcm_close(in->FmRecHandle);
        in->FmRecHandle = NULL;
    }
#endif
    if (in->fmUlDlHandle) {
        ALOGE("fmUlDlHandle fm_pcm_open pcm_in driver ");
        fm_pcm_close(in->fmUlDlHandle);
        in->fmUlDlHandle = NULL;
    }
	adev->active_input = NULL;
    in_deinit_resampler(in);

    if (in->active_rec_proc) {
        AUDPROC_DeInitDp();
        in->active_rec_proc = 0;
        adev->aud_proc_init = 0;
    }
    if(in->rec_nr_handle)
    {
        AudioRecordNr_Deinit(in->rec_nr_handle);
        in->rec_nr_handle = NULL;
        adev->aud_proc_init = 0;
    }
    if(in->Tras48To44) {
        SprdSrc_To_44K_DeInit(in->Tras48To44);
        in->Tras48To44 = NULL;
    }
    if(in->bt_sco_buf) {
        free(in->bt_sco_buf);
        in->bt_sco_buf = NULL;
    }
    return -1;

}

static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;

    return in->requested_rate;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    return 0;
}


static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    size_t size;
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    int period_size=pcm_config_mm_ul.period_size;

    if (check_input_parameters(in->requested_rate, AUDIO_FORMAT_PCM_16_BIT, in->config.channels) != 0)
        return 0;

    /* take resampling into account and return the closest majoring
       multiple of 16 frames, as audioflinger expects audio buffers to
       be a multiple of 16 frames */

    if((pcm_config_mm_ul.rate != in->requested_rate) && (in->requested_rate != 44100))
    {
        period_size=((((pcm_config_mm_ul.period_size * pcm_config_mm_ul.period_count*1000)/pcm_config_mm_ul.rate) * in->requested_rate ) /(1000 * pcm_config_mm_ul.period_count))/160 * 160;
    }
    size = ((period_size + 15) / 16) * 16;
    return size * in->config.channels * sizeof(short);
}

static uint32_t in_get_channels(const struct audio_stream *stream)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    if (in->config.channels == 1) {
        return AUDIO_CHANNEL_IN_MONO;
    } else {
        return AUDIO_CHANNEL_IN_STEREO;
    }
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    return 0;
}

/* must be called with hw device and input stream mutexes locked */
static int do_input_standby(struct tiny_stream_in *in)
{
    struct tiny_audio_device *adev = in->dev;
    struct mixer *in_mixer= NULL;
    struct mixer_ctl *ctl1;
    ALOGV("%s, standby=%d, in_devices=0x%08x", __func__, in->standby, adev->in_devices);
    if (!in->standby) {
#ifdef AUDIO_MUX_PCM
        if (in->mux_pcm) {
            mux_pcm_close(in->mux_pcm);
            in->mux_pcm = NULL;
        }
#endif

#ifdef NXP_SMART_PA
        if(adev->smart_pa_exist && in->FmRecHandle) {
            ALOGE("fm_pcm_close in do_input_standby");
            fm_pcm_close(in->FmRecHandle);
            in->FmRecHandle = NULL;
        }
#endif

        ALOGI("%s, fmUlDlHandle=0x%08x, fm_uldl=0x%08x, pcm = 0x%x", __func__,
                in->fmUlDlHandle, adev->fm_uldl,in->pcm);

        if (in->fmUlDlHandle) { //fixed fm start then calling
            ALOGE("fm_pcm_close in do_input_standby");
            fm_pcm_close(in->fmUlDlHandle);
            in->fmUlDlHandle = NULL;
        }

        if(in->is_voip) {
            if(!(adev->voip_state &(~VOIP_CAPTURE_STREAM))) {
                ALOGI("do_voip_mixer_standby in input_standby");
                do_voip_mixer_standby(adev);
            }
        }

        if (in->pcm) {
            pcm_close(in->pcm);
            in->pcm = NULL;
        }

        if (true == in->normal_c_open) {
            ALOGI("normal capture closed");
            vbc_ad01iis_select(adev, true);
            in->normal_c_open = false;
        }

        if(in->is_voip) {
            if(adev->voip_state) {
                adev->voip_state &= (~VOIP_CAPTURE_STREAM);
                if(!adev->voip_state)
                close_voip_codec_pcm(adev);
            }
            in->is_voip = false;
        }
        if(in->is_bt_sco) {
            /* we will stop writing zero data to bt_sco_card. */
            in->is_bt_sco = false;
            adev->bt_sco_state &= (~BT_SCO_UPLINK_IS_STARTED);
            ALOGE("bt sco : %s before", __func__);
            audio_bt_sco_duplicate_start(adev, false);
            ALOGE("bt sco : %s after", __func__);
        }
        adev->active_input = 0;

        if(in->resampler){
            in_deinit_resampler( in);
        }

        if (in->active_rec_proc) {
            AUDPROC_DeInitDp();
            in->active_rec_proc = 0;
            adev->aud_proc_init = 0;
        }
        if(in->rec_nr_handle) {
            AudioRecordNr_Deinit(in->rec_nr_handle);
            in->rec_nr_handle = NULL;
            adev->aud_proc_init = 0;
        }
        if(in->Tras48To44) {
            SprdSrc_To_44K_DeInit(in->Tras48To44);
            in->Tras48To44 = NULL;
        }
        if(in->bt_sco_buf) {
            free(in->bt_sco_buf);
            in->bt_sco_buf = NULL;
        }
        in->standby = 1;
    }

    if(in->source == AUDIO_SOURCE_VOICE_UPLINK ||
            in->source == AUDIO_SOURCE_VOICE_DOWNLINK){
        in_mixer = adev->cp_mixer[adev->cp_type];
        ctl1 = mixer_get_ctl_by_name(in_mixer,"PCM CAPTURE Route");
        mixer_ctl_set_value(ctl1, 0, 0);
        LOG_I("%s, set PCM CAPTURE Route to 0", __func__);
    }

    if(NULL!=in->conversion_buffer){
        free(in->conversion_buffer);
    }
    in->conversion_buffer=NULL;
    in->conversion_buffer_size=0;
    mp3_low_power_check(adev);
    ALOGV("do_input_standby out");
    return 0;
}

static int in_standby(struct audio_stream *stream)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    int status;
    pthread_mutex_lock(&in->dev->lock);
    pthread_mutex_lock(&in->lock);
    struct tiny_audio_device *adev = in->dev;
    status = do_input_standby(in);
    pthread_mutex_unlock(&in->lock);

#ifdef AUDIO_VOIP_VERSION_2
    if(adev->voip_start){
        adev->voip_start=false;
    }
#endif
#if defined(VBC_NOT_SUPPORT_AD23) && defined(NXP_SMART_PA)
    if((adev->fm_open)&&(adev->smart_pa_exist)){
        NxpTfa_Fm_Pause(adev->fmRes,false);
        adev->device_force_set =1;
        select_devices_signal(adev);
    }
#endif

    pthread_mutex_unlock(&in->dev->lock);
    return status;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    char buffer[DUMP_BUFFER_MAX_SIZE]={0};
    ALOGI("in_dump enter");

    if(in->standby){
        snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
            "\ninput dump ---->\n"
            "source:%d\n",
            in->source);
        AUDIO_DUMP_WRITE_STR(fd,buffer);
        memset(buffer,0,sizeof(buffer));
    }else{
        snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
            "\ninput dump ---->\n"
            "source:%d "
            "is_voip:%d "
            "is_bt_sco:%d "
            "device:0x%x "
            "requested_channels:%d "
            "requested_rate:%d "
            "frames_in:%d\n",
            in->source,
            in->is_voip,
            in->is_bt_sco,
            in->device,
            in->requested_channels,
            in->requested_rate,
            in->frames_in);
        AUDIO_DUMP_WRITE_STR(fd,buffer);
        memset(buffer,0,sizeof(buffer));

        snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
            "preprocessors:%d "
            "proc_buf_size:%d "
            "need_size:%d "
            "proc_frames_in:0x%x "
            "ref_buf_size:%d "
            "ref_frames_in:%d "
            "read_status:%d\n",
            in->num_preprocessors,
            in->proc_buf_size,
            in->need_size,
            in->proc_frames_in,
            in->ref_buf_size,
            in->ref_frames_in,
            in->read_status);
        AUDIO_DUMP_WRITE_STR(fd,buffer);
        memset(buffer,0,sizeof(buffer));

        snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
            "pop_mute:%d "
            "pop_mute_bytes:%d "
            "active_rec_proc:%d "
            "is_fm:0x%x "
            "conversion_buffer_size:%d "
            "frames_read:%d\n",
            in->pop_mute,
            in->pop_mute_bytes,
            in->active_rec_proc,
            in->is_fm,
            in->conversion_buffer_size,
            in->frames_read);
        AUDIO_DUMP_WRITE_STR(fd,buffer);
        memset(buffer,0,sizeof(buffer));

        snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
            "config:%d %d %d %d 0x%x 0x%x 0x%x\n",
            in->config.channels,
            in->config.rate,
            in->config.period_size,
            in->config.period_count,
            in->config.start_threshold,
            in->config.avail_min,
            in->config.stop_threshold);
        AUDIO_DUMP_WRITE_STR(fd,buffer);
        memset(buffer,0,sizeof(buffer));

        snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
            "point:%p %p %p %p %p %p %p %p %p\n",
            in->pcm,
            in->mux_pcm,
            in->buffer,
            in->proc_buf,
            in->bt_sco_buf,
            in->ref_buf,
            in->conversion_buffer,
            in->rec_nr_handle,
            in->resampler);
        AUDIO_DUMP_WRITE_STR(fd,buffer);
        memset(buffer,0,sizeof(buffer));
    }

    snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),"<----input dump\n");
    AUDIO_DUMP_WRITE_STR(fd,buffer);
    ALOGI("in_dump exit");
    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;
    struct str_parms *parms;
    char *str;
    char value[32];
    int ret, val = 0;
    int status = 0;

    BLUE_TRACE("[in_set_parameters], kvpairs=%s in_devices:0x%x mode:%d ", kvpairs,adev->in_devices,adev->mode);
    if (adev->call_start) {
        ALOGI("Voice call, no need care.");
        return 0;
    }
    /*set Empty Parameter then return 0*/
    if (0 == strncmp(kvpairs, "", sizeof(char))) {
        return status;
    }
    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_INPUT_SOURCE, value, sizeof(value));

    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&in->lock);
    if (ret >= 0) {
        val = atoi(value);
        ALOGE("AUDIO_PARAMETER_STREAM_INPUT_SOURCE in_set_parameters val %d",val);
        /* no audio source uses val == 0 */
        if(in->requested_rate) {
            if(val == AUDIO_SOURCE_CAMCORDER){
                in->pop_mute_bytes = RECORD_POP_MIN_TIME_CAM*in->requested_rate/1000*audio_stream_frame_size((const struct audio_stream *)(&(in->stream).common));
            } else {
                in->pop_mute_bytes = RECORD_POP_MIN_TIME_MIC*in->requested_rate/1000*audio_stream_frame_size((const struct audio_stream *)(&(in->stream).common));
            }
        }
        ALOGI("requested_rate %d,pop_mute_bytes %d frame_size %d",in->requested_rate,in->pop_mute_bytes,audio_stream_frame_size((const struct audio_stream *)(&(in->stream).common)));
#if 0
        if ((in->source != val) && (val != 0)) {
            in->source = val;
            adev->input_source =val;
        }
#endif
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        if (in->device != val && 0 != val) {
            in->device = val;
            adev->in_devices &= ~AUDIO_DEVICE_IN_ALL;
            adev->in_devices |= in->device;
            select_devices_signal(adev);
        }
        ret = 0 ;
    }

    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&adev->lock);

    str_parms_destroy(parms);
    return status;
}

static char * in_get_parameters(const struct audio_stream *stream,
        const char *keys)
{
    return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    return 0;
}

static int get_next_buffer(struct resampler_buffer_provider *buffer_provider,
        struct resampler_buffer* buffer)
{
    struct tiny_stream_in *in;
    struct tiny_audio_device *adev;

    if (buffer_provider == NULL || buffer == NULL)
        return -EINVAL;

    in = container_of(buffer_provider, struct tiny_stream_in, buf_provider);
    adev = in->dev;
#ifdef NXP_SMART_PA
    if( (in->pcm == NULL) &&(in->mux_pcm ==NULL)&&(in->fmUlDlHandle==NULL)&&(in->FmRecHandle==NULL)) {
#else
    if( (in->pcm == NULL) &&(in->mux_pcm ==NULL)&&(in->fmUlDlHandle==NULL)) {
#endif
        buffer->raw = NULL;
        buffer->frame_count = 0;
        in->read_status = -ENODEV;
        return -ENODEV;
    }

    if (in->frames_in == 0) {
        if(in->mux_pcm){
#ifdef AUDIO_MUX_PCM
            in->read_status = mux_pcm_read(in->mux_pcm,
                    (void*)in->buffer,
                    in->config.period_size *
                    audio_stream_frame_size((const struct audio_stream *)(&in->stream.common)));
 #endif
        }
#ifdef  NXP_SMART_PA
       else if(adev->smart_pa_exist && in->FmRecHandle) {
            ALOGD("Sprdfm: fm_pcm_read in get_next_buffer");
            in->read_status =  (fm_pcm_read(in->FmRecHandle, (void*)in->buffer, in->config.period_size *4, 1, 2)==(in->config.period_size *4))?0:-1;
            if(in->requested_channels == 1) {
                pcm_mixer(in->buffer,in->config.period_size*2);
            }
        }
#endif
        else if(in->fmUlDlHandle) {
            ALOGD("fmUlDlHandle Sprdfm: fm_pcm_read in get_next_buffer");
            in->read_status =  (fm_pcm_read(in->fmUlDlHandle, (void*)in->buffer,
                in->config.period_size *4, 1, 2)==(in->config.period_size *4))?0:-1;
            if(in->requested_channels == 1) {
                pcm_mixer(in->buffer,in->config.period_size*2);
            }
        }
        else{
            in->read_status = pcm_read(in->pcm,
                    (void*)in->buffer,
                    in->config.period_size *
                    audio_stream_frame_size((const struct audio_stream *)(&in->stream.common)));
        }

        audio_pcm_dump(in->buffer, in->config.period_size * audio_stream_frame_size(&in->stream.common),DUMP_RECORD_HWL_AFTER_VBC);
#ifdef AUDIO_DEBUG
        if(adev->ext_contrl->dump_info->dump_in_read_vbc){
            do_dump(adev->ext_contrl->dump_info,DUMP_MINI_RECORD_VBC,
                            (void*)in->buffer,
                           in->config.period_size *
                           audio_stream_frame_size((const struct audio_stream *)(&in->stream.common)));
        }
#endif

        if (in->read_status != 0) {
            if(in->pcm) {
                ALOGE("get_next_buffer() pcm_read sattus=%d, error: %s",
                        in->read_status, pcm_get_error(in->pcm));
            }
            buffer->raw = NULL;
            buffer->frame_count = 0;
            return in->read_status;
        }
        in->frames_in = in->config.period_size;
    }
    buffer->frame_count = (buffer->frame_count > in->frames_in) ?
        in->frames_in : buffer->frame_count;
    buffer->i16 = in->buffer + (in->config.period_size - in->frames_in) *
        in->config.channels;

    return in->read_status;
}

static void release_buffer(struct resampler_buffer_provider *buffer_provider,
        struct resampler_buffer* buffer)
{
    struct tiny_stream_in *in;

    if (buffer_provider == NULL || buffer == NULL)
        return;

    in = container_of(buffer_provider, struct tiny_stream_in, buf_provider);

    in->frames_in -= buffer->frame_count;
}

/* read_frames() reads frames from kernel driver, down samples to capture rate
 * if necessary and output the number of frames requested to the buffer specified */
static ssize_t read_frames(struct tiny_stream_in *in, void *buffer, ssize_t frames)
{
    ssize_t frames_wr = 0;
    //BLUE_TRACE("in voip3:read_frames, frames=%d", frames);
    while (frames_wr < frames) {
        size_t frames_rd = frames - frames_wr;
        //BLUE_TRACE("in voip4:frames_wr=%d, frames=%d, frames_rd=%d", frames_wr, frames, frames_rd);
        if (in->resampler != NULL) {
            in->resampler->resample_from_provider(in->resampler,
                    (int16_t *)((char *)buffer +
                        frames_wr * audio_stream_frame_size((const struct audio_stream *)(&in->stream.common))),
                    &frames_rd);
        } else {
            struct resampler_buffer buf = {
                { raw : NULL, },
frame_count : frames_rd,
            };
            get_next_buffer(&in->buf_provider, &buf);
            if (buf.raw != NULL) {
                memcpy((char *)buffer +
                        frames_wr * audio_stream_frame_size((const struct audio_stream *)(&in->stream.common)),
                        buf.raw,
                        buf.frame_count * audio_stream_frame_size((const struct audio_stream *)(&in->stream.common)));
                frames_rd = buf.frame_count;
            }
            release_buffer(&in->buf_provider, &buf);
        }
        /* in->read_status is updated by getNextBuffer() also called by
         * in->resampler->resample_from_provider() */
        if (in->read_status != 0)
            return in->read_status;

        frames_wr += frames_rd;
    }
    return frames_wr;
}

static bool in_bypass_data(struct tiny_stream_in *in,uint32_t frame_size, uint32_t sample_rate, void* buffer, size_t bytes)
{
    struct tiny_audio_device *adev = in->dev;
    /*
       1. If cp stopped calling and in-devices is AUDIO_DEVICE_IN_VOICE_CALL, it means that cp already stopped vt call, we should write
       0 data, otherwise, AudioRecord will obtainbuffer timeout.
       */

#ifdef AUDIO_DEBUG
        if((TEST_AUDIO_IN_OUT_LOOP==adev->ext_contrl->dev_test.test_mode)
            ||(TEST_AUDIO_CP_LOOP==adev->ext_contrl->dev_test.test_mode)){
            memset(buffer,0,bytes);
            ALOGE("in_bypass_data TEST_AUDIO_IN_OUT_LOOP");
            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_unlock(&in->lock);
            usleep((int64_t)bytes * 1000000 / frame_size / sample_rate);
            return true;
        }
#endif

    if ((!adev->call_start) && ((in->device == AUDIO_DEVICE_IN_VOICE_CALL))
        || adev->call_prestop
        ||(adev->fm_open && adev->record_bypass && ((in->source == AUDIO_SOURCE_VOICE_RECOGNITION)||(in->source == AUDIO_SOURCE_VOICE_COMMUNICATION))) ){
        LOG_D("in_bypass_data write 0 data call_start(%d) mode(%d)  in_device(0x%x) call_connected(%d) call_prestop(%d),fm_open:%d, record_bypass:%d,in_source:%d ",adev->call_start,adev->mode,in->device,adev->call_connected,
            adev->call_prestop,adev->fm_open , adev->record_bypass,in->source);
        memset(buffer,0,bytes);
        pthread_mutex_unlock(&adev->lock);
        pthread_mutex_unlock(&in->lock);
        usleep((int64_t)bytes * 1000000 / frame_size / sample_rate);
        return true;
    }else{
        return false;
    }
}
static ssize_t read_pcm_data(void *stream, void* buffer,
        size_t bytes)
{
    int ret =0;
    struct audio_stream_in *stream_tmp=(struct audio_stream_in *)stream;
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    size_t frames_rq = bytes / audio_stream_frame_size((const struct audio_stream *)(&stream_tmp->common));

    /*BLUE_TRACE("in_read start.num_preprocessors=%d, resampler=%d",
      in->num_preprocessors, in->resampler);*/
    if (in->resampler != NULL) {
            ret = read_frames(in, buffer, frames_rq);
#ifdef AUDIO_DEBUG
             if(in->dev->ext_contrl->dump_info->dump_in_read_afternr){
                  do_dump(in->dev->ext_contrl->dump_info,DUMP_MINI_RECORD_AFTER_SRC,
                                  (void*)buffer,bytes);
             }
#endif
            if (ret != frames_rq){
                ALOGE("ERR:in_read0");
                ret = -1;
            }
            else
                ret = 0;
    } else {

#ifdef  AUDIO_MUX_PCM
        if(in->mux_pcm){
            ALOGE("  peter: mux read  in");
            ret = mux_pcm_read(in->mux_pcm, buffer, bytes);
        }
        else
#endif
#ifdef  NXP_SMART_PA
        if(in->dev->smart_pa_exist && in->FmRecHandle){
            ret =  (fm_pcm_read(in->FmRecHandle, buffer, bytes, 1, 2)==bytes)?0:-1;
        }
        else
#endif
        if (in->fmUlDlHandle) {
            if(in->requested_channels == 1) {
                if (in->pFMBuffer) {
                    ret =  (fm_pcm_read(in->fmUlDlHandle, in->pFMBuffer, bytes * 2, 1, 2)==bytes * 2)?0:-1;
                    pcm_mixer((int16_t *)in->pFMBuffer, bytes);
                    memcpy(buffer, in->pFMBuffer, bytes);
                }
                else
                {
                    ALOGE(" FM Recourd Is Fail Memory is null");
                    return 0;
                }
            }
            else {
                ret =  (fm_pcm_read(in->fmUlDlHandle, buffer, bytes, 1, 2)==bytes)?0:-1;
            }
        }
        else {
            ret = pcm_read(in->pcm, buffer, bytes);
            audio_pcm_dump(buffer, bytes,DUMP_RECORD_HWL_AFTER_VBC);
#ifdef AUDIO_DEBUG
                if(in->dev->ext_contrl->dump_info->dump_in_read_vbc){
                    do_dump(in->dev->ext_contrl->dump_info,DUMP_MINI_RECORD_VBC,
                                    buffer,
                                   bytes);
                }
#endif
            LOG_V("  peter: normal read 1 in");
        }
    }
    return ret;
}

static int32_t  mono2stereo(int16_t *out, int16_t * in, uint32_t in_samples) {
    int i = 0;
    int out_samples = in_samples<<1;
    for(i = 0 ; i< in_samples; i++) {
        out[2*i] =in[i];
        out[2*i+1] = in[i];
    }
    return out_samples ;
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
        size_t bytes)
{
    int ret = 0;
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;
    bool is_normal_record=true;

    in->need_size = bytes;
    /* acquiring hw device mutex systematically is useful if a low priority thread is waiting
     * on the input stream mutex - e.g. executing select_mode() while holding the hw device
     * mutex
     */
    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&in->lock);

    LOG_V("into in_read1: start: in->is_voip is %d, voip_state is %d in_devices is %x bytes %d",in->is_voip,adev->voip_state,in->device, bytes);
    if(in_bypass_data(in,audio_stream_frame_size((const struct audio_stream *)(&stream->common)),in_get_sample_rate(&stream->common),buffer,bytes)){
        return bytes;
    }

#ifdef AUDIO_VOIP_VERSION_2
    if(adev->voip_playback_start) {
        if(false==adev->voip_start){
            ALOGI("in_read start voip");
        }
        if(adev->is_debug_btsco){
            adev->voip_start=false;
        } else {
            adev->voip_start=true;
        }
    }else{
        adev->voip_start=false;
    }
#endif

#ifdef VOIP_DSP_PROCESS
#ifndef AUDIO_OLD_MODEM
    if((adev->voip_start == 1)
                &&(!adev->call_start)&&(!voip_is_forbid(adev)))
#else
    if(((adev->voip_start == 1) && (!((in->device & ~ AUDIO_DEVICE_BIT_IN) & AUDIO_DEVICE_IN_ALL_SCO)))
            &&(!adev->call_start)&&(!voip_is_forbid(adev)))
#endif
    {
        if(!in->is_voip ) {
            ALOGD(": in_read sco start  and do standby");
            do_input_standby(in);
            adev->voip_state |= VOIP_CAPTURE_STREAM;
            force_standby_for_voip(adev);
            in->is_voip=true;
        }
    }
    else{
        if(in->is_voip){
            ALOGD(": in_read sco stop  and do standby");
            do_input_standby( in);
        }
        if(voip_is_forbid(adev)){
            LOG_V("in_read:voip is forbid");
        }
    }
#endif
    if((adev->call_start != 1) &&
#ifndef AUDIO_OLD_MODEM
            ((in->device & ~ AUDIO_DEVICE_BIT_IN) & AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET)
            && (!in->is_voip))
#else
            ((in->device & ~ AUDIO_DEVICE_BIT_IN) & AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET))
#endif
    {
        if(!in->is_bt_sco) {
            ALOGD("bt_sco:in_read start and do standby");
            do_input_standby(in);
            in->is_bt_sco=true;
        }
    }
    else{
        if(in->is_bt_sco){
            ALOGD("bt_sco:in_read stop and do standby");
            do_input_standby( in);
        }
    }

    if((!in->is_voip) && adev->voip_state){
        memset(buffer,0,bytes);
        pthread_mutex_unlock(&in->lock);
	pthread_mutex_unlock(&adev->lock);
	usleep(100000);
       return bytes;
    }

    if (in->standby) {
        in->standby = 0;
        ret = start_input_stream(in);
    }

    if((adev->fm_record)
        ||(adev->call_start)
        ||((in->is_bt_sco) ||( adev->i2sfm_flag))
        ||(in->is_voip)){
        is_normal_record=false;
    }else{
        is_normal_record=true;
    }

    pthread_mutex_unlock(&adev->lock);

    if (ret < 0) {
        ALOGE("start_input_stream  ret error %d", ret);
        goto exit;

    }

    if (in->rec_nr_handle){
        if ((in->rec_nr_handle)&&(in->Tras48To44))
        {
            ALOGV("do record nr and resample");
            ret = SprdSrc_To_44K(in->Tras48To44,buffer, bytes);
            if(ret <0)
            {
                ALOGE("record nr error");
                goto exit;
            }
        }
        else if (in->rec_nr_handle)
        {
            LOG_D("do record nr and no resample to44.1k from 48k");
            ret = AudioRecordNr_Proc(in->rec_nr_handle,buffer, bytes);
#ifdef AUDIO_DEBUG
            if(adev->ext_contrl->dump_info->dump_in_read_afternr){
                 do_dump(adev->ext_contrl->dump_info,DUMP_MINI_RECORD_AFTER_NR,
                                 (void*)buffer,bytes);
            }
#endif
            if(ret <0)
            {
                ALOGE("record nr error");
                goto exit;
            }
        }
    } else if(in->is_bt_sco && (in->requested_channels == 1)&&(in->config.channels == 2)) {
            in->read_status = read_pcm_data(in, (void *)in->bt_sco_buf,2*bytes);
            do_pcm_mixer(in->bt_sco_buf,2*bytes,(int16_t*)buffer);
    } else if (in->config.channels  != in->requested_channels){
            int num_read_buff_bytes=0;
            void * read_buff = buffer;
            num_read_buff_bytes=(in->config.channels * bytes) / in->requested_channels ;
            if (num_read_buff_bytes != bytes) {
                if (num_read_buff_bytes > in->conversion_buffer_size) {
                    LOG_I("in read channel conversion pre size:%d size:%d",in->conversion_buffer_size,num_read_buff_bytes);
                    in->conversion_buffer_size = num_read_buff_bytes;
                    in->conversion_buffer = realloc(in->conversion_buffer, in->conversion_buffer_size);
                }
                read_buff = in->conversion_buffer;
            }
            in->read_status = read_pcm_data(in, read_buff, num_read_buff_bytes);
            if (in->config.channels  != in->requested_channels) {
                mono2stereo(buffer,in->conversion_buffer,num_read_buff_bytes/2);
            }
    }else {
	    ret = read_pcm_data(in, buffer, bytes);
    }

    if(in->active_rec_proc && (in->rec_nr_handle == NULL)){
        aud_rec_do_process(buffer, bytes, in->requested_channels,in->proc_buf, in->proc_buf_size);
#ifdef AUDIO_DEBUG
         if(adev->ext_contrl->dump_info->dump_in_read_afternr){
              do_dump(adev->ext_contrl->dump_info,DUMP_MINI_RECORD_AFTER_PROCESS,
                              (void*)buffer,bytes);
         }
#endif
    }

    if(in->pop_mute) {
        memset(buffer, 1, bytes);
        // mute 240ms for pop
        ALOGE("set mute in_read bytes %d in->pop_mute_bytes %d",bytes,in->pop_mute_bytes);
        if((in->pop_mute_bytes -= bytes) <= 0) {
            in->pop_mute = false;
        }
    }
    if((2==in->requested_channels)&&(2==in->config.channels)
        &&(PCM_FORMAT_S16_LE==in->config.format)
        &&(true==is_normal_record)
        &&((AUDIO_DEVICE_IN_BUILTIN_MIC==in->device) || (AUDIO_DEVICE_IN_BACK_MIC==in->device))){
        left2right(buffer,bytes/2);
    }

    audio_pcm_dump(buffer, bytes,DUMP_RECORD_HWL_HAL);
#ifdef AUDIO_DEBUG
        if(adev->ext_contrl->dump_info->dump_in_read){
             do_dump(adev->ext_contrl->dump_info,DUMP_MINI_RECORD_HAL,
                             (void*)buffer,bytes);
        }
#endif

    if (ret > 0)
        ret = 0;
    //in voice ,mic mute can`t mute input data,because We have funtion voice record
    //mute input data will cause voice record no sound
    if (ret == 0 && adev->mic_mute && !adev->call_start){
        memset(buffer, 0, bytes);
    }
    //BLUE_TRACE("in_read final OK, bytes=%d", bytes);

exit:
    if (ret < 0) {
        dump_all_audio_reg(-1,adev->dump_flag);
        if(in->pcm) {
            ALOGW("in_read,warning: ret=%d, (%s)", ret, pcm_get_error(in->pcm));
        }
	pthread_mutex_unlock(&in->lock);
	pthread_mutex_lock(&adev->lock);
   	pthread_mutex_lock(&in->lock);
        do_input_standby(in);
	pthread_mutex_unlock(&adev->lock);
        memset(buffer, 0, bytes);
    }
    pthread_mutex_unlock(&in->lock);

    if (bytes > 0) {
        in->frames_read += bytes / audio_stream_in_frame_size(stream);
    }

    return bytes;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    return 0;
}

#ifdef AUDIO_HAL_ANDROID_N_API
static int in_get_capture_position(const struct audio_stream_in *stream,
                                   int64_t *frames, int64_t *time)
{
    if (stream == NULL || frames == NULL || time == NULL) {
        return -EINVAL;
    }
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    int ret = -ENOSYS;

    pthread_mutex_lock(&in->lock);
    if (in->pcm) {
        struct timespec timestamp;
        unsigned int avail;
        if (pcm_get_htimestamp(in->pcm, &avail, &timestamp) == 0) {
            if(in->requested_rate == 44100 && in->config.rate != 0)
                avail = (avail * in->requested_rate)/in->config.rate;

            *frames = in->frames_read + avail;
            *time = timestamp.tv_sec * 1000000000LL + timestamp.tv_nsec;
            ret = 0;
        }
    }
    pthread_mutex_unlock(&in->lock);
    return ret;
}
#endif

int audio_add_output(struct tiny_audio_device *adev,struct tiny_stream_out *out)
{
    struct listnode *item;
    struct listnode *item2;

    struct tiny_stream_out *out_tmp;

    LOG_I("audio_add_output type:%d out:%p %p",out->audio_app_type,out,out->node);

    pthread_mutex_lock(&adev->lock);
    list_for_each_safe(item, item2,(&adev->active_out_list)){
        out_tmp = node_to_item(item, struct tiny_stream_out, node);
        if (out_tmp == out) {
            LOG_W("audio_add_output:%p type:%d aready in list",out,out->audio_app_type);
            pthread_mutex_unlock(&adev->lock);
            return 0;
        }
    }

    list_add_tail(&adev->active_out_list, &out->node);
    pthread_mutex_unlock(&adev->lock);
    return 0;
}

int audio_del_output(struct tiny_audio_device *adev,struct tiny_stream_out *out)
{
    struct listnode *item;
    struct listnode *item2;
    struct tiny_stream_out * out_tmp = NULL;
    LOG_I("audio_del_output type:%d out:%p",out->audio_app_type,out);

    pthread_mutex_lock(&adev->lock);
    if(!list_empty(&adev->active_out_list)){
        //LOG_I("audio_del_output out:%p %p %p %p",item,item2,&adev->active_out_list,adev->active_out_list.next);
            list_for_each_safe(item, item2,&adev->active_out_list){
            out_tmp = node_to_item(item, struct tiny_stream_out, node);
            if(out == out_tmp){
                LOG_I("audio_del_output-:%p type:%d",out,out->audio_app_type);
                list_remove(item);
                break;
             }
        }
    }
    pthread_mutex_unlock(&adev->lock);

    return 0;
    LOG_I("audio_del_output:exit");
}


static int adev_open_output_stream(struct audio_hw_device *dev,
        audio_io_handle_t handle,
        audio_devices_t devices,
        audio_output_flags_t flags,
        struct audio_config *config,
        struct audio_stream_out **stream_out)
{
    struct tiny_audio_device *ladev = (struct tiny_audio_device *)dev;
    struct tiny_stream_out *out;
    SMART_AMP_STREAM_INFO streamInfo = {0};
    int ret;

    char bootvalue[PROPERTY_VALUE_MAX];
    property_get("sys.boot_completed", bootvalue, "");
    if (strncmp("1", bootvalue, 1) != 0) {
        char buf[12] = {'\0'};
        const char *pHeadsetPath1 = "/sys/class/switch/h2w/state";
        const char *pHeadsetPath2 = "/sys/kernel/headset/state";
        const char* headsetStatePath = NULL;

        if (0 == access(pHeadsetPath2, R_OK)) {
            headsetStatePath = pHeadsetPath2;
        } else if (0 == access(pHeadsetPath1, R_OK)) {
            headsetStatePath = pHeadsetPath1;
        }

        if (headsetStatePath != NULL) {
            int fd = open(headsetStatePath,O_RDONLY);
            ALOGI("%s fd:%d boot before Path:%s", __FUNCTION__, fd, headsetStatePath);
            if (fd >= 0) {
                ssize_t readSize = read(fd,(char*)buf,12);
                close(fd);
                if (readSize > 0) {
                    int value = atoi((char*)buf);
                    if (value == 1 || value == 2) {
                        devices =
                        (value == 1) ?  AUDIO_DEVICE_OUT_WIRED_HEADSET : AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
                    } else {
                        ALOGE("%s headset state is error! value%d", __FUNCTION__, value);
                    }
                } else {
                    ALOGE("%s read %s is fail! readSize :%d", __FUNCTION__, headsetStatePath, readSize);
                }
            } else {
                ALOGE("%s open %s is fail!", __FUNCTION__, headsetStatePath);
            }
        } else {
            ALOGE("%s headset state dev node is not access", __FUNCTION__);
        }
    } else {
        ALOGI("%s check headset state was boot after", __FUNCTION__);
    }

    BLUE_TRACE("adev_open_output_stream, devices=%d", devices);
    out = (struct tiny_stream_out *)calloc(1, sizeof(struct tiny_stream_out));
    if (!out){
        ALOGE("adev_open_output_stream calloc fail, size:%d", sizeof(struct tiny_stream_out));
        return -ENOMEM;
    }
    memset(out, 0, sizeof(struct tiny_stream_out));

    out->channel_mask = AUDIO_CHANNEL_OUT_STEREO;

    if (flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) {
        /* check offload supported information */
        if (config->offload_info.version != AUDIO_INFO_INITIALIZER.version ||
            config->offload_info.size != AUDIO_INFO_INITIALIZER.size) {
            ALOGE("%s: offload information is not supported ", __func__);
            ret = -EINVAL;
            goto err_open;
        }
        if (!audio_is_offload_support_format(config->offload_info.format)) {
            ALOGE("%s: offload audio format(%d) is not supported ",
                        __func__,config->offload_info.format);
            ret = -EINVAL;
            goto err_open;
        }
        /*  codec type and parameters requested */
        out->compress_config.codec = (struct snd_codec *)calloc(1, sizeof(struct snd_codec));
        out->audio_app_type = AUDIO_HW_APP_OFFLOAD;
        out->offload_format = config->format;
        out->offload_samplerate = config->sample_rate;

        if (config->offload_info.channel_mask)
            out->channel_mask = config->offload_info.channel_mask;
        else if (config->channel_mask)
            out->channel_mask = config->channel_mask;

        out->stream.set_callback = out_offload_set_callback;
        out->stream.pause = out_offload_pause;
        out->stream.resume = out_offload_resume;
        out->stream.drain = out_offload_drain;
        out->stream.flush = out_offload_flush;

        out->compress_config.codec->id =
                audio_get_offload_codec_id(config->offload_info.format);
        out->compress_config.fragment_size = AUDIO_OFFLOAD_FRAGMENT_SIZE;
        out->compress_config.fragments = AUDIO_OFFLOAD_FRAGMENTS_NUM;
        out->compress_config.codec->bit_rate =
                    config->offload_info.bit_rate;
        out->compress_config.codec->ch_in =
                    popcount(config->channel_mask);
        out->compress_config.codec->ch_out = out->compress_config.codec->ch_in;

        if (flags & AUDIO_OUTPUT_FLAG_NON_BLOCKING)
            out->is_offload_nonblocking = 1;
        out->is_offload_need_set_metadata = 1;
        audio_offload_create_thread(out);
        out->stream.common.standby = offload_out_standby;
        if(flags&AUDIO_OUTPUT_FLAG_DEEP_BUFFER) {
            out->config = pcm_config_mm_deepbuf;
            ladev->deepbuffer_enable=true;
            ladev->active_deepbuffer_output=NULL;
        }
        else {
            out->config = pcm_config_mm_fast;
        }
    }
    else {
        int channel_count = audio_channel_count_from_out_mask(config->channel_mask);
        if(channel_count > 2) {
                ALOGE("adev_open_output_stream channel_count fail, channel_count:%d", channel_count);
                goto err_open;
        }
        ret = create_resampler(config->sample_rate,
                DEFAULT_OUT_SAMPLING_RATE,
                channel_count,
                RESAMPLER_QUALITY_DEFAULT,
                NULL,
                &out->resampler);
        if (ret != 0)
        {
            ALOGE("adev_open_output_stream create_resampler fail, ret:%d", ret);
            goto err_open;
        }
        out->buffer = malloc(RESAMPLER_BUFFER_SIZE); /* todo: allow for reallocing */
        if (NULL==out->buffer)
        {
            ALOGE("adev_open_output_stream out->buffer alloc fail, size:%d", RESAMPLER_BUFFER_SIZE);
            goto err_open;
        }
        else
        {
            memset(out->buffer, 0, RESAMPLER_BUFFER_SIZE);
        }
        out->audio_app_type = AUDIO_HW_APP_PRIMARY;
        out->stream.common.standby = out_standby;
        if(flags&AUDIO_OUTPUT_FLAG_DEEP_BUFFER) {
                out->config = pcm_config_mm_deepbuf;
                ladev->deepbuffer_enable=true;
                ladev->active_deepbuffer_output=NULL;
        }
        else {
                out->config = pcm_config_mm_fast;
        }
        out->config.channels = channel_count;
        if (config->format != audio_format_from_pcm_format(out->config.format)) {
            out->config.format = pcm_format_from_audio_format(config->format);
        }
    }
    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;
    out->stream.get_presentation_position = out_get_presentation_position;

    if(flags&AUDIO_OUTPUT_FLAG_PRIMARY) {
        skd_init(ladev,out);
#ifdef AUDIO_TEST_INTERFACE_V2
        audio_endpoint_test_init(ladev,out);
#endif
    }

    out->dev = ladev;
    out->standby = 1;
    out->devices = devices;
    out->flags = flags;

    /* FIXME: when we support multiple output devices, we will want to
     * do the following:
     * adev->devices &= ~AUDIO_DEVICE_OUT_ALL;
     * adev->devices |= out->device;
     * select_output_device(adev);
     * This is because out_set_parameters() with a route is not
     * guaranteed to be called after an output stream is opened. */

    out->channel_mask = config->channel_mask;
    out->sample_rate = config->sample_rate;
    out->format = config->format;

#if 0
    config->format = out->stream.common.get_format(&out->stream.common);
    config->channel_mask = out->stream.common.get_channels(&out->stream.common);
    config->sample_rate = out->stream.common.get_sample_rate(&out->stream.common);
#endif

    if (ladev->pSmartAmpMgr->isEnable) {
        streamInfo.channel = config->channel_mask;
        streamInfo.format = config->format;
        streamInfo.flagDump = ladev->pSmartAmpMgr->isDump;
        streamInfo.mixer = ladev->mixer;
        out->pSmartAmpCtl = init_smart_amp(SMART_INIT_BIN_FILE, &streamInfo);
        if (NULL == out->pSmartAmpCtl) {
            ALOGE("adev_open_output_stream out->init_smart_amp is fail!");
            goto err_open;
        }
        // set config to speaker;
        ret = select_smart_amp_config(out->pSmartAmpCtl, HANDSFREE);
        if (0 != ret) {
            ALOGE("adev_open_output_stream out->select_smart_amp_config is fail!");
            goto err_open;
        }
        if (ladev->pSmartAmpMgr->isBoostDevice) {
            ret = smart_amp_set_battery_voltage_uv(out->pSmartAmpCtl,
                ladev->pSmartAmpMgr->boostVoltage * 1000);
            if (0 != ret) {
                ALOGE("adev_open_output_stream boost \
                    out->smart_amp_set_battery_voltage_uv is fail!");
                goto err_open;
            }
        } else {
            long int BatteryVoltage = 0;
            ret = smart_amp_get_battery_voltage(&BatteryVoltage, BATTERY_VOLTAGE_PATH);
            if (0 != ret) {
                ALOGE("adev_open_output_stream out->smart_amp_get_battery_voltage is fail!");
                goto err_open;
            }
            ret = smart_amp_set_battery_voltage_uv(out->pSmartAmpCtl, BatteryVoltage);
            if (0 != ret) {
                ALOGE("adev_open_output_stream vbat smart amp \
                    out->smart_amp_set_battery_voltage_uv is fail!");
                goto err_open;
            }
        }
        // get battery delta
        ret = smart_amp_get_battery_delta(&out->smartAmpBatteryDelta);
        if (0 != ret) {
            ALOGE("adev_open_output_stream smart_amp_get_battery_delta is fail!");
            goto err_open;
        } else {
            out->smartAmpBatteryDelta = out->smartAmpBatteryDelta * 1000; //from mv to uv
        }
        out->smartAmpLoopCnt = SMART_AMP_LOOP_CNT;
    }

    BLUE_TRACE("Successful, adev_open_output_stream");
    *stream_out = &out->stream;
    audio_add_output(ladev,out);
    return 0;

err_open:
    BLUE_TRACE("Error adev_open_output_stream");
    free(out);
    *stream_out = NULL;
    return ret;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
        struct audio_stream_out *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    BLUE_TRACE("adev_close_output_stream");
    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
            offload_out_standby(stream);
            audio_offload_destroy_thread(out);
            if (out->compress_config.codec != NULL)
                free(out->compress_config.codec);
    }
    else {
        out_standby(&stream->common);
        if (out->buffer)
            free(out->buffer);
        if (out->resampler){
            release_resampler(out->resampler);
            out->resampler = NULL;
        }

        if((adev->deepbuffer_enable) && (AUDIO_OUTPUT_FLAG_DEEP_BUFFER&out->flags)){
            adev->deepbuffer_enable=false;
        }

        if(out->buffer_vplayback)
            free(out->buffer_vplayback);
        if(out->resampler_vplayback)
            release_resampler(out->resampler_vplayback);
        if (out->pSmartAmpCtl && adev->pSmartAmpMgr->isEnable) {
            if (adev->pSmartAmpMgr->isBoostDevice) {
                smart_amp_set_boost(out->pSmartAmpCtl, BOOST_BYPASS);
            }
            deinit_smart_amp(out->pSmartAmpCtl);
        }
    }

#ifdef NXP_SMART_PA
    if(adev->smart_pa_exist && out->nxp_pa) {
        out->nxp_pa = false;
        if(out->handle_pa) {
            NxpTfa_Close(out->handle_pa);
            out->handle_pa = NULL;
        }
    }
#endif

    audio_del_output(adev,out);
    free(stream);
}

static void audiodebug_process(struct tiny_audio_device *adev,struct str_parms *parms)
{
    char value[128];
    int ret;
    int val=0;
    //Adie/Digital/VBC Loopback
    ret = str_parms_get_str(parms, "loop", value, sizeof(value));
    if (ret >= 0) {
        ALOGI("%s loop: value is %s",__func__,value);
        pthread_mutex_lock(&adev->lock);
        struct mixer_ctl *ctl;
        if (strcmp(value,"clear") == 0){
            ctl = mixer_get_ctl_by_name(adev->mixer,"ADC1-DAC Digital Loop switch");
            mixer_ctl_set_value(ctl, 0, 0);
            ctl = mixer_get_ctl_by_name(adev->mixer,"ADC-DAC Digital Loop switch");
            mixer_ctl_set_value(ctl, 0, 0);
            ctl = mixer_get_ctl_by_name(adev->mixer,"ADC1-DAC Adie Loop switch");
            mixer_ctl_set_value(ctl, 0, 0);
            ctl = mixer_get_ctl_by_name(adev->mixer,"ADC-DAC Adie Loop switch");
            mixer_ctl_set_value(ctl, 0, 0);
        } else if (strcmp(value,"digital0") == 0){
            ctl = mixer_get_ctl_by_name(adev->mixer,"ADC-DAC Digital Loop switch");
            mixer_ctl_set_value(ctl, 0, 1);
        } else if (strcmp(value,"digital1") == 0){
            ctl = mixer_get_ctl_by_name(adev->mixer,"ADC1-DAC Digital Loop switch");
            mixer_ctl_set_value(ctl, 0, 1);
        } else if (strcmp(value,"adie0") == 0){
            ctl = mixer_get_ctl_by_name(adev->mixer,"ADC-DAC Adie Loop switch");
            mixer_ctl_set_value(ctl, 0, 1);
        } else if (strcmp(value,"adie1") == 0){
            ctl = mixer_get_ctl_by_name(adev->mixer,"ADC1-DAC Adie Loop switch");
            mixer_ctl_set_value(ctl, 0, 1);
        } else if (strcmp(value,"vbc_enable") == 0){
            if(adev->pcm_fm_dl == NULL)
                adev->pcm_fm_dl= pcm_open(s_tinycard, PORT_FM, PCM_OUT, &pcm_config_fm_dl);
            if (!pcm_is_ready(adev->pcm_fm_dl)) {
                ALOGE("%s:AUDIO_DEVICE_OUT_FM cannot open pcm_fm_dl : %s", __func__,pcm_get_error(adev->pcm_fm_dl));
                pcm_close(adev->pcm_fm_dl);
                adev->pcm_fm_dl= NULL;
            } else {
                if( 0 != pcm_start(adev->pcm_fm_dl)){
                    ALOGE("%s:pcm_start pcm_fm_dl start unsucessfully: %s", __func__,pcm_get_error(adev->pcm_fm_dl));
                }
            }
            ctl = mixer_get_ctl_by_name(adev->mixer,"Aud Loop in VBC Switch");
            mixer_ctl_set_value(ctl, 0, 1);
            if (adev->private_ctl.fm_da0_mux)
                mixer_ctl_set_value(adev->private_ctl.fm_da0_mux, 0, 1);
            if (adev->private_ctl.fm_da1_mux)
                mixer_ctl_set_value(adev->private_ctl.fm_da1_mux, 0, 1);
        } else if (strcmp(value,"vbc_disable") == 0){
            if(adev->pcm_fm_dl != NULL){
                pcm_close(adev->pcm_fm_dl);
                adev->pcm_fm_dl= NULL;
            }
            ctl = mixer_get_ctl_by_name(adev->mixer,"Aud Loop in VBC Switch");
            mixer_ctl_set_value(ctl, 0, 0);
            if (adev->private_ctl.fm_da0_mux)
                mixer_ctl_set_value(adev->private_ctl.fm_da0_mux, 0, 0);
            if (adev->private_ctl.fm_da1_mux)
                mixer_ctl_set_value(adev->private_ctl.fm_da1_mux, 0, 0);
        }
        pthread_mutex_unlock(&adev->lock);
    }

    //modem voice dump
    ret = str_parms_get_str(parms, "ATSPPCMDUMP", value, sizeof(value));
    if (ret >= 0) {
        pthread_mutex_lock(&adev->lock);
        char at_cmd[32] = "AT+SPPCMDUMP=";
        strcat(at_cmd,value);
        ALOGI("%s at_cmd=%s,value=%s",__func__,at_cmd,value);
        at_cmd_cp_pcm_dump(at_cmd);
        pthread_mutex_unlock(&adev->lock);
    }

    //AudioTest AudioSink Device Routing
    ret = str_parms_get_str(parms, "test_stream_route", value, sizeof(value));
    if (ret >= 0) {
        pthread_mutex_lock(&adev->lock);
        val = atoi(value);
        ALOGI("adev_set_parameters test_stream_route: get adev lock adev->devices is %x,%x :%s",adev->out_devices,val,value);
        if(((val & (AUDIO_DEVICE_OUT_WIRED_HEADSET | AUDIO_DEVICE_OUT_WIRED_HEADPHONE | AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_EARPIECE | AUDIO_DEVICE_OUT_BLUETOOTH_A2DP |
            AUDIO_DEVICE_OUT_BLUETOOTH_SCO | AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET | AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT)) != 0)){
            adev->out_devices &= ~AUDIO_DEVICE_OUT_ALL;
            adev->out_devices |= val;
            if(adev->active_output)  {
                pthread_mutex_lock(&adev->active_output->lock);
                adev->active_output->devices &= ~AUDIO_DEVICE_OUT_ALL;
                adev->active_output->devices |= val;
                pthread_mutex_unlock(&adev->active_output->lock);
            }

            if((adev->deepbuffer_enable)&&(adev->active_deepbuffer_output)){
                pthread_mutex_lock(&adev->active_deepbuffer_output->lock);
                adev->active_deepbuffer_output->devices &= ~AUDIO_DEVICE_OUT_ALL;
                adev->active_deepbuffer_output->devices |= val;
                pthread_mutex_unlock(&adev->active_deepbuffer_output->lock);
            }

            ALOGW("adev_set_parameters test_stream_route want to set devices:0x%x mode:%d call_start:%d ",adev->out_devices,adev->mode,adev->call_start);
            select_devices_signal(adev);
            pthread_mutex_unlock(&adev->lock);
        } else {
            pthread_mutex_unlock(&adev->lock);
        }
    }

    //AudioTest AudioSource Device Routing
    ret = str_parms_get_str(parms, "test_in_stream_route", value, sizeof(value));
    if (ret >= 0) {
        pthread_mutex_lock(&adev->lock);
        val = strtoul(value,NULL,16);
        ALOGI("adev_set_parameters test_in_stream_route: get adev lock adev->in_devices is %x,%x value:%s",adev->in_devices,val,value);
        if(((val & AUDIO_DEVICE_IN_ALL) != 0) && ((adev->in_devices & AUDIO_DEVICE_IN_ALL) != val)){
            adev->in_devices &= ~AUDIO_DEVICE_IN_ALL;
            adev->in_devices |= val;
            ALOGW("adev_set_parameters test_stream_route want to set devices:0x%x mode:%d call_start:%d ",adev->out_devices,adev->mode,adev->call_start);
            select_devices_signal(adev);
            pthread_mutex_unlock(&adev->lock);
        } else {
            pthread_mutex_unlock(&adev->lock);
        }
    }

    //i2sfm switch
    ret = str_parms_get_str(parms, "i2sfm", value, sizeof(value));
    if (ret >= 0) {
        pthread_mutex_lock(&adev->lock);
        val = atoi(value);
        if(val==1) {
            adev->i2sfm_flag=1;
            if (!adev->i2s_mixer)
                adev->i2s_mixer = mixer_open(s_bt_sco);
            if (adev->i2s_mixer) {
                struct mixer_ctl *ctl;
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"fs");
                mixer_ctl_set_value(ctl, 0, 48000);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"hw_port");
                mixer_ctl_set_value(ctl, 0, 0);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"slave_timeout");
                mixer_ctl_set_value(ctl, 0, 3857);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"bus_type");
                mixer_ctl_set_value(ctl, 0, 0);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"byte_per_chan");
                mixer_ctl_set_value(ctl, 0, 1);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"mode");
                mixer_ctl_set_value(ctl, 0, 1);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"lsb");
                mixer_ctl_set_value(ctl, 0, 0);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"rtx_mode");
                mixer_ctl_set_value(ctl, 0, 1);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"sync_mode");
                mixer_ctl_set_value(ctl, 0, 0);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"lrck_inv");
                mixer_ctl_set_value(ctl, 0, 0);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"clk_inv");
                mixer_ctl_set_value(ctl, 0, 0);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"i2s_bus_mode");
                mixer_ctl_set_value(ctl, 0, 1);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"pcm_bus_mode");
                mixer_ctl_set_value(ctl, 0, 0);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"pcm_slot");
                mixer_ctl_set_value(ctl, 0, 0);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"pcm_cycle");
                mixer_ctl_set_value(ctl, 0, 0);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"tx_watermark");
                mixer_ctl_set_value(ctl, 0, 12);
                ctl = mixer_get_ctl_by_name(adev->i2s_mixer,"rx_watermark");
                mixer_ctl_set_value(ctl, 0, 20);
            }
            audio_modem_t *modem = adev->cp;
            if((modem->i2s_btsco_fm->ctrl_file_fd  <= 0)&&(NULL!=modem->i2s_btsco_fm)){
                modem->i2s_btsco_fm->ctrl_file_fd = open(modem->i2s_btsco_fm->ctrl_path,O_RDWR | O_SYNC);
            }

            if(modem->i2s_btsco_fm->ctrl_file_fd  > 0){
                write(modem->i2s_btsco_fm->ctrl_file_fd,"1",1);
            }
        }else{
            FILE *i2s_ctrl;
            char * i2s_ctrl_value="1";
            adev->i2sfm_flag=0;
            i2s_ctrl = fopen("/proc/pin_switch/iis0_sys_sel/vbc_iis0","wb");
            if(i2s_ctrl  != NULL){
                fputs(i2s_ctrl_value,i2s_ctrl);
                fclose(i2s_ctrl);
            }
        }
        pthread_mutex_unlock(&adev->lock);
    }

    //Audio HAL debug for Music/Vaudio/SCO/BT SCO/WAV/Cache /Log Level.
    ret = str_parms_get_str(parms, "hal_debug", value, sizeof(value));
    if (ret >= 0) {
        FILE* hal_audio_ctrl;
        pthread_mutex_lock(&adev->lock);
        hal_audio_ctrl = fopen("/dev/pipe/mmi.audio.ctrl","wb");
        if(hal_audio_ctrl == NULL){
            hal_audio_ctrl = fopen("/data/local/media/mmi.audio.ctrl","wb");
        }
        if(hal_audio_ctrl == NULL){
            ALOGE("%s, open pipe error!! ",__func__);
        }
        if(hal_audio_ctrl  != NULL){
            ALOGE("%s,hal_debug:value=%s",__func__,value);
            fputs(value,hal_audio_ctrl);
            fclose(hal_audio_ctrl);
        }
        pthread_mutex_unlock(&adev->lock);
    }

    ret = str_parms_get_str(parms, "audiodebug_bt_sco", value, sizeof(value));
    if(ret >=0) {
        pthread_mutex_lock(&adev->lock);
        val = atoi(value);
        ALOGI("audiodebug_process: audiodebug_bt_sco:%d", val);
        adev->is_debug_btsco = val;
        pthread_mutex_unlock(&adev->lock);
    }

    ret = str_parms_get_str(parms, "atcmd", value, sizeof(value));
    if(ret >= 0) {
        pthread_mutex_lock(&adev->lock);
        ALOGI("audiodebug_process: atcmd: %s", value);
        do_voice_command_direct(value);
        pthread_mutex_unlock(&adev->lock);
    }
}

#ifdef NXP_SMART_PA
#define I2S_CONFIG_MAX 17
static const char *i2s_config_names[I2S_CONFIG_MAX] = {
    "fs",
    "hw_port",
    "slave_timeout",
    "bus_type",
    "byte_per_chan",
    "mode",
    "lsb",
    "rtx_mode",
    "lrck_inv",
    "sync_mode",
    "clk_inv",
    "i2s_bus_mode",
    "pcm_bus_mode",
    "pcm_slot",
    "pcm_cycle",
    "tx_watermark",
    "rx_watermark",
};

/* The sequence must keep the same with i2s_config_names[] */
static const int ext_codec_configs[I2S_CONFIG_MAX] = {
    48000, /* fs */
    0, /* hw_port */
    0xf11, /* slave_timeout */
    0, /* bus_type */
    1, /* byte_per_chan */
    0, /* mode(master/slave mode) */
    0, /* lsb */
    3, /* rtx_mode */
    0, /* lrck_inv(low_for_left) */
    0, /* sync_mode(lrck) */
    1, /* clk_inv */
    1, /* i2s_bus_mode */
    0, /* pcm_bus_mode */
    0, /* pcm_slot */
    0, /* pcm_cycle */
    16, /* tx_watermark */
    16, /* rx_watermark */
};
/* The sequence must keep the same with i2s_config_names[] */
static const int btsco_nb_configs[I2S_CONFIG_MAX] = {
    8000, /* fs */
    0, /* hw_port */
    0xf11, /* slave_timeout */
    1, /* bus_type */
    1, /* byte_per_chan */
    0, /* mode(master/slave mode) */
    0, /* lsb */
    3, /* rtx_mode */
    0, /* lrck_inv(low_for_left) */
    1, /* sync_mode(lrck) */
    0, /* clk_inv */
    0, /* i2s_bus_mode */
    1, /* pcm_bus_mode(short frame) */
    1, /* pcm_slot */
    1, /* pcm_cycle */
    12, /* tx_watermark */
    20, /* rx_watermark */
};
/* The sequence must keep the same with i2s_config_names[] */
static const int btsco_wb_configs[I2S_CONFIG_MAX] = {
    16000, /* fs */
    0, /* hw_port */
    0xf11, /* slave_timeout */
    0, /* bus_type */
    1, /* byte_per_chan */
    0, /* mode(master/slave mode) */
    0, /* lsb */
    3, /* rtx_mode */
    0, /* lrck_inv(low_for_left) */
    0, /* sync_mode(lrck) */
    1, /* clk_inv */
    1, /* i2s_bus_mode */
    1, /* pcm_bus_mode(short frame) */
    1, /* pcm_slot */
    1, /* pcm_cycle */
    12, /* tx_watermark */
    20, /* rx_watermark */
};

static int set_i2s_parameter_shared(struct mixer *i2s_mixer,int i2s_type)
{
    struct mixer_ctl *ctl;
    int i;
    int ret;
    int set_ctl_err = 0;
    int *configs;
    static int i2s_type_last = -1;

    if (i2s_type == i2s_type_last)
        return 0;

    if (i2s_type == I2S_TYPE_EXT_CODEC)
        configs = ext_codec_configs;
    else if (i2s_type == I2S_TYPE_BTSCO_NB)
        configs = btsco_nb_configs;
    else if (i2s_type == I2S_TYPE_BTSCO_WB)
        configs = btsco_wb_configs;
    else {
        ALOGE("Invalid i2s_type(%d)!", i2s_type);
        return -1;
    }

    for (i = 0; i < I2S_CONFIG_MAX; i++) {
        ctl = mixer_get_ctl_by_name(i2s_mixer, i2s_config_names[i]);
        ret = mixer_ctl_set_value(ctl, 0, configs[i]);
        if (ret < 0) {
            set_ctl_err = 1;
            ALOGE("%s, set '%s' to '%d' failed!", __func__, i2s_config_names[i], configs[i]);
        }
    }

    if (set_ctl_err)
        i2s_type_last = -1;

    return 0;
}
#endif

static int set_i2s_parameter(struct mixer *i2s_mixer,int i2s_type)
{
    struct mixer_ctl *ctl;

    if(!i2s_mixer) {
        i2s_mixer = mixer_open(s_bt_sco);
        if(!i2s_mixer) {
	     ALOGE("cann't open bt_sco mixer!");
            return -1;
        }
    }

#ifdef NXP_SMART_PA
    if (s_adev->ext_codec->i2s_shared)
        return set_i2s_parameter_shared(i2s_mixer, i2s_type);
#endif

    if (i2s_type == I2S_TYPE_BTSCO_WB) {
        ctl = mixer_get_ctl_by_name(i2s_mixer, "fs");
            mixer_ctl_set_value(ctl, 0, 16000);
        ctl = mixer_get_ctl_by_name(i2s_mixer, "bus_type");
            mixer_ctl_set_value(ctl, 0, 0);
        ctl = mixer_get_ctl_by_name(i2s_mixer, "sync_mode");
            mixer_ctl_set_value(ctl, 0, 0);
        ctl = mixer_get_ctl_by_name(i2s_mixer, "lrck_inv");
            mixer_ctl_set_value(ctl, 0, 0);
        ctl = mixer_get_ctl_by_name(i2s_mixer, "clk_inv");
            mixer_ctl_set_value(ctl, 0, 1);
        ctl = mixer_get_ctl_by_name(i2s_mixer, "i2s_bus_mode");
            mixer_ctl_set_value(ctl, 0, 1);
    }else if (i2s_type== I2S_TYPE_BTSCO_NB) {
        ctl = mixer_get_ctl_by_name(i2s_mixer, "fs");
            mixer_ctl_set_value(ctl, 0, 8000);
        ctl = mixer_get_ctl_by_name(i2s_mixer, "bus_type");
            mixer_ctl_set_value(ctl, 0, 1);
        ctl = mixer_get_ctl_by_name(i2s_mixer, "sync_mode");
            mixer_ctl_set_value(ctl, 0, 1);
        ctl = mixer_get_ctl_by_name(i2s_mixer, "lrck_inv");
            mixer_ctl_set_value(ctl, 0, 0);
        ctl = mixer_get_ctl_by_name(i2s_mixer, "clk_inv");
            mixer_ctl_set_value(ctl, 0, 0);
        ctl = mixer_get_ctl_by_name(i2s_mixer, "pcm_bus_mode");
            mixer_ctl_set_value(ctl, 0, 1);
    }
    return 0;
}

static int set_fm_mute(struct audio_hw_device *dev, bool mute)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    uint32_t fm_gain = 0;
    ALOGI("%s, mute:%d", __func__, mute);

    pthread_mutex_lock(&adev->device_lock);
    pthread_mutex_lock(&adev->lock);
    if(mute){
        int i = 0;
        adev->fm_mute = true;
        if(adev->fm_open){
            for(i=adev->fm_volume; i >= 0;i--) {
                usleep(5000);
                fm_gain = 0;
                if (adev->out_devices & AUDIO_DEVICE_OUT_SPEAKER) {
                    fm_gain |= fm_speaker_volume_tbl[i];
                    fm_gain |= fm_speaker_volume_tbl[i]<<16;
                } else {
                    fm_gain |= fm_headset_volume_tbl[i];
                    fm_gain |= fm_headset_volume_tbl[i]<<16;
                }
                SetAudio_gain_fmradio(adev,fm_gain);
            }
            if (adev->private_ctl.fm_da0_mux)
                mixer_ctl_set_value(adev->private_ctl.fm_da0_mux, 0, 0);
            if (adev->private_ctl.fm_da1_mux)
                mixer_ctl_set_value(adev->private_ctl.fm_da1_mux, 0, 0);
        }
    } else {
            int i = 0;
            adev->fm_mute = false;
            if(adev->fm_open){
                if (adev->private_ctl.fm_da0_mux)
                    mixer_ctl_set_value(adev->private_ctl.fm_da0_mux, 0, 1);
                if (adev->private_ctl.fm_da1_mux)
                    mixer_ctl_set_value(adev->private_ctl.fm_da1_mux, 0, 1);

                for(i=1; i <= adev->fm_volume; i++) {
                    usleep(5000);
                    fm_gain = 0;
                    if (adev->out_devices & AUDIO_DEVICE_OUT_SPEAKER) {
                        fm_gain |= fm_speaker_volume_tbl[i];
                        fm_gain |= fm_speaker_volume_tbl[i]<<16;
                    } else {
                        fm_gain |= fm_headset_volume_tbl[i];
                        fm_gain |= fm_headset_volume_tbl[i]<<16;
                    }
                    SetAudio_gain_fmradio(adev,fm_gain);
                }
            }
    }
    pthread_mutex_unlock(&adev->lock);
    pthread_mutex_unlock(&adev->device_lock);

    return 0;
}

static int set_linein_fm_mute(struct audio_hw_device *dev, bool mute)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;

    ALOGD("set linein fm mute: %d", mute);

    if (adev->private_ctl.linein_mute)
        return mixer_ctl_set_value(adev->private_ctl.linein_mute, 0, mute);

    return 0;
}

static void set_linein_fm_volume(struct tiny_audio_device *adev, int vol)
{
    int out_dev = AUDIO_DEVICE_OUT_WIRED_HEADPHONE;

    if (vol < 0 || vol > FM_VOLUME_MAX) {
        ALOGD("%s vol(%d) is invalid!", __func__, vol);
        return;
    }

    if (adev->out_devices & AUDIO_DEVICE_OUT_SPEAKER)
        out_dev = AUDIO_DEVICE_OUT_SPEAKER;

    SetAudio_gain_route(adev, vol, out_dev,
                        AUDIO_DEVICE_IN_BUILTIN_MIC & ~AUDIO_DEVICE_BIT_IN);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    struct str_parms *parms;
    char *str;
    char value[128];
    int ret;
    int val=0;
    uint32_t gain = 0;
    int status = 0;

    BLUE_TRACE("adev_set_parameters, kvpairs : %s", kvpairs);
    /*set Empty Parameter then return 0*/
    if (0 == strncmp(kvpairs, "", sizeof(char))) {
        return status;
    }
    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_BT_NREC, value, sizeof(value));
    if (ret >= 0) {
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0)
            adev->bluetooth_nrec = true;
        else
            adev->bluetooth_nrec = false;
    }

#ifdef FM_VERSION_IS_GOOGLE
     /*Modify for Android M FM */
    ret = str_parms_get_str(parms, "AudioFmPreStop", value, sizeof(value));
    if (ret >=0) {
        val = atoi(value);
        if (0 != val) {
            adev->fm_uldl = false;
            adev->fm_open = false;
            adev->fm_record = false;
        }
    }
#endif

    ret = str_parms_get_str(parms, "policy_force_fm_mute", value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        adev->force_fm_mute = val;
        bool mute = false;
        mute = adev->force_fm_mute ? true:false;

        if (adev->fm_type != AUD_FM_TYPE_LINEIN_3GAINS) {
            set_fm_mute(adev, mute);
        }
    }

    /* specify the sampling rate supported by bt headset
     * 8KHz for BT headset NB, as default.
     * 16KHz for BT headset WB.
     * */
    ret = str_parms_get_str(parms, "bt_samplerate", value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        adev->bluetooth_type = val;
    }

    //audioflinger mixer dump
    ret = str_parms_get_str(parms, "media.audiofw.dump", value, sizeof(value));
    if (ret >= 0) {
        ALOGI("media.audiofw.dump:%s",value);
        property_set("media.audiofw.dump",value);
    }

    //audioflinger mixer dump devices switch
    ret = str_parms_get_str(parms, "media.audiofw.devices.dump", value, sizeof(value));
    if (ret >= 0) {
        ALOGI("media.audiofw.devices.dump:%s",value);
        property_set("media.audiofw.devices.dump",value);
    }

    //audioflinger mixer dump devices switch
    ret = str_parms_get_str(parms, "adev_dump_cmd", value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        if(1==val){
            adev_dump(adev,-1);
        }else{
            if(val&(1<<ADEV_DUMP_TINYMIX)){
                dump_tinymix_infor(-1,1<<ADEV_DUMP_TINYMIX,adev->mixer);
            }
            dump_all_audio_reg(-1,val);
        }
        ALOGI("media.audiofw.devices.dump:%s",value);
        property_set("media.audiofw.devices.dump",value);
    }

    ret = str_parms_get_str(parms, "adev_dump_config", value, sizeof(value));
    if (ret >= 0) {
        adev->dump_flag = atoi(value);
        ALOGI("adev_dump_config:%s",value);
        property_set(DUMP_AUDIO_PROPERTY,value);
    }

    //audio hal dump
    ret = str_parms_get_str(parms, "media.dump.switch", value, sizeof(value));
    if (ret >= 0) {
        ALOGI("media.dump.switch:%s",value);
        ext_property_set("media.dump.switch", value);
        val=strtoul(value,NULL,0);
        set_dump_switch(val);
        set_dump_status(adev,val);
    }

    ret = str_parms_get_str(parms,"mini_dumpcache", value, sizeof(value));
    if(ret >= 0){
        val = atoi(value);
        if(val){
            adev->ext_contrl->dump_info->dump_to_cache = true;
         }else{
            adev->ext_contrl->dump_info->dump_to_cache = false;
         }
    }

    ret = str_parms_get_str(parms,"dumpwav", value, sizeof(value));
    if(ret >= 0){
        val = atoi(value);
        if(val){
            adev->ext_contrl->dump_info->dump_as_wav = true;
         }else{
            adev->ext_contrl->dump_info->dump_as_wav = false;
         }
    }

    ret = str_parms_get_str(parms,"mini_dumpbufferlength", value, sizeof(value));
    {
        if(ret >= 0){
            val = atoi(value);
            adev->ext_contrl->dump_info->out_music->buffer_length = val;
            adev->ext_contrl->dump_info->out_bt_sco->buffer_length = val;
            adev->ext_contrl->dump_info->out_voip->buffer_length = val;
            adev->ext_contrl->dump_info->out_vaudio->buffer_length = val;
            adev->ext_contrl->dump_info->in_read->buffer_length = val;
            adev->ext_contrl->dump_info->in_read_afterprocess->buffer_length = val;
            adev->ext_contrl->dump_info->in_read_aftersrc->buffer_length = val;
            adev->ext_contrl->dump_info->in_read_vbc->buffer_length = val;
            adev->ext_contrl->dump_info->in_read_afternr->buffer_length = val;
        }
    }

    ret = str_parms_get_str(parms, "adev_dumpflag", value, sizeof(value));
    if (ret >= 0) {
        adev->dump_flag=strtoul(value,NULL,0);
    }

    ret = str_parms_get_str(parms, "bt_wbs", value, sizeof(value));
    if (ret > 0) {
        if(strcmp(value, "on") == 0) {
            ALOGI("%s bt_wbs on",__FUNCTION__);
            adev->bluetooth_type = VX_WB_SAMPLING_RATE;
        } else if (strcmp(value, "off") == 0) {
            ALOGI("%s, bt_wbs off", __FUNCTION__);
            adev->bluetooth_type = VX_NB_SAMPLING_RATE;
        }
    }

    //this para for Phone to set realcall state,because mode state may be not accurate
    ret = str_parms_get_str(parms, "real_call", value, sizeof(value));
    if(ret >= 0){
        pthread_mutex_lock(&adev->lock);
        if(strcmp(value, "true") == 0){
            ALOGV("%s set realCall true",__func__);
            voip_forbid(adev,true);
        }else{
            ALOGV("%s set realCall false",__func__);
            voip_forbid(adev,false);
        }
        pthread_mutex_unlock(&adev->lock);
    }

    ret = str_parms_get_str(parms, "screen_state", value, sizeof(value));
    if (ret >= 0) {
        pthread_mutex_lock(&adev->lock);
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0) {
            adev->low_power = false;
        }
        else {
            adev->low_power = true;
        }
        mp3_low_power_check(adev);
        pthread_mutex_unlock(&adev->lock);
    }

    audiodebug_process(adev,parms);

 #if VOIP_DSP_PROCESS
    ret = str_parms_get_str(parms, "sprd_voip_start", value, sizeof(value));
    if (ret > 0) {
        if(strcmp(value, "true") == 0) {
            ALOGI("%s, voip turn on by output", __FUNCTION__);
            //if now in real call we do nothing
            pthread_mutex_lock(&adev->lock);
#ifdef AUDIO_VOIP_VERSION_2
            adev->voip_playback_start = 1;
#else
            adev->voip_start = 1;
#endif
            pthread_mutex_unlock(&adev->lock);
        } else if (strcmp(value, "false") == 0) {
            ALOGI("%s, voip turn off by output", __FUNCTION__);
#ifdef AUDIO_VOIP_VERSION_2
            adev->voip_playback_start = 0;
#else
            adev->voip_start = 0;
#endif
        }
    }
#endif
    ret = str_parms_get_str(parms, "FM_Volume", value, sizeof(value));
    if (ret >= 0) {
        bool checkvalid = false;

        val = atoi(value);
        if(0 <= val && val < FM_VOLUME_MAX)
        {
            checkvalid = true;
        }
        if(checkvalid && val != adev->fm_volume)
        {
            pthread_mutex_lock(&adev->device_lock);
            pthread_mutex_lock(&adev->lock);
            if (adev->fm_type != AUD_FM_TYPE_LINEIN_3GAINS) {
                if(val == 0 && (false==is_output_active(adev)) && adev->call_start == 0){
                    uint32_t  i, fm_gain,step;
                    if (adev->out_devices & AUDIO_DEVICE_OUT_SPEAKER)
                        step = (fm_speaker_volume_tbl[0] - fm_speaker_volume_tbl[adev->fm_volume])/5;
                    else
                        step = (fm_headset_volume_tbl[0] - fm_headset_volume_tbl[adev->fm_volume])/5;

                    for(i=0;i<5;i++) {
                        fm_gain=0;
                        if (adev->out_devices & AUDIO_DEVICE_OUT_SPEAKER) {
                            fm_gain |= (int)(fm_speaker_volume_tbl[adev->fm_volume]+step*i);
                            fm_gain |= (int)(fm_speaker_volume_tbl[adev->fm_volume]+step*i)<<16;
                        } else {
                            fm_gain |= (int)(fm_headset_volume_tbl[adev->fm_volume]+step*i);
                            fm_gain |= (int)(fm_headset_volume_tbl[adev->fm_volume]+step*i)<<16;
                        }
                        SetAudio_gain_fmradio(adev,fm_gain);
                        usleep(8000);
                    }
                }else if(val != 0 && adev->call_start == 0){
                    pthread_mutex_unlock(&adev->lock);
                    pthread_mutex_lock(&adev->mute_lock);
                    if (adev->master_mute) {
                        adev->master_mute = false;
                        adev->cache_mute = true;
                        adev->fm_volume = val;
                        select_devices_signal(adev);
                    }
                    pthread_mutex_unlock(&adev->mute_lock);
                    pthread_mutex_lock(&adev->lock);
                }
            } else {/* Line-in fm */
                ALOGV("%s, master_mute: %d, fm_volume: %d, call_start: %d", __func__,
                    adev->master_mute, adev->fm_volume, adev->call_start);
                if (val != 0 && adev->fm_volume <= 0 && adev->call_start == 0) {
                    /* There is pop noise when set_linein_fm_volume(). So do an action
                     * of mute/unmute codec here.
                     */
                    pthread_mutex_lock(&adev->mute_lock);
                    if (!adev->cache_mute)
                        codec_lowpower_open(adev,true);
                    pthread_mutex_unlock(&adev->mute_lock);

                    set_linein_fm_volume(adev, val);
                    if (adev->out_devices & AUDIO_DEVICE_OUT_SPEAKER)
                        usleep(1000 * 450);

                    set_linein_fm_mute(adev, 0);
                    pthread_mutex_lock(&adev->mute_lock);
                    adev->master_mute = false;
                    adev->cache_mute = false;
                    codec_lowpower_open(adev, false);
                     pthread_mutex_unlock(&adev->mute_lock);
                } else if(adev->call_start == 0) {
                    pthread_mutex_lock(&adev->mute_lock);
                    if (val == 0 && !adev->cache_mute) {
                            codec_lowpower_open(adev,true);
                    pthread_mutex_unlock(&adev->mute_lock);

                        /* When open fm, adev->fm_open is false, so set fm volume to 0
                         * through SetAudio_gain_route() will failed. So add
                         * set_linein_fm_mute() here to do the fm mute action.
                         */
                        if (adev->fm_open == false)
                            set_linein_fm_mute(adev, 1);
                    } else {
                        pthread_mutex_unlock(&adev->mute_lock);
                    }

                    set_linein_fm_volume(adev, val);
                    pthread_mutex_lock(&adev->mute_lock);
                    if (val == 0 && !adev->cache_mute)
                        codec_lowpower_open(adev,false);
                    pthread_mutex_unlock(&adev->mute_lock);
                }
            }

            adev->fm_volume = val;
            out_codec_mute_check(adev);
            if (adev->fm_type != AUD_FM_TYPE_LINEIN_3GAINS) {
                if(!adev->fm_mute){
                    if (adev->out_devices & AUDIO_DEVICE_OUT_SPEAKER) {
                        gain |=fm_speaker_volume_tbl[val];
                        gain |=fm_speaker_volume_tbl[val]<<16;
                    } else {
                        gain |=fm_headset_volume_tbl[val];
                        gain |=fm_headset_volume_tbl[val]<<16;
                    }
                    SetAudio_gain_fmradio(adev,gain);
                    ALOGE("adev_set_parameters fm_open: %d, fm_volume_tbl[%d] %x", adev->fm_open, val, gain);
                    usleep(10000);
                    if(val !=0) {
                        if (false == adev->force_fm_mute) {
                            if (adev->private_ctl.fm_da0_mux)
                                mixer_ctl_set_value(adev->private_ctl.fm_da0_mux, 0, 1);
                            if (adev->private_ctl.fm_da1_mux)
                                mixer_ctl_set_value(adev->private_ctl.fm_da1_mux, 0, 1);
                        } else {
                            ALOGI("FM state is force_fm_mute so can't unmute");
                        }
                    } else {
                        if (adev->private_ctl.fm_da0_mux)
                            mixer_ctl_set_value(adev->private_ctl.fm_da0_mux, 0, 0);
                        if (adev->private_ctl.fm_da1_mux)
                            mixer_ctl_set_value(adev->private_ctl.fm_da1_mux, 0, 0);
                    }
                } else {
                    if (adev->private_ctl.fm_da0_mux)
                        mixer_ctl_set_value(adev->private_ctl.fm_da0_mux, 0, 0);
                    if (adev->private_ctl.fm_da1_mux)
                        mixer_ctl_set_value(adev->private_ctl.fm_da1_mux, 0, 0);
                }
            }
        pthread_mutex_unlock(&adev->lock);
        pthread_mutex_unlock(&adev->device_lock);
        }
    }

   ret = str_parms_get_str(parms,"record_bypass", value, sizeof(value));
   if(ret >= 0){
        val = atoi(value);
        pthread_mutex_lock(&adev->lock);
        if(not_support_ad23()) {
            if(val){
                adev->record_bypass = true;
            }else{
                adev->record_bypass = false;
            }
        }
        pthread_mutex_unlock(&adev->lock);
   }

    ret = str_parms_get_str(parms, "FM_mute", value, sizeof(value));
    if( ret >= 0 ){
        val = atoi(value);
        if (adev->fm_type != AUD_FM_TYPE_LINEIN_3GAINS) {
            if( val == 1){
                ret = set_fm_mute(adev, 1);
            } else {
                if (false == adev->force_fm_mute) {
                    ret = set_fm_mute(adev, 0);
                } else {
                    ALOGI("FM state is force mute, so can't unmute");
                }
            }
        } else {//Line-in
            int i;
            int out_dev = AUDIO_DEVICE_OUT_WIRED_HEADPHONE;

            if (adev->out_devices & AUDIO_DEVICE_OUT_SPEAKER)
                out_dev = AUDIO_DEVICE_OUT_SPEAKER;

            if (val == 1) {
                for (i = adev->fm_volume; i >= 0; i--) {
                    usleep(5000);
                    SetAudio_gain_route(adev, i, out_dev,
                                        AUDIO_DEVICE_IN_BUILTIN_MIC & ~AUDIO_DEVICE_BIT_IN);
                }
            } else {
                for (i=1; i <= adev->fm_volume; i++) {
                    usleep(5000);
                    SetAudio_gain_route(adev, i, out_dev,
                                        AUDIO_DEVICE_IN_BUILTIN_MIC & ~AUDIO_DEVICE_BIT_IN);
                }
            }
        }
    }

    ret = str_parms_get_str(parms, "hfp_enable", value, sizeof(value));//add for bt client,withch iis channel
    if( ret >= 0 ){
        if(strcmp(value, "true") == 0) {
            char *at_cmd ="AT+SPTEST=31,1";
            do_voice_command_direct(at_cmd);
        } else if (strcmp(value, "false") == 0) {
            char *at_cmd ="AT+SPTEST=31,0";
            do_voice_command_direct(at_cmd);
        }
    }

    ret = str_parms_get_str(parms, "line_in", value, sizeof(value));
    if(ret >= 0){
        if(strcmp(value,"on") == 0){
            pthread_mutex_lock(&adev->device_lock);
            pthread_mutex_lock(&adev->lock);
            //adev->in_devices |= AUDIO_DEVICE_IN_LINE_IN;
            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_unlock(&adev->device_lock);
        }else if(strcmp(value,"off") == 0){
            pthread_mutex_lock(&adev->device_lock);
            pthread_mutex_lock(&adev->lock);
            //adev->in_devices &= ~AUDIO_DEVICE_IN_LINE_IN;
            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_unlock(&adev->device_lock);
        }
        select_devices_signal(adev);
    }
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        pthread_mutex_lock(&adev->lock);
        ALOGI("get adev lock adev->devices is %x,%x",adev->out_devices,adev->out_devices&AUDIO_DEVICE_OUT_ALL);
        if(((adev->mode == AUDIO_MODE_IN_CALL) && (adev->call_connected) && ((adev->out_devices & AUDIO_DEVICE_OUT_ALL) != val))||adev->voip_state){
            if(val&AUDIO_DEVICE_OUT_ALL) {
                ALOGE("adev set device in val is %x",val);
                adev->out_devices &= ~AUDIO_DEVICE_OUT_ALL;
                adev->out_devices |= val;

                if(adev->active_output)  {
                    adev->active_output->devices = val;
                }

                if((adev->deepbuffer_enable)&&(adev->active_deepbuffer_output)){
                    adev->active_deepbuffer_output->devices = val;
                }

                if(adev->out_devices & AUDIO_DEVICE_OUT_ALL_SCO) {
                    if(adev->cp_type == CP_TG)
                        i2s_pin_mux_sel(adev,1);
                    else if(adev->cp_type == CP_W)
                        i2s_pin_mux_sel(adev,0);
                    else if( adev->cp_type == CP_CSFB)
                        i2s_pin_mux_sel(adev,CP_CSFB);
                }
                ret = at_cmd_route(adev);  //send at command to cp
                if(ret) {
                    ALOGE("at_cmd_route error ret=%d",ret);
                }
                pthread_mutex_unlock(&adev->lock);
            }
            else {
                pthread_mutex_unlock(&adev->lock);
            }

        } else if(((val & (AUDIO_DEVICE_OUT_WIRED_HEADSET | AUDIO_DEVICE_OUT_WIRED_HEADPHONE)) != 0) && ((adev->out_devices & AUDIO_DEVICE_OUT_ALL) != val)) {
            adev->out_devices &= ~AUDIO_DEVICE_OUT_ALL;
            adev->out_devices |= val;
            ALOGW("adev_set_parameters want to set devices:0x%x mode:%d call_start:%d ",adev->out_devices,adev->mode,adev->call_start);
            select_devices_signal(adev);
            pthread_mutex_unlock(&adev->lock);
        }
        else {
            pthread_mutex_unlock(&adev->lock);
        }
    }
    ret = str_parms_get_str(parms,"handleFm",value,sizeof(value));
    if(ret >= 0){
        val = atoi(value);
        pthread_mutex_lock(&adev->device_lock);
        if(val){
            pthread_mutex_lock(&adev->lock);
            adev->fm_open = true;
            pthread_mutex_unlock(&adev->lock);
        }else{
            pthread_mutex_lock(&adev->lock);
            adev->fm_open = false;
            pthread_mutex_unlock(&adev->lock);
        }

        /* There is pop noise when turn on/off line-in fm device.
         * So do an action of mute/unmute codec here.
         */
        pthread_mutex_lock(&adev->lock);
        if (adev->fm_type == AUD_FM_TYPE_LINEIN_3GAINS && adev->call_start == 0) {
            pthread_mutex_lock(&adev->mute_lock);
            if (!adev->cache_mute) {
                codec_lowpower_open(adev, true);
            }
            pthread_mutex_unlock(&adev->mute_lock);
        }
        pthread_mutex_unlock(&adev->lock);
        do_select_devices_static(adev);
        pthread_mutex_lock(&adev->lock);
        if (adev->fm_type == AUD_FM_TYPE_LINEIN_3GAINS && adev->call_start == 0) {
            pthread_mutex_lock(&adev->mute_lock);
            if (!adev->master_mute) {
                codec_lowpower_open(adev, false);
            }
            pthread_mutex_unlock(&adev->mute_lock);
        }
        pthread_mutex_unlock(&adev->lock);

        pthread_mutex_unlock(&adev->device_lock);

#ifdef NXP_SMART_PA
        if (adev->smart_pa_exist)
            adev->device_force_set =1;
#endif
        select_devices_signal(adev);
    }
    ret = str_parms_get_str(parms,"fm_record",value,sizeof(value));
    if(ret >= 0){
        val = atoi(value);
        pthread_mutex_lock(&adev->device_lock);
        pthread_mutex_lock(&adev->lock);
        if(val){
            adev->fm_record = true;
        }else{
            if ((NULL!=adev->active_input)  && (adev->fm_record==true)){
                struct tiny_stream_in *in=NULL;
                in = adev->active_input;
                pthread_mutex_lock(&in->lock);
                do_input_standby(in);
                pthread_mutex_unlock(&in->lock);
            }
            adev->fm_record = false;
        }
        pthread_mutex_unlock(&adev->lock);
        pthread_mutex_unlock(&adev->device_lock);
    }

    ret = str_parms_get_str(parms, "play_sprd_record_tone", value, sizeof(value));
    if (ret > 0) {
        val = atoi(value);
        if(val == 1){
            ALOGW("%s, record tone by output, value:%d, call_connected:%d", __FUNCTION__,val, adev->call_connected);
            ret = adev_record_tone_start(adev, NULL);
        } else if (val == 0) {
            ALOGW("%s, record tone by output, value:%s", __FUNCTION__,value);
            ret = adev_record_tone_stop(adev);
        }
    }

    ret = str_parms_get_str(parms, "play_record_tone", value, sizeof(value));
    if (ret > 0) {
        char file_name[1024] = {0};
        val = atoi(value);

        if(val != 0){
            ALOGW("%s, record tone by output, value:%d, call_connected:%d, adev->record_tone_info.fd:%d",
                __FUNCTION__,val, adev->call_connected, adev->record_tone_info->fd);
            sprintf(file_name,"record_tone_%d.pcm",val);
            ret = adev_record_tone_start(adev, file_name);
        }
        else if (val == 0) {
            ALOGW("%s, record tone by output, value:%s", __FUNCTION__,value);
            ret = adev_record_tone_stop(adev);
        }
    }

    ret = str_parms_get_str(parms, "mixer_route", value, sizeof(value));
    if (ret > 0) {
        pthread_mutex_lock(&adev->lock);
        adev_get_cp_card_name(adev);
        val = atoi(value);
        ALOGW("%s, mixer_route, val:%d", __FUNCTION__, val);
        adev->mixer_route = val;
        set_cp_mixer_type(adev->cp_mixer[adev->cp_type], val);
        pthread_mutex_unlock(&adev->lock);
    }

    ret = str_parms_get_str(parms, "voice_record_updownlink", value, sizeof(value));
    if (ret > 0) {
        struct mixer *in_mixer= NULL;
        struct mixer_ctl *ctl1 = NULL;
        val = atoi(value);
        ALOGI("%s, voice_record_updownlink, val:%d", __func__, val);
        pthread_mutex_lock(&adev->lock);
        if (val > 2){
            ALOGE("the route val(%d) is error, must <= 2!!!", val);
        } else {
            in_mixer = adev->cp_mixer[adev->cp_type];
            ctl1 = mixer_get_ctl_by_name(in_mixer,"PCM CAPTURE Route");
            mixer_ctl_set_value(ctl1, 0, val);
            ALOGI("%s, voice_record_updownlink, set PCM CAPTURE Route to %d", __func__, val);
        }
        pthread_mutex_unlock(&adev->lock);
    }

    ret = AudioMMIParse(parms, adev->mmiHandle);
    if (MMI_TEST_IS_OFF == ret) {
        ALOGV("AudioCustom_MmiParse Is OFF");
        ret = 0;
    } else if (MMI_TEST_NO_ERR != ret) {
        ret = -EINVAL;
    }
    str_parms_destroy(parms);
    if (adev->pSmartAmpMgr->isEnable && !adev->pSmartAmpMgr->isBoostDevice) {
        ret = smart_amp_set_battery_voltage(kvpairs);
    }
    if (ret >= 0){
        return status;
    } else {
        return ret;
    }
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
        const char *keys)
{

    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    if (strcmp(keys, "point_info") == 0) {
        char* point_info;
        ALOGE("keys: %s",keys);
        point_info=get_pointinfo_hal(adev->cp_nbio_pipe);
        return strdup(point_info);
    } else if(strcmp(keys, "vbc_codec_reg") == 0) {
        int max_size = 5000;
        int read_size;
        char *pReturn;
        void *dumpbuffer =NULL;
        FILE *pFileVbc = NULL;
        FILE *pFileCodec = NULL;

        pFileVbc = fopen ("/proc/asound/sprdphone/vbc", "rb" );
        if (pFileVbc==NULL) {
           ALOGE(" %s open vbc failed ",__func__);
           return NULL;
        }
        pFileCodec = fopen ("/proc/asound/sprdphone/sprd-codec", "rb" );
        if (pFileCodec==NULL) {
           ALOGE(" %s open codec failed ",__func__);
           if(pFileVbc) {
                fclose(pFileVbc);
           }
           return NULL;
        }
        dumpbuffer = (void*)malloc(max_size);
        memset(dumpbuffer,0,max_size);
        //read vbc reg
        fread(dumpbuffer, max_size,1,pFileVbc);
        read_size =strlen(dumpbuffer);
        ALOGD(" %s ", dumpbuffer);

        //read codec reg
        void *tmpbuffer = (char*)dumpbuffer+read_size;
        fread(tmpbuffer, max_size -read_size ,1,pFileCodec);
        ALOGD("%s", tmpbuffer);

        pReturn = strdup(dumpbuffer);
        free(dumpbuffer);
        if(pFileVbc) {
            fclose(pFileVbc);
        }
        if(pFileCodec) {
            fclose(pFileCodec);
        }
        return pReturn;
    } else if (strcmp(keys, "tinymix") == 0) {
        int max_size = 20000;
        int read_size = max_size;
        char *pReturn;
        void *dumpbuffer = (void*)malloc(max_size);
        tinymix_list_controls(adev->mixer,dumpbuffer,&read_size);
        pReturn = strdup(dumpbuffer);
        free(dumpbuffer);
        return pReturn;
    } else if ((adev->pSmartAmpMgr->isEnable) &&
              (strncmp(keys, "vol_guard", strlen("vol_guard")) == 0)) {
        int rc = 0;
        int delta = 0;
        rc = smart_amp_get_battery_delta(&delta);
        if (0 == rc) {
            char deltaStr[32] = {0};
            char *str = NULL;
            snprintf(deltaStr, sizeof(deltaStr), "%d", delta);
            str = strdup(deltaStr);
            ALOGI("smart_amp_get_battery_delta: %s deltaStr :%s", str, deltaStr);
            return str;
        }
    }
    return strdup("");
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    ALOGW("adev_init_check");
    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    if(1){//(adev->mode == AUDIO_MODE_IN_CALL || adev->call_start ){
        BLUE_TRACE("adev_set_voice_volume in...volume:%f mode:%d call_start:%d ",volume,adev->mode,adev->call_start);
        adev->voice_volume = volume;
        /*Send at command to cp side*/
        at_cmd_volume(volume,adev->mode);
    }else{
        BLUE_TRACE("%s in, not MODE_IN_CALL (%d), return ",__func__,adev->mode);
    }
    return 0;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    ALOGW("adev_set_master_volume in...devices:0x%x ,volume:%f ",adev->out_devices,volume);
    SetAudio_gain_route(adev,1,adev->out_devices,adev->in_devices);
    return -ENOSYS;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    bool need_unmute = false;
    BLUE_TRACE("adev_set_mode, mode=%d", mode);
    pthread_mutex_lock(&adev->lock);
    if (adev->mode != mode) {
        adev->mode = mode;
        need_unmute = true;
        select_mode(adev);
    }else{
        BLUE_TRACE("adev_set_mode,the same mode(%d)",mode);
    }
    pthread_mutex_unlock(&adev->lock);
    //there is lock in function adev_set_master_mute,so we can`t call this funciont under lock
    if(need_unmute)
        adev_set_master_mute(dev, false);

    return 0;
}
static int set_codec_mute(struct tiny_audio_device *adev ,bool mute,bool in_call)
{
    ALOGI("%s in  mute:%d",__func__, mute);
    struct mixer_ctl *ctl;
    if(!in_call) {
		set_virt_output_switch(adev,true);
    }

    if (adev->private_ctl.speaker_mute)
        mixer_ctl_set_value(adev->private_ctl.speaker_mute, 0, mute);

    if (adev->private_ctl.speaker2_mute)
        mixer_ctl_set_value(adev->private_ctl.speaker2_mute, 0, mute);

    if (adev->private_ctl.earpiece_mute)
        mixer_ctl_set_value(adev->private_ctl.earpiece_mute, 0, mute);

    if (adev->private_ctl.headphone_mute)
        mixer_ctl_set_value(adev->private_ctl.headphone_mute, 0, mute);

    return 0;

}

static int adev_set_master_mute(struct audio_hw_device *dev, bool mute)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;

    pthread_mutex_lock(&adev->mute_lock);
    if (adev->master_mute == mute) {
        pthread_mutex_unlock(&adev->mute_lock);
        return 0;
    }
    if (!adev->master_mute && adev->mode == AUDIO_MODE_IN_CALL) {
        pthread_mutex_unlock(&adev->mute_lock);
        return 0;
    }
    pthread_mutex_unlock(&adev->mute_lock);
    if (adev->active_output && (adev->active_deepbuffer_output
                    ||(adev->active_offload_output && adev->is_offload_running)))
        return 0;
    if (adev->fm_open && (adev->fm_volume != 0)) {
        return 0;
    }
    ALOGD("%s, mute=%d, master_mute=%d", __func__, mute, adev->master_mute);
    pthread_mutex_lock(&adev->device_lock);
    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&adev->mute_lock);
    adev->master_mute = mute;
    pthread_mutex_unlock(&adev->mute_lock);
    pthread_mutex_unlock(&adev->lock);
    pthread_mutex_unlock(&adev->device_lock);

    select_devices_signal(adev);
    ALOGD("adev_set_master_mute(%d)", adev->master_mute);
    return 0;
}

static int adev_get_master_mute(const struct audio_hw_device *dev, bool *mute)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    pthread_mutex_lock(&adev->mute_lock);
    *mute = adev->master_mute;
    pthread_mutex_unlock(&adev->mute_lock);

    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;

    pthread_mutex_lock(&adev->lock);
    if (adev->mic_mute != state)
    {
        adev->mic_mute = state;
  //    if (adev->mode == AUDIO_MODE_IN_CALL || adev->call_start ||adev->voip_start)
        {
            at_cmd_mic_mute(adev->mic_mute);
        }
    }
    pthread_mutex_unlock(&adev->lock);

    return 0;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    pthread_mutex_lock(&adev->lock);
    *state = adev->mic_mute;
    pthread_mutex_unlock(&adev->lock);
    return 0;
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
        const struct audio_config *config)
{
    size_t size;
    int channel_count = popcount(config->channel_mask);

    if (check_input_parameters(config->sample_rate, config->format, channel_count) != 0)
        return 0;

    return get_input_buffer_size(config->sample_rate, config->format, channel_count);
}

static int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    return 0;
}

static int adev_open_input_stream(struct audio_hw_device *dev,
        audio_io_handle_t handle,
        audio_devices_t devices,
        struct audio_config *config,
        struct audio_stream_in **stream_in,
        audio_input_flags_t flags,
        const char *address __unused,
        audio_source_t source)
{
    struct tiny_audio_device *ladev = (struct tiny_audio_device *)dev;
    struct tiny_stream_in *in;
    int ret = 0;
    int channel_count = popcount(config->channel_mask);

    BLUE_TRACE("[TH], adev_open_input_stream,devices=0x%x,sample_rate=%d, channel_count=%d",
            devices, config->sample_rate, channel_count);

    if (check_input_parameters(config->sample_rate, config->format, channel_count) != 0)
        return -EINVAL;

    in = (struct tiny_stream_in *)calloc(1, sizeof(struct tiny_stream_in));
    if (!in)
    {
        ALOGE("adev_open_input_stream alloc fail, size:%d", sizeof(struct tiny_stream_in));
        return -ENOMEM;
    }
    memset(in, 0, sizeof(struct tiny_stream_in));
    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.common.add_audio_effect = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;
#ifdef AUDIO_HAL_ANDROID_N_API
    in->stream.get_capture_position = in_get_capture_position;
#endif

    in->requested_rate = config->sample_rate;

    if (ladev->call_start)
        memcpy(&in->config, &pcm_config_record_incall, sizeof(pcm_config_record_incall));
    else {
        memcpy(&in->config, &pcm_config_mm_ul, sizeof(pcm_config_mm_ul));
        if(in->requested_rate != pcm_config_mm_ul.rate) {
            in->config.period_size  = ((((pcm_config_mm_ul.period_size * pcm_config_mm_ul.period_count*1000)/pcm_config_mm_ul.rate) * in->requested_rate ) /(1000 *in->config.period_count))/160 * 160;
            in->config.rate = in->requested_rate;
        }
    }
    in->config.channels = channel_count;
    in->requested_channels = channel_count;

    in->conversion_buffer = NULL;
    in->conversion_buffer_size = 0;

    ladev->requested_channel_cnt = channel_count;
    in->source = source;

    {
	int size = 0;
	size = in->config.period_size ;
	size = ((size + 15) / 16) * 16;
	in->proc_buf_size = size * 2 * sizeof(short);
	in->proc_buf = malloc(in->proc_buf_size);
	if(!in->proc_buf) {
	    goto err;
	}
    }

    in->dev = ladev;
    in->standby = 1;
    in->device = devices;
    in->pop_mute = true;

    *stream_in = &in->stream;
    BLUE_TRACE("Successfully, adev_open_input_stream.");
    return 0;

err:
    BLUE_TRACE("Failed(%d), adev_open_input_stream.", ret);
    if (in->buffer) {
        free(in->buffer);
        in->buffer = NULL;
    }
    if (in->resampler) {
        release_resampler(in->resampler);
           in->resampler = NULL;
    }
    if(in->proc_buf) {
        free(in->proc_buf);
        in->proc_buf = NULL;
    }

    if (in->pFMBuffer) {
        free(in->pFMBuffer);
        in->pFMBuffer = NULL;
    }

    free(in);
    *stream_in = NULL;
    return ret;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
        struct audio_stream_in *stream)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;

    in_standby(&stream->common);

    if (in->resampler) {
        free(in->buffer);
        release_resampler(in->resampler);
    }
    if (in->proc_buf) {
        free(in->proc_buf);
        in->proc_buf = NULL;
        in->proc_buf_size = 0;
    }
    if (in->ref_buf)
        free(in->ref_buf);

    if (in->fmUlDlHandle) {
        ALOGE("fm_pcm_open pcm_in driver ");
        fm_pcm_close(in->fmUlDlHandle);
        in->fmUlDlHandle = NULL;
    }

    if (in->is_fm) {
        in->is_fm = false;
    }

    if (in->pFMBuffer) {
        free(in->pFMBuffer);
        in->pFMBuffer= NULL;
    }

    free(stream);
    return;
}


static int dump_audio_reg_to_buffer(void *buffer,int max_size,const char *reg_file_name){
    FILE *reg_file=NULL;
    int ret=0;

    reg_file=fopen(reg_file_name,"rb");
    if((NULL!=reg_file)&&(NULL!=buffer)){
        ret=fread(buffer, 1, max_size, reg_file);
        ALOGI("read reg size:%d %d",max_size,ret);
        fclose(reg_file);
        reg_file=NULL;
    }else{
       ret=-1;
    }
    if (reg_file) {
        fclose(reg_file);
        reg_file = NULL;
    }
    return ret;
}

static  void dump_audio_reg(int fd,const char *reg_file_name){
    FILE *reg_file=NULL;
    int max_size=2048;
    void *buffer = (void*)malloc(max_size);
    int ret=0;

    ALOGI("\ndump_audio_reg:%s",reg_file_name);
    if(NULL!=buffer){
        memset(buffer,0,max_size);
        ret=dump_audio_reg_to_buffer(buffer,max_size-1,reg_file_name);
        if(ret>0){
            AUDIO_DUMP_WRITE_BUFFER(fd,buffer,ret);
        }else{
            ALOGI("dump_audio_reg open:%s failed",reg_file_name);
            snprintf(buffer,max_size-1,
                "\ndump_audio_reg open:%s failed\n",reg_file_name);
            AUDIO_DUMP_WRITE_STR(fd,buffer);
        }
        free(buffer);
        buffer=NULL;
    }
}
void wirte_buffer_to_log(void *buffer,int size){
    char *line=NULL;
    char *tmpstr;
    char *data_buf=malloc(size+1);
    if(NULL!=data_buf){
        memcpy(data_buf, buffer, size);
        *(data_buf + size) = '\0';

        line = strtok_r(data_buf, REG_SPLIT, &tmpstr);
        while (line != NULL) {
            ALOGI("%s\n",line);
            line = strtok_r(NULL, REG_SPLIT, &tmpstr);
        }
        free(data_buf);
    }else{
        ALOGW("wirte_buffer_to_log malloc buffer failed");
    }
}


void dump_all_audio_reg(int fd,int dump_flag){
    /* dump codec reg */
    if(dump_flag&(1<<ADEV_DUMP_CODEC_REG)){
        dump_audio_reg(fd,SPRD_CODEC_REG_FILE);
    }

    /* dump vbc reg */
    if(dump_flag&(1<<ADEV_DUMP_VBC_REG)){
        dump_audio_reg(fd,VBC_REG_FILE);
    }

    /* dump dma reg */
    if(dump_flag&(1<<ADEV_DUMP_DMA_REG)){
        dump_audio_reg(fd,AUDIO_DMA_REG_FILE);
    }
}

static void dump_tinymix_infor(int fd,int dump_flag,struct mixer *mixer){
    int max_size = 20000;
    int read_size = max_size;

    if(dump_flag&(1<<ADEV_DUMP_TINYMIX)){
        void *buffer = (void*)malloc(max_size);
        if(NULL!=buffer){
            tinymix_list_controls(mixer,buffer,&read_size);
            AUDIO_DUMP_WRITE_BUFFER(fd,buffer,read_size);
            free(buffer);
            buffer=NULL;
        }
    }
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    char buffer[DUMP_BUFFER_MAX_SIZE]={0};
    char tmp[128]={0};
    int ret=-1;
    struct tiny_audio_device *adev = (struct tiny_audio_device *)device;

    ret=property_get(DUMP_AUDIO_PROPERTY, tmp, "0");
    if (ret < 0) {
        ALOGI("adev_dump property_get Failed");
    }else{
        adev->dump_flag=strtoul(tmp,NULL,0);
    }

    snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),"\nadev_dump ===>\n");
    AUDIO_DUMP_WRITE_STR(fd,buffer);
    memset(buffer,0,sizeof(buffer));
    ALOGI("adev_dump enter");

    snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
        "devices:out_devices:0x%x "
        "in_devices:0x%x "
        "prev_out_devices:0x%x "
        "prev_in_devices:0x%x "
        "routeDev:0x%x "
        "device_force_set:0x%x\n",
        adev->out_devices,
        adev->in_devices,
        adev->prev_out_devices,
        adev->prev_in_devices,
        adev->routeDev,
        adev->device_force_set);
    AUDIO_DUMP_WRITE_STR(fd,buffer);
    memset(buffer,0,sizeof(buffer));

    snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
        "voice infor:cur_vbpipe_fd:0x%x "
#ifdef AUDIO_VOIP_VERSION_2
        "voip_playback_start:0x%x "
#endif
        "cp_type:0x%d "
        "call_start:%d "
        "call_connected:%d "
        "call_prestop:%d "
        "realCall:%d "
        "mode:%d "
        "voip_state:0x%x "
        "voip_start:0x%x\n",
        adev->cur_vbpipe_fd,
#ifdef AUDIO_VOIP_VERSION_2
        adev->voip_playback_start,
#endif
        adev->cp_type,
        adev->call_connected,
        adev->call_prestop,
        adev->realCall,
        adev->mode,
        adev->voip_state,
        adev->voip_start);
    AUDIO_DUMP_WRITE_STR(fd,buffer);
    memset(buffer,0,sizeof(buffer));

    snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
        "volume:mic_mute:%d "
        "master_mute:%d "
        "cache_mute:%d "
        "fm_mute:%d "
        "force_fm_mute:%d\n"
        "voice_volume:0x%x\n",
        adev->mic_mute,
        adev->master_mute,
        adev->cache_mute,
        adev->fm_mute,
        adev->force_fm_mute,
        adev->voice_volume);
    AUDIO_DUMP_WRITE_STR(fd,buffer);
    memset(buffer,0,sizeof(buffer));

    snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
        "fm:fm_volume:%d "
        "fm_open:%d "
        "fm_record:%d "
        "fm_type:%d "
        "i2sfm_flag:0x%x\n",
        adev->fm_volume,
        adev->fm_open,
        adev->fm_record,
        adev->fm_type,
        adev->i2sfm_flag);
    AUDIO_DUMP_WRITE_STR(fd,buffer);
    memset(buffer,0,sizeof(buffer));

    snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
        "other status:vbc_2arm:%d "
        "deepbuffer_enable:%d "
        "bluetooth_nrec:%d "
        "bluetooth_type:%d "
        "low_power:%d "
        "hfp_start:%d "
        "is_offload_running:%d "
        "audio_outputs_state:0x%x "
        "aud_proc_init:%d\n",
        adev->vbc_2arm,
        adev->deepbuffer_enable,
        adev->bluetooth_nrec,
        adev->bluetooth_type,
        adev->low_power,
        adev->hfp_start,
        adev->is_offload_running,
        adev->audio_outputs_state,
        adev->aud_proc_init);
    AUDIO_DUMP_WRITE_STR(fd,buffer);
    memset(buffer,0,sizeof(buffer));

    snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
        "other status:dump_flag:0x%x ",
        "nr enable flag:0x%x 0x%x ",
        "log_level:%d\n",
        adev->dump_flag,
        adev->nr_mask,
        adev->enable_other_rate_nr,
        log_level);
    AUDIO_DUMP_WRITE_STR(fd,buffer);
    memset(buffer,0,sizeof(buffer));

    if(NULL!=adev->pSmartAmpMgr){
        snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),
            "SmartAmp:isEnable:%d "
            "isDump:%d "
            "isBoostDevice:%d "
            "boostVoltage:%d\n",
            adev->pSmartAmpMgr->isEnable,
            adev->pSmartAmpMgr->isDump,
            adev->pSmartAmpMgr->isBoostDevice,
            adev->pSmartAmpMgr->boostVoltage);
        AUDIO_DUMP_WRITE_STR(fd,buffer);
        memset(buffer,0,sizeof(buffer));
    }

/* dump output */
{
    struct tiny_stream_out *out =NULL;
    struct listnode *item;
    struct listnode *item2;

    if(!list_empty(&adev->active_out_list)){
        list_for_each_safe(item, item2,&adev->active_out_list){
            out = node_to_item(item, struct tiny_stream_out, node);
            if(NULL!=out->stream.common.dump){
                out->stream.common.dump(&out->stream,fd);
            }
        }
    }
}

/* dump input */
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)adev->active_input;
    if(in!=NULL){
        if(NULL!=in->stream.common.dump){
            in->stream.common.dump(&in->stream,fd);
        }
    }
}
    /* dump tinymix */
    dump_tinymix_infor(fd,adev->dump_flag,adev->mixer);

    /* dump audio reg */
    dump_all_audio_reg(fd,adev->dump_flag);

    snprintf(buffer,(DUMP_BUFFER_MAX_SIZE-1),"\nadev_dump <===\n");
    AUDIO_DUMP_WRITE_STR(fd,buffer);
    ALOGI("adev_dump exit");
    return 0;
}

#ifdef NXP_SMART_PA
static void ext_codec_i2s_switch_free(struct tiny_audio_device *adev)
{
    audio_modem_t *cp = adev->cp;

    ALOGI("Free ext codec i2s switch info.");

    if (!cp)
        return;

    if (cp->i2s_switch_ext_codec_ap) {
        free(cp->i2s_switch_ext_codec_ap->strval);
        free(cp->i2s_switch_ext_codec_ap->ctl_name);
        free(cp->i2s_switch_ext_codec_ap);
    }
    if (cp->i2s_switch_ext_codec_cp) {
        free(cp->i2s_switch_ext_codec_cp->strval);
        free(cp->i2s_switch_ext_codec_cp->ctl_name);
        free(cp->i2s_switch_ext_codec_cp);
    }
}

static void ext_codec_free(struct tiny_audio_device *adev)
{
    struct ext_codec_info *ext_codec = adev->ext_codec;
    struct ext_codec_config *config;
    unsigned int num_configs;
    int i, j;
    unsigned int num_routes;
    struct route_setting *route;

    ALOGI("Free ext codec info.");

    if (!ext_codec)
        return;

    config = ext_codec->configs;
    if (!config)
        goto free_ext_codec;

    num_configs = ext_codec->num_configs;
    for (i = 0; i < num_configs; i++) {
        config += i;
        num_routes = config->num_routes;
        route = config->routes;
        for (j = 0; j < num_routes; j++) {
            route += j;
            free(route->ctl_name);
            free(route->strval);
        }
        free(config->routes);
    }
    free(ext_codec->configs);

free_ext_codec:
    free(ext_codec->name);
    free(ext_codec);
    adev->ext_codec = NULL;
}
#endif

static void adev_free_modem_mem(struct tiny_audio_device *adev)
{
    int i;
    for (i = 0; i < I2S_CTL_INF; i++) {
        if(adev->cp->i2s_switch_btcall[i] != NULL){
            if (adev->cp->i2s_switch_btcall[i]->ctl_name != NULL) {
                free(adev->cp->i2s_switch_btcall[i]->ctl_name);
                adev->cp->i2s_switch_btcall[i]->ctl_name = NULL;
            }
            if (adev->cp->i2s_switch_btcall[i]->strval != NULL) {
                free(adev->cp->i2s_switch_btcall[i]->strval);
                adev->cp->i2s_switch_btcall[i]->strval = NULL;
            }
            free(adev->cp->i2s_switch_btcall[i]);
            adev->cp->i2s_switch_btcall[i] = NULL;
        }
        if(adev->cp->i2s_switch_fm[i] != NULL) {
            if (adev->cp->i2s_switch_fm[i]->ctl_name != NULL) {
                free(adev->cp->i2s_switch_fm[i]->ctl_name);
                adev->cp->i2s_switch_fm[i]->ctl_name = NULL;
            }
            if (adev->cp->i2s_switch_fm[i]->strval != NULL) {
                free(adev->cp->i2s_switch_fm[i]->strval);
                adev->cp->i2s_switch_fm[i]->strval = NULL;
            }
            free(adev->cp->i2s_switch_fm[i]);
            adev->cp->i2s_switch_fm[i] = NULL;
        }
        if(adev->cp->i2s_switch_ap[i] != NULL) {
            if (adev->cp->i2s_switch_ap[i]->ctl_name != NULL) {
                free(adev->cp->i2s_switch_ap[i]->ctl_name);
                adev->cp->i2s_switch_ap[i]->ctl_name = NULL;
            }
            if (adev->cp->i2s_switch_ap[i]->strval != NULL) {
                free(adev->cp->i2s_switch_ap[i]->strval);
                adev->cp->i2s_switch_ap[i]->strval = NULL;
            }
            free(adev->cp->i2s_switch_ap[i]);
            adev->cp->i2s_switch_ap[i] = NULL;
        }
        if(adev->cp->i2s_switch_pubcp[i] != NULL) {
            if (adev->cp->i2s_switch_pubcp[i]->ctl_name != NULL) {
                free(adev->cp->i2s_switch_pubcp[i]->ctl_name);
                adev->cp->i2s_switch_pubcp[i]->ctl_name = NULL;
            }
            if (adev->cp->i2s_switch_pubcp[i]->strval != NULL) {
                free(adev->cp->i2s_switch_pubcp[i]->strval);
                adev->cp->i2s_switch_pubcp[i]->strval = NULL;
            }
            free(adev->cp->i2s_switch_pubcp[i]);
            adev->cp->i2s_switch_pubcp[i] = NULL;
        }
    }

}

static int adev_close(hw_device_t *device)
{
    unsigned int i, j;
    struct tiny_audio_device *adev = (struct tiny_audio_device *)device;
    struct listnode *item;
    struct listnode *item2;
    struct tiny_stream_out *out;

    if (!adev)
        return 0;

    ALOGI("adev_close begin adev = 0x%p", adev);
    pthread_mutex_lock(&adev_init_lock);
    if ((--audio_device_ref_count) == 0) {
        list_for_each_safe(item, item2,(&adev->active_out_list)){
            out = node_to_item(item, struct tiny_stream_out, node);
            adev_close_output_stream(adev, out);
        }

#ifdef NXP_SMART_PA
        if (adev->smart_pa_exist) {
            if(adev->callHandle) {
                NxpTfa_Close(adev->callHandle);
                adev->callHandle = NULL;
            }

            ext_codec_free(adev);
            ext_codec_i2s_switch_free(adev);
        }
#endif

#ifdef AUDIO_DEBUG
        ext_control_close(&adev->ext_contrl->entryMgr);
#endif
        audiopara_tuning_manager_close(&adev->tunningMgr);
        stream_routing_manager_close(adev);
        voice_command_manager_close(adev);
        cp_mixer_close(adev->record_tone_info);
        adev->record_tone_info = NULL;
        audio_bt_sco_thread_destory(adev);
        /* free audio PGA */
        audio_pga_free(adev->pga);
        vbc_ctrl_close();
        vbc_ctrl_voip_close(&adev->cp->voip_res);
        //Need to free mixer configs here.
        for (i=0; i < adev->num_dev_cfgs; i++) {
            for (j=0; j < adev->dev_cfgs[i].on_len; j++) {
                if (adev->dev_cfgs[i].on[j].ctl_name) {
                    free(adev->dev_cfgs[i].on[j].ctl_name);
                }
                //Is there a string of strval?
            }
            if (adev->dev_cfgs[i].on) {
                free(adev->dev_cfgs[i].on);
            }
            for (j=0; j < adev->dev_cfgs[i].off_len; j++) {
                if (adev->dev_cfgs[i].off[j].ctl_name) {
                    free(adev->dev_cfgs[i].off[j].ctl_name);
                }
            }
            if (adev->dev_cfgs[i].off) {
                free(adev->dev_cfgs[i].off);
            }
        }
        if (adev->dev_cfgs) {
            free(adev->dev_cfgs);
        }

    adev_free_modem_mem(adev);

#ifdef VB_CONTROL_PARAMETER_V2
            //Need to free mixer configs here.
        for (i=0; i < adev->num_dev_linein_cfgs; i++) {
            for (j=0; j < adev->dev_linein_cfgs->on_len; j++) {
                if (adev->dev_linein_cfgs[i].on[j].ctl_name) {
                    free(adev->dev_linein_cfgs[i].on[j].ctl_name);
                }
                //Is there a string of strval?
            }
            if (adev->dev_linein_cfgs[i].on) {
                free(adev->dev_linein_cfgs[i].on);
            }
            for (j=0; j < adev->dev_linein_cfgs->off_len; j++) {
                if (adev->dev_linein_cfgs[i].off[j].ctl_name) {
                    free(adev->dev_linein_cfgs[i].off[j].ctl_name);
                }
            }
            if (adev->dev_linein_cfgs[i].off) {
                free(adev->dev_linein_cfgs[i].off);
            }
        }
        if (adev->dev_linein_cfgs) {
            free(adev->dev_linein_cfgs);
        }
#endif

        if (adev->cp->vbc_ctrl_pipe_info) {
            free(adev->cp->vbc_ctrl_pipe_info);
        }

        if(NULL!=adev->cp->i2s_fm){
            free(adev->cp->i2s_fm);
        }
        if (adev->cp) {
            free(adev->cp);
        }
        if (adev->pga_gain_nv) {
            free(adev->pga_gain_nv);
        }
        //free ext_contrl
#ifdef AUDIO_DEBUG
        if(adev->ext_contrl->dump_info->out_bt_sco){
            free(adev->ext_contrl->dump_info->out_bt_sco);
        }
        if(adev->ext_contrl->dump_info->out_voip){
            free(adev->ext_contrl->dump_info->out_voip);
        }
        if(adev->ext_contrl->dump_info->out_vaudio){
            free(adev->ext_contrl->dump_info->out_vaudio);
        }
        if(adev->ext_contrl->dump_info->out_music){
            free(adev->ext_contrl->dump_info->out_music);
        }
        if(adev->ext_contrl->dump_info->in_read){
            free(adev->ext_contrl->dump_info->in_read);
        }
        if(adev->ext_contrl->dump_info->in_read_afterprocess){
            free(adev->ext_contrl->dump_info->in_read_afterprocess);
        }
        if(adev->ext_contrl->dump_info->in_read_vbc){
            free(adev->ext_contrl->dump_info->in_read_vbc);
        }
        if(adev->ext_contrl->dump_info->in_read_aftersrc){
            free(adev->ext_contrl->dump_info->in_read_aftersrc);
        }
        if(adev->ext_contrl->dump_info->in_read_afternr){
            free(adev->ext_contrl->dump_info->in_read_afternr);
        }
        if(adev->ext_contrl->dump_info){
            free(adev->ext_contrl->dump_info);
        }
        if (adev->ext_contrl) {
            free(adev->ext_contrl);
        }
#endif
        adev_free_audmode();
        mixer_close(adev->mixer);
        if (adev->i2s_mixer)  mixer_close(adev->i2s_mixer);
        if (adev->cp_mixer[CP_TG]) {
            mixer_close(adev->cp_mixer[CP_TG]);
        }
        if (adev->cp_mixer[CP_W]) {
            mixer_close(adev->cp_mixer[CP_W]);
        }
        if (adev->cp_mixer[CP_CSFB]) {
            mixer_close(adev->cp_mixer[CP_CSFB]);
        }
        // free mmi test handle
        if (adev->mmiHandle) {
            free(adev->mmiHandle);
            adev->mmiHandle = NULL;
        }
        if(adev->misc_ctl) {
            aud_misc_ctl_close(adev->misc_ctl);
            adev->misc_ctl = NULL;
        }
        MP3_ARM_DEC_release();
        vb_ctl_modem_monitor_close(adev);

        memset(adev, 0, sizeof(struct tiny_audio_device));
        if (device) {
            free(device);
        }
    }
    ALOGI("adev_close exit");
    pthread_mutex_unlock(&adev_init_lock);
    return 0;
}

static uint32_t adev_get_supported_devices(const struct audio_hw_device *dev)
{
    return (/* OUT */
            AUDIO_DEVICE_OUT_EARPIECE |
            AUDIO_DEVICE_OUT_SPEAKER |
            AUDIO_DEVICE_OUT_WIRED_HEADSET |
            AUDIO_DEVICE_OUT_WIRED_HEADPHONE |
            AUDIO_DEVICE_OUT_AUX_DIGITAL |
            AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET |
            AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET |
            AUDIO_DEVICE_OUT_ALL_SCO |
            AUDIO_DEVICE_OUT_FM |
            AUDIO_DEVICE_OUT_DEFAULT |
            /* IN */
            AUDIO_DEVICE_IN_COMMUNICATION |
            AUDIO_DEVICE_IN_AMBIENT |
            AUDIO_DEVICE_IN_BUILTIN_MIC |
            AUDIO_DEVICE_IN_WIRED_HEADSET |
            AUDIO_DEVICE_IN_AUX_DIGITAL |
            AUDIO_DEVICE_IN_BACK_MIC |
            AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET |
            AUDIO_DEVICE_IN_ALL_SCO|
            AUDIO_DEVICE_IN_VOICE_CALL |
            //AUDIO_DEVICE_IN_LINE_IN |
            AUDIO_DEVICE_IN_DEFAULT);
}

static bool not_support_ad23(void)
{
	#ifdef VBC_NOT_SUPPORT_AD23
	return true;
	#else
	return false;
	#endif
}

static void vbc_ad01iis_select(struct tiny_audio_device *adev,
	bool is_to_dfm)
{
	if (false == not_support_ad23()) {
		ALOGI("%s vbc support ad23 do nothing", __func__);
		return;
	}
	if (adev->private_ctl.vbc_ad01iis_to_dfm == NULL) {
		ALOGI("%s vbc_ad01iis_to_dfm is null", __func__);
		return;
	}

	if (is_to_dfm == true)
		mixer_ctl_set_value(adev->private_ctl.vbc_ad01iis_to_dfm, 0, 1);
	else
		mixer_ctl_set_value(adev->private_ctl.vbc_ad01iis_to_dfm, 0, 0);

	ALOGI("%s is_to_dfm=%s", __func__, is_to_dfm?"true":"false");
}

/* parse the private field of xml config file. */
static void adev_config_parse_private(struct config_parse_state *s, const XML_Char *name)
{
    if (s && name) {
        if (strcmp(s->private_name, PRIVATE_VBC_CONTROL) == 0) {
            s->adev->private_ctl.vbc_switch =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_switch);
        } else if (strcmp(s->private_name, PRIVATE_VBC_EQ_SWITCH) == 0) {
            s->adev->private_ctl.vbc_eq_switch =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_eq_switch);
        } else if (strcmp(s->private_name, PRIVATE_VBC_EQ_UPDATE) == 0) {
            s->adev->private_ctl.vbc_eq_update =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_eq_update);
        } else if (strcmp(s->private_name, PRIVATE_VBC_EQ_PROFILE) == 0) {
            s->adev->private_ctl.vbc_eq_profile_select =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_eq_profile_select);
        } else if (strcmp(s->private_name, PRIVATE_MIC_BIAS) == 0) {
            s->adev->private_ctl.mic_bias_switch =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.mic_bias_switch);
        } else if (strcmp(s->private_name, PRIVATE_FM_DA0_MUX) == 0) {
            s->adev->private_ctl.fm_da0_mux =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.fm_da0_mux);
        } else if (strcmp(s->private_name, PRIVATE_FM_DA1_MUX) == 0) {
            s->adev->private_ctl.fm_da1_mux =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.fm_da1_mux);
        } else if (strcmp(s->private_name, PRIVATE_INTERNAL_PA) == 0) {
            s->adev->private_ctl.internal_pa =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.internal_pa);
        } else if (strcmp(s->private_name, PRIVATE_INTERNAL_HP_PA) == 0) {
            s->adev->private_ctl.internal_hp_pa =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.internal_hp_pa);
        } else if (strcmp(s->private_name, PRIVATE_INTERNAL_HP_PA_DELAY) == 0){
            s->adev->private_ctl.internal_hp_pa_delay =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.internal_hp_pa_delay);
        } else if (strcmp(s->private_name, PRIVATE_VBC_DA_EQ_SWITCH) == 0) {
            s->adev->private_ctl.vbc_da_eq_switch =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_da_eq_switch);
        }
        else if (strcmp(s->private_name, PRIVATE_VBC_AD01_EQ_SWITCH) == 0) {
            s->adev->private_ctl.vbc_ad01_eq_switch =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_ad01_eq_switch);
        }
        else if (strcmp(s->private_name, PRIVATE_VBC_AD23_EQ_SWITCH) == 0) {
            s->adev->private_ctl.vbc_ad23_eq_switch =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_ad23_eq_switch);
        }
        else if (strcmp(s->private_name, PRIVATE_VBC_DA_EQ_PROFILE) == 0) {
            s->adev->private_ctl.vbc_da_eq_profile_select =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_da_eq_switch);
        }
        else if (strcmp(s->private_name, PRIVATE_VBC_AD01_EQ_PROFILE) == 0) {
            s->adev->private_ctl.vbc_ad01_eq_profile_select =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_ad01_eq_switch);
        }
        else if (strcmp(s->private_name, PRIVATE_VBC_AD23_EQ_PROFILE) == 0) {
            s->adev->private_ctl.vbc_ad23_eq_profile_select =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_ad23_eq_switch);
        }
        else if (strcmp(s->private_name, PRIVATE_SPEAKER_MUTE) == 0) {
            s->adev->private_ctl.speaker_mute =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.speaker_mute);
        }
        else if (strcmp(s->private_name, PRIVATE_SPEAKER2_MUTE) == 0) {
            s->adev->private_ctl.speaker2_mute =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.speaker2_mute);
        }
        else if (strcmp(s->private_name, PRIVATE_EARPIECE_MUTE) == 0) {
            s->adev->private_ctl.earpiece_mute =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.earpiece_mute);
        }
        else if (strcmp(s->private_name, PRIVATE_HEADPHONE_MUTE) == 0) {
            s->adev->private_ctl.headphone_mute =
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.headphone_mute);
        }
        else if (strcmp(s->private_name, PRIVATE_AUD_LOOP_VBC) == 0) {
            s->adev->private_ctl.fm_loop_vbc=
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.fm_loop_vbc);
        }
        else if (strcmp(s->private_name, PRIVATE_AUD1_LOOP_VBC) == 0) {
            s->adev->private_ctl.ad1_fm_loop_vbc=
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.ad1_fm_loop_vbc);
        }
        else if (strcmp(s->private_name, PRIVATE_AUD_CODEC_INFO) == 0) {
            s->adev->private_ctl.aud_codec_info=
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.aud_codec_info);
        }
        else if (strcmp(s->private_name, PRIVATE_LINEIN_MUTE) == 0) {
            s->adev->private_ctl.linein_mute=
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.linein_mute);
        }
        else if (strcmp(s->private_name, PRIVATE_VBC_AD01IIS_TO_DFM) == 0) {
            s->adev->private_ctl.vbc_ad01iis_to_dfm=
                mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_ad01iis_to_dfm);
        }
    }
}

static void adev_config_start(void *data, const XML_Char *elem,
        const XML_Char **attr)
{
    struct config_parse_state *s = data;
    struct tiny_dev_cfg *dev_cfg;
    const XML_Char *name = NULL;
    const XML_Char *val = NULL;
    unsigned int i, j;
    char value[PROPERTY_VALUE_MAX];
    unsigned int dev_num = 0;

    if (s->adev->fm_type != AUD_FM_TYPE_LINEIN_3GAINS) {
        dev_names = dev_names_digitalfm;
        dev_num = sizeof(dev_names_digitalfm) / sizeof(dev_names_digitalfm[0]);
    } else {
        dev_names = dev_names_linein;
        dev_num = sizeof(dev_names_linein) / sizeof(dev_names_linein[0]);
    }

    /* default if not set it 0 */
    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "name") == 0)
            name = attr[i + 1];

        if (strcmp(attr[i], "val") == 0)
            val = attr[i + 1];
    }

    if (!name) {
        ALOGE("unnamed entry %s, %d", elem, i);
        return;
    }

    if (strcmp(elem, "device") == 0) {
        for (i = 0; i < dev_num; i++) {
            if (strcmp((dev_names+i)->name, name) == 0) {
                ALOGV("Allocating device %s\n", name);
                dev_cfg = realloc(s->adev->dev_cfgs,
                        (s->adev->num_dev_cfgs + 1)
                        * sizeof(*dev_cfg));
                if (!dev_cfg) {
                    ALOGE("Unable to allocate dev_cfg\n");
                    return;
                }

                s->dev = &dev_cfg[s->adev->num_dev_cfgs];
                memset(s->dev, 0, sizeof(*s->dev));
                s->dev->mask = (dev_names+i)->mask;

                s->adev->dev_cfgs = dev_cfg;
                s->adev->num_dev_cfgs++;
            }
        }

    } else if (strcmp(elem, "path") == 0) {
        if (s->path_len)
            ALOGW("Nested paths\n");

        /* If this a path for a device it must have a role */
        if (s->dev) {
            /* Need to refactor a bit... */
            if (strcmp(name, "on") == 0) {
                s->on = true;
            } else if (strcmp(name, "off") == 0) {
                s->on = false;
            } else {
                ALOGW("Unknown path name %s\n", name);
            }
        }

    } else if (strcmp(elem, "ctl") == 0) {
        struct route_setting *r;

        if (!name) {
            ALOGE("Unnamed control\n");
            return;
        }

        if (!val) {
            ALOGE("No value specified for %s\n", name);
            return;
        }

        ALOGV("Parsing control %s => %s\n", name, val);

        r = realloc(s->path, sizeof(*r) * (s->path_len + 1));
        if (!r) {
            ALOGE("Out of memory handling %s => %s\n", name, val);
            return;
        }

        r[s->path_len].ctl_name = strdup(name);
        r[s->path_len].strval = NULL;

        /* This can be fooled but it'll do */
        r[s->path_len].intval = atoi(val);
        if (!r[s->path_len].intval && strcmp(val, "0") != 0)
            r[s->path_len].strval = strdup(val);

        s->path = r;
        s->path_len++;
        ALOGV("s->path_len=%d", s->path_len);
    }
    else if (strcmp(elem, "private") == 0) {
        memset(s->private_name, 0, PRIVATE_NAME_LEN);
        memcpy(s->private_name, name, strlen(name));
    }
    else if (strcmp(elem, "func") == 0) {
        adev_config_parse_private(s, name);
    }
}

static void adev_config_end(void *data, const XML_Char *name)
{
    struct config_parse_state *s = data;
    unsigned int i;

    if (strcmp(name, "path") == 0) {
        if (!s->path_len)
            ALOGW("Empty path\n");

        if (!s->dev) {
            ALOGV("Applying %d element default route\n", s->path_len);

            set_route_by_array(s->adev->mixer, s->path, s->path_len);

            for (i = 0; i < s->path_len; i++) {
                free(s->path[i].ctl_name);
                s->path[i].ctl_name = NULL;
                free(s->path[i].strval);
                s->path[i].strval = NULL;
            }

            free(s->path);

            /* Refactor! */
        } else if (s->on) {
            ALOGV("%d element on sequence\n", s->path_len);
            s->dev->on = s->path;
            s->dev->on_len = s->path_len;

        } else {
            ALOGV("%d element off sequence\n", s->path_len);

            /* Apply it, we'll reenable anything that's wanted later */
            set_route_by_array(s->adev->mixer, s->path, s->path_len);

            s->dev->off = s->path;
            s->dev->off_len = s->path_len;
        }

        s->path_len = 0;
        s->path = NULL;

    } else if (strcmp(name, "device") == 0) {
        s->dev = NULL;
    }
}

static int adev_config_parse(struct tiny_audio_device *adev)
{
    struct config_parse_state s;
    FILE *f;
    XML_Parser p;
    char property[PROPERTY_VALUE_MAX];
    char file[80];
    int ret = 0;
    bool eof = false;
    int len;

    //property_get("ro.product.device", property, "tiny_hw");
#ifdef FLAG_VENDOR_ETC
    snprintf(file, sizeof(file), "/vendor/etc/%s", "tiny_hw.xml");
#else
    snprintf(file, sizeof(file), "/system/etc/%s", "tiny_hw.xml");
#endif

    ALOGV("Reading configuration from %s\n", file);
    f = fopen(file, "r");
    if (!f) {
        ALOGE("Failed to open %s\n", file);
        return -ENODEV;
    }

    p = XML_ParserCreate(NULL);
    if (!p) {
        ALOGE("Failed to create XML parser\n");
        ret = -ENOMEM;
        goto out;
    }

    memset(&s, 0, sizeof(s));
    s.adev = adev;
    XML_SetUserData(p, &s);

    XML_SetElementHandler(p, adev_config_start, adev_config_end);

    while (!eof) {
        len = fread(file, 1, sizeof(file), f);
        if (ferror(f)) {
            ALOGE("I/O error reading config\n");
            ret = -EIO;
            goto out_parser;
        }
        eof = feof(f);

        if (XML_Parse(p, file, len, eof) == XML_STATUS_ERROR) {
            ALOGE("Parse error at line %u:\n%s\n",
                    (unsigned int)XML_GetCurrentLineNumber(p),
                    XML_ErrorString(XML_GetErrorCode(p)));
            ret = -EINVAL;
            goto out_parser;
        }
    }

out_parser:
    XML_ParserFree(p);
out:
    fclose(f);

    return ret;
}

static void aud_vb_effect_start(struct tiny_audio_device *adev)
{
    if (adev)
    {
        if(adev->private_ctl.vbc_eq_switch)
        {
            mixer_ctl_set_value(adev->private_ctl.vbc_eq_switch, 0, 1);
        }
        if(adev->private_ctl.vbc_da_eq_switch)
        {
            mixer_ctl_set_value(adev->private_ctl.vbc_da_eq_switch, 0, 1);
        }
        if(adev->private_ctl.vbc_ad01_eq_switch)
        {
            //mixer_ctl_set_value(adev->private_ctl.vbc_ad01_eq_switch, 0, 1);
        }
        if(adev->private_ctl.vbc_ad23_eq_switch)
        {
            //mixer_ctl_set_value(adev->private_ctl.vbc_ad23_eq_switch, 0, 1);
        }
    }
}

static void aud_vb_effect_stop(struct tiny_audio_device *adev)
{
    if (adev)
    {
        if(adev->private_ctl.vbc_eq_switch)
        {
            mixer_ctl_set_value(adev->private_ctl.vbc_eq_switch, 0, 0);
        }
        if(adev->private_ctl.vbc_da_eq_switch)
        {
            mixer_ctl_set_value(adev->private_ctl.vbc_da_eq_switch, 0, 0);
        }
        if(adev->private_ctl.vbc_ad01_eq_switch)
        {
            mixer_ctl_set_value(adev->private_ctl.vbc_ad01_eq_switch, 0, 0);
        }
        if(adev->private_ctl.vbc_ad23_eq_switch)
        {
            mixer_ctl_set_value(adev->private_ctl.vbc_ad23_eq_switch, 0, 0);
        }
    }
}


int load_fm_volume(struct tiny_audio_device *adev)
{
    int i;
    AUDIO_TOTAL_T * aud_params_ptr = NULL;
    char * dev_name = NULL;

    aud_params_ptr = &adev->audio_para[0]; //headset
    for (i = 1; i < FM_VOLUME_MAX; i++) {
        fm_headset_volume_tbl[i] = aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[15].arm_volume[i];
        ALOGD("fm headset volume index %d vol = %d", i, fm_headset_volume_tbl[i]);
    }

    aud_params_ptr = &adev->audio_para[3]; //handsfree
    for (i = 1; i < FM_VOLUME_MAX; i++) {
        fm_speaker_volume_tbl[i] = aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[15].arm_volume[i];
        ALOGD("fm speaker volume index %d vol = %d", i, fm_speaker_volume_tbl[i]);
    }
    return 0;
}

int apply_fm_volume(struct tiny_audio_device *adev) {
    uint32_t gain = 0;
    load_fm_volume(adev);
    if (adev->fm_open) {
        int val = adev->fm_volume;
        if (adev->out_devices & AUDIO_DEVICE_OUT_SPEAKER) {
            gain |=fm_speaker_volume_tbl[val];
            gain |=fm_speaker_volume_tbl[val]<<16;
        } else {
            gain |=fm_headset_volume_tbl[val];
            gain |=fm_headset_volume_tbl[val]<<16;
        }
        SetAudio_gain_fmradio(adev,gain);
    }
    return 0;
}

/*
 * Read audproc params from nv and config.
 * return value: TRUE:success, FALSE:failed
 */
static int init_rec_process(int rec_mode, int sample_rate)
{
    int ret0 = 0; //failed
    int ret1 = 0;
    off_t offset = 0;
    AUDIO_TOTAL_T *aud_params_ptr = NULL;
    DP_CONTROL_PARAM_T *ctrl_param_ptr = 0;
    RECORDEQ_CONTROL_PARAM_T *eq_param_ptr = 0;
    unsigned int extendArraySize = 0;

    ALOGW("rec_mode(%d), sample_rate(%d)", rec_mode, sample_rate);

    aud_params_ptr = audio_para_ptr;//(AUDIO_TOTAL_T *)mmap(0, 4*sizeof(AUDIO_TOTAL_T),PROT_READ,MAP_SHARED,audio_fd,0);
    if ( NULL == aud_params_ptr ) {
        ALOGE("mmap failed %s",strerror(errno));
        return 0;
    }
    ctrl_param_ptr = (DP_CONTROL_PARAM_T *)((aud_params_ptr+rec_mode)->audio_enha_eq.externdArray);

    ret0 = AUDPROC_initDp(ctrl_param_ptr, sample_rate);

    //get total items of extend array.
    extendArraySize = sizeof((aud_params_ptr+rec_mode)->audio_enha_eq.externdArray);
    ALOGW("extendArraySize=%d, eq_size=%d, dp_size=%d",
            extendArraySize, sizeof(RECORDEQ_CONTROL_PARAM_T), sizeof(DP_CONTROL_PARAM_T));
    if ((sizeof(RECORDEQ_CONTROL_PARAM_T) + sizeof(DP_CONTROL_PARAM_T)) <= extendArraySize)
    {
        eq_param_ptr =(RECORDEQ_CONTROL_PARAM_T *)&((aud_params_ptr+rec_mode)->audio_enha_eq.externdArray[19]);
        ret1 = AUDPROC_initRecordEq(eq_param_ptr, sample_rate);
    }else{
        ALOGE("Parameters error: No EQ params to init.");
    }

    return (ret0 || ret1);
}

static int aud_rec_do_process(void * buffer,size_t bytes,int channels,void * tmp_buffer,size_t tmp_buffer_bytes)
{
    int16_t *temp_buf = NULL;
    size_t read_bytes = bytes;
    unsigned int dest_count = 0;
    temp_buf = (int16_t *)tmp_buffer;
    if (temp_buf && (tmp_buffer_bytes >= 2)) {
        do {
            if(tmp_buffer_bytes <=  bytes) {
                read_bytes = tmp_buffer_bytes;
            }
            else {
                read_bytes = bytes;
            }
            bytes -= read_bytes;
        if(1==channels){
            AUDPROC_ProcessDp((int16 *) buffer, (int16 *) buffer, read_bytes >> 1, temp_buf, temp_buf, &dest_count);
            memcpy(buffer, temp_buf, read_bytes);
         }else if(2==channels){
            //AUDPROC_ProcessDp_2((int16 *) buffer, read_bytes >> 2, 2, temp_buf, &dest_count);
            //memcpy(buffer, temp_buf, read_bytes);
            AUDPROC_ProcessDp_2((int16 *) buffer, read_bytes >> 1, 2, buffer, &dest_count);
         }else{
            ALOGW("aud_rec_do_process do not supprot channels=%d",channels);
         }
         buffer = (uint8_t *) buffer + read_bytes;
        }while(bytes);
    } else {
        ALOGE("temp_buf malloc failed.(len=%d)", (int) read_bytes);
        return -1;
    }
    return 0;
}

static void *stream_routing_thread_entry(void * param)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)param;
    pthread_attr_t attr;
    struct sched_param m_param;
    int newprio=39;

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_getschedparam(&attr, &m_param);
    m_param.sched_priority=newprio;
    pthread_attr_setschedparam(&attr, &m_param);

    while(!adev->routing_mgr.is_exit) {
        ALOGI("stream_routing_thread looping now...");
        sem_wait(&adev->routing_mgr.device_switch_sem);
        if (!adev->routing_mgr.is_exit) {
            do_select_devices(adev);
        }
        ALOGI("stream_routing_thread looping done.");
    }
    ALOGW("stream_routing_thread_entry exit!!!");
    return 0;
}

static int stream_routing_manager_create(struct tiny_audio_device *adev)
{
    int ret;

    adev->routing_mgr.is_exit = false;
    /* init semaphore to signal thread */
    ret = sem_init(&adev->routing_mgr.device_switch_sem, 0, 0);
    if (ret) {
        ALOGE("sem_init falied, code is %s", strerror(errno));
        return ret;
    }
    /* create a thread to manager the device routing switch.*/
    ret = pthread_create(&adev->routing_mgr.routing_switch_thread, NULL,
            stream_routing_thread_entry, (void *)adev);
    if (ret) {
        ALOGE("pthread_create falied, code is %s", strerror(errno));
        return ret;
    }

    return ret;
}

static void stream_routing_manager_close(struct tiny_audio_device *adev)
{
    adev->routing_mgr.is_exit = true;
    sem_post(&adev->routing_mgr.device_switch_sem);
    pthread_join(adev->routing_mgr.routing_switch_thread, NULL);
    /* release associated thread resource.*/
    sem_destroy(&adev->routing_mgr.device_switch_sem);
}


static void *voice_command_thread_entry(void * param)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)param;
    pthread_attr_t attr;
    struct sched_param m_param;
    int newprio=39;

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_getschedparam(&attr, &m_param);
    m_param.sched_priority=newprio;
    pthread_attr_setschedparam(&attr, &m_param);

    while(!adev->voice_command_mgr.is_exit) {
        ALOGI(" %s looping now...",__func__);
        sem_wait(&adev->voice_command_mgr.device_switch_sem);
        if (!adev->voice_command_mgr.is_exit) {
            do_voice_command(adev);
        }
        ALOGI(" %s looping done.",__func__);
    }
    ALOGW("%s exit!!!",__func__);
    return 0;
}

static int voice_command_manager_create(struct tiny_audio_device *adev)
{
    int ret;

    adev->voice_command_mgr.is_exit = false;
    /* init semaphore to signal thread */
    ret = sem_init(&adev->voice_command_mgr.device_switch_sem, 0, 0);
    if (ret) {
        ALOGE("sem_init falied, code is %s", strerror(errno));
        return ret;
    }
    adev->at_cmd_vectors = malloc(sizeof(T_AT_CMD));
    if (adev->at_cmd_vectors == NULL) {
        ALOGE("Unable to allocate at_cmd_vectors ");
        return -1;
    }
    memset(adev->at_cmd_vectors,0x00,sizeof(T_AT_CMD));
    /* create a thread to manager the device routing switch.*/
    ret = pthread_create(&adev->voice_command_mgr.routing_switch_thread, NULL,
            voice_command_thread_entry, (void *)adev);
    if (ret) {
        ALOGE("pthread_create falied, code is %s", strerror(errno));
        return ret;
    }

    return ret;
}

static void voice_command_manager_close(struct tiny_audio_device *adev)
{
    adev->voice_command_mgr.is_exit = true;
    sem_post(&adev->voice_command_mgr.device_switch_sem);
    pthread_join(adev->voice_command_mgr.routing_switch_thread, NULL);
    if(adev->at_cmd_vectors != NULL)
    {
        free(adev->at_cmd_vectors);
        adev->at_cmd_vectors = NULL;
    }
    /* release associated thread resource.*/
    sem_destroy(&adev->voice_command_mgr.device_switch_sem);
}

static  vbc_ctrl_pipe_para_t *adev_modem_create(audio_modem_t  *modem, const char *num)
{
    vbc_ctrl_pipe_para_t *a;
    if (!atoi((char *)num)) {
        ALOGE("Unnormal modem num!");
        return NULL;
    }

    modem->num = atoi((char *)num);
    /* check if we need to allocate  space for modem profile */
    if(!modem->vbc_ctrl_pipe_info)
    {
        modem->vbc_ctrl_pipe_info = malloc(modem->num *
                sizeof(vbc_ctrl_pipe_para_t));

        if (modem->vbc_ctrl_pipe_info == NULL) {
            ALOGE("Unable to allocate modem profiles");
            return NULL;
        }
        else
        {
            /* initialise the new profile */
            memset((void*)modem->vbc_ctrl_pipe_info,0x00,modem->num *
                    sizeof(vbc_ctrl_pipe_para_t));
        }
    }

	ALOGD("peter: modem num is %d",modem->num);
    /* return the profile just added */
    return modem->vbc_ctrl_pipe_info;
}


static  char *set_nbio_pipename(const char *name)
{
    char *pipename = NULL;
    int len = strlen(name)+1;
    pipename = malloc(len);
    if (pipename == NULL) {
        ALOGE("Unable to allocate mem for set_nbio_pipename");
        return NULL;
    }
    else
    {
        /* initialise the new profile */
        memset(pipename,0x00,len);
    }

    strncpy(pipename,name,len);
    ALOGD("nbio_pipe name %s",pipename);
    return pipename;
}

static  i2s_ctl_t *adev_I2S_create(i2s_bt_t  *i2s_btcall_info, const char *num)
{
//    vbc_ctrl_pipe_para_t *a;
    if (!atoi((char *)num)) {
        ALOGE("Unnormal modem num!");
        return NULL;
    }

    i2s_btcall_info->num = atoi((char *)num);
    /* check if we need to allocate  space for modem profile */
    if(!i2s_btcall_info->i2s_ctl_info)
    {
        i2s_btcall_info->i2s_ctl_info = malloc(i2s_btcall_info->num *
                sizeof(i2s_ctl_t));

        if (i2s_btcall_info->i2s_ctl_info == NULL) {
            ALOGE("Unable to allocate modem profiles");
            return NULL;
        }
        else
        {
            /* initialise the new profile */
            memset((void*)i2s_btcall_info->i2s_ctl_info,0x00,i2s_btcall_info->num *
                    sizeof(i2s_ctl_t));
        }
    }

	ALOGD("peter: modem num is %d",i2s_btcall_info->num);
    /* return the profile just added */
    return i2s_btcall_info->i2s_ctl_info;
}
static  i2s_ctl_t *adev_I2S_create_speaker(int in_num)
{
    i2s_ctl_t *i2s_speaker_info = NULL;
    if (in_num == 0) {
        ALOGE("Unnormal modem num!");
        return NULL;
    }

    /* check if we need to allocate  space for modem profile */
    if(! i2s_speaker_info)
    {
        i2s_speaker_info = (i2s_ctl_t*)calloc(in_num, sizeof(i2s_ctl_t));
        if (i2s_speaker_info == NULL) {
            ALOGE("Unable to allocate modem profiles");
            return NULL;
        }
        else
        {
            memset((void*)i2s_speaker_info,0x0,in_num * sizeof(i2s_ctl_t));
        }
    }
    return i2s_speaker_info;
}

#ifdef NXP_SMART_PA
static void ext_codec_parse_abort(void *data)
{
    struct modem_config_parse_state *state = data;

    ALOGE("Abort the whole ext codec parsing!");

    ext_codec_free(state->adev);
    state->parsing_ext_codec = false;
}

static void ext_codec_start_tag(void *data, const XML_Char *tag_name,
        const XML_Char **attr)
{
    struct modem_config_parse_state *state = data;
    struct ext_codec_info *ext_codec = state->adev->ext_codec;

    if (!ext_codec)
        return;

    ALOGI("%s, tag name: %s", __func__, tag_name);
    if (strcmp(tag_name, "config") == 0) {
        struct ext_codec_config *config = ext_codec->configs;
        unsigned int num_configs = ext_codec->num_configs;

        config = realloc(config, sizeof(struct ext_codec_config) * (num_configs + 1));
        if (!config) {
            ALOGE("realloc ext_codec_config err!");
            goto err_abort;
        } else {
            ext_codec->configs = config;
            config += num_configs;
            memset(config, 0, sizeof(struct ext_codec_config));
            config->scene = atoi(attr[1]);
            num_configs++;
            ext_codec->num_configs = num_configs;
        }
    } else if (strcmp(tag_name, "ctl") == 0) {
        unsigned int num_configs = ext_codec->num_configs;
        struct ext_codec_config *config = ext_codec->configs + num_configs - 1;
        unsigned int num_routes;
        struct route_setting *route;
        struct route_setting tmp_route = {0};

        if (!config) {
            ALOGE("Ext codec config is NULL when parsing ctl!");
            goto err_abort;
        }

        num_routes = config->num_routes;
        route = config->routes;

        ALOGI("%s, %s, %s, %s, %s", __func__, attr[0], attr[1], attr[2], attr[3]);
        if (!strcmp(attr[0], "name") && !strcmp(attr[2], "val")) {
            const char *val = attr[3];

            tmp_route.ctl_name = strdup(attr[1]);
            tmp_route.intval = atoi(val);
            if (!tmp_route.intval && strcmp(val, "0") != 0)
                tmp_route.strval = strdup(val);

            route = realloc(route, sizeof(struct route_setting) * (num_routes + 1));
            if (!route) {
                ALOGE("realloc ctl of ext_codec_config err");
                goto err_abort;
            } else {
                config->routes = route;
                route += num_routes;
                num_routes++;
                config->num_routes = num_routes;
                memcpy(route, &tmp_route, sizeof(tmp_route));
            }
        } else {
            ALOGE("Invalid ctl attr of ext codec! attr[0]: %s, attr[2]: %s.", attr[0], attr[2]);
            goto err_abort;
        }
    } else if (strcmp(tag_name, "ap_i2s_shared") == 0) {
        ALOGI("%s, %s, %s", __func__, attr[0], attr[1]);
        ext_codec->i2s_shared = !!atoi(attr[1]);
    }

    return;

err_abort:
    ext_codec_parse_abort(data);
}
#endif

static void adev_modem_start_tag(void *data, const XML_Char *tag_name,
        const XML_Char **attr)
{
    struct modem_config_parse_state *state = data;
    audio_modem_t *modem = state->modem_info;
    unsigned int i;
    int value;
    struct mixer_ctl *ctl;
    vbc_ctrl_pipe_para_t item;
    vbc_ctrl_pipe_para_t *vbc_ctrl_pipe_info = NULL;

    i2s_bt_t  *i2s_btcall_info = state->i2s_btcall_info;
   ctrl_node *p_ctlr_node = NULL;

    /* Look at tags */
    if (strcmp(tag_name, "audio") == 0) {
        if (strcmp(attr[0], "device") == 0) {
            ALOGI("The device name is %s", attr[1]);
        } else {
            ALOGE("Unnamed audio!");
        }
    }
    else if (strcmp(tag_name, "modem") == 0) {
        /* Obtain the modem num */
        if (strcmp(attr[0], "num") == 0) {
            ALOGV("The modem num is '%s'", attr[1]);
            state->vbc_ctrl_pipe_info = adev_modem_create(modem, attr[1]);
        } else {
            ALOGE("no modem num!");
        }
    }
    else if (strcmp(tag_name, "cp") == 0) {
        if (state->vbc_ctrl_pipe_info) {
            /* Obtain the modem name  \pipe\vbc   filed */
            if (strcmp(attr[0], "name") != 0) {
                ALOGE("Unnamed modem!");
                goto attr_err;
            }
            if (strcmp(attr[2], "pipe") != 0) {
                ALOGE("'%s' No pipe filed!", attr[0]);
                goto attr_err;
            }
            if (strcmp(attr[4], "vbchannel") != 0) {
                ALOGE("'%s' No vbc filed!", attr[0]);
                goto attr_err;
            }
		if (strcmp(attr[6], "cpu_index") != 0) {
                ALOGE("'%s' No cpu index filed!", attr[0]);
                goto attr_err;
            }


            ALOGI("cp name is '%s', pipe is '%s',vbc is '%s',cpu index is '%s'", attr[1], attr[3],attr[5],attr[7]);
            if(strcmp(attr[1], "w") == 0)
            {
                state->vbc_ctrl_pipe_info->cp_type = CP_W;
            }
            else if(strcmp(attr[1], "t") == 0)
            {
                state->vbc_ctrl_pipe_info->cp_type = CP_TG;
            }
            else if(strcmp(attr[1], "csfb") == 0)
            {
                state->vbc_ctrl_pipe_info->cp_type = CP_CSFB;
            }
            memcpy((void*)state->vbc_ctrl_pipe_info->s_vbc_ctrl_pipe_name,(void*)attr[3],strlen((char *)attr[3]));
            state->vbc_ctrl_pipe_info->channel_id = atoi((char *)attr[5]);
            state->vbc_ctrl_pipe_info->cpu_index = atoi((char *)attr[7]);
            state->vbc_ctrl_pipe_info++;
        } else {
            ALOGE("error profile!");
        }
    }
     else if (strcmp(tag_name, "cp_nbio_dump") == 0)
	{

           ALOGE("The cp_nbio_dump is %s = '%s'", attr[0] ,attr[1]);
        if (strcmp(attr[0], "spipe") == 0) {
            ALOGE("nbio pipe name is '%s'", attr[1]);
            state->cp_nbio_pipe =   set_nbio_pipename(attr[1]);
        } else {
            ALOGE("no nbio_pipe!");
        }
    }
   else if (strcmp(tag_name, "i2s_for_btcall") == 0)
	{
        /* Obtain the modem num */
           ALOGV("The i2s_for_btcall num is %s = '%s'", attr[0] ,attr[1]);
        if (strcmp(attr[0], "cpu_num") == 0) {
            ALOGV("The i2s_for_btcall num is '%s'", attr[1]);
            state->i2s_ctl_info =   adev_I2S_create(i2s_btcall_info, attr[1]);
            state->i2s_ctl_info->max_num = atoi((char *)attr[1]);//max_num
        } else {
            ALOGE("no i2s_for_btcall  num!");
        }
    }
   else if (strcmp(tag_name, "fm_type") == 0) {
        if (strcmp(attr[0], "type") == 0) {
            if (strcmp(attr[1], "digital") == 0) {
                state->fm_type = AUD_FM_TYPE_DIGITAL;
            } else if (strcmp(attr[1], "linein") == 0) {
                state->fm_type = AUD_FM_TYPE_LINEIN;
            } else if (strcmp(attr[1], "linein-vbc") == 0) {
                state->fm_type = AUD_FM_TYPE_LINEIN_VBC;
            } else if (strcmp(attr[1], "linein 3gains") == 0) {
                state->fm_type = AUD_FM_TYPE_LINEIN_3GAINS;
            } else {
                ALOGE("fm type is unkown");
                state->fm_type = -1;
            }
            ALOGI("fm type is %s    fm_type=%d", attr[1], state->fm_type);
        }
    }
    else if (strncmp(tag_name, "smart_amp_mgr", strlen("smart_amp_mgr")) == 0) {
        if (strcmp(attr[0], "enable") == 0) {
              ALOGV("Smart Amp Function Is '%s'", attr[1]);
              state->pSmartAmpMgr->isEnable = atoi((char *)attr[1]);
        } else {
            ALOGE("Smart Amp XML Define Is Error '%s'", attr[0]);
        }
        if (strcmp(attr[2], "dump") == 0) {
              ALOGV("Smart Amp dump Is '%s'", attr[3]);
              state->pSmartAmpMgr->isDump = atoi((char *)attr[3]);
        } else {
            ALOGE("Smart Amp XML Define Is Error '%s'", attr[2]);
        }
        if (strcmp(attr[4], "boost_device") == 0) {
              ALOGV("Smart Amp boost_device Is '%s'", attr[5]);
              state->pSmartAmpMgr->isBoostDevice = atoi((char *)attr[5]);
        } else {
            ALOGE("Smart Amp XML Define Is Error '%s'", attr[4]);
        }
        if (strcmp(attr[6], "boost_voltage") == 0) {
            ALOGV("Smart Amp boost_voltage Is '%s'", attr[7]);
            state->pSmartAmpMgr->boostVoltage = atoi((char *)attr[7]);
        } else {
            ALOGE("Smart Amp XML Define Is Error '%s'", attr[6]);
        }
   }
    else if (strcmp(tag_name, "fm_btsco_i2s") == 0){
	 modem->i2s_btsco_fm = malloc(sizeof(ctrl_node));
	 if(modem->i2s_btsco_fm == NULL){
             ALOGE("malloc i2s_fm err ");
	 }else{
            if (strcmp(attr[0], "ctl_file") == 0) {
                memcpy(modem->i2s_btsco_fm->ctrl_path,attr[1],strlen(attr[1])+1);
		   modem->i2s_btsco_fm->ctrl_file_fd=open(modem->i2s_btsco_fm->ctrl_path,O_RDWR | O_SYNC);
		   if(modem->i2s_btsco_fm->ctrl_file_fd <= 0){
                    ALOGE("open i2s_fm file err");
		   }
            }
	 }
    }
    else if (strcmp(tag_name, "btcal_I2S") == 0)
    {

           ALOGV("The btcal_I2S num is %s = '%s--- %s = '%s  --- %s = '%s '", attr[0] ,attr[1] ,attr[12],attr[13],  attr[16],attr[17]);

        if (strcmp(attr[0], "cpu_index") == 0) {
            ALOGV("The iis_for_btcall cpu index is '%s'", attr[1]);
            state->i2s_ctl_info->cpu_index = atoi((char *)attr[1]);
        } else {
            ALOGE("no iis_ctl index for bt call!");
        }

	if (strcmp(attr[2], "i2s_index") == 0) {
            ALOGV("The iis_for_btcall i2s index is '%s'", attr[3]);
            state->i2s_ctl_info->i2s_index= atoi((char *)attr[3]);
        } else {
            ALOGE("no iis_ctl index for bt call!");
        }


        if (strcmp(attr[4], "switch") == 0) {
            ALOGV("The iis_for_btcall switch is '%s'", attr[5]);
            if(strcmp(attr[5],"1") == 0)
                state->i2s_ctl_info->is_switch= true;
            else if(strcmp(attr[5],"0") == 0)
                state->i2s_ctl_info->is_switch = false;
        } else {
            ALOGE("no iis_ctl switch for bt call!");
        }
        if (strcmp(attr[6], "dst") == 0) {
            ALOGV("The iis_for_btcall dst  is '%s'", attr[7]);
            if (strcmp(attr[7], "internal") == 0)
               state->i2s_ctl_info->is_ext= 0;
            else if (strcmp(attr[7], "external") == 0)
               state->i2s_ctl_info->is_ext = 1;
        } else {
            ALOGE("no dst path for bt call!");
        }
    ALOGV("------data = cpu_index(%d)i2s_index(%d)is_switch(%d)is_ext(%d)  len(%d) " , state->i2s_ctl_info->cpu_index ,
	state->i2s_ctl_info->i2s_index ,
	state->i2s_ctl_info->is_switch,
	state->i2s_ctl_info->is_ext ,
	sizeof(*attr)/sizeof( XML_Char *)
);



	state->i2s_ctl_info->p_ctlr_node_head = NULL;
	p_ctlr_node = NULL;
	i = 8;
	while(( attr[i]  ) && ( (i - 8 )  /2  <MAX_CTRL_FILE))
	{

	        if (strstr(attr[i], "ctl_file") > 0)
		{
	            if((strlen(attr[i+1]) +1) <= I2S_CTL_PATH_MAX){

					if( state->i2s_ctl_info->p_ctlr_node_head== NULL )
					{
						p_ctlr_node = malloc(sizeof(ctrl_node));
						state->i2s_ctl_info->p_ctlr_node_head = p_ctlr_node;
					}
					else
					{
						p_ctlr_node->next = malloc(sizeof(ctrl_node));
						p_ctlr_node = p_ctlr_node->next;
					}
					p_ctlr_node->next = NULL;

	                memcpy(p_ctlr_node->ctrl_path , attr[i+1], strlen(attr[i+1])+1);
	                ALOGV(" -11---ctl_file[%d] is %s",i,p_ctlr_node->ctrl_path);
	                p_ctlr_node->ctrl_file_fd= open(p_ctlr_node->ctrl_path,O_RDWR | O_SYNC);
	                if(p_ctlr_node->ctrl_file_fd <= 0 ) {
	                    ALOGE(" open ctl_file[%d] ,errno is %d", i-8 , errno);
	                }
			if (strstr(attr[i+2], "value") > 0) {
				if(( (strlen(attr[i+3]) +1) <= I2S_CTL_VALUE_MAX )&&( p_ctlr_node != NULL))
					memcpy(p_ctlr_node->ctrl_value , attr[i+3], strlen(attr[i+3])+1);
			}
	              ALOGV(" - ---------att[%d] is %s",   i ,attr[i]   );
	            }
	        }

		i += 4;
	}

	state->i2s_ctl_info++;



}
    else if (strcmp(tag_name, "fm_i2s") == 0){
	 modem->i2s_fm = malloc(sizeof(ctrl_node));
	 if(modem->i2s_fm == NULL){
             ALOGE("malloc i2s_fm err ");
	 }else{
            if (strcmp(attr[0], "ctl_file") == 0) {
                memcpy(modem->i2s_fm->ctrl_path,attr[1],strlen(attr[1])+1);
		   modem->i2s_fm->ctrl_file_fd=open(modem->i2s_fm->ctrl_path,O_RDWR | O_SYNC);
		   if(modem->i2s_fm->ctrl_file_fd <= 0){
                    ALOGE("open i2s_fm file err");
		   }
            }
	 }
    }
     else if (strcmp(tag_name, "mic_default_channel") == 0) {

        ALOGE("The mic_default_channel is %s = '%s'", attr[0] ,attr[1]);
        if (strcmp(attr[0], "value") == 0) {
            modem->mic_default_channel = atoi(attr[1]);
            ALOGE("mic_default_channel is '%d", modem->mic_default_channel);
        } else {
            ALOGE("no mic_default_channel!");
			modem->mic_default_channel=2;
        }
    } else if (strstr(tag_name, "i2s_switch_fm") != NULL) {
        int i;
        for (i = 0; i < I2S_CTL_INF; i++) {
            if (modem->i2s_switch_fm[i] == NULL) {
                modem->i2s_switch_fm[i] = malloc(sizeof(struct route_setting));
                if(modem->i2s_switch_fm[i] == NULL) {
                    ALOGE("malloc i2s_switch_fm[%d] err ", i);
                } else {
                    modem->i2s_switch_fm[i]->ctl_name = NULL;
                    modem->i2s_switch_fm[i]->strval = NULL;
                    LOG_I("i2s_switch_fm[%d] :%s-->%s",i,attr[1],attr[3]);
                    if (strcmp(attr[0], "ctl") == 0) {
                        modem->i2s_switch_fm[i]->ctl_name = strdup(attr[1]);
                    }
                    if (strcmp(attr[2], "value") == 0) {
                        modem->i2s_switch_fm[i]->strval = strdup(attr[3]);
                    }
                break;
                }
            }
        }
    } else if (strstr(tag_name, "i2s_switch_btcall") != NULL) {
        int j;
        for (j = 0; j < I2S_CTL_INF; j++) {
            if (modem->i2s_switch_btcall[j] == NULL) {
                modem->i2s_switch_btcall[j] = malloc(sizeof(struct route_setting));
                if (modem->i2s_switch_btcall[j] == NULL){
                    ALOGE("malloc i2s_switch_btcall[%d] err ", j);
                } else {
                    modem->i2s_switch_btcall[j]->ctl_name = NULL;
                    modem->i2s_switch_btcall[j]->strval = NULL;
                    LOG_I("i2s_switch_btcall :%s-->%s",attr[1],attr[3]);
                    if (strcmp(attr[0], "ctl") == 0) {
                        modem->i2s_switch_btcall[j]->ctl_name = strdup(attr[1]);
                    }
                    if (strcmp(attr[2], "value") == 0) {
                        modem->i2s_switch_btcall[j]->strval = strdup(attr[3]);
                    }
                break;
                }
            }
        }
    } else if (strstr(tag_name, "i2s_switch_ap") != NULL) {
        int k;
        for (k = 0; k < I2S_CTL_INF; k++) {
            if (modem->i2s_switch_ap[k] == NULL) {
                modem->i2s_switch_ap[k] = malloc(sizeof(struct route_setting));
                if (modem->i2s_switch_ap[k] == NULL) {
                    ALOGE("malloc i2s_switch_ap[%d] err ", k);
                } else {
                    modem->i2s_switch_ap[k]->ctl_name = NULL;
                    modem->i2s_switch_ap[k]->strval = NULL;
                    LOG_I("i2s_switch_ap :%s-->%s",attr[1],attr[3]);
                    if (strcmp(attr[0], "ctl") == 0) {
                        modem->i2s_switch_ap[k]->ctl_name = strdup(attr[1]);
                    }
                    if (strcmp(attr[2], "value") == 0) {
                        modem->i2s_switch_ap[k]->strval = strdup(attr[3]);
                    }
                break;
                }
            }
        }
    } else if (strstr(tag_name, "i2s_switch_pubcp") != NULL) {
        int m;
        for (m = 0; m < I2S_CTL_INF; m++) {
            if (modem->i2s_switch_pubcp[m] == NULL) {
                modem->i2s_switch_pubcp[m] = malloc(sizeof(struct route_setting));
                if (modem->i2s_switch_pubcp[m] == NULL) {
                    ALOGE("malloc i2s_switch_pubcp[%d] err ",m);
                } else {
                    modem->i2s_switch_pubcp[m]->ctl_name = NULL;
                    modem->i2s_switch_pubcp[m]->strval = NULL;
                    LOG_I("i2s_switch_pubcp :%s-->%s",attr[1],attr[3]);
                    if (strcmp(attr[0], "ctl") == 0) {
                        modem->i2s_switch_pubcp[m]->ctl_name = strdup(attr[1]);
                    }
                    if (strcmp(attr[2], "value") == 0) {
                        modem->i2s_switch_pubcp[m]->strval=strdup(attr[3]);
                    }
                break;
                }
            }
        }
#ifdef NXP_SMART_PA
    } else if (strcmp(tag_name, "i2s_switch_ext_codec_ap") == 0) {
        modem->i2s_switch_ext_codec_ap = malloc(sizeof(struct route_setting));
        if (modem->i2s_switch_ext_codec_ap == NULL){
             ALOGE("malloc i2s_switch_ext_codec_ap err ");
        } else {
            modem->i2s_switch_ext_codec_ap->ctl_name=NULL;
            modem->i2s_switch_ext_codec_ap->strval=NULL;
            LOG_I("i2s_switch_ext_codec_ap :%s-->%s",attr[1],attr[3]);
            if (strcmp(attr[0], "ctl") == 0) {
                modem->i2s_switch_ext_codec_ap->ctl_name=strdup(attr[1]);
            }
            if (strcmp(attr[2], "value") == 0) {
                modem->i2s_switch_ext_codec_ap->strval=strdup(attr[3]);
            }
        }
    } else if (strcmp(tag_name, "i2s_switch_ext_codec_cp") == 0) {
        modem->i2s_switch_ext_codec_cp = malloc(sizeof(struct route_setting));
        if (modem->i2s_switch_ext_codec_cp == NULL){
             ALOGE("malloc i2s_switch_ext_codec_ap err ");
        } else {
            modem->i2s_switch_ext_codec_cp->ctl_name=NULL;
            modem->i2s_switch_ext_codec_cp->strval=NULL;
            LOG_I("i2s_switch_ext_codec_ap :%s-->%s",attr[1],attr[3]);
            if (strcmp(attr[0], "ctl") == 0) {
                modem->i2s_switch_ext_codec_cp->ctl_name=strdup(attr[1]);
            }
            if (strcmp(attr[2], "value") == 0) {
                modem->i2s_switch_ext_codec_cp->strval=strdup(attr[3]);
            }
        }
    } else if (strcmp(tag_name, "ext_codec") == 0) {
        struct ext_codec_info *ext_codec;

        ext_codec = malloc(sizeof(struct ext_codec_info));
        if (!ext_codec) {
            ALOGE("malloc ext_codec err ");
        } else {
            memset(ext_codec, 0, sizeof(struct ext_codec_info));
            ext_codec->name = strdup(attr[1]);
            ALOGI("Ext codec: %s", attr[1]);
            state->adev->ext_codec = ext_codec;
            state->parsing_ext_codec = true;
        }
    } else if (state->parsing_ext_codec) {
        ext_codec_start_tag(data, tag_name, attr);
#endif
    } else if (strcmp(tag_name, "interrupt_playback_support") == 0) {
        if ((strcmp(attr[0], "value") == 0) &&(strcmp(attr[1], "false") == 0) ){
            modem->interrupt_playback_support=0;
        }
    }
   /*else if (strcmp(tag_name, "i2s_for_extspeaker") == 0)
    {
        if (strcmp(attr[0], "index") == 0) {
            ALOGD("The i2s_for_extspeaker index is '%s'", attr[1]);
            modem->i2s_extspk.index = atoi((char *)attr[1]);
        } else {
            ALOGE("no iis_ctl index for extspk call!");
        }
        if (strcmp(attr[2], "switch") == 0) {
            ALOGD("The iis_for_btcall switch is '%s'", attr[3]);
            if(strcmp(attr[3],"1") == 0)
                modem->i2s_extspk.is_switch = true;
            else if(strcmp(attr[3],"0") == 0)
                modem->i2s_extspk.is_switch = false;
        } else {
            ALOGE("no iis_ctl switch for extspk call!");
        }
        if (strcmp(attr[4], "dst") == 0) {
            if (strcmp(attr[5], "external") == 0)
                modem->i2s_extspk.is_ext = 1;
            else if(strcmp(attr[5], "internal") == 0)
                modem->i2s_extspk.is_ext = 0;

            ALOGD("The i2s_for_extspeaker dst  is '%d'", modem->i2s_extspk.is_ext);

        }
        else {
                ALOGE("no dst path for bt call!");
        }
       if (strcmp(attr[6], "cp0_ctl_file") == 0) {
            ALOGD("i2s_for_extspeaker cp0_bt_ctl_file");
            if((strlen(attr[7]) +1) <= I2S_CTL_PATH_MAX){
                memcpy(modem->i2s_extspk.fd_sys_cp0_path , attr[7], strlen(attr[7])+1);
                ALOGE(" cp0_ctl_file is %s",modem->i2s_extspk.fd_sys_cp0_path);
                modem->i2s_extspk.fd_sys_cp0 = open(modem->i2s_extspk.fd_sys_cp0_path,O_RDWR | O_SYNC);
                if(modem->i2s_extspk.fd_sys_cp0 == -1) {
                    ALOGE(" audio_hw_primary: could not open i2s sys_cp0_ctl fd,errno is %d",errno);
                }
            }
        }
        if (strcmp(attr[8], "cp1_ctl_file") == 0) {
            if((strlen(attr[9]) +1) <= I2S_CTL_PATH_MAX){
                memcpy(modem->i2s_extspk.fd_sys_cp1_path , attr[9], strlen(attr[9])+1);
                ALOGE(" cp1_ctl_file is %s",modem->i2s_extspk.fd_sys_cp1_path);
                modem->i2s_extspk.fd_sys_cp1 = open(modem->i2s_extspk.fd_sys_cp1_path,O_RDWR | O_SYNC);
                if(modem->i2s_extspk.fd_sys_cp1 == -1) {
                    ALOGE(" audio_hw_primary: could not open i2s sys_cp1 ctl fd,errno is %d",errno);
                }
            }
        }
        if (strcmp(attr[10], "cp2_ctl_file") == 0) {
            if((strlen(attr[11]) +1) <= I2S_CTL_PATH_MAX){
                memcpy(modem->i2s_extspk.fd_sys_cp2_path , attr[11], strlen(attr[11])+1);
                ALOGE(" cp2_ctl_file is %s",modem->i2s_extspk.fd_sys_cp2_path);
                modem->i2s_extspk.fd_sys_cp2 = open(modem->i2s_extspk.fd_sys_cp2_path,O_RDWR | O_SYNC);
                if(modem->i2s_extspk.fd_sys_cp2 == -1) {
                    ALOGE(" audio_hw_primary: could not open i2s sys_cp2 ctl fd,errno is %d",errno);
                }
            }
        }
        if (strcmp(attr[12], "ap_ctl_file") == 0) {
            if((strlen(attr[13]) +1) <= I2S_CTL_PATH_MAX){
                memcpy(modem->i2s_extspk.fd_sys_ap_path , attr[13], strlen(attr[13])+1);
                ALOGE(" ap_ctl_file is %s",modem->i2s_extspk.fd_sys_ap_path);
                modem->i2s_extspk.fd_sys_ap = open(modem->i2s_extspk.fd_sys_ap_path,O_RDWR | O_SYNC);
                if(modem->i2s_extspk.fd_sys_ap == -1) {
                    ALOGE(" audio_hw_primary: could not open i2s sys_ap ctl fd,errno is %d",errno);
                }
            }
        }
    }*/
    else if ((strcmp(tag_name, "voip") == 0)&& !modem->voip_res.is_done){

            char prop_t[PROPERTY_VALUE_MAX] = {0};
            char prop_w[PROPERTY_VALUE_MAX] = {0};
            bool t_enable = false;
            bool w_enalbe = false;
            bool csfb_enable = false;

            if(property_get(MODEM_T_ENABLE_PROPERTY, prop_t, "") && 0 == strcmp(prop_t, "1") )
            {
                MY_TRACE("%s:%s",__func__,MODEM_T_ENABLE_PROPERTY);
                t_enable = true;
            }
            if(property_get(MODEM_W_ENABLE_PROPERTY, prop_w, "") && 0 == strcmp(prop_w, "1"))
            {
                MY_TRACE("%s:%s",__func__,MODEM_W_ENABLE_PROPERTY);
                w_enalbe = true;
            }
            if(property_get(MODEM_TDDCSFB_ENABLE_PROPERTY, prop_w, "") && 0 == strcmp(prop_w, "1"))
            {
                MY_TRACE("%s:%s",__func__,MODEM_TDDCSFB_ENABLE_PROPERTY);
                csfb_enable = true;
            }
            if(property_get(MODEM_FDDCSFB_ENABLE_PROPERTY, prop_w, "") && 0 == strcmp(prop_w, "1"))
            {
                MY_TRACE("%s:%s", __func__, MODEM_FDDCSFB_ENABLE_PROPERTY);
                csfb_enable = true;
            }
            if(property_get(MODEM_CSFB_ENABLE_PROPERTY, prop_w, "") && 0 == strcmp(prop_w, "1"))
            {
                MY_TRACE("%s:%s", __func__, MODEM_CSFB_ENABLE_PROPERTY);
                csfb_enable = true;
            }


           /* Obtain the modem num */

           if (strcmp(attr[0], "modem") == 0) {

                    if(strcmp(attr[1], "w") == 0)
                    {
                        if(w_enalbe){
                            ALOGV("The voip run on modem  is '%s'", attr[1]);
                            modem->voip_res.cp_type = CP_W;
                            modem->voip_res.is_done = true;
                        }
                        else
                            return ;
                    }
                    else if(strcmp(attr[1], "t") == 0)
                    {

                        if(t_enable){
                            ALOGV("The voip run on modem  is '%s'", attr[1]);
                            modem->voip_res.cp_type = CP_TG;
                            modem->voip_res.is_done = true;
                        }
                        else
                            return;
                    }
                    else if(strcmp(attr[1], "csfb") == 0)
                    {

                        if(csfb_enable){
                            ALOGV("The voip run on modem  is '%s'", attr[1]);
                            modem->voip_res.cp_type = CP_CSFB;
                            modem->voip_res.is_done = true;
                        }
                        else
                            return;
                    }
           } else {
                    ALOGE("no modem type for voip!");
                    goto attr_err;
           }

           if (strcmp(attr[2], "pipe") == 0) {
                    ALOGV("voip pipe name is %s", attr[3]);
                    if((strlen(attr[3]) +1) <= VOIP_PIPE_NAME_MAX)
                       memcpy(modem->voip_res.pipe_name, attr[3], strlen(attr[3])+1);
            }
            else {
                    ALOGE("'%s' No pipe filed!", attr[2]);
                    goto attr_err;
            }
            if (strcmp(attr[4], "vbchannel") == 0) {
                   modem->voip_res.channel_id = atoi((char *)attr[5]);
            }
            else {
                    ALOGE("'%s' No vbc filed!", attr[4]);
                    goto attr_err;
            }

           if (strcmp(attr[6], "enable") == 0) {
                   ALOGV("The enable_for_voip is '%s'", attr[7]);
                   if(strcmp(attr[7],"1") == 0)
                       modem->voip_res.enable = true;
           } else {
                   ALOGE("no iis_ctl index for bt call!");
           }
   }
   else if (strcmp(tag_name, "debug") == 0) //parse debug info
   {
        if (strcmp(attr[0], "enable") == 0)
        {
            if (strcmp(attr[1], "0") == 0)
            {
                modem->debug_info.enable = 0;
            }
            else
            {
                modem->debug_info.enable = 1;
            }
        }
        else
        {
            ALOGE("no adaptable type for debug!");
            goto attr_err;
        }
    }
    else if (strcmp(tag_name, "debuginfo") == 0) //parse debug info
    {
        if (strcmp(attr[0], "sleepdeltatimegate") == 0)
        {
            ALOGV("The sleepdeltatimegate is  '%s'", attr[1]);
            modem->debug_info.sleeptime_gate=atoi((char *)attr[1]);
        }
        else if (strcmp(attr[0], "pcmwritetimegate") == 0)
        {
            ALOGV("The pcmwritetimegate is  '%s'", attr[1]);
            modem->debug_info.pcmwritetime_gate=atoi((char *)attr[1]);
        }
        else if (strcmp(attr[0], "lastthiswritetimegate") == 0)
        {
            ALOGV("The lastthiswritetimegate is  '%s'", attr[1]);
            modem->debug_info.lastthis_outwritetime_gate=atoi((char *)attr[1]);
        }
        else
        {
            ALOGE("no adaptable info for debuginfo!");
            goto attr_err;
        }
    }else if (strcmp(tag_name, "nr_support") == 0) //parse nr support info
       {
            if((strcmp(attr[0], "rate") == 0)&&(strcmp(attr[2], "enable") ==0)){
                if(strcmp(attr[1], "other_rate") == 0){
                    if(strcmp(attr[3], "true") == 0){
                        state->adev->enable_other_rate_nr =true;
                    }else{
                        state->adev->enable_other_rate_nr =false;
                    }
                }else{
                    int rate=strtoul(attr[1],NULL,0);
                    int nr_mask=RECORD_UNSUPPORT_RATE_NR;
                    switch(rate){
                        case 48000:
                            nr_mask=RECORD_48000_NR;
                            break;
                        case 44100:
                            nr_mask=RECORD_44100_NR;
                            break;
                        case 16000:
                            nr_mask=RECORD_16000_NR;
                            break;
                        case 8000:
                            nr_mask=RECORD_8000_NR;
                            break;
                        default:
                            nr_mask=RECORD_UNSUPPORT_RATE_NR;
                            break;
                    }

                    if(nr_mask!=RECORD_UNSUPPORT_RATE_NR){
                        if(strcmp(attr[3], "true") == 0){
                            state->adev->nr_mask |=1<<nr_mask;
                        }else{
                            state->adev->nr_mask &=~(1<<nr_mask);
                        }
                    }
                }
            }
            ALOGI("nr_support nr_mask:%x",state->adev->nr_mask);
        }
    else{
        ALOGE("no adaptable info  :%s",tag_name);
    }
attr_err:
    return;
}
static void adev_modem_end_tag(void *data, const XML_Char *tag_name)
{
    struct modem_config_parse_state *state = data;

#ifdef NXP_SMART_PA
    if (!strcmp(tag_name, "ext_codec"))
        state->parsing_ext_codec = false;
#endif
}

/* Initialises  the audio params,the modem profile and variables , */
static int adev_modem_parse(struct tiny_audio_device *adev)
{
    struct modem_config_parse_state state;
    XML_Parser parser;
    FILE *file;
    int bytes_read;
    void *buf;
    int i;
    int ret = 0;

    vbc_ctrl_pipe_para_t *vbc_ctrl_pipe_info = NULL;
    audio_modem_t *modem = NULL;

    modem = calloc(1, sizeof(audio_modem_t));
    if (!modem)
    {
        ALOGE("adev_modem_parse alloc fail, size:%d", sizeof(audio_modem_t));
        ret = -ENOMEM;
        goto err_calloc;
    }
    else
    {
        memset(modem, 0, sizeof(audio_modem_t));
    }

    /*smart amp manager*/
    SMART_AMP_MGR *pSmartAmpMgr = malloc(sizeof (SMART_AMP_MGR));
    if (NULL != pSmartAmpMgr) {
        memset(pSmartAmpMgr, 0, sizeof(SMART_AMP_MGR));
    } else {
        ALOGE("adev_modem_parse malloc pSmartAmpMgr fail, size:%d", sizeof(SMART_AMP_MGR));
        ret = -ENOMEM;
        goto err_calloc;
    }

    modem->num = 0;
    modem->vbc_ctrl_pipe_info = NULL;
    modem->mic_default_channel=2;
    modem->interrupt_playback_support=1;

	ALOGE("----adev_modem_i2s_parse----000- ");
	i2s_bt_t  *i2s_btcall_info;
    i2s_btcall_info = calloc(1, sizeof(i2s_bt_t));
    if (!i2s_btcall_info)
    {
        ALOGE("adev_modem_parse alloc fail, size:%d", sizeof(i2s_bt_t));
        ret = -ENOMEM;
        goto err_calloc;
    }
    else
    {
        memset(i2s_btcall_info, 0, sizeof(i2s_bt_t));
    }

    i2s_btcall_info->num = 0;
    i2s_btcall_info->i2s_ctl_info = NULL;


    file = fopen(AUDIO_XML_PATH, "r");
    if (!file) {
        ALOGE("Failed to open %s", AUDIO_XML_PATH);
        ret = -ENODEV;
        goto err_fopen;
    }

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        ALOGE("Failed to create XML parser");
        ret = -ENOMEM;
        goto err_parser_create;
    }

    memset(&state, 0, sizeof(state));
    state.modem_info = modem;
    state.i2s_btcall_info = i2s_btcall_info;
    state.fm_type = -1;
    state.pSmartAmpMgr = pSmartAmpMgr;
    state.adev = adev;
    XML_SetUserData(parser, &state);
    XML_SetElementHandler(parser, adev_modem_start_tag, adev_modem_end_tag);

    for (;;) {
        buf = XML_GetBuffer(parser, BUF_SIZE);
        if (buf == NULL)
        {
            ret = -EIO;
            goto err_parse;
        }
        bytes_read = fread(buf, 1, BUF_SIZE, file);
        if (bytes_read < 0)
        {
            ret = -EIO;
            goto err_parse;
        }
        if (XML_ParseBuffer(parser, bytes_read,
                    bytes_read == 0) == XML_STATUS_ERROR) {
            ALOGE("Error in xml(%s)", AUDIO_XML_PATH);
            ret = -EINVAL;
            goto err_parse;
        }

        if (bytes_read == 0)
            break;
    }

    adev->cp = modem;
    adev->i2s_btcall_info = i2s_btcall_info;
    adev->cp_nbio_pipe =   state.cp_nbio_pipe;
    adev->fm_type = state.fm_type;
    adev->pSmartAmpMgr = state.pSmartAmpMgr;
    ALOGE("adev->cp_nbio_pipe (%s)", adev->cp_nbio_pipe);
    XML_ParserFree(parser);
    fclose(file);
    return ret;

err_parse:
    XML_ParserFree(parser);
err_parser_create:
    fclose(file);
err_fopen:
err_calloc:
    if(modem){
        free(modem);
        modem = NULL;
    }
    if(pSmartAmpMgr) {
        free(pSmartAmpMgr);
        pSmartAmpMgr = NULL;
    }
    return ret;
}

static void vb_effect_getpara(struct tiny_audio_device *adev)
{
    static bool read_already=0;
    off_t offset = 0;
    AUDIO_TOTAL_T * aud_params_ptr;
    int len = sizeof(AUDIO_TOTAL_T)*adev_get_audiomodenum4eng();
    int srcfd;
    char *filename = NULL;

    adev->audio_para = calloc(1, len);
    if (!adev->audio_para)
    {
        ALOGE("vb_effect_getpara alloc fail, size:%d", len);
        return;
    }
    memset(adev->audio_para, 0, len);
    
    
   char value[PROPERTY_VALUE_MAX]={0}; 
   property_get("ro.debuggable", value, "0"); 

   if(!strncmp(value,"1",1)){ 
 
    srcfd = open((char *)(ENG_AUDIO_PARA_DEBUG), O_RDONLY);
    filename = (srcfd < 0 )? ( ENG_AUDIO_PARA):(ENG_AUDIO_PARA_DEBUG);
    if(srcfd >= 0)
    {
        close(srcfd);
    }

    } 
    else 
        filename =  ENG_AUDIO_PARA; 
     
    ALOGI("vb_effect_getpara read name:%s.", filename);
    stringfile2nvstruct(filename, adev->audio_para, len); //get data from audio_hw.txt.
    audio_para_ptr = adev->audio_para;
}

static void *audiopara_tuning_thread_entry(void * param)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)param;
    int fd_aud = -1;
    int fd_dum = -1;
    int send_fd_aud = -1;
    int ops_bit = 0;
    int result = -1;
    AUDIO_TOTAL_T ram_from_eng;
    int mode_index = 0;
    int buffersize = 0;
    void* pmem = NULL;
    int length = 0;
    //mode_t mode_f = 0;
    ALOGE("%s E\n",__FUNCTION__);
    memset(&ram_from_eng,0x00,sizeof(AUDIO_TOTAL_T));
    if (mkfifo(AUDFIFO,S_IFIFO|0666) <0) {
        if (errno != EEXIST) {
            ALOGE("%s create audio fifo error %s\n",__FUNCTION__,strerror(errno));
            return NULL;
        }
    }
    if(chmod(AUDFIFO, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) != 0) {
        ALOGE("%s Cannot set RW to \"%s\": %s", __FUNCTION__,AUDFIFO, strerror(errno));
    }
    if (mkfifo(AUDFIFO_2,S_IFIFO|0666) <0) {
        if (errno != EEXIST) {
            ALOGE("%s create audio fifo_2 error %s\n",__FUNCTION__,strerror(errno));
            return NULL;
        }
    }
    if(chmod(AUDFIFO_2, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) != 0) {
        ALOGE("%s Cannot set RW to \"%s\": %s", __FUNCTION__,AUDFIFO_2, strerror(errno));
    }
    fd_aud = open(AUDFIFO,O_RDONLY);
    if (fd_aud == -1) {
        ALOGE("%s open audio FIFO error %s\n",__FUNCTION__,strerror(errno));
        return NULL;
    }
    fd_dum = open(AUDFIFO,O_WRONLY);
    if (fd_dum == -1) {
        ALOGE("%s open dummy audio FIFO error %s\n",__FUNCTION__,strerror(errno));
        close(fd_aud);
        return NULL;
    }
    adev->tunningMgr.fd = fd_dum;
    while (ops_bit != -1) {
        result = read(fd_aud,&ops_bit,sizeof(int));
        ALOGE("%s read audio FIFO result %d,ram_req:%d\n",__FUNCTION__,result,ops_bit);
        if (result >0) {
            pthread_mutex_lock(&adev->lock);
            //1、write parameter to Flash or RAM
            if (ops_bit == 0xabccba) {
                ALOGI("%s exit while!", __FUNCTION__);
                pthread_mutex_unlock(&adev->lock);
                break;
            }
            if(ops_bit & ENG_FLASH_OPS){
                ALOGE("%s audio para --> update from flash\n",__FUNCTION__);
                if (adev->audio_para){
                    free(adev->audio_para);
                }
                vb_effect_getpara(adev);
                vb_effect_setpara(adev->audio_para);
            }else if(ops_bit & ENG_RAM_OPS){
                ALOGE("%s audio para --> update from RAM\n",__FUNCTION__);
                result = read(fd_aud,&mode_index,sizeof(int));
                result = read(fd_aud,&ram_from_eng,sizeof(AUDIO_TOTAL_T));
                ALOGE("%s read audio FIFO result %d,mode_index:%d,size:%d\n",__FUNCTION__,result,mode_index,sizeof(AUDIO_TOTAL_T));
                adev->audio_para[mode_index] = ram_from_eng;
                if (mode_index == 0 || mode_index == 3) {
                    apply_fm_volume(adev);
                }
            }
            //2、mandatory to set PGA GAIN
            if(ENG_PGA_OPS & ops_bit) {
                SetAudio_gain_route(adev,1,adev->out_devices,adev->in_devices);
            }

            adev->device_force_set = 1;
            select_devices_signal_asyn(adev);
            //3、mandatory to get Phone information,include hardware version,FM type,DSP-Process-Voip,VBC LoopBack
            if(ENG_PHONEINFO_OPS & ops_bit) {
                buffersize = AUDIO_AT_HARDWARE_NAME_LENGTH + (AUDIO_AT_ITEM_NAME_LENGTH + AUDIO_AT_ITEM_VALUE_LENGTH )*AUDIO_AT_ITEM_NUM ;
                pmem = malloc(buffersize);
                if(pmem!=NULL){
                    memset(pmem,0,buffersize);
                    length = audiopara_get_compensate_phoneinfo(pmem);
                    send_fd_aud = open(AUDFIFO_2,O_WRONLY);
                    if (send_fd_aud == -1) {
                        ALOGE("%s open audio FIFO_2 error %s\n",__FUNCTION__,strerror(errno));
                    } else {
                        ALOGE("%s ENG_PHONEINFO_OPS enter :%d!\n",__FUNCTION__,length);
                        result = write(send_fd_aud ,&length,sizeof(int));
                        result = write(send_fd_aud ,pmem,length);
                        close(send_fd_aud );
                        ALOGE("%s ENG_PHONEINFO_OPS complete!\n",__FUNCTION__);
                    }
					free(pmem);
                    pmem = NULL;
                } else {
                    ALOGE("%s allocate ENG_PHONEINFO_OPS error!\n",__FUNCTION__);
                }
            }

            if(ENG_PHONELOOP_OPS & ops_bit) {
                int LOOPEnable = 0;
                int LOOPRoute = 0;
                int result1 = -1;
                int result2 = -1;
                int w_value = 0;
                int pipe_fd = -1;
                char loop_cmd[1024]={0};

                result1 = read(fd_aud,&LOOPEnable,sizeof(int));
                result2 = read(fd_aud,&LOOPRoute,sizeof(int));

                pipe_fd = open(AUDIO_EXT_CONTROL_PIPE, O_WRONLY);
                if (pipe_fd == -1)
                    pipe_fd = open(AUDIO_EXT_DATA_CONTROL_PIPE, O_WRONLY);
                if (pipe_fd == -1) {
                    ALOGE("%s ENG_PHONELOOP_OPS open pipe error %s\n",__FUNCTION__,strerror(errno));
                }else{
                    ALOGE("%s ENG_PHONELOOP_OPS entry!!! read audio FIFO result1 %d,LOOPEnable:%d,result2 %d,LOOPRoute:%d\n",
                        __FUNCTION__,result1,LOOPEnable,result2,LOOPRoute);
                    w_value = (LOOPEnable<<8)|LOOPRoute;
                    sprintf(loop_cmd,"audio_cp_loop_test=%d;",w_value);
                    result1 = write(pipe_fd, loop_cmd, strlen(loop_cmd));
                    ALOGE("%s ENG_PHONELOOP_OPS loop_cmd:%s,size:%d!!!write result %d, w_value:%x \n",
                        __FUNCTION__, loop_cmd, strlen(loop_cmd), result1, w_value);
                    close(pipe_fd);
                }
            }

            pthread_mutex_unlock(&adev->lock);
            ALOGE("%s read audio FIFO X.\n",__FUNCTION__);
        }
    }
    ALOGE("exit from audio tuning thread");
    if(pmem)
        free(pmem);
    close(fd_dum);
    close(fd_aud);
    //unlink(AUDFIFO);
    //pthread_exit(NULL);
    return NULL;
}

static int audiopara_tuning_manager_close(audio_tunning_mgr *pMgr)
{
    if (NULL == pMgr) {
        ALOGE("%s argin is error!", __FUNCTION__);
        return -1;
    }

    int w_size = 0;
    int data = 0xabccba;
    int fd = -1;
    fd = open(AUDFIFO,O_RDWR);

    if (fd < 0) {
        ALOGE("%s open pipe is error!", __FUNCTION__);
        return -2;
    }

    w_size = write(fd, &data, sizeof(data));
    if (w_size > 0) {
        pthread_join(pMgr->thread_id, (void **) NULL);
    }
    close(fd);
    ALOGI("audiopara_tuning_manager_close exit w_size = %d", w_size);
    return 0;
}

static int audiopara_tuning_manager_create(struct tiny_audio_device *adev)
{
    int ret;
    /* create a thread to manager audiopara tuning.*/
    adev->tunningMgr.fd = -1;
    ret = pthread_create(&adev->tunningMgr.thread_id, NULL,
            audiopara_tuning_thread_entry, (void *)adev);
    if (ret) {
        ALOGE("pthread_create falied, code is %d", ret);
        return ret;
    }
    return ret;
}

static int audiopara_get_compensate_phoneinfo(void* pmsg)
{
    char value[PROPERTY_VALUE_MAX]={0};
    int result = true;
    int codec_info = 0;
    char* currentPosition = (char*)pmsg;
    char* startPosition = (char*)pmsg;
    //1,get and fill product hareware info.
    if (!property_get("ro.product.hardware", value, "0")){
        result = false;
    }
    ALOGE("%s produc.hardware:%s",__func__,value);
    memcpy(currentPosition,value,sizeof(value));

    //2,get and fill build.version info.
    currentPosition = currentPosition + AUDIO_AT_HARDWARE_NAME_LENGTH;
    if (!property_get("ro.build.version.release", value, "0")){
        result = false;
    }
    ALOGE("%s ro.build.version.release:%s",__func__,value);
    memcpy(currentPosition,value,sizeof(value));


    //3,get and fill digital/linein fm flag.
    currentPosition = currentPosition + AUDIO_AT_HARDWARE_NAME_LENGTH;
    strcpy(currentPosition,AUDIO_AT_DIGITAL_FM_NAME);
    currentPosition = currentPosition + AUDIO_AT_ITEM_NAME_LENGTH;
    if (property_get(FM_DIGITAL_SUPPORT_PROPERTY, value, "0") && strcmp(value, "1") == 0){
        sprintf(currentPosition,"%d",1);
    } else {
        sprintf(currentPosition,"%d",0);
    }
    ALOGE("%s :%s:%s",__func__,(currentPosition - AUDIO_AT_ITEM_NAME_LENGTH),currentPosition);

    //4,get and fill wether fm loop vbc or not.
    currentPosition = currentPosition + AUDIO_AT_ITEM_VALUE_LENGTH;
    strcpy(currentPosition,AUDIO_AT_FM_LOOP_VBC_NAME );
    currentPosition = currentPosition + AUDIO_AT_ITEM_NAME_LENGTH;

    if(s_adev->private_ctl.fm_loop_vbc !=NULL && 1 == mixer_ctl_get_value(s_adev->private_ctl.fm_loop_vbc, 0)){
        sprintf(currentPosition,"%d",1);
    ALOGE("%s :%s:%s,ctrl:%0x,value:%d,ctrl1:%0x,value:%d \n",__func__,(currentPosition - AUDIO_AT_ITEM_NAME_LENGTH),currentPosition,s_adev->private_ctl.fm_loop_vbc,mixer_ctl_get_value(s_adev->private_ctl.fm_loop_vbc, 0),s_adev->private_ctl.ad1_fm_loop_vbc,mixer_ctl_get_value(s_adev->private_ctl.ad1_fm_loop_vbc, 0));
    } else {
        sprintf(currentPosition,"%d",0);
    }

    //5,get and fill whether voip process by DSP.
    currentPosition = currentPosition + AUDIO_AT_ITEM_VALUE_LENGTH;
    strcpy(currentPosition,AUDIO_AT_VOIP_DSP_PROCESS_NAME);
    currentPosition = currentPosition + AUDIO_AT_ITEM_NAME_LENGTH;
#ifdef VOIP_DSP_PROCESS
        sprintf(currentPosition,"%d",1);
#else
        sprintf(currentPosition,"%d",0);
#endif
    ALOGE("%s :%s:%s",__func__,(currentPosition - AUDIO_AT_ITEM_NAME_LENGTH),currentPosition);

    //6,get and fill whether 9620 modem.
    currentPosition = currentPosition + AUDIO_AT_ITEM_VALUE_LENGTH;
    strcpy(currentPosition,AUDIO_AT_9620_MODEM);
    currentPosition = currentPosition + AUDIO_AT_ITEM_NAME_LENGTH;
#ifdef VB_CONTROL_PARAMETER_V2
        sprintf(currentPosition,"%d",1);
#else
        sprintf(currentPosition,"%d",0);
#endif
    ALOGE("%s :%s:%s",__func__,(currentPosition - AUDIO_AT_ITEM_NAME_LENGTH),currentPosition);

    //7,get and fill audio codec info.
    currentPosition = currentPosition + AUDIO_AT_ITEM_VALUE_LENGTH;
    strcpy(currentPosition,AUDIO_AT_CODEC_INFO);
    currentPosition = currentPosition + AUDIO_AT_ITEM_NAME_LENGTH;
    codec_info = mixer_ctl_get_value(s_adev->private_ctl.aud_codec_info, 0);
    if (codec_info < 0) {
        codec_info = AUDIO_CODEC_2713;
    }
    sprintf(currentPosition,"%d",codec_info);
    ALOGE("%s :%s:%s",__func__,(currentPosition - AUDIO_AT_ITEM_NAME_LENGTH),currentPosition);

    //8, get fm type info
    currentPosition = currentPosition + AUDIO_AT_ITEM_VALUE_LENGTH;
    strcpy(currentPosition, AUDIO_AT_FM_TYPE_INFO);
    currentPosition = currentPosition + AUDIO_AT_ITEM_NAME_LENGTH;
    sprintf(currentPosition,"%d",s_adev->fm_type);

    //9, get and fill anthoer item.
    currentPosition = currentPosition + AUDIO_AT_ITEM_VALUE_LENGTH;
    result = currentPosition - startPosition;
    ALOGE("%s :result length:%d",__func__,result);
    return result;
}

char * adev_get_cp_card_name(struct tiny_audio_device *adev){
    char *card_name = PNULL;
    if(adev){
        if(adev->cp_type == CP_TG) {
            card_name = CARD_VAUDIO;
            if(!adev->cp_mixer[CP_TG]){
                s_vaudio = get_snd_card_number(card_name);
                adev->cp_mixer[CP_TG] = mixer_open(s_vaudio);
            }
        }
        else if (adev->cp_type == CP_W) {
            card_name = CARD_VAUDIO_W;
            if(!adev->cp_mixer[CP_W]){
                s_vaudio_w = get_snd_card_number(card_name);
                adev->cp_mixer[CP_W] = mixer_open(s_vaudio_w);
            }
        }
        else if (adev->cp_type == CP_CSFB) {
            card_name = CARD_VAUDIO_LTE;
            if(!adev->cp_mixer[CP_CSFB]){
                s_vaudio_lte = get_snd_card_number(card_name);
                adev->cp_mixer[CP_CSFB] = mixer_open(s_vaudio_lte);
            }
        }
        LOG_E("yaye audio_get_cp_card_name name:%s\n", card_name);
    }
    return card_name;
}

int adev_record_tone_stop(struct tiny_audio_device *adev)
{
    int ret = 0;

    ALOGW("yaye %s, entry!!!", __FUNCTION__);
    ret = cp_mixer_disable(adev->record_tone_info, adev->cp_mixer[adev->cp_type]);
    ALOGW("yaye %s, exit!!!", __FUNCTION__);
    return ret;
}

int adev_record_tone_start(struct tiny_audio_device *adev, char* file_name)
{
    struct tiny_stream_out *out;
    int ret = 0;
    int card = -1;

    ALOGW("yaye %s, entry!!!", __FUNCTION__);

    pthread_mutex_lock(&adev->lock);
    if(!adev->call_connected){
        ALOGW("yaye %s, fail! call not connected!", __FUNCTION__);
        pthread_mutex_unlock(&adev->lock);
        return ret;
    }

    if (adev->active_output) {
        out = adev->active_output;
        pthread_mutex_lock(&out->lock);
        do_output_standby(out);
        pthread_mutex_unlock(&out->lock);
    }

    card = get_snd_card_number(adev_get_cp_card_name(adev));
    ret = cp_mixer_enable(adev->record_tone_info, adev->cp_mixer[adev->cp_type], card, file_name);

    pthread_mutex_unlock(&adev->lock);
    ALOGW("yaye %s, exit!!!", __FUNCTION__);
    return ret;
}

#ifdef NXP_SMART_PA
#define EXT_SMARTPA_EXIST_FILE "/sys/kernel/ext_smartpa_exist"
static int get_ext_smartpa_exist(void)
{
    int fd;
    char exist = 1;
    char *file = EXT_SMARTPA_EXIST_FILE;

    if (!access(file , F_OK)) {
        fd = open(file , O_RDONLY);
        if (fd < 0) {
            ALOGE("%s, open '%s' failed!", __func__, file );
            return 1;
        }
        if (read(fd, &exist, sizeof(exist)) < 0) {
            ALOGE("%s, read '%s' failed!", __func__, file );
            return 1;
        }

        exist = (exist != '0');
    }

    ALOGI("External smartpa exist: %d", exist);

    return exist;
}
#endif

static int adev_open(const hw_module_t* module, const char* name,
        hw_device_t** device)
{
    struct tiny_audio_device *adev;
    int ret,i;
    BLUE_TRACE("adev_open");
    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    pthread_mutex_lock(&adev_init_lock);
    BLUE_TRACE("adev_open begin");
    if (audio_device_ref_count != 0) {
        *device = &s_adev->hw_device.common;
        audio_device_ref_count++;
        ALOGE("%s: returning existing instance of adev", __func__);
        ALOGE("%s: exit", __func__);
        pthread_mutex_unlock(&adev_init_lock);
        return 0;
    }

    adev = calloc(1, sizeof(struct tiny_audio_device));
    if (!adev)
    {
        ALOGE("vb_effect_getpara alloc fail, size:%d", sizeof(struct tiny_audio_device));
        pthread_mutex_unlock(&adev_init_lock);
        return -ENOMEM;
    }
    memset(adev, 0, sizeof(struct tiny_audio_device));
    s_adev = adev;

    adev->hw_device.common.tag = HARDWARE_DEVICE_TAG;
    adev->hw_device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    adev->hw_device.common.module = (struct hw_module_t *) module;
    adev->hw_device.common.close = adev_close;

    adev->hw_device.get_supported_devices = adev_get_supported_devices;
    adev->hw_device.init_check = adev_init_check;
    adev->hw_device.set_voice_volume = adev_set_voice_volume;
    adev->hw_device.set_master_volume = adev_set_master_volume;
    adev->hw_device.set_mode = adev_set_mode;
    adev->hw_device.set_master_mute = adev_set_master_mute;
    adev->hw_device.get_master_mute = adev_get_master_mute;
    adev->hw_device.set_mic_mute = adev_set_mic_mute;
    adev->hw_device.get_mic_mute = adev_get_mic_mute;
    adev->hw_device.set_parameters = adev_set_parameters;
    adev->hw_device.get_parameters = adev_get_parameters;
    adev->hw_device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->hw_device.open_output_stream = adev_open_output_stream;
    adev->hw_device.close_output_stream = adev_close_output_stream;
    adev->hw_device.open_input_stream = adev_open_input_stream;
    adev->hw_device.close_input_stream = adev_close_input_stream;
    adev->hw_device.dump = adev_dump;
    adev->realCall = false;
    //init ext_contrl
#ifdef AUDIO_DEBUG
    adev->ext_contrl = (ext_contrl_t*)malloc(sizeof(ext_contrl_t));
    memset(adev->ext_contrl, 0, sizeof(ext_contrl_t));
    adev->ext_contrl->dump_info = (dump_info_t*)malloc(sizeof(dump_info_t));
    adev->ext_contrl->dump_info->out_music = (out_dump_t*)malloc(sizeof(out_dump_t));
    adev->ext_contrl->dump_info->out_voip  = (out_dump_t*)malloc(sizeof(out_dump_t));
    adev->ext_contrl->dump_info->out_bt_sco  = (out_dump_t*)malloc(sizeof(out_dump_t));
    adev->ext_contrl->dump_info->out_vaudio  = (out_dump_t*)malloc(sizeof(out_dump_t));
    adev->ext_contrl->dump_info->in_read = (out_dump_t*)malloc(sizeof(out_dump_t));
    adev->ext_contrl->dump_info->in_read_afterprocess = (out_dump_t*)malloc(sizeof(out_dump_t));
    adev->ext_contrl->dump_info->in_read_aftersrc = (out_dump_t*)malloc(sizeof(out_dump_t));
    adev->ext_contrl->dump_info->in_read_vbc = (out_dump_t*)malloc(sizeof(out_dump_t));
    adev->ext_contrl->dump_info->in_read_afternr = (out_dump_t*)malloc(sizeof(out_dump_t));

    adev->ext_contrl->dump_info->dump_to_cache = false;
    adev->ext_contrl->dump_info->dump_as_wav = false;
    adev->ext_contrl->dump_info->dump_music = false;

    adev->ext_contrl->dump_info->dump_bt_sco = false;
    adev->ext_contrl->dump_info->dump_voip = false;
    adev->ext_contrl->dump_info->dump_vaudio = false;
    adev->ext_contrl->dump_info->dump_in_read = false;
    adev->ext_contrl->dump_info->dump_in_read_afterprocess = false;
    adev->ext_contrl->dump_info->dump_in_read_aftersrc = false;
    adev->ext_contrl->dump_info->dump_in_read_vbc = false;
    adev->ext_contrl->dump_info->dump_in_read_afternr = false;

    adev->ext_contrl->dump_info->out_bt_sco->buffer_length = DefaultBufferLength;
    adev->ext_contrl->dump_info->out_voip->buffer_length = DefaultBufferLength;
    adev->ext_contrl->dump_info->out_vaudio->buffer_length = DefaultBufferLength;
    adev->ext_contrl->dump_info->out_music->buffer_length = DefaultBufferLength;
    adev->ext_contrl->dump_info->in_read->buffer_length = DefaultBufferLength;
    adev->ext_contrl->dump_info->in_read_afterprocess->buffer_length = DefaultBufferLength;
    adev->ext_contrl->dump_info->in_read_aftersrc->buffer_length = DefaultBufferLength;
    adev->ext_contrl->dump_info->in_read_vbc->buffer_length = DefaultBufferLength;
    adev->ext_contrl->dump_info->in_read_afternr->buffer_length = DefaultBufferLength;

#endif

    adev->nr_mask=0;
    adev->enable_other_rate_nr= false;

    adev->deepbuffer_enable=false;
    adev->active_deepbuffer_output=NULL;

    pthread_mutex_lock(&adev->lock);
    ret = adev_modem_parse(adev);
    pthread_mutex_unlock(&adev->lock);
    if (ret < 0) {
        ALOGE("Warning:Unable to locate all audio modem parameters from XML.");
    }
    /* get audio para from audio_para.txt*/
    vb_effect_getpara(adev);
    vb_effect_setpara(adev->audio_para);
    /* query sound cards*/
    s_tinycard = get_snd_card_number(CARD_SPRDPHONE);
    s_vaudio = get_snd_card_number(CARD_VAUDIO);
    s_voip = get_snd_card_number(CARD_SCO);
    s_bt_sco = get_snd_card_number(CARD_BT_SCO);
    s_vaudio_w = get_snd_card_number(CARD_VAUDIO_W);
    s_vaudio_lte = get_snd_card_number(CARD_VAUDIO_LTE);

    ALOGI("s_tinycard = %d, s_vaudio = %d,s_voip = %d, s_bt_sco = %d,s_vaudio_w is %d, s_vaudio_lte is %d",
            s_tinycard,s_vaudio,s_voip,s_bt_sco,s_vaudio_w, s_vaudio_lte);
    if (s_tinycard < 0 && s_vaudio < 0&&(s_voip < 0 ) && (s_vaudio_w < 0)&&(s_bt_sco < 0) &&(s_vaudio_lte < 0)) {
        ALOGE("Unable to load sound card, aborting.");
        goto ERROR;
    }
    adev->mixer = mixer_open(s_tinycard);
    if (!adev->mixer) {
        ALOGE("Unable to open the mixer, aborting.");
        goto ERROR;
    }
    adev->vbc_access = mixer_get_ctl_by_name(adev->mixer,"vbc_access_en");
    if (adev->vbc_access == NULL) {
        ALOGE("warning no vbc_access_en control");
        adev->vbc_access = NULL;
    }
    if (adev->vbc_access)
        mixer_ctl_set_value(adev->vbc_access, 0, 0);
    /* parse mixer ctl */
    ret = adev_config_parse(adev);
    if (ret < 0) {
        ALOGE("Unable to locate all mixer controls from XML, aborting.");
        goto ERROR;
    }
    BLUE_TRACE("ret=%d, num_dev_cfgs=%d", ret, adev->num_dev_cfgs);
    BLUE_TRACE("dev_cfgs_on depth=%d, dev_cfgs_off depth=%d", adev->dev_cfgs->on_len,  adev->dev_cfgs->off_len);

#ifdef VB_CONTROL_PARAMETER_V2
    /* parse mixer ctl */
    ret = adev_config_parse_linein(adev);
    if (ret < 0) {
        ALOGE("Unable to locate all mixer controls from XML, aborting.");
        goto ERROR;
    }
    BLUE_TRACE("ret=%d, num_dev_cfgs=%d", ret, adev->num_dev_linein_cfgs);
    BLUE_TRACE("dev_cfgs_on depth=%d, dev_cfgs_off depth=%d", adev->dev_linein_cfgs->on_len,  adev->dev_linein_cfgs->off_len);
#endif

    ret = dump_parse_xml();
    if (ret < 0) {
        ALOGE("Unable to locate dump information  from XML, aborting.");
        goto ERROR;
    }
    set_audio_mode_num(adev_get_audiomodenum4eng());

    /* generate eq params file of vbc effect*/
    adev->eq_available = false;
    ret = create_vb_effect_params();
    if (ret != 0) {
        ALOGW("Warning: Failed to create the parameters file of vbc_eq");
    } else {
        get_partial_wakeLock();
        ret = mixer_ctl_set_enum_by_string(adev->private_ctl.vbc_eq_update, "loading");
        release_wakeLock();
        if (ret == 0) adev->eq_available = true;
        ALOGI("eq_loading, ret(%d), eq_available(%d)", ret, adev->eq_available);
    }
    if (adev->eq_available) {
        vb_effect_config_mixer_ctl(adev->private_ctl.vbc_eq_update, adev->private_ctl.vbc_eq_profile_select);
        vb_da_effect_config_mixer_ctl(adev->private_ctl.vbc_da_eq_profile_select);
        vb_ad_effect_config_mixer_ctl(adev->private_ctl.vbc_ad01_eq_profile_select, adev->private_ctl.vbc_ad23_eq_profile_select);
        aud_vb_effect_start(adev);
    }
    /*Parse PGA*/
    adev->pga = audio_pga_init(adev->mixer);
    if (!adev->pga) {
        ALOGE("Warning: Unable to locate PGA from XML.");
    }
    adev->pga_gain_nv = calloc(1, sizeof(pga_gain_nv_t));
    if (0==adev->pga_gain_nv) {
        ALOGW("Warning: Failed to create the parameters file of pga_gain_nv");
        goto ERROR;
    }

    /*Custom MMI*/
    adev->mmiHandle = AudioMMIInit();
    if (NULL == adev->mmiHandle) {
        ALOGW("Warning: Failed to AudioMMIInit");
    }

    /* Set the default route before the PCM stream is opened */
    pthread_mutex_lock(&adev->lock);
    adev->mode = AUDIO_MODE_NORMAL;
    adev->out_devices = AUDIO_DEVICE_OUT_SPEAKER;
    adev->in_devices = 0;

    adev->pcm_modem_dl = NULL;
    adev->pcm_modem_ul = NULL;
    adev->call_start = 0;
    adev->call_connected = 0;
    adev->call_prestop = 0;
    adev->voice_volume = 1.0f;
    adev->bluetooth_nrec = false;
    //init fm status
    adev->fm_open = false;
    adev->fm_record = false;
    adev->fm_uldl = false;
    adev->fm_volume = -1;
    adev->i2sfm_flag = 0;
    adev->mic_mute = false;
    adev->fm_mute = false;
    adev->bluetooth_type = VX_NB_SAMPLING_RATE;
    adev->dump_flag=0;
    adev->record_bypass = true;

    adev->input_source = 0;
    list_init(&adev->active_out_list);
    mixer_ctl_set_value(adev->private_ctl.vbc_switch, 0, VBC_ARM_CHANNELID);  //switch to arm
    adev->vbc_2arm = mixer_ctl_get_value(adev->private_ctl.vbc_switch,0);
    pthread_mutex_unlock(&adev->lock);

    *device = &adev->hw_device.common;
    /* Create a task to get vbpipe message from cp when voice-call */
    if (!s_is_active) {
        ret = vbc_ctrl_open(adev);
        if (ret < 0) {
            ALOGE("vbc_ctrl_open is fail! ret = %d", ret);
            goto ERROR;
        }
    }
    adev->cp->voip_res.adev = adev;
#ifdef VOIP_DSP_PROCESS
    ret = vbc_ctrl_voip_open(&(adev->cp->voip_res));
    //ret = 0;//vbc_ctrl_voip_open(&(adev->cp->voip_res));
    if (ret < 0) {
    ALOGE("voip: vbc_ctrl_voip_open error ");
    goto ERROR;
    }
#endif

vb_ctl_modem_monitor_open (adev);

/*
this is used to loopback test.
*/
#ifdef AUDIO_DEBUG
    ret = ext_control_open(adev);
    if (ret)  ALOGW("Warning: audio ext_contrl can NOT work.");
#endif
    ret =audiopara_tuning_manager_create(adev);
    if (ret)  ALOGW("Warning: audio tuning can NOT work.");

    ret = stream_routing_manager_create(adev);
    if (ret) {
        ALOGE("Unable to create stream_routing_manager, aborting.");
        goto ERROR;
    }

    ret = audio_bt_sco_thread_create(adev);
    if (ret) {
        ALOGE("bt sco : Unable to create audio_bt_sco_thread_create, aborting.");
        goto ERROR;
    }

    ret = voice_command_manager_create(adev);
    if (ret) {
        ALOGE("Unable to create voice_command_manager_create, aborting.");
        goto ERROR;
    }
#ifdef NXP_SMART_PA
    adev->smart_pa_exist = get_ext_smartpa_exist();
    if (adev->smart_pa_exist) {
        adev->fmRes = NxpTfa_Fm_Open();
        adev->callHandle =NULL;
    }

    adev->i2s_mixer = mixer_open(s_bt_sco);
    if (!adev->i2s_mixer) {
        ALOGE("Unable to open the i2s mixer,s aborting.");
        goto ERROR;
    }
    if(adev->smart_pa_exist){
    	get_partial_wakeLock();
    	ALOGD("%d %s,start to tfa_firmware_load ",__LINE__, __func__);
    	adev->private_ctl.tfa_fw_load = mixer_get_ctl_by_name(adev->i2s_mixer,"TFA Firmware Load");
    	if (adev->private_ctl.tfa_fw_load != NULL) {
        	ret = mixer_ctl_set_enum_by_string(adev->private_ctl.tfa_fw_load, "loading");
        	ALOGE("%s tfa_firmware_load ok ret %d", __func__, ret);
    }
    	release_wakeLock();
    }
#endif
    load_fm_volume(adev);

    adev->cp_mixer[CP_TG] = mixer_open(s_vaudio);
    adev->cp_mixer[CP_W] = mixer_open(s_vaudio_w);
    adev->cp_mixer[CP_CSFB] = mixer_open(s_vaudio_lte);
    adev->record_tone_info = cp_mixer_open();

LOG_I("adev_open:cp_mixer TG:%p W:%p CSFB:%p",adev->cp_mixer[CP_TG],adev->cp_mixer[CP_W],adev->cp_mixer[CP_CSFB]);

    for (i = 0; i < I2S_CTL_INF; i++) {
        if(adev->cp->i2s_switch_btcall[i] != NULL){
            if((NULL!=adev->cp->i2s_switch_btcall[i]->ctl_name) && (NULL!=adev->cp->i2s_switch_btcall[i]->strval)){
                ALOGI("i2s_switch_sel:BT_CALL %s %s",adev->cp->i2s_switch_btcall[i]->ctl_name,adev->cp->i2s_switch_btcall[i]->strval);
            }
        }
    }
    for (i = 0; i < I2S_CTL_INF; i++) {
        if(adev->cp->i2s_switch_fm[i] != NULL) {
            if((NULL!=adev->cp->i2s_switch_fm[i]->ctl_name) && (NULL!=adev->cp->i2s_switch_fm[i]->strval)) {
                ALOGI("i2s_switch_sel:INNER_FM %s %s",adev->cp->i2s_switch_fm[i]->ctl_name,adev->cp->i2s_switch_fm[i]->strval);
            }
        }
    }

    ret = MP3_ARM_DEC_init();
    ALOGE("MP3_ARM_DEC_init ret :%d",ret);
    adev->misc_ctl = aud_misc_ctl_open();
    ALOGI("mic_default_channel:%d nr_mask:0x%x %d",
        adev->cp->mic_default_channel,adev->nr_mask,adev->enable_other_rate_nr);
    audio_device_ref_count++;
    /* vbc ad01 iis select to dfm default */
    vbc_ad01iis_select(adev, true);
    ALOGI("adev_open adev = 0x%p", adev);
    pthread_mutex_unlock(&adev_init_lock);
    if(adev->smart_pa_exist){
#ifdef NXP_SMART_PA
    	ALOGI("Start setprop");
    	if(property_set("media.dump.switch", "0x80000000")){
		ALOGW("setprop media.dump.switch failed");
    	}
    	ALOGI("ADEV_OPEN EXIT");

#ifdef NXP_SMART_PA_RE_CALIB
    	i2s_pin_mux_sel_new(adev, EXT_CODEC, EXT_CODEC_AP);
    	NXPTfa_calibration(&NxpTfa9890_Info,adev->i2s_mixer);
#endif
#endif
	}

    return 0;

ERROR:
    if (adev->pga)    audio_pga_free(adev->pga);
    if (adev->mixer)  mixer_close(adev->mixer);
    if (adev->audio_para)  free(adev->audio_para);
    if (adev->pga_gain_nv) free(adev->pga_gain_nv);
    if (adev) free(adev);
    pthread_mutex_unlock(&adev_init_lock);
    return -EINVAL;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "Spreadtrum Audio HW HAL",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};
