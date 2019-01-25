#ifndef AUDIO_OFFLOAD_H
#define AUDIO_OFFLOAD_H

#include <stdbool.h>
#include "cutils/list.h"
#include "sys/resource.h"
#include <system/thread_defs.h>
#include <cutils/sched_policy.h>
#include <sys/prctl.h>
#include "tinycompress/tinycompress.h"
#include "sound/compress_params.h"
#include <hardware/audio.h>
#include <system/audio.h>


/* Flags used to indicate current application type */
typedef enum {
    AUDIO_HW_APP_INVALID = -1,
    AUDIO_HW_APP_PRIMARY = 0,
    AUDIO_HW_APP_OFFLOAD
}AUDIO_HW_APP_T;

typedef enum {
    AUDIO_OUTPUT_DESC_NONE = 0,
    AUDIO_OUTPUT_DESC_PRIMARY = 0x1,
    AUDIO_OUTPUT_DESC_OFFLOAD = 0x10
}AUDIO_OUTPUT_DESC_T;

typedef enum {
    AUDIO_OFFLOAD_CMD_EXIT,               /* exit compress offload thread loop*/
    AUDIO_OFFLOAD_CMD_DRAIN,              /* send a full drain request to driver */
    AUDIO_OFFLOAD_CMD_PARTIAL_DRAIN,      /* send a partial drain request to driver */
    AUDIO_OFFLOAD_CMD_WAIT_FOR_BUFFER    /* wait for buffer released by driver */
}AUDIO_OFFLOAD_CMD_T;

/* Flags used to indicate current offload playback state */
typedef enum {
    AUDIO_OFFLOAD_STATE_STOPED,
    AUDIO_OFFLOAD_STATE_PLAYING,
    AUDIO_OFFLOAD_STATE_PAUSED
}AUDIO_OFFLOAD_STATE_T;

typedef enum {
    AUDIO_OFFLOAD_MIXER_INVALID = -1,
    AUDIO_OFFLOAD_MIXER_CARD0 = 0,      //ap main
    AUDIO_OFFLOAD_MIXER_CARD1,          //ap compress
    AUDIO_OFFLOAD_MIXER_CARD2          //cp compress-mixer
}AUDIO_OFFLOAD_MIXER_CARD_T;

struct audio_offload_cmd {
    struct listnode node;
    AUDIO_OFFLOAD_CMD_T cmd;
    int data[];
};

#define AUDIO_OFFLOAD_FRAGMENT_SIZE (64 * 1024)
#define AUDIO_OFFLOAD_FRAGMENTS_NUM 15
#define AUDIO_OFFLOAD_PLAYBACK_LATENCY 60000
#define AUDIO_OFFLOAD_PLAYBACK_VOLUME_MAX 0x1000
#define OFFLOAD_CARD 3



#endif // AUDIO_OFFLOAD_H

