/*
 * Copyright (c) 2015, Spreadtrum.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <poll.h>

enum loglevel {
    LOG_ERROR,
    LOG_CRITICAL,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
};

int debug_level = LOG_DEBUG;
#ifdef ANDROID
#define LOG_TAG "RPMB_CLI"
#include "cutils/log.h"
#include <cutils/sockets.h>
#define ___rpmb_printf_trace___ SLOGW
#define ___rpmb_printf___ printf
#else
#define ___rpmb_printf_trace___(x...)
#define ___rpmb_printf___ printf
#endif
#define cli_trace(msg...)  ___rpmb_printf_trace___(msg)
#define cli_printf(msg...) ___rpmb_printf___(msg)
#define cli_printf_debug(l, msg...) if (debug_level >= l) cli_printf(msg)
#define cli_debug(msg...) cli_printf_debug(LOG_DEBUG, "rpmb<debug> "msg)
#define cli_info(msg...) cli_printf_debug(LOG_INFO, "rpmb<info> "msg)
#define cli_warn(msg...) cli_printf_debug(LOG_WARN, "rpmb<warn> "msg)
#define cli_critical(msg...) cli_printf_debug(LOG_CRITICAL, "rpmb<critical> "msg)
#define cli_error(msg...) { cli_printf_debug(LOG_ERROR, "rpmb<error> "msg); cli_trace("rpmb<error> "msg); }

#ifdef ANDROID
int connect_socket_local_client(char *name) {
    int fd = socket_local_client(name, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (fd < 0) {
        cli_error("%s open %s failed: %s\n", __func__, name, strerror(errno));
        return -1;
    }
    return fd;
}
#else
int connect_socket_local_client(char *name) {
    struct sockaddr_un address;
    int fd;
    int namelen;
    /* init unix domain socket */
    fd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        cli_error("%s open %s failed: %s\n", __func__, name, strerror(errno));
        return -1;
    }

    namelen = strlen(name);
    /* Test with length +1 for the *initial* '\0'. */
    if ((namelen + 1) > (int)sizeof(address.sun_path)) {
        cli_critical("%s %s length is too long\n", __func__, name);
        close(fd);
        return -1;
    }
    /* Linux-style non-filesystem Unix Domain Sockets */
    memset(&address, 0, sizeof(address));
    address.sun_family = PF_LOCAL;
    strcpy(&address.sun_path[1], name); /* local abstract socket server */

    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        cli_error("%s connect %s failed: %s\n", __func__, name, strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}
#endif

int rpmb_program_key(uint8_t *key_byte, size_t key_len){
    int rpmb_fd;
    int ret = 0;
    int result = 0;
    cli_error("Enter %s, key_len is %d \n", __func__, key_len);

    rpmb_fd = connect_socket_local_client("rpmb_cli");
    if (rpmb_fd < 0)
        return -1;

    char buf[] = "rpmbkey";
    write(rpmb_fd, buf, strlen(buf));

    write(rpmb_fd, key_byte, key_len);
    ret = read(rpmb_fd, &result, sizeof(result));
    cli_error("%s read from server, ret is %d, result is %d \n", __func__, ret, result);

    close(rpmb_fd);
    return result;
}
