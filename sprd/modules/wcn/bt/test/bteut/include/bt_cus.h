#ifndef __LIBBT_SPRD_BT_CUS_H__
#define __LIBBT_SPRD_BT_CUS_H__

#include "stdio.h"
#include "stdlib.h"


void get_bt_status_parse(char *buf, char *rsp);




#define OK_STR "OK"
#define FAIL_STR "FAIL"

#define SNPRINT(z, str) snprintf(z, snprintf(NULL, 0, str), str)

#define NUM_ELEMS(x) (sizeof(x) / sizeof(x[0]))


#endif
