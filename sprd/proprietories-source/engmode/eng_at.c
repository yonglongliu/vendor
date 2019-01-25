#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "engopt.h"
#include "eng_pcclient.h"
#include "eng_cmd4linuxhdlr.h"

static eng_dev_info_t* s_dev_info;
static int at_mux_fd = -1;
static int pc_fd = -1;

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <cutils/str_parms.h>
#include <cutils/sockets.h>
#include <dlfcn.h>
#include "eng_diag.h"


static int start_gser(char* ser_path)
{
    struct termios ser_settings;

    if (pc_fd>=0){
        ENG_LOG("%s ERROR : %s\n", __FUNCTION__, strerror(errno));
        close(pc_fd);
    }

    ENG_LOG("open serial\n");
    pc_fd = open(ser_path,O_RDWR);
    if(pc_fd < 0) {
        ENG_LOG("cannot open vendor serial\n");
        return -1;
    }

    tcgetattr(pc_fd, &ser_settings);
    cfmakeraw(&ser_settings);

    ser_settings.c_lflag |= (ECHO | ECHONL);
    ser_settings.c_lflag &= ~ECHOCTL;


    tcsetattr(pc_fd, TCSANOW, &ser_settings);

    return 0;
}

int at_write_to_pc(char *rsp, int len){
  int ret = -1;

  ENG_LOG("%s: pc_fd:%d\n", __FUNCTION__, pc_fd);
  if (NULL == rsp) {
      ENG_LOG("%s rsp == NULL", __FUNCTION__);
      return ret;
  }

at_write:
  if (pc_fd >= 0) {
    ret = write(pc_fd, rsp, len);
    if (ret <= 0) {
      ENG_LOG("%s: write length error %s\n", __FUNCTION__, strerror(errno));
      sleep(1);
      start_gser(s_dev_info->host_int.dev_at);
      goto at_write;
    }
  } else {
    ENG_LOG("%s: write fail : pc_fd < 0 \n", __FUNCTION__);
  }

  return ret;
}

static void *eng_readpcat_thread(void *par)
{
    int len;
    int written;
    int cur;
    char engbuf[ENG_BUFFER_SIZE];
    char databuf[ENG_BUFFER_SIZE];
    char eng_rsp_buf[2 * ENG_DIAG_SIZE];
    int i, offset_read, length_read, status;
    eng_dev_info_t* dev_info = (eng_dev_info_t*)par;
    int ret;
    int max_fd = pc_fd;
    fd_set read_set;

    for (;;) {
        ENG_LOG("wait for command / byte stream");
        FD_ZERO(&read_set);

        if (pc_fd > 0) {
            FD_SET(pc_fd, &read_set);
        } else {
            sleep(1);
            start_gser(dev_info->host_int.dev_at);
            continue;
        }

        ret = select(max_fd+1, &read_set, NULL, NULL, NULL);
        if (ret == 0) {
            ENG_LOG("select timeout");
            continue;
        } else if (ret < 0) {
            ENG_LOG("select failed %s", strerror(errno));
            continue;
        }
        if (FD_ISSET(pc_fd, &read_set)) {
            memset(engbuf, 0, ENG_BUFFER_SIZE);
            len = read(pc_fd, engbuf, ENG_BUFFER_SIZE);
            if (len <= 0) {
                ENG_LOG("%s: read pc_fd buffer error %s",__FUNCTION__,strerror(errno));
                sleep(1);
                start_gser(dev_info->host_int.dev_at);
                continue;
            }

            ENG_LOG("pc got: %s: %d", engbuf, len);

            memset(eng_rsp_buf, 0, 2 * ENG_DIAG_SIZE);
            if (DYMIC_RET_DEAL_SUCCESS ==
                eng_at_dymic_hdlr(engbuf, len, eng_rsp_buf,
                                  sizeof(eng_rsp_buf), at_write_to_pc)) {
              ENG_LOG("%s eng_at_dymic_hdrl deal success!", __FUNCTION__);
              continue;
            }
        } else {
            ENG_LOG("warning !!!");
        }

        {
            if(at_mux_fd >= 0) {
                cur = 0;
                while(cur < len) {
                    do {
                        written = write(at_mux_fd, engbuf + cur, len -cur);
                        ENG_LOG("muxfd=%d, written=%d\n", at_mux_fd, written);
                    }while(written < 0 && errno == EINTR);

                    if(written < 0) {
                        ENG_LOG("%s: write length error %s\n", __FUNCTION__, strerror(errno));
                        break;
                    }
                    cur += written;
                }
            }else {
                ENG_LOG("muxfd fail?");
            }
        }
    }
    return NULL;
}

static void *eng_readmodemat_thread(void *par)
{
    int ret;
    int len;
    char engbuf[ENG_BUFFER_SIZE];
    eng_dev_info_t* dev_info = (eng_dev_info_t*)par;

    for(;;){
        ENG_LOG("%s: wait pcfd=%d\n",__func__,pc_fd);
        memset(engbuf, 0, ENG_BUFFER_SIZE);
        len = read(at_mux_fd, engbuf, ENG_BUFFER_SIZE);
        ENG_LOG("muxfd =%d buf=%s,len=%d\n",at_mux_fd,engbuf,len);
        if (len <= 0) {
            ENG_LOG("%s: read length error %s\n",__FUNCTION__,strerror(errno));
            sleep(1);
            continue;
        }else{
write_again:
            if (pc_fd>=0){
                ret = write(pc_fd,engbuf,len);
                if (ret <= 0) {
                    ENG_LOG("%s: write length error %s\n",__FUNCTION__,strerror(errno));
                    sleep(1);
                    start_gser(dev_info->host_int.dev_at);
                    goto write_again;
                }
            }else{
                sleep(1);
            }
        }
    }
    return NULL;
}

int eng_at_pcmodem(eng_dev_info_t* dev_info)
{
    eng_thread_t t1,t2;

    start_gser(dev_info->host_int.dev_at);

    at_mux_fd = open(dev_info->modem_int.at_chan, O_RDWR);
    if(at_mux_fd < 0){
        ENG_LOG("%s: open %s fail [%s]\n",__FUNCTION__, dev_info->modem_int.at_chan,strerror(errno));
        return -1;
    }

    if (0 != eng_thread_create( &t1, eng_readpcat_thread, (void*)dev_info)){
        ENG_LOG("read pcat thread start error");
    }

    if (0 != eng_thread_create( &t2, eng_readmodemat_thread, (void*)dev_info)){
        ENG_LOG("read modemat thread start error");
    }
    return 0;
}
