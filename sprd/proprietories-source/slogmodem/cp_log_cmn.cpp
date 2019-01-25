/*
 *  cp_log_cmn.cpp - Common functions for the CP log and dump program.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "cp_log_cmn.h"
#include "def_config.h"
#include "parse_utils.h"

int get_timezone_diff(time_t tnow) {
  struct tm lt;

  if (!localtime_r(&tnow, &lt)) {
    return 0;
  }

  char tzs[16];
  if (!strftime(tzs, sizeof tzs, "%z", &lt)) {
    return 0;
  }

  // The time zone diff is in ISO 8601 format, i.e. one of:
  //   +|-hhmm
  //   +|-hh:mm
  //   +|-hh
  int sign;
  if ('+' == tzs[0]) {
    sign = 1;
  } else if ('-' == tzs[0]) {
    sign = -1;
  } else {
    return 0;
  }

  unsigned n;
  int toffset;
  if (parse_number(reinterpret_cast<uint8_t*>(tzs + 1), 2, n)) {
    return 0;
  }
  toffset = 3600 * static_cast<int>(n);

  if (!tzs[3]) {
    return sign * toffset;
  }

  // Skip the ':' if one exists.
  const uint8_t* m{reinterpret_cast<uint8_t*>(tzs + 3)};
  if (':' == tzs[3]) {
    ++m;
  }
  if (parse_number(m, 2, n)) {
    return 0;
  }

  toffset += (60 * static_cast<int>(n));
  return toffset * sign;
}

int copy_file_seg(int src_fd, int dest_fd, size_t m) {
  int err = 0;
  static const size_t FILE_COPY_BUF_SIZE = (1024 * 32);
  static uint8_t s_copy_buf[FILE_COPY_BUF_SIZE];
  size_t cum = 0;
  size_t left_len = m;

  while (cum < m) {
    size_t to_wr;
    left_len = m - cum;

    if (left_len > FILE_COPY_BUF_SIZE) {
      to_wr = FILE_COPY_BUF_SIZE;
    } else {
      to_wr = left_len;
    }

    ssize_t n = read(src_fd, s_copy_buf, to_wr);

    if (-1 == n) {
      err = -1;
      break;
    }
    if (!n) {  // End of file
      break;
    }
    cum += to_wr;

    n = write(dest_fd, s_copy_buf, to_wr);
    if (-1 == n || static_cast<size_t>(n) != to_wr) {
      err = -1;
      break;
    }
  }

  return err;
}

int copy_file(int src_fd, int dest_fd) {
  int err = 0;
  static const size_t FILE_COPY_BUF_SIZE = (1024 * 32);
  static uint8_t s_copy_buf[FILE_COPY_BUF_SIZE];

  while (true) {
    ssize_t n = read(src_fd, s_copy_buf, FILE_COPY_BUF_SIZE);

    if (-1 == n) {
      err = -1;
      break;
    }
    if (!n) {  // End of file
      break;
    }
    size_t to_wr = n;
    n = write(dest_fd, s_copy_buf, to_wr);
    if (-1 == n || static_cast<size_t>(n) != to_wr) {
      err = -1;
      break;
    }
  }

  return err;
}

int copy_file(const char* src, const char* dest) {
  // Open the source and the destination file
  int src_fd;
  int dest_fd;

  src_fd = open(src, O_RDONLY);
  if (-1 == src_fd) {
    err_log("open source file %s failed", src);
    return -1;
  }

  dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (-1 == dest_fd) {
    close(src_fd);
    err_log("open dest file %s failed", dest);
    return -1;
  }

  int err = copy_file(src_fd, dest_fd);

  close(dest_fd);
  close(src_fd);

  return err;
}

int set_nonblock(int fd) {
  long flag = fcntl(fd, F_GETFL);
  int ret = -1;

  if (flag != -1) {
    flag |= O_NONBLOCK;
    ret = fcntl(fd, F_SETFL, flag);
    if (ret < 0) {
      return -1;
    }
    ret = 0;
  }

  return ret;
}

void data2HexString(uint8_t* out_buf, const uint8_t* data, size_t len) {
  static uint8_t s_hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                              '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  int n = 0;
  for (size_t i = 0; i < len; ++i, n += 2) {
    out_buf[n] = s_hex[data[i] >> 4];
    out_buf[n + 1] = s_hex[data[i] & 0xf];
  }
}

int connect_tcp_server(int protocol_family,
                       const char* address, unsigned short port) {
  int sk = -1;

  sk = socket(protocol_family, SOCK_STREAM, 0);
  if (-1 == sk) {
    err_log("can't create client socket");
    return sk;
  }

  struct sockaddr_in addr;

  memset(&addr, 0, sizeof addr);
  addr.sin_family = protocol_family;
  addr.sin_port = htons(port);
  if (inet_pton(protocol_family, address, &addr.sin_addr) <= 0) {
    ::close(sk);
    sk = -1;
    err_log("fail to convert %s to network address", address);
  } else {
    if (connect(sk, (const struct sockaddr *)&addr, sizeof addr)) {
      ::close(sk);
      sk = -1;
      err_log("can't connect to server %s:%hu",
              address, port);
    }
  }

  return sk;
}

int get_sys_run_mode(SystemRunMode& rm) {
  // parse /proc/cmdline to determine if
  // bootmode is calibration, autotest or normal
  FILE* pf;
  pf = fopen("/proc/cmdline", "r");
  if (!pf) {
    err_log("can't open /proc/cmdline");
    return -1;
  }

  int ret = 0;
  uint8_t cmdline[LINUX_KERNEL_CMDLINE_SIZE] = {0};

  if (fgets(reinterpret_cast<char*>(cmdline), LINUX_KERNEL_CMDLINE_SIZE, pf)) {
    rm = SRM_NORMAL;
    size_t tok_len;
    const uint8_t* mode_str = cmdline;
    const uint8_t* tok;

    while (tok = get_token(mode_str, tok_len)) {
      if ((tok_len > 17) &&
          (!memcmp(tok, "androidboot.mode=", 17))) {
        if ((4 == tok_len - 17) &&
            (!memcmp(tok + 17, "cali", 4))) {
          rm = SRM_CALIBRATION;
        } else if ((8 == tok_len - 17) &&
                   (!memcmp(tok + 17, "autotest", 8))) {
          rm = SRM_AUTOTEST;
        }
        break;
      }

      mode_str = tok + tok_len;
    }
  } else if (ferror(pf)) {
    ret = -1;
    err_log("error when reading /proc/cmdline");
  } else {
    rm = SRM_NORMAL;
  }

  fclose(pf);

  return ret;
}

int operator - (const timeval& t1, const timeval& t2) {
  time_t sdiff = t1.tv_sec - t2.tv_sec;
  int diff;

  if (t1.tv_usec >= t2.tv_usec)
  {
    diff = static_cast<int>((t1.tv_usec - t2.tv_usec) / 1000 + sdiff * 1000);
  }
  else
  {
    --sdiff;
    diff = static_cast<int>((t1.tv_usec + 1000000 - t2.tv_usec) / 1000 + sdiff * 1000);
  }

  return diff;
}
