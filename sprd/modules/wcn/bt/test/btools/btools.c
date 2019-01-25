#include <getopt.h>
#include <stdint.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cutils/sockets.h>
#include "btools.h"

#define  BTD(format, ...)                          \
    do{                                                 \
        printf(format "\n", ## __VA_ARGS__);    \
    }while(0)


#define ENCRYPT_CODE 0x83456734
#define HCI_WRITE_LOCAL_HW_REGISTER 0xFC2D
#define HCI_READ_LOCAL_HW_REGISTER 0xFC2F
#define HCI_WRITE_LOCAL_RF_REGISTER 0xFC30
#define HCI_READ_LOCAL_RF_REGISTER 0xFC31

#define VENDOR_HCI_SOCKET "bt_vendor_sprd_hci"

#define UINT32_TO_STREAM(p, u32)     \
  {                                  \
    *(p)++ = (uint8_t)(u32);         \
    *(p)++ = (uint8_t)((u32) >> 8);  \
    *(p)++ = (uint8_t)((u32) >> 16); \
    *(p)++ = (uint8_t)((u32) >> 24); \
  }
#define UINT16_TO_STREAM(p, u16)    \
  {                                 \
    *(p)++ = (uint8_t)(u16);        \
    *(p)++ = (uint8_t)((u16) >> 8); \
  }

#define STREAM_TO_ARRAY32(a, p)                     \
  {                                                 \
    int ijk;                                        \
    uint8_t* _pa = (uint8_t*)(a) + 31;              \
    for (ijk = 0; ijk < 32; ijk++) *_pa-- = *(p)++; \
  }

#define STREAM_TO_UINT32(u32, p)                                      \
  {                                                                   \
    (u32) = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + \
             ((((uint32_t)(*((p) + 2)))) << 16) +                     \
             ((((uint32_t)(*((p) + 3)))) << 24));                     \
    (p) += 4;                                                         \
}

#define STREAM_TO_UINT8(u8, p) \
  {                            \
    (u8) = (uint8_t)(*(p));    \
    (p) += 1;                  \
  }

#define UINT8_TO_STREAM(p, u8) \
  { *(p)++ = (uint8_t)(u8); }


static int connect_fd = -1;

typedef enum {
    ACTION_NONE,
    ACTION_READ,
    ACTION_WRITE,
} btools_action_t;

static char *action_string[] = {"UNKNOWN", "Read", "Write"};

typedef enum {
    TARGET_NONE,
    TARGET_BB,
    TARGET_RF,
} btools_target_t;

static char *target_string[] = {"NONE", "Bluetooth BB register", "Bluetooth RF register"};

static btools_action_t bt_action = ACTION_NONE;
static btools_target_t bt_target = TARGET_NONE;


static void usage(void){
    printf(
    "Usage: btools [OPTION...] [filter]\n"
    "  -R, --Read\n"
    "  -w, --Write\n"
    "  -h, --help                 Give this help list\n"
    "  -v, --version              Give version information\n"
    "      --usage                Give a short usage message\n"
    );
}

static void hrw_usage(void){
    printf(
    "Usage: btools [OPTION...] [filter]\n"
    "bttools -r rf 10 0x01b9\n"
    "bttools -w rf 10 0x01b9 0x01 0x02 0x03 0x04 0x05\n"
    "bttools -r bb 10 0x50040600\n"
    "bttools -w bb 10 0x50040600 0x01 0x02 0x03 0x04 0x05\n"
    );
}

static struct option main_options[] = {
    { "Read",            1, 0, 'r' },
    { "Write",        0, 0, 'w' },
    { "target",            1, 0, 't' },
    { "help",            0, 0, 'h' },
    { "version",        0, 0, 'v' },
    { 0 , 0, 0, 0}
};


static void signal_handler(int signo){
    BTD("%s got : %d", __func__, signo);
    if (connect_fd < 0){
        BTD("%s close fd: %d", __func__, connect_fd);
        close(connect_fd);
        connect_fd = -1;
    }
}

static int skt_connect(const char *path, size_t buffer_sz)
{
    int ret;
    int skt_fd;
    int len;

    BTD("connect to %s (sz %zu)", path, buffer_sz);

    if((skt_fd = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1)
    {
        BTD("failed to socket (%s)", strerror(errno));
        return -1;
    }

    if(socket_local_client_connect(skt_fd, path,
            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM) < 0)
    {
        BTD("failed to connect (%s)", strerror(errno));
        close(skt_fd);
        return -1;
    }

    if (buffer_sz != 0) {
        len = buffer_sz;
        ret = setsockopt(skt_fd, SOL_SOCKET, SO_SNDBUF, (char*)&len, (int)sizeof(len));

        /* only issue warning if failed */
        if (ret < 0)
            BTD("setsockopt failed (%s)", strerror(errno));

        ret = setsockopt(skt_fd, SOL_SOCKET, SO_RCVBUF, (char*)&len, (int)sizeof(len));

        /* only issue warning if failed */
        if (ret < 0)
            BTD("setsockopt failed (%s)", strerror(errno));

        BTD("connected to stack fd = %d", skt_fd);
    }
    return skt_fd;
}

static int call_wait_result(uint8_t *data, size_t length, uint8_t *recv_t)
{
    uint8_t recv[1024] = {0}, *p = NULL, status, len;
    uint32_t reg;
    int ret, socket_fd = skt_connect(VENDOR_HCI_SOCKET, 0);
    if (socket_fd < 0) {
        BTD("connect: %d error: %s", HCITOOLS_SOCKET, strerror(errno));
        return -1;
    }

    ret = write(socket_fd, data, length);
    ret = read(socket_fd, recv, sizeof(recv));
    if (ret < 0) {
        BTD("read error exit");
        return -1;
    }

    p = recv;
    STREAM_TO_UINT8(status, p);

    if (status != 0) {
        hrw_usage();
        close(socket_fd);
        exit(0);
    }

    STREAM_TO_UINT32(reg, p);
    STREAM_TO_UINT8(len, p);

    BTD("\tReturn status: 0x%02x, register: 0x%08x, length: %u", status, reg, len);

    close(socket_fd);

    ret = ret - 8;
    memcpy(recv_t, p , ret);
    return ret;
}



void action_hci_register_handle(uint8_t read_t, int length, int argc, char *argv[])
{
    uint8_t ehci_data[1024] = {0}, *p;
    uint8_t recv[1024] = {0};
    uint32_t reg, value;
    int i, ret, delta = 1;

    p = ehci_data;
    UINT32_TO_STREAM(p, ENCRYPT_CODE);

    if (bt_action == ACTION_READ && bt_target == TARGET_BB) {
        UINT16_TO_STREAM(p, HCI_READ_LOCAL_HW_REGISTER);
        delta = 4;
    } else if (bt_action == ACTION_WRITE && bt_target == TARGET_BB) {
        UINT16_TO_STREAM(p, HCI_WRITE_LOCAL_HW_REGISTER);
        delta = 4;
    } else if (bt_action == ACTION_READ && bt_target == TARGET_RF) {
        UINT16_TO_STREAM(p, HCI_READ_LOCAL_RF_REGISTER);

    } else if (bt_action == ACTION_WRITE && bt_target == TARGET_RF) {
        UINT16_TO_STREAM(p, HCI_WRITE_LOCAL_RF_REGISTER);

    } else {
        UINT16_TO_STREAM(p, 0);
    }

    reg = strtoul(argv[0], 0, 0);
    UINT8_TO_STREAM(p, length & 0xFF);
    UINT32_TO_STREAM(p, reg);

    if (read_t) {
        printf("Action:\n");
        printf("\t%s %s: 0x%08X length: %d\n", action_string[bt_action],
            target_string[bt_target], reg, length);

        ret = call_wait_result(ehci_data, p - ehci_data, recv);

        printf("\n\tRegister\tValue\n");

        p = recv;

        for (i = 0; i < length; i++) {
            STREAM_TO_UINT32(value, p);
            printf("\t0x%08X\t0x%08X\n", reg + (i * delta), value);
        }

    } else {
        printf("Action:\n");
        printf("\t%s %s: 0x%08X length: %d\n", action_string[bt_action],
            target_string[bt_target], reg, length);

        for (i = 0; i < argc -1; i++) {
            value = strtoul(argv[i + 1], 0, 0);
            UINT32_TO_STREAM(p, value);

        }

        ret = call_wait_result(ehci_data, p - ehci_data, recv);

        printf("\n\tRegister\tValue\n");

        p = recv;

        for (i = 0; i < length; i++) {
            STREAM_TO_UINT32(value, p);
            printf("\t0x%08X\t0x%08X\n", reg + (i * delta), value);
        }


    }
}


static void get_target(char *target_t)
{
    if (!strcmp(target_t, "bb")) {
        bt_target = TARGET_BB;
    } else if (!strcmp(target_t, "rf")) {
        bt_target = TARGET_RF;
    }
}

int main(int argc, char *argv[])
{
    int opt, i, length = 0;

    if (argc == 1)
        usage();

    while ((opt=getopt_long(argc, argv, "r:w:hv", main_options, NULL)) != -1) {
        switch(opt) {
        case 'r':
            bt_action = ACTION_READ;
            get_target(optarg);
            break;

        case 'w':
            bt_action = ACTION_WRITE;
            get_target(optarg);
            break;

        case 'v':
        case 'h':
        default:
            usage();
            printf("\n#############\n");
            hrw_usage();
        }
    }

    switch (bt_action) {
        case ACTION_READ:
        case ACTION_WRITE:
            if (argc - optind < 1) {
                hrw_usage();
                exit(0);
            }
            length = strtoul(argv[optind], 0, 0);
            action_hci_register_handle(bt_action == ACTION_READ, length, argc - optind - 1, argv + optind + 1);
            break;

        default:
            break;
    }
    return 0;
}
