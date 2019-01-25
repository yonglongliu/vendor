/*
 *  modem_parse_library_save.cpp - save modem log parse library
 *
 *  Copyright (C) 2017 Spreadtrum Communications Inc.
 *
 *  History:
 *  2017-4-1 Yan Zhihang
 *  Initial version.
 *
 *  2018-6-22 Zhang Ziyi
 *  Adapt to SC9820E platform.
 */

#include <cstdlib>
#ifdef HOST_TEST_
#include "prop_test.h"
#else
#include <cutils/properties.h>
#endif

#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "cp_dir.h"
#include "cp_log_cmn.h"
#include "def_config.h"
#include "log_config.h"
#include "modem_parse_library_save.h"
#include "multiplexer.h"

// Requests
const uint8_t ModemParseLibrarySave::kQuit = 0;
const uint8_t ModemParseLibrarySave::kSave = 1;
// Results
const uint8_t ModemParseLibrarySave::kSuccess = 0;
const uint8_t ModemParseLibrarySave::kFailure = 1;

ModemParseLibrarySave::ModemParseLibrarySave(LogController* ctrl,
                                             Multiplexer* multiplexer)
    :FdHandler{-1, ctrl, multiplexer},
     state_{NOT_INITED},
     save_thread_{},
     thrd_sock_{-1} {
  pthread_mutex_init(&mutex_, nullptr);
}

ModemParseLibrarySave::~ModemParseLibrarySave() {
  if (NOT_INITED != state_) {
    if (WORKING == state_) {
      wait_result();
    }
    write(fd(), &kQuit, 1);
    pthread_join(save_thread_, nullptr);

    multiplexer()->unregister_fd(this);
    ::close(fd());
    m_fd = -1;
    ::close(thrd_sock_);
    thrd_sock_ = -1;

    state_ = NOT_INITED;
  }

  pthread_mutex_destroy(&mutex_);
}

int ModemParseLibrarySave::init() {
  // Create socket pair first.
  int sk[2];

  if (-1 == socketpair(AF_LOCAL, SOCK_STREAM, 0, sk)) {
    err_log("create socket pair error");
    return -1;
  }

  m_fd = sk[0];
  thrd_sock_ = sk[1];

  int ret{};
  int err = pthread_create(&save_thread_, nullptr, save_parse_lib, this);

  if (err) {
    err_log("fail to create save modem library thread %d", err);

    ::close(sk[0]);
    ::close(sk[1]);
    m_fd = -1;
    thrd_sock_ = -1;

    ret = -1;
  } else {
    multiplexer()->register_fd(this, POLLIN);

    state_ = IDLE;
  }

  return ret;
}

int ModemParseLibrarySave::trigger_parse_lib(const char* dir) {
  if (IDLE != state_) {
    err_log("trigger on wrong state %d", state_);
    return -1;
  }

  pthread_mutex_lock(&mutex_);
  path_ = dir;
  pthread_mutex_unlock(&mutex_);

  int ret{};
  ssize_t nw = ::write(fd(), &kSave, 1);

  if (1 != nw) {
    err_log("fail to request to save parsing lib");
    ret = -1;
  } else {
    state_ = WORKING;
    info_log("start saving parsing lib ...");
  }

  return ret;
}

int ModemParseLibrarySave::wait_result() {
  pollfd pfd;

  pfd.fd = fd();
  pfd.events = POLLIN;

  int ret;

  while (true) {
    ret = poll(&pfd, 1, -1);
    if (ret < 0) {
      err_log("poll saving thread error");
      return -1;
    }
    if (pfd.revents & POLLIN) {
      break;
    }
  }

  uint8_t data;
  ssize_t nr;

  nr = read(fd(), &data, 1);
  if (1 == nr) {
    state_ = IDLE;

    if (kSuccess == data) {
      info_log("save parsing lib succeeded");
    } else {
      err_log("save parsing lib failed %u",
              static_cast<unsigned>(data));
    }
    ret = 0;
  } else {
    err_log("read thread result failed %ld",
            static_cast<long>(nr));
    ret = -1;
  }

  return ret;
}

void* ModemParseLibrarySave::save_parse_lib(void* arg) {
  ModemParseLibrarySave* pls = static_cast<ModemParseLibrarySave*>(arg);

  // Wait for requests
  bool run{true};
  pollfd pfd;

  pfd.fd = pls->thrd_sock_;
  pfd.events = POLLIN;

  while (run) {
    int ret = poll(&pfd, 1, -1);

    if (ret <= 0) {
      continue;
    }

    uint8_t req;
    ssize_t nr = read(pfd.fd, &req, 1);

    if (1 != nr) {
      err_log("read request error");
      continue;
    }

    switch (req) {
      case kQuit:
        run = false;
        break;
      case kSave:
        pls->proc_save_req();
        break;
      default:
        err_log("unknown request %u", static_cast<unsigned>(req));
        break;
    }
  }

  return nullptr;
}

void ModemParseLibrarySave::proc_save_req() {
  uint8_t result{kSuccess};

  if (save_log_parsing_lib()) {
    result = kFailure;
  }

  ::write(thrd_sock_, &result, 1);
}

int ModemParseLibrarySave::check_cur_parsing_lib(const LogString& lib_name,
                                                 const LogString& sha1_name,
                                                 uint8_t* old_sha1,
                                                 size_t& old_len) {
  struct stat file_stat;

  if (lstat(ls2cstring(lib_name), &file_stat)) {
    err_log("lstat(%s) error", ls2cstring(lib_name));
    return -1;
  }

  int sha1_fd = ::open(ls2cstring(sha1_name), O_RDONLY);

  if (-1 == sha1_fd) {
    err_log("open(%s) error", ls2cstring(sha1_name));
    return -1;
  }

  ssize_t read_len = read(sha1_fd, old_sha1, 40);
  ::close(sha1_fd);

  int ret;

  if (40 == read_len) {
    old_len = static_cast<size_t>(file_stat.st_size);
    ret = 0;
  } else {
    err_log("read SHA-1 file error: %d", static_cast<int>(read_len));
    ret = -1;
  }

  return ret;
}


int ModemParseLibrarySave::get_image_path(CpType ct, char* img_path, size_t len) {
  char path_prefix[PROPERTY_VALUE_MAX];

  property_get("ro.product.partitionpath", path_prefix, "");
  if (!path_prefix[0]) {
    err_log("no ro.product.partitionpath property");
    return -1;
  }

  char prop[32];
  const char* prop_suffix;

  switch (ct) {
    case CT_WCDMA:
      prop_suffix = "w.nvp";
      break;
    case CT_TD:
      prop_suffix = "t.nvp";
      break;
    case CT_3MODE:
      prop_suffix = "tl.nvp";
      break;
    case CT_4MODE:
      prop_suffix = "lf.nvp";
      break;
    default:  // CT_5MODE
      prop_suffix = "l.nvp";
      break;
  }
  snprintf(prop, sizeof prop, "%s%s", PERSIST_MODEM_CHAR, prop_suffix);

  char path_mid[PROPERTY_VALUE_MAX];

  property_get(prop, path_mid, "");
  if (!path_mid[0]) {
    err_log("no %s property", prop);
    return -1;
  }

  snprintf(img_path, len, "%s%smodem", path_prefix, path_mid);

  return 0;
}

int ModemParseLibrarySave::open_modem_img() {
  CpType ct = LogConfig::get_wan_modem_type();

  if (CT_UNKNOWN == ct) {
    err_log("can not get MODEM type");
    return -1;
  }

  char img_path[128];

  if (get_image_path(ct, img_path, sizeof img_path)) {
    err_log("can not get MODEM image file path");
    return -1;
  }

  return ::open(img_path, O_RDONLY);
}

int ModemParseLibrarySave::get_modem_img(int img_fd, uint8_t* new_sha1,
                                      size_t& lib_offset, size_t& lib_len) {

#ifdef SECURE_BOOT_ENABLE
  if (SECURE_BOOT_SIZE != lseek(img_fd, SECURE_BOOT_SIZE, SEEK_SET)) {
    err_log("lseek secure boot fail.");
    return -1;
  }
#endif

  // There are two headers at least
  uint8_t buf[24];
  ssize_t n = read(img_fd, buf, 24);

  if (24 != n) {
    err_log("MODEM image length less than two headers");
    return -1;
  }

  if (memcmp(buf, "SCI1", 4)) {
    err_log("MODEM image not SCI1 format");
    return -1;
  }

  bool found = false;

  while (true) {
    if (2 == buf[12]) {
      if (buf[13] & 4) {
        found = true;
      } else {
        err_log("MODEM image has no SHA-1 checksum");
      }
      break;
    }
    if (buf[13] & 1) {
      break;
    }
    n = read(img_fd, buf + 12, 12);
    if (12 != n) {
      err_log("can not read more headers %d", static_cast<int>(n));
      break;
    }
  }

  if (found) {
    // Offset
    uint32_t offset = buf[16];

    offset += (static_cast<uint32_t>(buf[17]) << 8);
    offset += (static_cast<uint32_t>(buf[18]) << 16);
    offset += (static_cast<uint32_t>(buf[19]) << 24);
    lib_offset = offset + 20;
#ifdef SECURE_BOOT_ENABLE
    lib_offset += SECURE_BOOT_SIZE;
#endif

    // Length
    uint32_t val = buf[20];
    val += (static_cast<uint32_t>(buf[21]) << 8);
    val += (static_cast<uint32_t>(buf[22]) << 16);
    val += (static_cast<uint32_t>(buf[23]) << 24);
    lib_len = val - 20;

    // Get SHA-1 checksum
    if (static_cast<off_t>(offset) == lseek(img_fd, offset, SEEK_SET)) {
      uint8_t sha1[20];

      n = read(img_fd, sha1, 20);
      if (20 != n) {
        found = false;
        err_log("read SHA-1 error %d", static_cast<int>(n));
      } else {
        data2HexString(new_sha1, sha1, 20);
      }
    } else {
      err_log("lseek MODEM image error");
    }
  }

  return found ? 0 : -1;
}

int ModemParseLibrarySave::save_log_lib(const LogString& lib_name,
                                        const LogString& sha1_name, int modem_part,
                                        size_t offset, size_t len,
                                        const uint8_t* sha1) {
  if (static_cast<off_t>(offset) != lseek(modem_part, offset, SEEK_SET)) {
    err_log("lseek MODEM image file %u error", static_cast<unsigned>(offset));
    return -1;
  }

  int log_lib_file = ::open(ls2cstring(lib_name), O_CREAT | O_WRONLY | O_TRUNC,
                            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (-1 == log_lib_file) {
    err_log("open %s failed", ls2cstring(lib_name));
    return -1;
  }

  int ret = copy_file_seg(modem_part, log_lib_file, len);

  ::close(log_lib_file);
  if (ret < 0) {
    err_log("save parsing lib error");
    unlink(ls2cstring(lib_name));
    return ret;
  }

  int sha1_file = ::open(ls2cstring(sha1_name), O_CREAT | O_WRONLY | O_TRUNC,
                         S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (-1 == sha1_file) {
    err_log("create SHA-1 file error");
    unlink(ls2cstring(lib_name));
    return -1;
  }

  ssize_t wr_len = write(sha1_file, sha1, 40);

  ::close(sha1_file);
  if (40 != wr_len) {
    err_log("write SHA-1 file error %d", static_cast<int>(wr_len));
    unlink(ls2cstring(sha1_name));
    unlink(ls2cstring(lib_name));
    ret = -1;
  } else {
    ret = 0;
  }

  return ret;
}

int ModemParseLibrarySave::save_log_parsing_lib() {
  uint8_t old_sha1[40];
  size_t old_len;
  const LogString lib_name = path_ + "/modem_db.gz";
  const LogString sha1_name = path_ + "/sha1.txt";
  int ret = check_cur_parsing_lib(lib_name, sha1_name, old_sha1, old_len);
  int modem_part = open_modem_img();

  if (-1 == modem_part) {
    err_log("open MODEM image failed");
    return -1;
  }

  uint8_t new_sha1[40];
  size_t lib_offset;
  size_t lib_len;
  int modem_img_ret = get_modem_img(modem_part, new_sha1, lib_offset, lib_len);

  if (modem_img_ret) {  // Get parsing lib offset and length failed
    ::close(modem_part);
    err_log("get MODEM image info error");
    return -1;
  }

  bool is_same = false;

  if (!ret && (old_len == lib_len) && !memcmp(old_sha1, new_sha1, 40)) {
    is_same = true;
  }

  if (!is_same) {  // Different parsing lib
    ret = save_log_lib(lib_name, sha1_name, modem_part, lib_offset, lib_len,
                       new_sha1);
  } else {
    ret = 0;
  }

  ::close(modem_part);

  return ret;
}

void ModemParseLibrarySave::process(int /*events*/) {
  if (WORKING != state_) {
    err_log("result received on wrong state %u",
            static_cast<unsigned>(state_));

    uint8_t buf[16];

    while (true) {
      ssize_t nr = read(fd(), buf, 16);
      if (nr <= 0) {
        break;
      }
    }

    return;
  }

  uint8_t result;

  ssize_t nr = read(fd(), &result, 1);

  if (1 == nr) {
    switch (result) {
      case kSuccess:
        state_ = IDLE;
        info_log("save parsing lib success");
        break;
      case kFailure:
        state_ = IDLE;
        err_log("save parsing lib error");
        break;
      default:
        err_log("unknown saving result %d",
                static_cast<unsigned>(result));
        break;
    }
  } else {
    err_log("read saving result failed");
  }
}
