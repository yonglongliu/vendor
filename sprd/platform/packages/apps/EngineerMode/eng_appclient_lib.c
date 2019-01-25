/*
 * File:         eng_appclient.c
 * Based on:
 * Author:       Yunlong Wang <Yunlong.Wang@spreadtrum.com>
 *
 * Created:   2011-03-16
 * Description:  create pc client in android for engneer mode
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/delay.h>
#include <sys/socket.h>

#include "engopt.h"
#include "engat.h"
#include "engclient.h"

#include "cutils/sockets.h"
#include "cutils/properties.h"
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include "stdlib.h"
#include "unistd.h"
#include <errno.h>

#include "engopt.h"
#include "engapi.h"
#include "engclient.h"
#include <cutils/sockets.h>

#ifndef ENG_SOCKET_PORT
#define ENG_SOCKET_PORT  65000
#endif
#if 0
int  eng_close(int fd)
 {
   return close(fd);
 }

 int  eng_read(int  fd, void*  buf, size_t  len)
 {
     return read(fd, buf, len);
 }

 int  eng_write(int  fd, const void*  buf, size_t  len)
 {
     return write(fd, buf, len);
 }

 int eng_client(char *name, int type)
  {
    int fd;
    fd = socket_local_client(name, ANDROID_SOCKET_NAMESPACE_RESERVED, type);
    if (fd < 0) {
        printf("eng_client Unable to connect socket errno:%d[%s]", errno, strerror(errno));
    }

    return fd;
  }

/*******************************************************************************
* Function    :  eng_pcclient_handshake
* Description :  client register to server
* Parameters  :  fd: socket fd
* Return      :    none
*******************************************************************************/
static int eng_at_handshake( int fd, int type)
{

    struct eng_buf_struct data;

    memset(data.buf,0,ENG_BUF_LEN*sizeof(unsigned char));
    sprintf((char*)data.buf, "%s:%d", ENG_APPCLIENT, type);
    eng_write(fd, data.buf, strlen((char*)data.buf));

    memset(data.buf,0,ENG_BUF_LEN*sizeof(unsigned char));
    eng_read(fd,data.buf, ENG_BUF_LEN);
    ENG_LOG("%s: data.buf=%s\n",__func__,data.buf);
    if ( strncmp((const char*)data.buf,ENG_WELCOME,strlen(ENG_WELCOME)) == 0){
        return 0;
    }
    printf("%s: handshake error", __FUNCTION__);
    return -1;
}

int eng_at_open(char *name, int type)
{
    int counter=0;
    int err=-1;
    int soc_fd=-1;

    //connect to server
    soc_fd = eng_client(name, SOCK_STREAM);
    if (soc_fd < 0) {
        LOGE ("%s: opening engmode server socket failed", __FUNCTION__);
        return err;
    }

    if(eng_at_handshake(soc_fd, type)!=0) {
        eng_close(soc_fd);
        return err;
    }

    return soc_fd;
}

int eng_at_write(int fd, char *buf, int buflen)
{
    return eng_write(fd, buf, buflen);
}

int eng_at_read(int fd, char *buf, int buflen)
{
    int n;
    int readlen;
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    n = select(fd+1, &readfds, NULL, NULL, NULL);

    readlen=eng_read(fd, buf, buflen);

    return readlen;
}

void eng_at_wtemp(int fd,int cmd)
{
    int cmdlen=0;
    char cmdbuf[16];
    memset(cmdbuf, 0, sizeof(cmdbuf));
    switch(cmd) {
        case 0:
            sprintf(cmdbuf, "%d,%d", ENG_AT_REQUEST_MODEM_VERSION,0);
            cmdlen=strlen(cmdbuf);
            ENG_LOG("CMD: %s\n", cmdbuf);
            break;
        case 0xff:
            sprintf(cmdbuf, "%d,%d", ENG_AT_EXIT,0);
            cmdlen=strlen(cmdbuf);
            break;
        default:
            break;
    }

    if(eng_at_write(fd, cmdbuf, cmdlen)!=cmdlen) {
        ENG_LOG(" ENG AT WRITE Failed!\n");
        exit(-1);
    }

}

int eng_request(char *request, int requestlen, char *response, int *responselen)
{
    //char cmdbuf[16];
    char *cmdbuf = request;
    int cmdlen = requestlen;

    char readbuf[512];
    int readlen;
    int fd;

    ENG_LOG("Run Engineer Mode PC2SERVER Client!\n");

    fd = eng_at_open("engtd",0);
    if(fd<0) {
        ENG_LOG("ENG Open Failed!\n");
        //exit(-1);
        goto exit;
    }

        if(cmdlen > 0) {
            ENG_LOG("Write Command, fd=%d\n", fd);
            if(eng_at_write(fd, cmdbuf, cmdlen)!=cmdlen) {
                ENG_LOG(" ENG AT WRITE Failed!\n");
                //exit(-1);
                goto exit;
            }
        }

        memset(readbuf, 0, sizeof(readbuf));
        readlen=eng_at_read(fd, readbuf, 512);
        memcpy(response, readbuf, readlen);
        *responselen = readlen;
        ENG_LOG("ENG AT READ: %s\n", readbuf);
exit:
    if (fd>0){
        eng_close(fd );
    }
    return 0;
}

/*
  * |-cmd id-|-param num-|-param1-|-param2-|---|
  */
//int main(int argc, char *argv[])
int eng_appctest(void)
{
    char cmdbuf[16];
    char readbuf[512];
    int readlen;
    int fd;
    int cmdlen=0;
    int cmd;

    ENG_LOG("Run Engineer Mode PC2SERVER Client!\n");

    fd = eng_at_open("engtd",0);

    if(fd<0) {
        ENG_LOG("ENG Open Failed!\n");
        exit(-1);
    }

    while(1) {
        scanf("%d",&cmd);

        ENG_LOG("Send AT Command %d\n", cmd);
        memset(cmdbuf, 0, sizeof(cmdbuf));
        switch(cmd) {
            case 0:
                sprintf(cmdbuf, "%d,%d", ENG_AT_REQUEST_MODEM_VERSION,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 1:
                sprintf(cmdbuf, "%d,%d", ENG_AT_REQUEST_IMEI,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 2:
                sprintf(cmdbuf, "%d,%d, %d", ENG_AT_SELECT_BAND,1, 2);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 3:
                sprintf(cmdbuf, "%d,%d", ENG_AT_CURRENT_BAND,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 4:
                sprintf(cmdbuf, "%d,%d,%d", ENG_AT_SETARMLOG, 1, 0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 5:
                sprintf(cmdbuf, "%d,%d", ENG_AT_GETARMLOG,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 6:
                sprintf(cmdbuf, "%d,%d,%d", ENG_AT_SETAUTOANSWER, 1, 0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 7:
                sprintf(cmdbuf, "%d,%d", ENG_AT_GETAUTOANSWER,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 8:
                sprintf(cmdbuf, "%d,%d,%d,%d,%d", ENG_AT_SETSPPSRATE,3,0,777,111);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 9:
                sprintf(cmdbuf, "%d,%d", ENG_AT_GETSPPSRATE,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 10:
                sprintf(cmdbuf, "%d,%d,%d,%d", ENG_AT_SETSPTEST,2, 6, 2);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 11:
                sprintf(cmdbuf, "%d,%d", ENG_AT_GETSPTEST,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 12:
                sprintf(cmdbuf, "%d,%d", ENG_AT_SPID,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 13:
                sprintf(cmdbuf, "%d,%d,%d,%d,%d", ENG_AT_SETSPFRQ,3,0,0,10088);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;

            case 14:
                sprintf(cmdbuf, "%d,%d", ENG_AT_GETSPFRQ, 0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 15:
                sprintf(cmdbuf, "%d,%d,%d,%d,%d,%d", ENG_AT_SPAUTE, 4, 1, 1000, 6, 8);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 16:
                sprintf(cmdbuf, "%d,%d,%d",ENG_AT_SETSPDGCNUM, 1, 3);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 17:
                sprintf(cmdbuf, "%d,%d",ENG_AT_GETSPDGCNUM, 0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 18:
                sprintf(cmdbuf, "%d,%d,%d,%d,%d,%d",ENG_AT_SETSPDGCINFO,4,2,850,88,88);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 19:
                sprintf(cmdbuf, "%d,%d",ENG_AT_GETSPDGCINFO,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 20:
                sprintf(cmdbuf, "%d,%d,%d,%d,%d,%d,%d,%d",ENG_AT_GRRTDNCELL,6,1, 10054,11,22,11,22);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 21:
                sprintf(cmdbuf, "%d,%d",ENG_AT_GRRTDNCELL,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 22:
                sprintf(cmdbuf, "%d,%d,%d", ENG_AT_SPL1ITRRSWITCH, 1, 1);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n", cmdbuf);
                break;
            case 23:
                sprintf(cmdbuf, "%d,%d",ENG_AT_GETSPL1ITRRSWITCH,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;
            case 24:
                sprintf(cmdbuf, "%d,%d,%d,%d",ENG_AT_PCCOTDCELL, 2, 10054, 88);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;
            case 25:
                sprintf(cmdbuf, "%d,%d,%d,%s,%s,%d",ENG_AT_SDATACONF, 4, 1, "UDP", "211.144.193.27", 7000);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;
            case 26:
                sprintf(cmdbuf, "%d,%d,%d,%d",ENG_AT_L1PARAM, 2, 3, 0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;
            case 27:
                sprintf(cmdbuf, "%d,%d,%d",ENG_AT_L1PARAM, 1, 3);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;
            case 28:
                sprintf(cmdbuf,"%d,%d,%d,%d",ENG_AT_TRRPARAM,2,5,1);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;
            case 29:
                sprintf(cmdbuf,"%d,%d,%d",ENG_AT_TRRPARAM,1,5);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;
            case 30:
                sprintf(cmdbuf, "%d,%d,%d",ENG_AT_SETTDMEASSWTH,1,2);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;
            case 31:
                sprintf(cmdbuf, "%d,%d",ENG_AT_GETTDMEASSWTH,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;
            case 32:
                sprintf(cmdbuf, "%d,%d,%d,%d", ENG_AT_RRDMPARAM, 2, 8, 3);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 33:
                sprintf(cmdbuf, "%d,%d,%d", ENG_AT_RRDMPARAM,1,8);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 34:
                sprintf(cmdbuf,"%d,%d",ENG_AT_DMPARAMRESET,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 35:
                sprintf(cmdbuf,"%d,%d,%d",ENG_AT_SMTIMER, 1, 28);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 36:
                sprintf(cmdbuf, "%d,%d",ENG_AT_SMTIMER,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 37:
                sprintf(cmdbuf, "%d,%d,%d,%s,%s,%d,%d",ENG_AT_SDATACONF, 5, 1, "UDP", "211.144.193.27", 7000,6000);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 38:
                sprintf(cmdbuf, "%d,%d,%d,%d",ENG_AT_TRRDCFEPARAM, 2, 1, 188);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 39:
                sprintf(cmdbuf, "%d,%d,%d",ENG_AT_TRRDCFEPARAM,1,1);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 40:
                sprintf(cmdbuf, "%d,%d",ENG_AT_CIMI, 0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 41:
                sprintf(cmdbuf, "%d,%d, %d",ENG_AT_MBCELLID,1,1);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 42:
                sprintf(cmdbuf, "%d,%d,%s,%s",ENG_AT_MBAU,2,"123", "ABC");
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 43:
                sprintf(cmdbuf, "%d,%d",ENG_AT_EUICC,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 44:
                sprintf(cmdbuf, "%d,%d, %d",ENG_AT_MBCELLID,1,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 45:
                sprintf(cmdbuf, "%d,%d, %d",ENG_AT_MBCELLID,1,2);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 46:
                sprintf(cmdbuf, "%d,%d,%s",ENG_AT_MBAU,1,"XYZ");
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 47:
                sprintf(cmdbuf, "%d,%d",ENG_AT_CGREG,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD: %s\n",cmdbuf);
                break;

            case 48:
                sprintf(cmdbuf, "%d,%d,%s",ENG_AT_NOHANDLE_CMD,1,"AT+ARMLOG=1");
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD %s\n",cmdbuf);
                break;
            case 49:
                sprintf(cmdbuf, "%d,%d",ENG_AT_SYSINFO,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD %s\n",cmdbuf);
                break;
            case 50:
                sprintf(cmdbuf, "%d,%d",ENG_AT_HVER,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD %s\n",cmdbuf);
                break;
            case 51:
                sprintf(cmdbuf, "%d,%d",ENG_AT_GETSYSCONFIG,0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD %s\n",cmdbuf);
                break;
            case 52:
                sprintf(cmdbuf, "%d,%d,%d,%d,%d,%d",ENG_AT_SETSYSCONFIG,4,2,0,1,1);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD %s\n",cmdbuf);
                break;
            case 53:
                sprintf(cmdbuf, "%d,%d,%d",ENG_AT_SPVER,0,0);
                cmdlen = strlen(cmdbuf);
                ENG_LOG("CMD %s\n",cmdbuf);
                break;
            case 54:
                sprintf(cmdbuf, "%d,%d,%d",ENG_AT_SETAUTOATTACH, 1, 1);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD %s\n",cmdbuf);
                break;
            case 55:
                sprintf(cmdbuf, "%d,%d",ENG_AT_AUTOATTACH, 0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD %s\n",cmdbuf);
                break;
            case 56:
                sprintf(cmdbuf, "%d,%d,%d,%d",ENG_AT_PDPACTIVE, 2, 0, 1);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD %s\n",cmdbuf);
                break;
            case 57:
                sprintf(cmdbuf, "%d,%d",ENG_AT_GETPDPACT, 0);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD %s\n",cmdbuf);
                break;
            case 58:
                sprintf(cmdbuf, "%d,%d,%d",ENG_AT_SGPRSDATA, 1, 20);
                cmdlen=strlen(cmdbuf);
                ENG_LOG("CMD %s\n",cmdbuf);
                break;
            case 59:
                sprintf(cmdbuf, "%d,%d",ENG_AT_SADC,0);
                cmdlen = strlen(cmdbuf);
                ENG_LOG("CMD %s\n",cmdbuf);
                break;

            case 60:
                sprintf(cmdbuf, "%d,%d,%d",ENG_AT_CFUN,1,0);
                cmdlen = strlen(cmdbuf);
                ENG_LOG("CMD %s\n",cmdbuf);
                break;

            case 61:
                sprintf(cmdbuf, "%d,%d",ENG_AT_CGMR,0);
                cmdlen = strlen(cmdbuf);
                ENG_LOG("CMD %s\n",cmdbuf);
                break;

            default:
                cmdlen = -1;
                break;
        }

        if(cmdlen > 0) {
            ENG_LOG("Write Command, fd=%d\n", fd);
            if(eng_at_write(fd, cmdbuf, cmdlen)!=cmdlen) {
                ENG_LOG(" ENG AT WRITE Failed!\n");
                exit(-1);
            }
        }

        memset(readbuf, 0, sizeof(readbuf));
        readlen=eng_at_read(fd, readbuf, 512);

        ENG_LOG("ENG AT READ: %s\n", readbuf);
    }
    eng_close(fd );
    exit(0);

}

#endif

int eng_request(char *request, int requestlen, char *response, int *responselen, int sim)
{
    //char cmdbuf[16];
    char *cmdbuf = request;
    int cmdlen = requestlen;

    char readbuf[512];
    int readlen;
    int fd;

    ENG_LOG("Run Engineer Mode PC2SERVER Client!\n");

    fd = engapi_open(sim);
    if(fd<0) {
        ENG_LOG("ENG Open Failed!\n");
        //exit(-1);
        goto exit;
    }

        if(cmdlen > 0) {
            ENG_LOG("Write Command, fd=%d\n", fd);
            if(engapi_write(fd, cmdbuf, cmdlen)!=cmdlen) {
                ENG_LOG(" ENG AT WRITE Failed!\n");
                //exit(-1);
                goto exit;
            }
        }

        memset(readbuf, 0, sizeof(readbuf));
        readlen=engapi_read(fd, readbuf, 512);
        memcpy(response, readbuf, readlen);
        *responselen = readlen;
        ENG_LOG("ENG AT READ: %s\n", readbuf);
exit:
    if (fd>0){
        engapi_close(fd );
    }
    return 0;
}