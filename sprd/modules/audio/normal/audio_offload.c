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
#define LOG_TAG "audio_hw_offload"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>
#include <expat.h>


#include "audio_offload.h"

struct pcm_config pcm_config_mm_ffloadbuf = {
    .channels = 2,
    .rate = DEFAULT_OUT_SAMPLING_RATE,
     /* driver ddr max is 228 *1024
        * period_size*period_count*btyes per channl* channe can not
        * larger than driver ddr max.
         * 0x2580*6*4= 230400 < 228*1024=233472(0x3900)
         */
    .period_size = 0x2580,
    .period_count = 6,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = 0x2580,
    .avail_min = 0x2580 * 4,
};

struct pcm_config pcm_config_scoplayback1 = {
    .channels = 1,
#ifndef AUDIO_OLD_MODEM
    .rate = VX_WB_SAMPLING_RATE,
    .period_size = 640,
#else
    .rate = VX_NB_SAMPLING_RATE,
    .period_size = 320,
#endif
    .period_count = 5,
    .format = PCM_FORMAT_S16_LE,
};

extern int32_t compress_set_gain(struct compress *compress,long left_vol, long right_vol);
extern int32_t compress_set_dev(struct compress *compress,unsigned int card,
        unsigned int device, struct pcm_config  *config,void * mixerctl);
extern int compress_resume_async(struct compress *compress);

ssize_t audio_offload_mute(struct tiny_stream_out *out)
{
    if(out->offload_volume_ctl) {
        long volume[2];
        volume[0] = out->left_volume;
        volume[1] = out->right_volume;
        while(volume[0] || volume[1]){
            if(volume[0] > 100) {
                volume[0] -= 100;
            }
            else {
                volume[0] = 0;
            }

            if(volume[1] > 100) {
                volume[1] -= 100;
            }
            else {
                volume[1] = 0;
            }
            mixer_ctl_set_array(out->offload_volume_ctl, volume, sizeof(volume)/sizeof(volume[0]));
            usleep(10000);
        }
    }
    return 0;
}


static ssize_t audio_offload_unmute(struct tiny_stream_out *out)
{
    if(out->offload_volume_ctl) {
        long volume[2];
        volume[0] = out->left_volume ;
        volume[1] = out->right_volume ;
        mixer_ctl_set_array(out->offload_volume_ctl, volume, sizeof(volume)/sizeof(volume[0]));
    }
    return 0;
}


static int audio_offload_get_dev_config(struct tiny_audio_device *adev,uint32_t *pcard,uint32_t *pdevice, struct pcm_config *pcm_cfg)
{
    unsigned int card = -1;
    unsigned int port = PORT_MM;
    int ret=0;
    int cp_type;
    
    if(adev->call_start == 1) {
        cp_type = adev->cp_type;
        if(cp_type == CP_TG) {
    	    card = get_snd_card_number(CARD_VAUDIO);
        }
        else if (cp_type == CP_W) {
    	    card = get_snd_card_number(CARD_VAUDIO_W);
        }
        else if (cp_type == CP_CSFB) {
            card = get_snd_card_number(CARD_VAUDIO_LTE);
        }
        *pdevice = 1;
        memcpy(pcm_cfg,&pcm_config_vplayback,sizeof(struct pcm_config));
    }
    else if(adev->voip_state) {
        card = get_snd_card_number(CARD_SCO);
        *pdevice = 1;
        memcpy(pcm_cfg,&pcm_config_scoplayback1,sizeof(struct pcm_config));
    }
    else {
        card = get_snd_card_number(CARD_SPRDPHONE);
        *pdevice = PORT_DEPP_BUFFER;
        memcpy(pcm_cfg,&pcm_config_mm_ffloadbuf,sizeof(struct pcm_config));
    }
    ALOGE("%s,card: %d, port: %d,adev->call_start: %d,adev->voip_state: %d,p_c:%d,p_size: %d,reate:%d,channel:%d",__func__,
    card,*pdevice,adev->call_start,adev->voip_state,pcm_cfg->period_count,pcm_cfg->period_size,pcm_cfg->rate,pcm_cfg->channels);
    if(card != -1) {
        *pcard = card;
        return ret;
    }
    else {
        return -1;
    }
}


//#define OFFLOAD_DEBUG
static bool audio_is_offload_support_format(audio_format_t format)
{
    if (format == AUDIO_FORMAT_MP3 ||
            format == AUDIO_FORMAT_AAC) {
        return true;
    }
    return false;
}

static int audio_get_offload_codec_id(audio_format_t format)
{
    int id = 0;

    switch (format) {
    case AUDIO_FORMAT_MP3:
        id = SND_AUDIOCODEC_MP3;
        break;
    default:
        ALOGE("%s: audio format (%d) is not supported ", __func__,format);
    }

    return id;
}

static int audio_start_compress_output(struct tiny_stream_out *out)
{
    int ret = 0;
    unsigned int port = PORT_DEPP_BUFFER;
 
    uint32_t card = 0;
    uint32_t device = 0;
    struct pcm_config pcm_cfg = {0};
    struct tiny_audio_device *adev = out->dev;
    out->pcm = NULL;    /*set pcm to NULL to avoid pcm_write after starting offload-playback*/
    ALOGE("%s: vbc _switch is %d", __func__, mixer_ctl_get_value(adev->private_ctl.vbc_switch,0));
    if(out->compress) {
        ALOGE("%s: compres already openned  is_offload_running: %d",__func__,adev->is_offload_running);
        return 0;
    }
    if(adev->is_offload_running == 1) {
        ALOGE("%s: error is_offload_running: %d",__func__,adev->is_offload_running);
        goto error;
    }
    out->compress = compress_open(s_tinycard, port,
                               COMPRESS_IN, &out->compress_config);
    if (!out->compress) {
        ALOGE("%s:, return ", __func__);
        return -1;
    }
    ret = audio_offload_get_dev_config(adev,&card,&device, &pcm_cfg);
    if(ret) {
        ALOGE(" %s error audio_offload_get_dev_config ret: %d",__func__,ret);
        goto error;
    }
    ret = compress_set_dev(out->compress,card, device,&pcm_cfg,adev->mixer);
    
    if (out->audio_offload_callback)
        compress_nonblock(out->compress, out->is_offload_nonblocking);

    if(adev->master_mute && (out->left_volume  || out->right_volume)){
	    adev->master_mute = false;
    }
    compress_set_gain(out->compress,out->left_volume, out->right_volume);
    select_devices_signal(adev);
    adev->is_offload_running = 1;
    adev->active_offload_output = out;
    ALOGE("%s: successfully, out ", __func__);
    return 0;
error:
    ALOGE("%s: error is_offload_running: %d",__func__,adev->is_offload_running);
    if(out->compress) {
        compress_close(out->compress);
        out->compress = NULL;
    }

    return -1;
    
}

static void audio_stop_compress_output(struct tiny_stream_out *out,int is_for_flush)
{
    struct tiny_audio_device *adev = out->dev;
    ALOGE("%s in, audio_app_type:%d, audio_offload_state:%d ",
            __func__, out->audio_app_type, out->audio_offload_state);
     ALOGE("%s in",__FUNCTION__);
    out->audio_offload_state = AUDIO_OFFLOAD_STATE_STOPED;
    out->is_offload_compress_started = false;
    out->is_offload_need_set_metadata = true;  /* need to set metadata to driver next time */
    if (out->compress != NULL) {
        //audio_offload_mute(out);
        compress_stop(out->compress);
        /* wait for finishing processing the command */
        while (out->is_audio_offload_thread_blocked) {
            pthread_cond_wait(&out->audio_offload_cond, &out->lock);
        }
        out->is_paused_by_hal =0;
    }
    if((is_for_flush == 0) && (adev->active_offload_output == out)) {
        adev->is_offload_running = 0;
    }
     ALOGE("%s out",__FUNCTION__);
}

static int audio_get_compress_metadata(struct tiny_stream_out *out, struct str_parms *parms)
{
    int ret = 0;
    char value[32];
    struct compr_gapless_mdata tmp_compress_metadata;

    if (!out || !parms) {
        return -1;
    }
    /* get meta data from audio framework */
    ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_DELAY_SAMPLES, value, sizeof(value));
    if (ret >= 0) {
        tmp_compress_metadata.encoder_delay = atoi(value);
    } else {
        return -1;
    }

    ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_PADDING_SAMPLES, value, sizeof(value));
    if (ret >= 0) {
        tmp_compress_metadata.encoder_padding = atoi(value);
    } else {
        return -1;
    }

    out->gapless_mdata = tmp_compress_metadata;
    out->is_offload_need_set_metadata = true;
    ALOGE("%s successfully, new encoder_delay: %u, encoder_padding: %u ", 
        __func__, out->gapless_mdata.encoder_delay, out->gapless_mdata.encoder_padding);

    return 0;
}

static int audio_send_offload_cmd(struct tiny_stream_out* out, AUDIO_OFFLOAD_CMD_T command)
{
    struct audio_offload_cmd *cmd = (struct audio_offload_cmd *)calloc(1, sizeof(struct audio_offload_cmd));

    ALOGE("%s, cmd:%d, offload_state:%d ",
        __func__, command, out->audio_offload_state);
    /* add this command to list, then send signal to offload thread to process the command list */
    cmd->cmd = command;
    list_add_tail(&out->audio_offload_cmd_list, &cmd->node);
    pthread_cond_signal(&out->audio_offload_cond);
    return 0;
}

static void *audio_offload_thread_loop(void *param)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *) param;
    struct listnode *item;
    /* init the offload state */
    out->audio_offload_state = AUDIO_OFFLOAD_STATE_STOPED;
    out->is_offload_compress_started = false;

    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_AUDIO);
    set_sched_policy(0, SP_FOREGROUND);
    prctl(PR_SET_NAME, (unsigned long)"Audio Offload Thread", 0, 0, 0);

    ALOGE("%s in", __func__);
    pthread_mutex_lock(&out->lock);
    for (;;) {
        bool need_send_callback = false;
        struct audio_offload_cmd *cmd = NULL;
        stream_callback_event_t event;

        ALOGE("%s, offload_cmd_list:%d, offload_state:%d",
              __func__, list_empty(&out->audio_offload_cmd_list), out->audio_offload_state);

        /*
           If the command list is not empty, don't need to wait for new, just process it.
           Otherwise, wait for new command.
         */
        if (list_empty(&out->audio_offload_cmd_list)) {
            ALOGE("%s: Waiting for signal ", __func__);
            pthread_cond_wait(&out->audio_offload_cond, &out->lock);
            ALOGE("%s: Get signal ", __func__);
            continue;
        }
        /* get the command from the list, then process the command */
        item = list_head(&out->audio_offload_cmd_list);
        cmd = node_to_item(item, struct audio_offload_cmd, node);
        list_remove(item);

        ALOGE("%s, offload_state:%d, offload_cmd:%d, out->compress:%p ",
               __func__, out->audio_offload_state, cmd->cmd, out->compress);

        if (cmd->cmd == AUDIO_OFFLOAD_CMD_EXIT) {
            free(cmd);
            break;
        }

        if (out->compress == NULL) {
            ALOGE("%s: Compress handle is NULL", __func__);
            pthread_cond_signal(&out->audio_offload_cond);
            continue;
        }

        out->is_audio_offload_thread_blocked = true;
        pthread_mutex_unlock(&out->lock);
        need_send_callback = false;
        switch(cmd->cmd) {
        case AUDIO_OFFLOAD_CMD_WAIT_FOR_BUFFER:
            compress_wait(out->compress, -1);
            need_send_callback = true;
            event = STREAM_CBK_EVENT_WRITE_READY;
            break;
        case AUDIO_OFFLOAD_CMD_PARTIAL_DRAIN:
		ALOGE("keokeo before AUDIO_OFFLOAD_CMD_PARTIAL_DRAIN");
            compress_next_track(out->compress);
            compress_partial_drain(out->compress);
		ALOGE("keokeo after AUDIO_OFFLOAD_CMD_PARTIAL_DRAIN");
            need_send_callback = true;
            event = STREAM_CBK_EVENT_DRAIN_READY;
            break;
        case AUDIO_OFFLOAD_CMD_DRAIN:
		ALOGE("keokeo before AUDIO_OFFLOAD_CMD_DRAIN");
            compress_drain(out->compress);
		ALOGE("keokeo after AUDIO_OFFLOAD_CMD_DRAIN");
            need_send_callback = true;
            event = STREAM_CBK_EVENT_DRAIN_READY;
            break;
        default:
            ALOGE("%s unknown command received: %d", __func__, cmd->cmd);
            break;
        }
        pthread_mutex_lock(&out->lock);
        out->is_audio_offload_thread_blocked = false;
        /* send finish processing signal to awaken where is waiting for this information */
        pthread_cond_signal(&out->audio_offload_cond);
        if (need_send_callback && out->audio_offload_callback) {
            out->audio_offload_callback(event, NULL, out->audio_offload_cookie);
        }
        free(cmd);
    }

    pthread_cond_signal(&out->audio_offload_cond);
    /* offload thread loop exit, free the command list */
    while (!list_empty(&out->audio_offload_cmd_list)) {
        item = list_head(&out->audio_offload_cmd_list);
        list_remove(item);
        free(node_to_item(item, struct audio_offload_cmd, node));
    }
    pthread_mutex_unlock(&out->lock);

    return NULL;
}


static int audio_offload_create_thread(struct tiny_stream_out *out)
{
    pthread_cond_init(&out->audio_offload_cond, (const pthread_condattr_t *) NULL);
    list_init(&out->audio_offload_cmd_list);
    pthread_create(&out->audio_offload_thread, (const pthread_attr_t *) NULL,
                    audio_offload_thread_loop, out);
    ALOGE("%s, successful, id:%lu ",__func__, out->audio_offload_thread);
    return 0;
}

static int audio_offload_destroy_thread(struct tiny_stream_out *out)
{
    pthread_mutex_lock(&out->lock);
    if (out->compress != NULL)
        audio_offload_mute(out);
    audio_stop_compress_output(out, 0);
    /* send command to exit the thread_loop */
    audio_send_offload_cmd(out, AUDIO_OFFLOAD_CMD_EXIT);
    pthread_mutex_unlock(&out->lock);
    pthread_join(out->audio_offload_thread, (void **) NULL);
    pthread_cond_destroy(&out->audio_offload_cond);
    ALOGE("%s, successful ",__func__);
    return 0;
}

 int audio_offload_pause(struct tiny_audio_device *adev)
{
    int status = 0;
    struct tiny_stream_out *out = NULL;
    struct tiny_stream_out *out_offload = NULL;
    struct listnode *item;
    struct listnode *item2;
    
   
    if(!list_empty(&adev->active_out_list)){
        list_for_each_safe(item, item2,&adev->active_out_list){
            out = node_to_item(item, struct tiny_stream_out, node);
            if((out->audio_app_type == AUDIO_HW_APP_OFFLOAD)
                && (out->audio_offload_state == AUDIO_OFFLOAD_STATE_PLAYING)
                &&(out->is_paused_by_hal == 0)){
                out_offload = out;
                break;
            }
        }
    }
    if(out_offload == NULL) {
        //ALOGE("%s out, out offload output",__FUNCTION__);
        return 0;
    }
   
   ALOGE("%s in, audio_app_type:%d, audio_offload_state:%d ", 
                 __func__, out_offload->audio_app_type, out_offload->audio_offload_state);
                 
    pthread_mutex_lock(&out_offload->lock);
    if (out_offload->compress != NULL && out_offload->audio_offload_state == AUDIO_OFFLOAD_STATE_PLAYING) {
        status = compress_pause(out_offload->compress);
        out_offload->is_paused_by_hal = 1;
    }
    pthread_mutex_unlock(&out_offload->lock);


     ALOGE("%s out",__FUNCTION__);
    return status;
}




 int audio_offload_resume(struct tiny_audio_device *adev)
{
    struct tiny_stream_out *out =  NULL;
    struct tiny_stream_out *out_offload = NULL;
    int ret = 0;
    struct pcm_config pcm_cfg = {0};
    uint32_t card = 0;
    uint32_t device = 0;
    

    struct listnode *item;
    struct listnode *item2;

    if(!list_empty(&adev->active_out_list)){
        list_for_each_safe(item, item2,&adev->active_out_list){
            out = node_to_item(item, struct tiny_stream_out, node);
            if((out->audio_app_type == AUDIO_HW_APP_OFFLOAD)
                &&(out->is_paused_by_hal == 1)){
                out_offload = out;
                break;
            }
        }
    }
    if(out_offload == NULL) {
       // ALOGE("%s out, out offload output",__FUNCTION__);
        return 0;
    } 

    out_offload->is_paused_by_hal =0;

    ALOGE("%s in, audio_app_type:%d, audio_offload_state:%d ",
    __func__, out_offload->audio_app_type, out_offload->audio_offload_state);
    
  //  adev->prev_out_devices = ~adev->out_devices;
   // select_devices_signal(adev);

    pthread_mutex_lock(&out_offload->lock);
    if (out_offload->compress != NULL && out_offload->audio_offload_state == AUDIO_OFFLOAD_STATE_PLAYING) {
        ret = audio_offload_get_dev_config(adev,&card,&device, &pcm_cfg);
        if(ret) {
            ALOGE(" %s error audio_offload_get_dev_config ret: %d",__func__,ret);
        }
        ret = compress_set_dev(out_offload->compress,card, device,&pcm_cfg,adev->mixer);
        ret = compress_resume_async(out_offload->compress);
    }
    pthread_mutex_unlock(&out_offload->lock);

    if(adev->master_mute && (out->left_volume  || out->right_volume)){
	    adev->master_mute = false;
    }
    select_devices_signal(adev);
     ALOGE("%s out",__FUNCTION__);
    return ret;
}



void dump_pcm(const void* pBuffer, size_t aInBufSize)
{
    FILE *fp = fopen("/data/local/media/leodump.wav","ab");
    fwrite(pBuffer,1,aInBufSize,fp);
    fclose(fp);
}

static ssize_t out_write_compress(struct tiny_stream_out *out, const void* buffer,
        size_t bytes)
{
    int ret = 0;
    struct tiny_audio_device *adev = out->dev;
    ALOGE("%s: want to write buffer (%d bytes) to compress device, offload_state:%d ",
        __func__, bytes, out->audio_offload_state);
    if(out->compress == NULL) {
        ALOGE("%s: out->compress == NULL",__func__);
        return 0;
    }

    if (out->is_offload_need_set_metadata) {
        ALOGE("%s: need to send new metadata to driver ",__func__);
        compress_set_gapless_metadata(out->compress, &out->gapless_mdata);
        out->is_offload_need_set_metadata = 0;
    }

    //dump_pcm(buffer, bytes);
    ret = compress_write(out->compress, buffer, bytes);
    ALOGE("%s: finish writing buffer (%d bytes) to compress device, and return %d",
            __func__, bytes, ret);
    /* neet to wait for ring buffer to ready for next read or write */
    if (ret >= 0 && ret < (ssize_t)bytes) {
        audio_send_offload_cmd(out, AUDIO_OFFLOAD_CMD_WAIT_FOR_BUFFER);
    }
    if (!out->is_offload_compress_started) {
        compress_start(out->compress);
        audio_offload_unmute(out);
        out->is_offload_compress_started = true;
        adev->is_offload_running = 1;
        out->audio_offload_state = AUDIO_OFFLOAD_STATE_PLAYING;
    }
    return ret;
}

static int out_offload_set_callback(struct audio_stream_out *stream,
            stream_callback_t callback, void *cookie)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    ALOGE("%s in, audio_app_type:%d ", __func__, out->audio_app_type);
    pthread_mutex_lock(&out->lock);
    out->audio_offload_callback = callback;
    out->audio_offload_cookie = cookie;
    pthread_mutex_unlock(&out->lock);
    return 0;
}

static int out_offload_pause(struct audio_stream_out* stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    int status = 0;
    ALOGE("%s in, audio_app_type:%d, audio_offload_state:%d ", 
            __func__, out->audio_app_type, out->audio_offload_state);

    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        pthread_mutex_lock(&out->lock);
        if (out->compress != NULL && out->audio_offload_state == AUDIO_OFFLOAD_STATE_PLAYING) {
            audio_offload_mute(out);
            status = compress_pause(out->compress);
            out->audio_offload_state = AUDIO_OFFLOAD_STATE_PAUSED;
        }
        pthread_mutex_unlock(&out->lock);
    }

     ALOGE("%s out",__FUNCTION__);
    return status;
}

static int out_offload_resume(struct audio_stream_out* stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    struct pcm_config pcm_cfg = {0};
    uint32_t card = 0;
    uint32_t device = 0;

    int ret = 0;
    ALOGE("%s in, audio_app_type:%d, audio_offload_state:%d ",
        __func__, out->audio_app_type, out->audio_offload_state);
    pthread_mutex_lock(&adev->lock);     
    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        ret = 0;
        pthread_mutex_lock(&out->lock);
        if (out->compress != NULL && out->audio_offload_state == AUDIO_OFFLOAD_STATE_PAUSED) {
            ret = audio_offload_get_dev_config(adev,&card,&device, &pcm_cfg);
            if(ret) {
                ALOGE(" %s error audio_offload_get_dev_config ret: %d",__func__,ret);
            }
            ret = compress_set_dev(out->compress,card, device,&pcm_cfg,adev->mixer);
            audio_offload_unmute(out);
            ret = compress_resume(out->compress);
            out->audio_offload_state = AUDIO_OFFLOAD_STATE_PLAYING;
        }
        pthread_mutex_unlock(&out->lock);
    }
     pthread_mutex_unlock(&adev->lock);  
         adev->prev_out_devices = ~adev->out_devices;
        select_devices_signal(adev);
     ALOGE("%s out",__FUNCTION__);
    return ret;
}

static int out_offload_drain(struct audio_stream_out* stream, audio_drain_type_t type )
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    int ret = -ENOSYS;
    ALOGE("%s in, audio_app_type:%d, audio_offload_state:%d, type:%d ",
        __func__, out->audio_app_type, out->audio_offload_state, type);

    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        pthread_mutex_lock(&out->lock);
        if (type == AUDIO_DRAIN_EARLY_NOTIFY) {
            ret = audio_send_offload_cmd(out, AUDIO_OFFLOAD_CMD_PARTIAL_DRAIN);
        }else {
            ret = audio_send_offload_cmd(out, AUDIO_OFFLOAD_CMD_DRAIN);
        }
        pthread_mutex_unlock(&out->lock);
    }
     ALOGE("%s out",__FUNCTION__);
    return ret;
}

static int out_offload_flush(struct audio_stream_out* stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    ALOGE("%s in, audio_app_type:%d, audio_offload_state:%d ",
        __func__, out->audio_app_type, out->audio_offload_state);

    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        pthread_mutex_lock(&out->lock);
        if (out->compress != NULL)
            audio_offload_mute(out);
        audio_stop_compress_output(out, 1);
        pthread_mutex_unlock(&out->lock);
        return 0;
    }
    return -ENOSYS;
}



