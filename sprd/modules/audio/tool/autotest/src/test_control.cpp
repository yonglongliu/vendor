/*
    Copyright (C) 2012 The Android Open Source Project

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#define LOG_TAG "hw_test_control"
#include <math.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <media/AudioSystem.h>
#include <system/audio.h>

#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include <tinyxml.h>

#include"unistd.h"
#include"sys/types.h"
#include"fcntl.h"
#include"dirent.h"
#include"stdio.h"

#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include "autotest.h"


#define AUDIO_EXT_CONTROL_PIPE "/dev/pipe/mmi.audio.ctrl"
#define AUDIO_EXT_DATA_CONTROL_PIPE "/data/local/media/mmi.audio.ctrl"


using namespace android;
int _setMusicVolume(int level){
    int ret=0;
    float volume=0.000000;
    LOG_I("setMusicVolume:%d",level);
    if((level>=0) && (level<=15)){
        switch(level){
            case 0:
                volume=0.000000;
                break;
            case 1:
                volume=0.002172;
                break;
            case 2:
                volume=0.004660;
                break;
            case 3:
                volume=0.010000;
                break;
            case 4:
                volume=0.014877;
                break;
            case 5:
                volume=0.023646;
                break;
            case 6:
                volume=0.037584;
                break;
            case 7:
                volume=0.055912;
                break;
            case 8:
                volume=0.088869;
                break;
            case 9:
                volume=0.141254;
                break;
            case 10:
                volume=0.189453;
                break;
            case 11:
                volume=0.266840;
                break;
            case 12:
                volume=0.375838;
                break;
            case 13:
                volume=0.504081;
                break;
            case 14:
                volume=0.709987;
                break;
            case 15:
                volume=1.000000;
                break;
            default:
                ret=-1;
                volume=0.0;
                break;
        }
    }

    if(ret==0){
        ret = AudioSystem::setStreamVolume(AUDIO_STREAM_MUSIC,volume,audio_io_handle_t(0));
    }

    if(NO_ERROR==ret){
       ret=0;
    }else{
       ret=-1;
    }

    return ret;
}


int _set_register_value(int paddr, int value)
{
    char cmd[128]={0};
    FILE *pipe_file=NULL;
    char result[128]={0};

    if (paddr & 0x3) {
        LOG_E("address should be 4-byte aligned\n");
        return -1;
    }

    snprintf(cmd,sizeof(cmd),"lookat -s 0x%x 0x%x",value,paddr);
    if((pipe_file=popen(cmd, "r"))!=NULL){
        fread(result, 1, sizeof(result), pipe_file);
        pclose(pipe_file);
        pipe_file = NULL;
    }
    LOG_I("_set_register_value cmd:%s result:%s",cmd,result);
    return 0;
}


int _get_register_value(int paddr, int nword, int *reg)
{
    char *result=NULL;
    char cmd[128]={0};
    FILE *pipe_file;

    int result_size=nword*32+96;
    result=(char *)malloc(result_size);
    if(NULL==result){
        LOG_E("%s malloc :%d size failed\n",nword);
        return -1;
    }

    snprintf(cmd,sizeof(cmd),"lookat -l %d 0x%x",nword,paddr);
    if((pipe_file=popen(cmd, "r"))!=NULL){
        fread(result, 1, result_size, pipe_file);
        pclose(pipe_file);
        pipe_file = NULL;
    }

    LOG_I("cmd:%s \nresult:%s\n", cmd, result);
    {
        char *tmpstr=NULL;
        char data_buf[64]={0};
        char *line=NULL;
        char *reg_addr=NULL;
        char *reg_values=NULL;
        char *eq =NULL;
        int addr=0;
        int offset=0;
        line = strtok_r(result, SPLIT, &tmpstr);
        while (line != NULL) {
            memcpy(data_buf,line,strlen(line));
            char *eq = strchr(data_buf, '|');
            if (eq) {
                reg_addr = strndup(data_buf, eq - data_buf);
                if (*(++eq)) {
                    reg_values = strdup(eq);
                    addr=strtoul(reg_addr,NULL,0);
                    LOG_D("_get_register_value addr:%x--> %s",addr,reg_addr);
                    if(addr>=paddr){
                        offset=(addr-paddr)/4;
                        if(offset>=nword){
                            LOG_E("addr err:%x :%s *----\n",addr,line);
                        }else{
                            reg[offset]=strtoul(reg_values,NULL,0);
                            LOG_D("_get_register_value reg_values:%x--> %s offset:%d",reg[offset],reg_values,offset);
                        }
                    }
                }
            }

            if(reg_addr!=NULL){
                free(reg_addr);
                reg_addr=NULL;
            }

            if(reg_values!=NULL){
                free(reg_values);
                reg_values=NULL;
            }
            line = strtok_r(NULL, SPLIT, &tmpstr);
        }

    }
    return 0;
}

static struct mixer_ctl *_mixer_get_ctl_by_name(struct audio_mixer_contrl *_mixer_ctl, const char *name)
{
    int ret=-1;
    int i = 0;
    struct mixer * test_mixer=NULL;

    struct mixer_ctl *_mixer=NULL;
    LOG_D("_mixer_get_ctl_by_name size:%d %p %p %p %p",
            _mixer_ctl->size,_mixer_ctl->mixer[0],
            _mixer_ctl->mixer[1],_mixer_ctl->mixer[2],_mixer_ctl->mixer[3]);

    for (int i = 0; i < _mixer_ctl->size; i++) {
        _mixer=NULL;
        if(NULL!=_mixer_ctl->mixer[i]){
            _mixer= mixer_get_ctl_by_name(_mixer_ctl->mixer[i], name);
           if(NULL!=_mixer){
                return _mixer;
           }
       }
    }
    LOG_E("_mixer_get_ctl_by_name ERR:%s %p \n",name,_mixer);
    return _mixer;
}

int parse_mixer_control(struct audio_mixer_contrl *ctl,void *mixer_element)
{
    const char *ctl_name;
    int value;
    const char *str=NULL;
    TiXmlElement * ele=(TiXmlElement *)mixer_element;
    struct mixer_ctl *mixer_ctl=NULL;

    ctl_name = ele->Attribute("ctl");

    mixer_ctl=_mixer_get_ctl_by_name(ctl, ctl_name);
    if(NULL!=mixer_ctl){
        str=ele->Attribute("val");
        if(str!=NULL){
            /* This can be fooled but it'll do */
            value = atoi(str);
            LOG_I("set mixer :[%s] to [%s]\n",ctl_name,str);
            if (!value && strcmp(str, "0") != 0)
                mixer_ctl_set_enum_by_string(mixer_ctl, str);
            else
                mixer_ctl_set_value(mixer_ctl, 0, value);
        }
    }
    return 0;
}

int SendAudioTestCmd(char *cmd, int bytes) {
    int fd = -1;
    int ret = -1;
    int bytes_to_read = bytes;
    char *write_ptr=NULL;
    int writebytes=0;
    if (cmd == NULL) {
        return -1;
    }

    fd = open(AUDIO_EXT_CONTROL_PIPE, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        fd = open(AUDIO_EXT_DATA_CONTROL_PIPE, O_WRONLY | O_NONBLOCK);
    }

    if (fd < 0) {
        return -1;
    } else {
        write_ptr=cmd;
        writebytes=0;
        do {
            writebytes = write(fd, write_ptr, bytes);
            if (ret > 0) {
                if (writebytes <= bytes) {
                    bytes -= writebytes;
                    write_ptr+=writebytes;
                }
            } else if ((!((errno == EAGAIN) || (errno == EINTR))) || (0 == ret)) {
                LOG_E("pipe write error %d, bytes read is %d", errno, bytes_to_read - bytes);
                break;
            } else {
                LOG_D("pipe_write_warning: %d, ret is %d", errno, ret);
            }
        } while (bytes);
    }

    if (fd > 0) {
        close(fd);
    }

    if (bytes == bytes_to_read)
        return ret;
    else
        return (bytes_to_read - bytes);
}

int init_audio_mixer(struct audio_mixer_contrl *ctl)
{
    int  ret;
    struct mixer **new_mixer=NULL;
    int card_num=-1;
    DIR *p_dir;
    struct dirent *p_dirent;
    struct mixer * mixer_test=NULL;
    int *new_card=NULL;

    ctl->mixer=NULL;
    ctl->size=0;
    ctl->card=NULL;

    LOG_D("init_audio_mixer");
    if((p_dir=opendir("/proc/asound"))==NULL)
    {
        LOG_E("Can no't open /proc/asound\n");
        return -1;
    }
    while((p_dirent=readdir(p_dir)))
    {
        LOG_D("%s\n",p_dirent->d_name);
        if(0 == strncmp(p_dirent->d_name,"cards",strlen("cards"))){
            continue;
        }
        if(0 == strncmp(p_dirent->d_name,"card",strlen("card"))){
            card_num=-1;
            card_num=strtoul(p_dirent->d_name+4,NULL,10);

            mixer_test=NULL;
            mixer_test=mixer_open(card_num);
            //DEBUG_D("CARD:%d %p\n",card_num,mixer_test);

            if (NULL==mixer_test) {
                LOG_D("init_audio_control Unable to open the mixer card_num:%d %s, aborting.\n",card_num,p_dirent->d_name);
            }else{
                new_mixer=(struct mixer  **)realloc((ctl->mixer),(ctl->size+1)*sizeof(struct mixer  *));
                if (new_mixer == NULL) {
                    LOG_E("alloc device_route failed");
                    return -2;
                } else {
                    ctl->mixer = new_mixer;
                }
                ctl->mixer[ctl->size]=mixer_test;

                new_card=(int *)realloc(ctl->card,ctl->size*sizeof(int*));
                if (new_card == NULL) {
                    LOG_E("alloc new_card failed");
                    return -2;
                } else {
                    ctl->card = new_card;
                }
                ctl->card[ctl->size]=card_num;

                LOG_D("open %s success mixer:%p %p card num:%d\n",p_dirent->d_name,ctl->mixer[ctl->size],mixer_test,ctl->card[ctl->size]);
                ctl->size++;
            }
        }
    }
    closedir(p_dir);
    return 0;
}

