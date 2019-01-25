/*
    Copyright (C) 2014 The Android Open Source Project

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
#define LOG_TAG "hw_test_primary"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdint.h>
#include <sys/types.h>
#include <semaphore.h>
#include <pthread.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <string.h>

#include "autotest.h"

extern int _setMusicVolume(int level);
extern int SendAudioTestCmd(char *cmd, int bytes);
void append_g_data_buf(struct hardware_test_handle *tunning,const char *value)
{
    if (value == NULL) {
        return;
    }

    if(tunning==NULL){
        return;
    }
    strcat((char *)tunning->data_buf, value);
    tunning->cur_len += strlen(value);
}

void clear_g_data_buf(struct hardware_test_handle *tunning)
{
    memset(tunning->data_buf, 0, MAX_SOCKT_LEN);
    tunning->cur_len = 0;
}

void send_error(struct hardware_test_handle *tunning,const char *error)
{
    int index = 0;
    MSG_HEAD_T m_head;
    DATA_HEAD_T data_head;
    char error_buf[1000];

    LOG_E("send audiotester err:  %s", error);
    *(error_buf + index) = 0x7e;
    index += 1;
    m_head.seq_num = tunning->diag_seq++;
    m_head.type = 0x99;//spec
    //m_head.subtype = sub_type;
    m_head.len = sizeof(MSG_HEAD_T) + sizeof(DATA_HEAD_T) + strlen(error);
    memcpy(error_buf + index, &m_head, sizeof(MSG_HEAD_T));
    index += sizeof(MSG_HEAD_T);

    data_head.data_state = DATA_STATUS_ERROR;
    memcpy(error_buf + index, &data_head, sizeof(DATA_HEAD_T));
    index += sizeof(DATA_HEAD_T);
    strcpy(error_buf + index, error);
    index += strlen(error);
    *(error_buf + index) = 0x7e;
    index += 1;

    write(tunning->sockfd, error_buf, index);
}

void send_buffer(struct hardware_test_handle *tunning,int sub_type, int data_state)
{
    int index = 0;
    int ret;
    MSG_HEAD_T m_head;
    DATA_HEAD_T data_head;
    uint8_t * send_buf;

    send_buf=tunning->send_buf;

    LOG_D("send_buffer:%p %p :%d",tunning,send_buf,index);

    *(send_buf + index) = 0x7e;
    index += 1;
    m_head.seq_num = tunning->diag_seq++;
    m_head.seq_num = tunning->diag_seq;
    m_head.type = 0x99;//spec
    m_head.subtype = sub_type;
    m_head.len = tunning->cur_len + sizeof(MSG_HEAD_T) + sizeof(DATA_HEAD_T);
    memcpy(send_buf + index, &m_head, sizeof(MSG_HEAD_T));
    index += sizeof(MSG_HEAD_T);

    data_head.data_state = data_state;
    memcpy(send_buf + index, &data_head, sizeof(DATA_HEAD_T));
    index += sizeof(DATA_HEAD_T);
    if (tunning->cur_len != 0) {
        memcpy(send_buf + index, tunning->data_buf, tunning->cur_len);
        index += tunning->cur_len;
    }
    *(send_buf + index) = 0x7e;
    index += 1;

    ret = write(tunning->sockfd, send_buf, index);

    LOG_I("send the param data to AudioTester  send=%d:%d state:%d\n",  index, 
    ret,data_state);

    clear_g_data_buf(tunning);
}

int check_diag_header_and_tail(uint8_t *received_buf, int rev_len)
{
    uint8_t diag_header;
    uint8_t diag_tail;

    diag_header = received_buf[0];
    diag_tail = received_buf[rev_len - 1];
    if (diag_header != 0x7e || diag_tail != 0x7e) {
        LOG_E("the diag header or tail is wrong  header=%x, tail=%x rev_len:%x\n", diag_header,
              diag_tail,rev_len);
        return -1;
    }
    return 0;
}


int is_sprd_full_diag_check(struct hardware_test_handle * tunning,uint8_t *rx_buf_ptr, int rx_buf_len)
{
    MSG_HEAD_T *msg_head;
    unsigned short msg_len=0;
    DATA_HEAD_T *data_command;
    unsigned char data_state;
    uint8_t *msg_start=NULL;
    uint8_t *cmd_buf_ptr=tunning->audio_cmd_buf;
    static unsigned int size=0;
    int ret=0;

    LOG_I("is_sprd_full_diag_check point 0x%p 0x%p 0x%p 0x%p",
        tunning->audio_received_buf,
        tunning->audio_cmd_buf,
        tunning->send_buf,
        tunning->data_buf);

    if(rx_buf_ptr[0]==0x7e){
        msg_start=(uint8_t *)(rx_buf_ptr + 1);
        msg_head = (MSG_HEAD_T *)(msg_start);
        size=msg_head->len+2;
        if((rx_buf_len>=msg_head->len) && (0x7e==*(msg_start+msg_head->len))){
            LOG_D("is_sprd_full_diag_check one pack size:%x rx_buf_len:%x",size,rx_buf_len);
            size=0;
            ret=0;
        }else{
            LOG_D("is_sprd_full_diag_check mutl pack size:%x",size);
            ret=1;
        }
        tunning->rx_packet_len=0;
        LOG_D("is_sprd_full_diag_check  1 rx_packet_len:%d cmd_buf_ptr:%p",tunning->rx_packet_len,cmd_buf_ptr);
        memcpy((void *)&cmd_buf_ptr[tunning->rx_packet_len], rx_buf_ptr, rx_buf_len);
        tunning->rx_packet_len+=rx_buf_len;
    }else{
        if(size<tunning->rx_packet_len+rx_buf_len){
            LOG_E("is_sprd_full_diag_check data err size:%d rx_packet_len:%d len:%d",
                size,tunning->rx_packet_len,rx_buf_len);
            ret=-1;
        }else{
                LOG_D("is_sprd_full_diag_check rx_packet_len:%d cmd_buf_ptr:%",cmd_buf_ptr);
            if(0x7e==*(rx_buf_ptr+rx_buf_len-1)){
                memcpy((void *)&cmd_buf_ptr[tunning->rx_packet_len], rx_buf_ptr, rx_buf_len);
                tunning->rx_packet_len+=rx_buf_len;
                ret=0;
                LOG_D("is_sprd_full_diag_check pack end");
            }else{
                memcpy((void *)&cmd_buf_ptr[tunning->rx_packet_len], rx_buf_ptr, rx_buf_len);
                tunning->rx_packet_len+=rx_buf_len;
                LOG_D("is_sprd_full_diag_check pack continue");
                ret=1;
            }
        }
    }

    if(ret<0){
        tunning->rx_packet_len=0;
        size=0;
    }
    return ret;
}

int is_full_diag_check(struct hardware_test_handle * tunning,uint8_t *rx_buf_ptr, int rx_buf_len,
                       uint8_t *cmd_buf_ptr,  int *cmd_len)
{
    int rtn;
    uint8_t *rx_ptr = rx_buf_ptr;
    uint8_t *cmd_ptr = cmd_buf_ptr;
    uint8_t packet_header = rx_buf_ptr[0];
    uint16_t packet_len = (rx_buf_ptr[6] << 0x08) | (rx_buf_ptr[5]);
    uint8_t packet_end = rx_buf_ptr[rx_buf_len - 1];
    int rx_len = rx_buf_len;
    int cmd_buf_offset = tunning->rx_packet_len;

    rtn = 0;
    *cmd_len = rx_buf_len;
    LOG_D("is_full_diag_check %p %p",rx_buf_ptr,cmd_buf_ptr);
    if ((0x7e == packet_header)
        && (0x00 == tunning->rx_packet_total_len)
        && (0x00 == tunning->rx_packet_len)) {
        tunning->rx_packet_total_len = packet_len + 2;
    }

    if ((0x7e == packet_header)
        && (0x7e == packet_end)) {
        /* one packet */
        LOG_D("%s %d", __func__, __LINE__);
    } else { /* multi packet */
        tunning->rx_packet_len += rx_buf_len;
        LOG_D("%s %d", __func__, __LINE__);
        if ((0x7e == packet_end)
            && (tunning->rx_packet_len == tunning->rx_packet_total_len)) {
            *cmd_len = tunning->rx_packet_len;
            LOG_D("%s %d", __func__, __LINE__);
        } else {
            LOG_D("%s %d not find end: offset:0x%x len:0x%x packet_size:0x%x",
                  __func__, __LINE__, cmd_buf_offset, rx_buf_len, tunning->rx_packet_total_len);
            rtn = 1;
        }
    }

    if(rx_buf_len+cmd_buf_offset>MAX_SOCKT_LEN-1){
        LOG_E("receive buffer is too long:cur_offset:0x%x size:0x%x",cmd_buf_offset,rx_buf_len);
        return -1;
    }

    memcpy((void *)&cmd_buf_ptr[cmd_buf_offset], rx_buf_ptr, rx_buf_len);

    if (0 == rtn) {
        tunning->rx_packet_len = 0x00;
        tunning->rx_packet_total_len = 0x00;
        LOG_I("%s %d", __func__, __LINE__);
    }

    return rtn;
}

static void dump_data(char *buf, int len)
{
    int i = len;
    int line = 0;
    int size = 0;
    int j = 0;
    char dump_buf[60] = {0};
    int dump_buf_len = 0;

    char *tmp = (char *) buf;
    line = i / 16 + 1;

    for(i = 0; i < line; i++) {
        dump_buf_len = 0;
        memset(dump_buf, 0, sizeof(dump_buf));

        if(i < line - 1) {
            size = 16;
        } else {
            size = len % 16;
        }
        tmp = (char *)buf + i * 16;

        sprintf(dump_buf + dump_buf_len, "%04x: ", i*16);
        dump_buf_len = 5;

        for(j = 0; j < size; j++) {
            sprintf(dump_buf + dump_buf_len, " %02x", tmp[j]);
            dump_buf_len += 3;
        }

        LOG_I("%s\n", dump_buf);

    }
}

static int handle_audio_cmd_data(struct hardware_test_handle * test,
                          uint8_t *buf, int len, int sub_type, int data_state)
{
    int ret = 0;

    switch (sub_type) {
    case HARDWARE_TEST_EXT_CMD: {
        int ret=0;
        if(0 == strncmp((const char *)buf,"setMusicVolume",strlen("setMusicVolume"))){
            int vol=strtoul((const char *)buf+strlen("setMusicVolume")+1,NULL,0);
            _setMusicVolume(vol);
        }else{
            ret=SendAudioTestCmd((char *)buf,len);
            if(ret==len)
                ret=0;
        }

        if(ret==0){
            send_buffer(test,sub_type,DATA_STATUS_OK);
        }else{
            send_buffer(test,sub_type,DATA_STATUS_ERROR);
        }
        break;
    }
    case HARDWARE_TEST_XML_CMD:
        ret=hardware_test_process(test,(char *)buf, len);
        if(ret==0){
            send_buffer(test,sub_type,DATA_STATUS_OK);
        }else{
            dump_data((char *)buf,len);
            send_buffer(test,sub_type,DATA_STATUS_ERROR);
        }
        break;
    default:
        send_error(test,"the audio tunning command is unknown");
        break;
    }

    return ret;
}


static int handle_received_data(struct hardware_test_handle * test, uint8_t *received_buf,
                         int rev_len)
{
    int ret, data_len;
    MSG_HEAD_T *msg_head;
    DATA_HEAD_T *data_command;
    uint8_t *audio_data_buf;

    ret = check_diag_header_and_tail(received_buf, rev_len);
    if (ret != 0) {
        LOG_E("Diag Header or Tail error");
        ret=-3;
        return ret;
    }
    msg_head = (MSG_HEAD_T *)(received_buf + 1);

    LOG_I("seq:%08x len:%04x type:%02x subtype:%02x",
          msg_head->seq_num, msg_head->len, msg_head->type, msg_head->subtype);
    /*0x99 is audio tunning specification define*/
    if (msg_head->type != AUDIO_CMD_TYPE) {
        LOG_E("the audio data's type is not equal 0X99, type=%d\n", msg_head->type);
        return -1;
    }

    if(received_buf[1 + msg_head->len] != 0x7e) {
        LOG_E("date format err:%x", received_buf[1 + msg_head->len]);
        return -2;
    }

    data_len = msg_head->len - sizeof(MSG_HEAD_T) - sizeof(DATA_HEAD_T);
    data_command = (DATA_HEAD_T *)(received_buf + sizeof(MSG_HEAD_T) + 1);
    audio_data_buf = received_buf + sizeof(MSG_HEAD_T) + sizeof(DATA_HEAD_T) + 1;

    ret = handle_audio_cmd_data(test, audio_data_buf,
                                data_len, msg_head->subtype, data_command->data_state);

    LOG_I("handle_received_data exit");
    return ret;
}

static void *tunning_thread(void *args)
{
    struct hardware_test_handle * test=(struct hardware_test_handle  *)args;
    int ret, rev_len, data_len;
    int sock_fd = test->sockfd;
    fd_set rfds;
    struct timeval tv;
    LOG_E("%s", __func__);

    while(true==test->wire_connected) {

        FD_ZERO(&rfds);
        FD_SET(sock_fd, &rfds);

        tv.tv_sec = 2; //wait 2 seconds
        tv.tv_usec = 0;
        FD_SET(sock_fd, &rfds);
       // ret = select(sock_fd + 1, &rfds, NULL, NULL, &tv);
        ret = select(sock_fd + 1, &rfds, NULL, NULL, NULL);
        if (ret <= 0) {
            LOG_D("havn't receive any data for 2 seconds from AudioTester:0x%x",sock_fd);
            continue;
        }

        if(FD_ISSET(sock_fd,&rfds)){
            rev_len = recv(sock_fd, test->audio_received_buf, MAX_SOCKT_LEN,
                           MSG_DONTWAIT);

            if (rev_len <= 0) {
                LOG_E("communicate with AudioTester error:%d",rev_len);
                break;
            }
            LOG_D("tunning_thread recv len:%d", rev_len);

            ret = is_sprd_full_diag_check(test,
                test->audio_received_buf, rev_len);
            if (ret == 0) {
                ret =handle_received_data(test,test->audio_cmd_buf, test->rx_packet_len);
                if(ret==-3){
                    LOG_D("tunning_thread recv err len:%d", test->rx_packet_len);
                }
                test->rx_packet_len=0;
            }else if(ret<0){
                test->rx_packet_len=0;
            }else{
                sleep(2);
            }
            memset(test->audio_received_buf,0,rev_len+1);
        }
    }
    return NULL;
}

static int disconnect_audiotester_process(struct hardware_test_handle * tunning_handle){
	LOG_E("disconnect_audiotester_process");
    if(false==tunning_handle->wire_connected){
        LOG_D("disconnect_audiotester_process:audiotester is not connect");
        return 0;
    }

    tunning_handle->seq=0;
    tunning_handle->rx_packet_total_len=0;
    tunning_handle->rx_packet_len=0;
    tunning_handle->wire_connected=false;

    if(tunning_handle->audio_received_buf!=NULL){
        free(tunning_handle->audio_received_buf);
        tunning_handle->audio_received_buf=NULL;
    }

    if(tunning_handle->audio_cmd_buf!=NULL){
        free(tunning_handle->audio_cmd_buf);
        tunning_handle->audio_cmd_buf=NULL;
    }

    if(tunning_handle->send_buf!=NULL){
        free(tunning_handle->send_buf);
        tunning_handle->send_buf=NULL;
    }

    if(tunning_handle->data_buf!=NULL){
        free(tunning_handle->data_buf);
        tunning_handle->data_buf=NULL;
    }

    tunning_handle->sockfd=-1;
    return 0;
}

static int connect_audiotester_process(struct hardware_test_handle * tunning_handle,int sockfd){
    tunning_handle->seq=0;
    tunning_handle->rx_packet_total_len=0;
    tunning_handle->rx_packet_len=0;
    tunning_handle->max_len=MAX_SOCKT_LEN-MAX_LINE_LEN;
    tunning_handle->data_state=0;

    if(NULL == tunning_handle->audio_received_buf){
        tunning_handle->audio_received_buf=(uint8_t *)malloc(MAX_SOCKT_LEN);
        if(tunning_handle->audio_received_buf==NULL){
            LOG_E("connect_audiotester_process malloc audio_received_buf failed");
            goto Err;
        }
    }

    if(NULL == tunning_handle->audio_cmd_buf){
        tunning_handle->audio_cmd_buf=(uint8_t *)malloc(MAX_SOCKT_LEN);
        if(tunning_handle->audio_cmd_buf==NULL){
            LOG_E("connect_audiotester_process malloc audio_cmd_buf failed");
            goto Err;
        }
    }

    if(NULL == tunning_handle->send_buf){
        tunning_handle->send_buf=(uint8_t *)malloc(MAX_SOCKT_LEN);
        if(tunning_handle->send_buf==NULL){
            LOG_E("connect_audiotester_process malloc send_buf failed");
            goto Err;
        }
    }

    if(NULL == tunning_handle->data_buf){
        tunning_handle->data_buf=(uint8_t *)malloc(MAX_SOCKT_LEN);
        if(tunning_handle->data_buf==NULL){
            LOG_E("connect_audiotester_process malloc data_buf failed");
            goto Err;
        }
    }

    LOG_I("connect_audiotester_process 0x%p 0x%p 0x%p 0x%p",
        tunning_handle->audio_received_buf,
        tunning_handle->audio_cmd_buf,
        tunning_handle->send_buf,
        tunning_handle->data_buf);
    tunning_handle->sockfd=sockfd;
    tunning_handle->wire_connected=true;
    return 0;

Err:

    if(tunning_handle->audio_received_buf!=NULL){
        free(tunning_handle->audio_received_buf);
        tunning_handle->audio_received_buf=NULL;
    }

    if(tunning_handle->audio_cmd_buf!=NULL){
        free(tunning_handle->audio_cmd_buf);
        tunning_handle->audio_cmd_buf=NULL;
    }

    return -1;
}

static void *listen_thread(void *args)
{
    int ser_fd, tunning_fd, n, ret;
    struct sockaddr_in sock_addr;
    struct sockaddr cli_addr;
    socklen_t addrlen;
    pthread_t tunning_t;
    pthread_attr_t attr;

    struct hardware_test_handle * test=(struct hardware_test_handle  *)args;

    memset(&sock_addr, 0, sizeof (struct sockaddr_in));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = htons(test->port);

    ser_fd = socket(sock_addr.sin_family, SOCK_STREAM, 0);

    if (ser_fd < 0) {
        LOG_E("can not create tunning server thread\n");
        return NULL;
    }

    if (setsockopt(ser_fd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n)) != 0) {
        LOG_E("setsockopt SO_REUSEADDR error\n");
        return NULL;
    }

    if (bind(ser_fd, (struct sockaddr *)&sock_addr,
             sizeof (struct sockaddr_in)) != 0) {
        LOG_E("bind server error \n");
        return NULL;
    }
    if (listen(ser_fd, 10) < 0) {
        LOG_E("tunning server listen failed\n");
        return NULL;
    }
    pthread_attr_init(&attr);

    tunning_fd=-1;
    while (1) {
        void *res;
        addrlen = sizeof(struct sockaddr);
        LOG_D("waiting audiotester connect\n");
        tunning_fd = accept(ser_fd, &cli_addr, &addrlen);

        if(tunning_fd<0){
            LOG_E("accept tunning client failed\n");
            break;
        }

        if((connect_audiotester_process(test,tunning_fd)<0)){
            LOG_E("connect_audiotester_process failed\n");
            disconnect_audiotester_process(test);
            continue;
        }

        //this is prevent process be killed by SIGPIPE
        signal(SIGPIPE, SIG_IGN);

        ret = pthread_create(&tunning_t, &attr, tunning_thread, test);
        if (ret < 0) {
            LOG_E("create tunning thread failed");
            break;
        }
        pthread_join(tunning_t , &res);
        if (tunning_fd > 0) {
            close(tunning_fd);
        }

        disconnect_audiotester_process(test);
        test->server_exit=true;
        break;
    }
    pthread_attr_destroy(&attr);
    if (ser_fd) {
        close(ser_fd);
    }
    return NULL;
}


void start_hardware_test_server(void  *args)
{
    pthread_t ser_id;
    pthread_attr_t attr;
    int ret;
    struct hardware_test_handle *test=(struct hardware_test_handle *)args;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&ser_id, &attr, listen_thread, args);
    if (ret < 0) {
        LOG_E("create hardware_test_server thread failed");
        test->server_exit=true;
        return;
    }
    pthread_attr_destroy(&attr);
}

int main(int argc, char *argv[])
{
    struct hardware_test_handle test;
    char property[PROPERTY_VALUE_MAX];

    memset(&test,0,sizeof(struct hardware_test_handle));

    /*need to delete*/
    if(argc != 2){
        test.port=AUDIO_TUNNING_PORT;
    }else{
        test.port=strtoul(argv[1],NULL,10);
     }

    test.xml_path=NULL;

    if(argc>=3){
      test.port=strtoul(argv[1],NULL,10);
      test.xml_path=strdup(argv[2]);
    }else if(argc==2){
        test.port=strtoul(argv[1],NULL,10);
    }else{
        test.port=AUDIO_TUNNING_PORT;
    }

    property_get(PROP_AUDIO_HARDWARE_TEST_STATE, property, "not_find");
    if(!strcmp(property, "running")){
        LOG_E("%s is running\n",argv[0]);
        //return 0;
    }else{
        property_set(PROP_AUDIO_HARDWARE_TEST_STATE, "running");
    }

    test.server_exit=false;

    if(init_sprd_xml(&test)<0){
        LOG_E("Load xml:%s failed\n",test.xml_path);
        goto error;
    }
    init_audio_mixer(&test.mixer_ctl);
    if(init_audio_mixer(&test.mixer_ctl)<0){
        LOG_E("init_audio_mixerfailed\n");
        goto error;
    }
    {
        int debug_fd=-1;
        int ret=0;
        debug_fd  = open("/data/local/media/test.txt", O_RDONLY);
        if (debug_fd > 0) {
            int ret=-1;
            char buf_debug[4096]={0};
            LOG_E("%s %d", __func__,__LINE__);
            ret=read(debug_fd,buf_debug,sizeof(buf_debug));
            hardware_test_process(&test,buf_debug,ret);
            close(debug_fd);
        }
    }
    start_hardware_test_server(&test);
    while(false==test.server_exit);
    test.server_exit=true;
error:
    if(test.xml_path){
        free((void *)test.xml_path);
        test.xml_path=NULL;
    }
    property_set(PROP_AUDIO_HARDWARE_TEST_STATE, "exit");
    return 0;
}

