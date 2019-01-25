#ifndef _BTOOLS_H_
#define _BTOOLS_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include <netinet/in.h>
#include <semaphore.h>

#define VERSION "0.0.1"
#define READ_BUFFER_MAX 128
#define CMD_BUF_MAX 4
#define HCITOOLS_SOCKET 4550
#define PROPERTY_SOCKET "bluetooth.hcitools.socket"
#define PROPERTY_SERVER "bluetooth.hcitools.server"
#define STRING_SERVER_STARTED "started"
#define STRING_SERVER_STOPPED "stopped"
#define STRING_SERVER_UNKNOWN "unknown"
#define PROPERTY_VALUE_LEN 128
#define STRING_SOCKET "4550"
#define HCI_TIMEOUT_VALUE 9
#define HCI_TIMEOUT_STRING "TIMEOUT"
#endif


