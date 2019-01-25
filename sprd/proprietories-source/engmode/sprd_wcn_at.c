#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <cutils/str_parms.h>
#include <cutils/sockets.h>
#include <dlfcn.h>

#include "engopt.h"
#include "eng_pcclient.h"
#include "eng_cmd4linuxhdlr.h"

#include "bqb.h"

static eng_dev_info_t* s_dev_info;
static int pc_fd = -1;

static const char *VENDOR_LIBRARY_NAME = "libbqbbt.so";
static const char *VENDOR_LIBRARY_SYMBOL_NAME = "BLUETOOTH_BQB_INTERFACE";
static void *lib_handle;
static bt_bqb_interface_t *lib_interface = NULL;


static bool bqb_vendor_open() {
    lib_handle = dlopen(VENDOR_LIBRARY_NAME, RTLD_NOW);
    if (!lib_handle) {
        ENG_LOG("%s unable to open %s: %s", __func__, VENDOR_LIBRARY_NAME, dlerror());
        goto error;
    }

    lib_interface = (bt_bqb_interface_t *)dlsym(lib_handle, VENDOR_LIBRARY_SYMBOL_NAME);
    if (!lib_interface) {
        ENG_LOG("%s unable to find symbol %s in %s: %s", __func__, VENDOR_LIBRARY_SYMBOL_NAME, VENDOR_LIBRARY_NAME, dlerror());
        goto error;
    }
    lib_interface->init();

      return true;

error:;
    lib_interface = NULL;
    if (lib_handle)
    dlclose(lib_handle);
    lib_handle = NULL;
    return false;
}

static int bqb_configure_fd(char* ser_path)
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

    if(lib_interface !=NULL)
        lib_interface->set_fd(pc_fd);

    tcgetattr(pc_fd, &ser_settings);
    cfmakeraw(&ser_settings);
    ser_settings.c_lflag |= (ECHO | ECHONL);
    ser_settings.c_lflag &= ~ECHOCTL;
    tcsetattr(pc_fd, TCSANOW, &ser_settings);

    return 0;
}

static void *eng_readwcnpcat_thread(void *par)
{
    int len;
    int written;
    int cur;
    char engbuf[ENG_BUFFER_SIZE];
    char databuf[ENG_BUFFER_SIZE];
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
            bqb_configure_fd(dev_info->host_int.dev_at);
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
                bqb_configure_fd(dev_info->host_int.dev_at);
                continue;
            }
            ENG_LOG("pc got: %s: %d", engbuf, len);

            if(lib_interface->check_received_str(pc_fd, engbuf, len))
                continue;
        } else {
            ENG_LOG("warning !!!");
        }

        if (lib_interface->get_bqb_state() == BQB_OPENED) {
            lib_interface->eng_send_data(engbuf, len);
        }
    }
    return NULL;
}



int eng_wcnat_pcmodem(eng_dev_info_t* dev_info)
{
    eng_thread_t t1;
    ENG_LOG("%s for bqb test", __func__);
    bqb_vendor_open();
    bqb_configure_fd(dev_info->host_int.dev_at);

    if (0 != eng_thread_create( &t1, eng_readwcnpcat_thread, (void*)dev_info)){
        ENG_LOG("read pcat thread start error");
    }
    return 0;
}
