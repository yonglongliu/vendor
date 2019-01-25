#ifndef _AUDIO__TEST_SERVER_H_
#define _AUDIO__TEST_SERVER_H_

#include "stdbool.h"
#include "stddef.h"
#include <cutils/log.h>
#include <tinyalsa/asoundlib.h>

#define AUDIO_CMD_TYPE  0x99
#define MAX_LINE_LEN 512
#define AUDIO_TUNNING_PORT 9999 //zzj  for test
#define MAX_SOCKT_LEN 65535
#define PROPERTY_VALUE_MAX 1024
#define PROP_AUDIO_HARDWARE_TEST_STATE  "system.audio.hardwartest.state"

#define LOG_I ALOGI
#define LOG_D ALOGD
#define LOG_V ALOGV
#define LOG_E  ALOGE

#define SPLIT "\n"
#ifdef FLAG_VENDOR_ETC
#define AUDIO_HARDWARE_TEST_XML     "/vendor/etc/audio_hardware_test.xml"
#else
#define AUDIO_HARDWARE_TEST_XML     "/etc/audio_hardware_test.xml"
#endif
#define AUDIO_HARDWARE_FIRST_NAME     "audio_hardware_test"

typedef void *param_group_t;
typedef void *param_doc_t;

typedef unsigned char  uint8_t;
typedef    short    int16_t;
typedef    int    int32_t;

struct xml_handle {
    param_doc_t param_doc;
    param_group_t param_root;
    char *first_name;
};

struct audio_mixer_contrl {
    struct mixer **mixer;
    int *card;
    int size;
};

struct hardware_test_handle {
    struct xml_handle xml;

    uint8_t * audio_received_buf;
    uint8_t * audio_cmd_buf;
    int sockfd;
    int seq;
    int rx_packet_len;
    int rx_packet_total_len;
    bool wire_connected;
    bool server_exit;
    int diag_seq;

    uint8_t * data_buf;
    uint8_t * send_buf;
    int max_len;
    int cur_len;
    int data_state;

    int port;

    struct audio_mixer_contrl mixer_ctl;
    const char *xml_path;
};

typedef enum {
    GET_INFO,
    GET_PARAM_FROM_RAM,
    SET_PARAM_FROM_RAM,
    GET_PARAM_FROM_FLASH,
    SET_PARAM_FROM_FLASH,
    GET_REGISTER,
    SET_REGISTER,
    SET_PARAM_VOLUME=14,
    SET_VOLUME_APP_TYPE=15,
    CONNECT_AUDIOTESTER,
    DIS_CONNECT_AUDIOTESTER,
    HARDWARE_TEST_EXT_CMD=0xf1,
    HARDWARE_TEST_XML_CMD=0xf2,
} AUDIO_CMD;

typedef enum {
    DATA_SINGLE = 1,
    DATA_START,
    DATA_END,
    DATA_MIDDLE,
    DATA_STATUS_OK,
    DATA_STATUS_ERROR,
} AUDIO_DATA_STATE;

// This is the communication frame head
typedef struct msg_head_tag {
    unsigned int  seq_num;      // Message sequence number, used for flow control
    unsigned short
    len;          // The totoal size of the packet "sizeof(MSG_HEAD_T)+ packet size"
    unsigned char   type;         // Main command type
    unsigned char   subtype;      // Sub command type
} __attribute__((packed)) MSG_HEAD_T;

//byte 1 is data state, and other 19 bytes is reserve
typedef struct data_command {
    unsigned char data_state;
    unsigned char reserve[19];
} __attribute__((packed)) DATA_HEAD_T;


#ifdef __cplusplus
extern "C" {
#endif
int init_sprd_xml(struct hardware_test_handle * test);
int hardware_test_process(struct hardware_test_handle * test, char * cmd, int len);
int init_audio_mixer(struct audio_mixer_contrl *ctl);
int parse_mixer_control(struct audio_mixer_contrl *ctl,void  *mixer_element);
int setMusicVolume(int level);
int _set_register_value(int paddr, int value);
int _get_register_value(int paddr, int nword, int *reg);


#ifdef __cplusplus
}
#endif

#endif

