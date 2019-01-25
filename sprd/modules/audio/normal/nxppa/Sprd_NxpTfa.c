#define LOG_TAG "nxptfa"
#include "Sprd_NxpTfa.h"
#include <audio_utils/resampler.h>
#include <string.h>
#include "dumpdata.h"
#include <math.h>
#define PRIVATE_TFA_IMPED_CALI  "TFA Imped Cali"

struct nxppa_res
{
    struct pcm *pcm_tfa;
    struct resampler_itfe  *resampler;
    char * buffer_resampler;
    pthread_mutex_t lock;
    AUDIO_PCMDUMP_FUN dumppcmfun;
    int is_clock_on;
    int StopWriteEmpty;
    pthread_t writewordthread;
    sem_t Sem_WriteWord;
};

int nxptfa_imped_calib(struct nxppa_res * nxp_res, struct mixer *i2s_mixer)
{
	float min = 7.0l;
	float max = 10.0l;
	int ret = 0;
	int min_imped = 7010;
	int max_imped = 10010;
	int imped_got = 0;
	struct mixer_ctl *tfa_imped_cali_ctl;

	pthread_mutex_lock(&nxp_res->lock);
	tfa_imped_cali_ctl = mixer_get_ctl_by_name(i2s_mixer,PRIVATE_TFA_IMPED_CALI);
	if (tfa_imped_cali_ctl != NULL) {
		ret = mixer_ctl_set_value(tfa_imped_cali_ctl, 0, 1);
		ALOGE("%s set '%s' ok ret %d", __func__,PRIVATE_TFA_IMPED_CALI, ret);
	} else {
		ALOGE("%s, '%s' ctl is not access", __FUNCTION__,PRIVATE_TFA_IMPED_CALI);
	}

	if (tfa_imped_cali_ctl != NULL) {
		imped_got = mixer_ctl_get_value(tfa_imped_cali_ctl, 0);
		if (imped_got < max_imped && imped_got > min_imped) {
			ret = 1;
		} else {
			ret = 0;
		}
		ALOGE("%s get '%s' ok, imped_got %d, ret %d", __func__,PRIVATE_TFA_IMPED_CALI,imped_got, ret);
	} else {
		ALOGE("%s, '%s' ctl is not access", __FUNCTION__,PRIVATE_TFA_IMPED_CALI);
	}
	pthread_mutex_unlock(&nxp_res->lock);

	return ret;
}

#define NXPTFA_CALIB_FILE "/productinfo/tfa98xx_cali_result.txt"
static int NxpTfa_Cali_Mark_Check(void)
{
	FILE *file;
	int max_size = 500;
	int ret = 0;
	int bytes_read;
	void *buf =NULL;
	int val = 0;

	file = fopen(NXPTFA_CALIB_FILE, "r");
	if (!file) {
		ALOGE("%s, Failed to open %s",__func__, NXPTFA_CALIB_FILE);
		ret = -ENODEV;
		goto err_check;
	}
	buf = (void*)malloc(max_size);
	if(!buf){
		ALOGE("%s, malloc fail", __func__);
		goto err_check;
	}
	memset(buf,0,max_size);
	bytes_read = fread(buf, 1, max_size, file);
	if (bytes_read < 0)
	{
		ret = -EIO;
		goto err_check;
	}
	if(strlen(buf) > 0) {
		val = strcmp(buf,"calibrated");
		if(val == 0) {
			ret = 1;
		} else {
			ret = 0;
			ALOGE("%s, wrong string got %s, val %d",__func__,buf,val);
		}
	}
	ALOGD("%s, read buf %s, ret %d \n", __func__,buf,ret);
	fclose(file);
	if(buf){
		free(buf);
		buf = NULL;
	}
	return ret;

err_check:
	if (file)
		fclose(file);
	if(buf) {
		free(buf);
		buf = NULL;
	}
	return ret;
}

static int NxpTfa_Cali_Mark_Set(void)
{
	FILE *file;
	int ret = 0;
	char buffer[] = "calibrated";

	ALOGD("%s,enter, write buffer %s \n",__FUNCTION__,buffer);
	file = fopen(NXPTFA_CALIB_FILE, "wb");
	if (!file) {
		ALOGE("Failed to open %s", NXPTFA_CALIB_FILE);
		ret = -ENODEV;
		return ret;
	}
    if (file) {
        ret = fwrite((uint8_t *)buffer, strlen(buffer), 1, file);
        if (ret != 1)
			ALOGE("%s, fwrite failed, length %d, ret %d",__func__, strlen(buffer),ret);
    } else {
        ALOGE("%s, file is NULL, cannot write.",__func__);
    }
	fclose(file);
	ALOGD("%s, buffer %s, length %d, ret %d",__func__, buffer,strlen(buffer),ret);
    return ret;
}

static void * NxpTfa_FillEmptyWord(struct nxppa_res * nxp_res)
{
    int write_count = 0;
    int ret = 0;
    int wait_notified = 0;
    int buffer_size = pcm_get_buffer_size(nxp_res->pcm_tfa);
    char *buffer = malloc(buffer_size);
    if(!buffer) {
		ALOGE("%s, malloc fail \n", __func__);
        return -1;
    }
    memset(buffer,0,sizeof(buffer));
	ALOGD("%s, start write empty words \n", __func__);

    while(!nxp_res->StopWriteEmpty)
    {
        ret = pcm_mmap_write(nxp_res->pcm_tfa, buffer, buffer_size);
        if(!ret) {
            write_count++;
            if((write_count > 2) &&(!wait_notified)){
                wait_notified = 1;
                sem_post(&nxp_res->Sem_WriteWord);
            }
        }
        //ALOGD("%s, writing, buffer_size %d \n", __func__, buffer_size);
    }
    ALOGD("%s, stop write empty words \n", __func__);
    free(buffer);
    return NULL;
}


int  NxpTfa_ClockInit(struct nxppa_res * nxp_res)
{
    int ret = 0;
    pthread_attr_t attr;

    pthread_mutex_lock(&nxp_res->lock);
    sem_init(&nxp_res->Sem_WriteWord,0,0);
    nxp_res->StopWriteEmpty=0;
    nxp_res->is_clock_on=0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    ret = pthread_create(&nxp_res->writewordthread, &attr, NxpTfa_FillEmptyWord, nxp_res);
     if (ret)
     {
        ALOGE("%s: pthread_create falied, code is %s", __func__, strerror(errno));
        pthread_mutex_unlock(&nxp_res->lock);
        return ret;
     }
     sem_wait(&nxp_res->Sem_WriteWord);
     nxp_res->is_clock_on = 1;
     pthread_mutex_unlock(&nxp_res->lock);
     ALOGD("%s,exit \n",__func__);

     return 0;
}

int  NxpTfa_ClockDeInit(struct nxppa_res * nxp_res)
{
	pthread_mutex_lock(&nxp_res->lock);
	if(nxp_res->is_clock_on){
		nxp_res->StopWriteEmpty= 1;
		if(nxp_res->writewordthread) {
			sem_post(&nxp_res->Sem_WriteWord);
			pthread_join(nxp_res->writewordthread, (void **)NULL);
			nxp_res->writewordthread = 0;
		}
		sem_destroy(&nxp_res->Sem_WriteWord);
		nxp_res->is_clock_on = 0;
	}
	pthread_mutex_unlock(&nxp_res->lock);
	ALOGD("%s,exit \n",__FUNCTION__);

	return 0;
}

static int NxpTfa_Init_Resampler(struct nxppa_res *nxp_res)
{
    int ret;

    if (!nxp_res)
        return -1;
    nxp_res->dumppcmfun =NULL;

    nxp_res->buffer_resampler = malloc(20*1024);
    if (!nxp_res->buffer_resampler ) {
        ALOGE("NxpTfa_Open, malloc for resampler buffer failed!");
        return -1;
    }
    if(nxp_res->resampler == NULL) {
        ret = create_resampler( 44100,
                    48000,
                    2,
                    RESAMPLER_QUALITY_DEFAULT,
                    NULL,
                    &nxp_res->resampler);
        if (ret != 0) {
            ALOGE("NxpTfa_Open,create resampler failed!");
            nxp_res->resampler = NULL;
            free(nxp_res->buffer_resampler);
            nxp_res->buffer_resampler = NULL;
            return -1;
        }
    }

    return 0;
}

extern int get_snd_card_number(const char *card_name);

nxp_pa_handle NxpTfa_Open(struct NxpTpa_Info *info, int need_src, int do_pcm_start)
{
    int card = 0;
    int ret = 0;
    struct nxppa_res * nxp_res;

    if (!info) {
        ALOGE("NxpTfa_Open, info is NULL.");
        return NULL;
    }
    if (!info->pCardName) {
        ALOGE("NxpTfa_Open, card name is NULL.");
        return NULL;
    }

    ALOGE("peter: NxpTfa_Open in. need_src: %d, do_pcm_start: %d", need_src,do_pcm_start);

    nxp_res =malloc(sizeof(struct nxppa_res));
    if(!nxp_res) {
        ALOGE("NxpTfa_Open, malloc for nxppa_res failed!");
        return NULL;
    }
    memset(nxp_res, 0, sizeof(struct nxppa_res));

    pthread_mutex_init(&nxp_res->lock, NULL);

    if (need_src && NxpTfa_Init_Resampler(nxp_res) != 0)
        goto err_free_nxp_res;

    card =  get_snd_card_number(info->pCardName);
    if(card < 0) {
        ALOGE("NxpTfa_Open, get sound card no failed!");
        goto err_deinit_resampler;
    }
    nxp_res->pcm_tfa=pcm_open(card, info->dev , info->pcm_flags, &info->pcmConfig);
    if(!pcm_is_ready(nxp_res->pcm_tfa)) {
        ALOGE("NxpTfa_Open,pcm is not ready!");
        goto err_deinit_resampler;
    }
    if (do_pcm_start && pcm_start(nxp_res->pcm_tfa) != 0) {
        ALOGE("NxpTfa_Open, pcm_start for card(%s), device(%d) failed! error: %s",
              info->pCardName, info->dev, pcm_get_error(nxp_res->pcm_tfa));
    }

    return (nxp_pa_handle)nxp_res;

err_deinit_resampler:
    release_resampler(nxp_res->resampler);
    nxp_res->resampler= NULL;
    free(nxp_res->buffer_resampler);
    nxp_res->buffer_resampler = NULL;
err_free_nxp_res:
    free(nxp_res);
    return NULL;

}

int NxpTfa_Write( nxp_pa_handle handle , const void *data, unsigned int count)
{
    int ret = 0;
    int OutSize;
    size_t out_frames = 0;
    size_t in_frames = count/4;
    int16_t  * buffer = data;
    struct nxppa_res * nxp_res = NULL;

    if(handle == NULL)
        return -1;

    nxp_res = (struct nxppa_res *) handle;

    ALOGV("peter:NxpTfa_Write in");

    pthread_mutex_lock(&nxp_res->lock);

    out_frames = in_frames*2;
    if(nxp_res->resampler && nxp_res->buffer_resampler) {
        //ALOGE("peter 1: NxpTfa_Write in_frames %d,out_frames %d", in_frames, out_frames);
        nxp_res->resampler->resample_from_input(nxp_res->resampler,
                    (int16_t *)buffer,
                    &in_frames,
                    (int16_t *)nxp_res->buffer_resampler,
                    &out_frames);
        //ALOGE("peter 2: NxpTfa_Write in_frames %d,out_frames %d", in_frames, out_frames);
    }

    if(NULL!=nxp_res->dumppcmfun){
        nxp_res->dumppcmfun(nxp_res->buffer_resampler,out_frames*4,DUMP_MUSIC_HWL_BEFOORE_VBC);
    }

    ret = pcm_mmap_write(nxp_res->pcm_tfa, nxp_res->buffer_resampler, out_frames*4);
    //ret = pcm_mmap_write(nxp_res->pcm_tfa, data, count);
    pthread_mutex_unlock(&nxp_res->lock);

    ALOGV("peter:NxpTfa_Write out");

    return ret;
}


void NxpTfa_Close( nxp_pa_handle handle )
{
    struct nxppa_res * nxp_res;

    if(handle == NULL)
        return;

    nxp_res = (struct nxppa_res *) handle;

    pthread_mutex_lock(&nxp_res->lock);

    ALOGE("peter: NxpTfa_Close in");

    if(nxp_res->pcm_tfa) {
        pcm_close(nxp_res->pcm_tfa);
    }

    if(nxp_res->resampler) {
        release_resampler(nxp_res->resampler);
        nxp_res->resampler= NULL;
    }
    if(nxp_res->buffer_resampler) {
        free(nxp_res->buffer_resampler);
        nxp_res->buffer_resampler = NULL;
    }
    pthread_mutex_unlock(&nxp_res->lock);

    free(nxp_res);
}

void register_nxp_pcmdump(nxp_pa_handle handle,void *fun){
    struct nxppa_res * nxp_res = NULL;

    if(handle == NULL)
        return ;

    nxp_res = (struct nxppa_res *) handle;
    nxp_res->dumppcmfun=fun;
    ALOGI("register_nxp_pcmdump");
}

void NXPTfa_calibration(struct NxpTpa_Info *info, struct mixer *i2s_mixer)
{
	struct nxppa_res * nxp_res;
	int ret;

	ret = NxpTfa_Cali_Mark_Check();
	ALOGI("%s, mark check ret %d", __func__,ret);
	if (ret != 1) {
		ALOGI("%s,tfa need to calibrate", __func__);
		nxp_res = NxpTfa_Open(info,0,0);
		if (!nxp_res) {
			ALOGE("%s, NxpTfa_Open faill.", __func__);
			return;
		}

		ret = NxpTfa_ClockInit(nxp_res);
		if (ret) {
			ALOGE("%s, NxpTfa_ClockInit faill.", __func__);
			NxpTfa_Close(nxp_res);
			return;
		}
		ALOGD("%s,prepare end \n",__FUNCTION__);

		ret = nxptfa_imped_calib(nxp_res,i2s_mixer);
		ALOGI("%s, tfa impedance calib %s",
			__func__, (ret == 1) ? "successful" : "fail");
		if (ret == 1) {
			ALOGI("%s, save cali result", __func__);
			ret = NxpTfa_Cali_Mark_Set();
			ALOGI("%s, ret %d", __func__,ret);
		}

		NxpTfa_ClockDeInit(nxp_res);
		NxpTfa_Close(nxp_res);
		ALOGD("%s,close end \n",__FUNCTION__);
	} else if (ret ==1 ) {
		ALOGI("%s,tfa already calibrated", __func__);
	}

}
