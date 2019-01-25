#ifndef __ENG_DIAG_MODULES_H__
#define __ENG_DIAG_MODULES_H__

#include "eng_list.h"

#ifndef byte
typedef unsigned char  byte;
#endif

#ifndef uchar 
typedef unsigned char  uchar;
#endif

#ifndef uint
typedef unsigned int   uint;
#endif // uint

#ifndef ushort
typedef unsigned short ushort;
#endif

struct eng_callback{
    unsigned int diag_ap_cmd; //data area: unsigned int for data command
    unsigned char type; //command
    unsigned char subtype; //data command
    char at_cmd[32];
    int also_need_to_cp;
    int (*eng_diag_func)(char *buf, int len, char *rsp, int rsplen);
    int (*eng_linuxcmd_func)(char *req, char *rsp);
};


typedef struct eng_modules_info
{
    struct  list_head node;
    struct  eng_callback callback;
}eng_modules;

int eng_modules_load(struct list_head *head);

#endif
