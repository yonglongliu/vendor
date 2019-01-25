#ifndef __ENG_DIAG_MODULES_H__
#define __ENG_DIAG_MODULES_H__

#include "autotest_list.h"

typedef unsigned char  byte;
typedef unsigned char  uchar;

#ifndef uint
typedef unsigned int   uint;
#endif // uint

typedef unsigned short ushort;

struct autotest_callback{
    unsigned short diag_ap_cmd;
    int (*autotest_func)(const uchar *buf, int len, uchar *rsp, int rsplen);
};


typedef struct autotest_modules_info
{
    struct  list_head node;
    struct  autotest_callback callback;
}autotest_modules;

int autotest_modules_load(struct list_head *head );

#endif
