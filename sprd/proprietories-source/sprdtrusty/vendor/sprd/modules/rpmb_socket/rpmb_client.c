/*
 * Copyright (c) 2017, Spreadtrum.com.
 * All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <cutils/sockets.h>
#include <poll.h>
#include "rpmb_server.h"
#include "rpmb_client.h"


#define LOG_TAG  "RPMB_CLIENT"
#include <cutils/log.h>

#define LOG_ERR  ALOGE
#define LOG_INF  ALOGI



int connect_rpmb_server(void)
{
    int fd = socket_local_client(RPMB_SERVER_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (fd < 0) {
        LOG_ERR("%s open %s failed: %s\n", __func__, RPMB_SERVER_NAME, strerror(errno));
        return -1;
    }

    return fd;
}


int wr_rpmb_key(uint8_t *key_byte, uint8_t key_len)
{
    int rpmb_fd;
    int ret = -1;
    int result = -1;
    enum rpmb_socket_cmd command;

    LOG_INF("Enter %s, key_len is %d \n", __func__, key_len);
    if (NULL == key_byte || 0 >= key_len) {
        LOG_ERR("%s parmeter error\n", __func__);
        return -1;
    }

    rpmb_fd = connect_rpmb_server();
    if (rpmb_fd < 0)
        return -1;

    command = WR_RPMB_KEY;
    ret = write(rpmb_fd, &command, sizeof(enum rpmb_socket_cmd));
    if (ret != sizeof(enum rpmb_socket_cmd)) {
        LOG_ERR("%s write command error, ret is %d, %s\n", __func__, ret, strerror(errno));
        close(rpmb_fd);
        return -1;
    }

    ret = write(rpmb_fd, key_byte, key_len);
    if (ret != key_len) {
        LOG_ERR("%s write key error, ret is %d, %s\n", __func__, ret, strerror(errno));
        close(rpmb_fd);
        return -1;
    }

    ret = read(rpmb_fd, &result, sizeof(result));
    if (ret != sizeof(result)) {
        LOG_ERR("%s read from server, ret is %d, result is %d \n", __func__, ret, result);
        close(rpmb_fd);
        return -1;
    }

    close(rpmb_fd);
    return result;
}

int is_wr_rpmb_key(void)
{
    int rpmb_fd;
    int ret = -1;
    int result = -1;
    enum rpmb_socket_cmd command;

    LOG_INF("Enter %s \n", __func__);

    rpmb_fd = connect_rpmb_server();
    if (rpmb_fd < 0)
        return -1;

    command = IS_WR_RPMB_KEY;
    ret = write(rpmb_fd, &command, sizeof(enum rpmb_socket_cmd));
    if (ret != sizeof(enum rpmb_socket_cmd)) {
        LOG_ERR("%s write command error, ret is %d, %s\n", __func__, ret, strerror(errno));
        close(rpmb_fd);
        return -1;
    }

    ret = read(rpmb_fd, &result, sizeof(result));
    if (ret != sizeof(result)) {
        LOG_ERR("%s read from server, ret is %d, result is %d \n", __func__, ret, result);
        close(rpmb_fd);
        return -1;
    }

    close(rpmb_fd);
    return result;
}

int run_storageproxyd(void)
{
    int rpmb_fd;
    int ret = -1;
    int result = -1;
    enum rpmb_socket_cmd command;

    LOG_INF("Enter %s \n", __func__);

    rpmb_fd = connect_rpmb_server();
    if (rpmb_fd < 0)
        return -1;

    command = RUN_STORAGEPROXY;
    ret = write(rpmb_fd, &command, sizeof(enum rpmb_socket_cmd));
    if (ret != sizeof(enum rpmb_socket_cmd)) {
        LOG_ERR("%s write command error, ret is %d, %s\n", __func__, ret, strerror(errno));
        close(rpmb_fd);
        return -1;
    }

    ret = read(rpmb_fd, &result, sizeof(result));
    if (ret != sizeof(result)) {
        LOG_ERR("%s read from server, ret is %d, result is %d \n", __func__, ret, result);
        close(rpmb_fd);
        return -1;
    }

    close(rpmb_fd);
    return result;
}
