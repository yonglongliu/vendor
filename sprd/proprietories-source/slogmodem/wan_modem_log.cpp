/*
 *  wan_modem_log.cpp - The WAN MODEM log and dump handler class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-7-13 Zhang Ziyi
 *  Initial version.
 */

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include "client_mgr.h"
#include "cp_dir.h"
#include "cp_stor.h"
#include "diag_dev_hdl.h"
#include "diag_stream_parser.h"
#include "log_config.h"
#include "log_ctrl.h"
#include "modem_parse_library_save.h"
#include "pm_modem_dump.h"
#include "trans_modem_ver.h"
#include "wan_modem_log.h"

WanModemLogHandler::WanModemLogHandler(LogController* ctrl, Multiplexer* multi,
                                       const LogConfig::ConfigEntry* conf,
                                       StorageManager& stor_mgr,
                                       const char* dump_path)
    : LogPipeHandler{ctrl, multi, conf, stor_mgr},
      m_dump_path{dump_path},
      m_wcdma_iq_mgr{this},
      m_save_wcdma_iq{false},
      m_save_md{false},
      m_md_to_int{false},
      m_timestamp_miss{false},
      m_ver_state{MVS_NOT_BEGIN},
      m_ver_stor_state{MVSS_NOT_SAVED},
      m_reserved_version_len{0},
      parse_lib_save_{} {
  m_time_sync_notifier = new WanModemTimeSync(controller(), multiplexer());
  m_time_sync_notifier->subscribe_cp_time_update_evt(this,
      notify_modem_time_update);
  m_time_sync_notifier->init();
}

WanModemLogHandler::~WanModemLogHandler() {
  if (m_time_sync_notifier) {
    m_time_sync_notifier->unsubscribe_cp_time_update_evt(this);
    delete m_time_sync_notifier;
    m_time_sync_notifier = nullptr;
  }
}

int WanModemLogHandler::start_dump(const struct tm& lt) {
  // Create the data consumer first
  int err = 0;
  PmModemDumpConsumer* dump;
  DiagDeviceHandler* diag;

  dump = new PmModemDumpConsumer(name(), *storage(), lt, m_dump_path,
                                 "\nmodem_memdump_finish", 0x33);

  dump->set_callback(this, diag_transaction_notify);

  diag = create_diag_device(dump);
  if (diag) {
    multiplexer()->register_fd(diag, POLLIN);
    dump->bind(diag);
    if (dump->start()) {
      err = -1;
    }
  } else {
    err = -1;
  }

  if (err) {
    delete diag;
    delete dump;
  } else {
    start_transaction(diag, dump, CWS_DUMP);
  }

  return err;
}

void WanModemLogHandler::diag_transaction_notify(
    void* client, DataConsumer::LogProcResult res) {
  WanModemLogHandler* cp = static_cast<WanModemLogHandler*>(client);

  if (CWS_DUMP == cp->cp_state()) {
    cp->end_dump();
    ClientManager* cli_mgr = cp->controller()->cli_mgr();
    cli_mgr->notify_cp_dump(cp->type(), ClientHandler::CE_DUMP_END);
  } else {
    err_log("Receive diag notify %d under state %d, ignore", res,
            cp->cp_state());
  }
}

int WanModemLogHandler::create_storage() {
  int ret = LogPipeHandler::create_storage();

  if (!ret) {
    storage()->set_new_log_callback(new_log_callback);
  }

  return ret;
}

int WanModemLogHandler::start_ver_query() {
  WanModemVerQuery* query = new WanModemVerQuery{name(), *storage()};
  query->set_callback(this, transaction_modem_ver_notify);
  DiagDeviceHandler* diag = create_diag_device(query);
  int err = 0;

  if (diag) {
    multiplexer()->register_fd(diag, POLLIN);
    query->bind(diag);
    if (query->start()) {
      err = -1;
    }
  } else {
    err = -1;
  }

  if (err) {
    delete diag;
    delete query;
  } else {
    start_transaction(diag, query, CWS_QUERY_VER);
  }

  return err;
}

void WanModemLogHandler::fill_smp_header(uint8_t* buf, size_t len,
                                         unsigned lcn, unsigned type) {
  memset(buf, 0x7e, 4);
  buf[4] = static_cast<uint8_t>(len);
  buf[5] = static_cast<uint8_t>(len >> 8);
  buf[6] = static_cast<uint8_t>(lcn);
  buf[7] = static_cast<uint8_t>(type);
  buf[8] = 0x5a;
  buf[9] = 0x5a;

  uint32_t cs = ((type << 8) | lcn);

  cs += (len + 0x5a5a);
  cs = (cs & 0xffff) + (cs >> 16);
  cs = ~cs;
  buf[10] = static_cast<uint8_t>(cs);
  buf[11] = static_cast<uint8_t>(cs >> 8);
}

void WanModemLogHandler::fill_diag_header(uint8_t* buf, uint32_t sn,
                                          unsigned len, unsigned cmd,
                                          unsigned subcmd) {
  buf[0] = static_cast<uint8_t>(sn);
  buf[1] = static_cast<uint8_t>(sn >> 8);
  buf[2] = static_cast<uint8_t>(sn >> 16);
  buf[3] = static_cast<uint8_t>(sn >> 24);

  buf[4] = static_cast<uint8_t>(len);
  buf[5] = static_cast<uint8_t>(len >> 8);
  buf[6] = static_cast<uint8_t>(cmd);
  buf[7] = static_cast<uint8_t>(subcmd);
}

uint8_t* WanModemLogHandler::frame_noversion_and_reserve(size_t& length) {
  const char* unavailable = "(unavailable)";
  uint8_t* ret = nullptr;

  if (log_diag_dev_same()) {
    // Keep the place holder for diag frame
    // modem version length * 2 (escaped length) +
    // 8 * 2 (length for escaped header) + 2 (two flags)
    length = (PRE_MODEM_VERSION_LEN << 1) + 18;
    ret = new uint8_t[length];

    size_t framed_len;
    uint8_t* diag_frame =
        DiagStreamParser::frame(0xffffffff, 0, 0,
                                reinterpret_cast<uint8_t*>(
                                    const_cast<char*>(unavailable)),
                                strlen(unavailable), framed_len);
    memcpy(ret, diag_frame, framed_len);
    delete [] diag_frame;
    memset(ret + framed_len, 0x7e, length - framed_len);
  } else {
    // Keep the place holder for smp frame
    // modem version length + 12(smp) + 8(diag)
    length = PRE_MODEM_VERSION_LEN + 20;
    ret = new uint8_t[length];
    memset(ret, ' ', length);

    fill_smp_header(ret, length - 4, 1, 0x9d);
    fill_diag_header(ret + 12, 0xffffffff, length - 12, 0, 0);
    memcpy(ret + 20, unavailable, strlen(unavailable));
  }

  return ret;
}

uint8_t* WanModemLogHandler::frame_version_info(const uint8_t* payload,
                                                size_t pl_len,
                                                size_t len_reserved,
                                                size_t& frame_len) {
  uint8_t* ret = nullptr;

  if (log_diag_dev_same()) {
    ret = DiagStreamParser::frame(0xffffffff, 0, 0, payload,
                                  pl_len, frame_len);
    if (len_reserved && (frame_len > len_reserved)) {
      // if framed info is larger than preserved, the next diag frame will
      // be destroyed.
      delete [] ret;
      ret = nullptr;
      err_log("reserved length %u is less than needed %u",
              static_cast<unsigned>(len_reserved),
              static_cast<unsigned>(frame_len));
    }
  } else {
    if (len_reserved) {
      if (len_reserved < pl_len + 20) {
        err_log("reserved length %u is less than needed %u",
                static_cast<unsigned>(len_reserved),
                static_cast<unsigned>(pl_len + 20));
        return nullptr;
      } else {
        frame_len = len_reserved;
      }
    } else {
      frame_len = 20 + pl_len;
    }

    ret = new uint8_t[frame_len];
    memset(ret, ' ', frame_len);

    fill_smp_header(ret, frame_len - 4, 1, 0x9d);
    fill_diag_header(ret + 12, 0xffffffff, frame_len - 12, 0, 0);
    memcpy(ret + 20, payload, pl_len);
  }

  return ret;
}

void WanModemLogHandler::correct_ver_info() {
  CpStorage* stor = storage();
  DataBuffer* buf = stor->get_buffer();

  if (!buf) {
    err_log("No buffer for correcting MODEM version info");
    return;
  }

  size_t size_to_frame = m_modem_ver.length();
  if (size_to_frame > PRE_MODEM_VERSION_LEN) {
    size_to_frame = PRE_MODEM_VERSION_LEN;
  }

  size_t framed_len;
  uint8_t* ver_framed =
      frame_version_info(reinterpret_cast<uint8_t*>(const_cast<char*>(
                             ls2cstring(m_modem_ver))),
                         size_to_frame, m_reserved_version_len, framed_len);
  if (!ver_framed) {
    err_log("fail to correct modem version information");
    stor->free_buffer(buf);
    return;
  }

  memcpy(buf->buffer, ver_framed, framed_len);
  delete [] ver_framed;

  buf->dst_offset = sizeof(modem_timestamp);
  buf->data_len = framed_len;

  if (!stor->amend_current_file(buf)) {
    stor->free_buffer(buf);
    err_log("amend_current_file MODEM version error");
  }
}

void WanModemLogHandler::transaction_modem_ver_notify(void* client,
    DataConsumer::LogProcResult res) {
  WanModemLogHandler* cp = static_cast<WanModemLogHandler*>(client);

  if (CWS_QUERY_VER == cp->cp_state()) {
    if (DataConsumer::LPR_SUCCESS == res) {
      WanModemVerQuery* query = static_cast<WanModemVerQuery*>(cp->consumer());
      size_t len;
      const char* ver = query->version(len);
      str_assign(cp->m_modem_ver, ver, len);
      cp->m_ver_state = MVS_GOT;

      info_log("WAN MODEM ver: %s", ls2cstring(cp->m_modem_ver));

      LogFile* cur_file = cp->storage()->current_file();

      if (cur_file && MVSS_NOT_SAVED == cp->m_ver_stor_state) {
        // Correct the version info
        cp->correct_ver_info();
        cp->m_ver_stor_state = MVSS_SAVED;
      }
    } else {  // Failed to get version
      cp->m_ver_state = MVS_FAILED;

      err_log("query MODEM software version failed");
    }

    cp->stop_transaction(CWS_WORKING);
  } else {
    err_log("receive MODEM version query result %d under state %d, ignore",
            res, cp->cp_state());
  }
}

bool WanModemLogHandler::save_timestamp(LogFile* f) {
  time_sync ts;
  bool ret = false;
  modem_timestamp modem_ts = {0, 0, 0, 0};

  if (m_time_sync_notifier->active_get_time_sync_info(ts)) {
    m_timestamp_miss = false;
    calculate_modem_timestamp(ts, modem_ts);
  } else {
    m_timestamp_miss = true;
    err_log("Wan modem timestamp is not available.");
  }

  // if timestamp is not available, 0 will be written to the first 16 bytes.
  // otherwise, save the timestamp.
  ssize_t n = f->write_raw(&modem_ts, sizeof(uint32_t) * 4);

  if (static_cast<size_t>(n) == (sizeof(uint32_t) * 4)) {
    ret = true;
  } else {
    err_log("write timestamp fail.");
  }

  if (n > 0) {
    f->add_size(n);
  }

  return ret;
}

void WanModemLogHandler::on_new_log_file(LogFile* f) {
  save_timestamp(f);

  // MODEM version info
  uint8_t* buf = nullptr;
  // resulted length to be written
  size_t ver_len = 0;

  if (MVS_GOT == m_ver_state) {
    buf = frame_version_info(reinterpret_cast<uint8_t*>(const_cast<char*>(
                                 ls2cstring(m_modem_ver))),
                             m_modem_ver.length(), 0, ver_len);
    m_ver_stor_state = MVSS_SAVED;
  } else {
    info_log("no MODEM version when log file is created");

    buf = frame_noversion_and_reserve(ver_len);
    m_reserved_version_len = ver_len;
    m_ver_stor_state = MVSS_NOT_SAVED;
  }

  ssize_t nwr = f->write_raw(buf, ver_len);
  delete [] buf;

  if (nwr > 0) {
    f->add_size(nwr);
  }
}

void WanModemLogHandler::notify_modem_time_update(void* client,
    const time_sync& ts) {
  WanModemLogHandler* wan = static_cast<WanModemLogHandler*>(client);

  if (!wan->enabled()) {
    return;
  }

  CpStorage* stor = wan->storage();

  if (stor->current_file()) {
    if (wan->m_timestamp_miss) {
      modem_timestamp modem_ts;

      wan->calculate_modem_timestamp(ts, modem_ts);
      // Amend the current log file with the correct time stamp.
      DataBuffer* buf = stor->get_buffer();

      if (buf) {
        memcpy(buf->buffer, &modem_ts, sizeof modem_ts);
        buf->data_len = sizeof modem_ts;
        buf->dst_offset = 0;
        if (stor->amend_current_file(buf)) {
          wan->m_timestamp_miss = false;
          info_log("WAN MODEM time stamp is corrected.");
        } else {
          stor->free_buffer(buf);
          err_log("amend_current_file error");
        }
      } else {
        err_log("no buffer to correct WAN MODEM time alignment info");
      }
    } else {
      err_log("New Time sync notification but current log file have timestamp.");
    }
  } else {
    info_log("No modem log file when modem time sync info is received.");
  }
}

void WanModemLogHandler::calculate_modem_timestamp(const time_sync& ts,
    modem_timestamp& mp) {
  struct timeval time_now;
  struct sysinfo sinfo;

  gettimeofday(&time_now, 0);
  sysinfo(&sinfo);

  mp.magic_number = 0x12345678;
  mp.sys_cnt = ts.sys_cnt + (sinfo.uptime - ts.uptime) * 1000;
  mp.tv_sec = time_now.tv_sec + get_timezone_diff(time_now.tv_sec);
  mp.tv_usec = time_now.tv_usec;
}

void WanModemLogHandler::save_minidump(const struct tm& lt) {
  char md_name[80];
  LogFile* f;

  snprintf(md_name, sizeof md_name,
           "mini_dump_%04d-%02d-%02d_%02d-%02d-%02d.bin", lt.tm_year + 1900,
           lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);

  if (m_md_to_int) {
    // First check storage quota
    if (storage()->check_quota(StorageManager::MT_INTERNAL) < 0) {
      err_log("no space for mini dump");
      return;
    }
    f = storage()->create_file(LogString(md_name), LogFile::LT_MINI_DUMP,
                               StorageManager::MT_INTERNAL);
  } else {
    // First check storage quota
    if (storage()->check_quota() < 0) {
      err_log("no space for mini dump");
      return;
    }
    f = storage()->create_file(LogString(md_name), LogFile::LT_MINI_DUMP);
  }

  if (f) {
    time_sync ts;
    modem_timestamp modem_ts = {0, 0, 0, 0};

    if (m_time_sync_notifier->active_get_time_sync_info(ts)) {
      calculate_modem_timestamp(ts, modem_ts);
      ssize_t n = f->write_raw(&modem_ts, sizeof(modem_timestamp));

      if (static_cast<size_t>(n) != sizeof(modem_timestamp)) {
        err_log("write timestamp fail.");
      }

      if (n > 0) {
        f->add_size(n);
      }
    } else {
      err_log("Wan modem timestamp is not available.");
    }

    int ret = -1;
    if (CT_WCDMA == type()) {
      ret = f->copy(MINI_DUMP_WCDMA_SRC_FILE);
    } else if (CT_TD == type()) {
      ret = f->copy(MINI_DUMP_TD_SRC_FILE);
    } else {
      ret = f->copy(MINI_DUMP_LTE_SRC_FILE);
    }
    f->close();
    if (ret) {
      err_log("save mini dump error");
      f->dir()->remove(f);
    }
  } else {
    err_log("create mini dump file %s failed", md_name);
  }
}

void WanModemLogHandler::save_sipc_and_minidump(const struct tm& lt) {
  LogController::save_sipc_dump(storage(), lt);
  LogController::save_etb(storage(), lt);

  if (m_save_md) {
    save_minidump(lt);
  }
}

void WanModemLogHandler::process_assert(const struct tm& lt) {
  if (CWS_NOT_WORKING == cp_state() || CWS_DUMP == cp_state()) {
    inform_cp_controller(CpStateHandler::CE_DUMP_END);
    return;
  }

  // If it's saving sleep log/RingBuf, stop them
  if (CWS_WORKING != cp_state()) {
    stop_transaction(CWS_WORKING);
  }

  bool will_reset_cp = will_be_reset();

  info_log("wan modem %s, cp state is %d, cp log is %s.",
           (will_reset_cp ? "will be reset" : "is asserted"),
           cp_state(), (enabled() ? "enabled" : "disabled"));

  if (!storage()) {
    if (create_storage()) {
      err_log("fail to create storage");
      inform_cp_controller(CpStateHandler::CE_DUMP_END);
      return;
    }
  }

  save_sipc_and_minidump(lt);

  if (will_reset_cp) {
    set_cp_state(CWS_NOT_WORKING);
    inform_cp_controller(CpStateHandler::CE_DUMP_END);
    return;
  }

  // Save MODEM dump if log is turned on
  if (enabled()) {
    ClientManager* cli_mgr = controller()->cli_mgr();
    cli_mgr->notify_cp_dump(type(), ClientHandler::CE_DUMP_START);

    if (!start_dump(lt)) {
      inform_cp_controller(CpStateHandler::CE_DUMP_START);
      if (log_diag_dev_same()) {
        del_events(POLLIN);
      }
      set_cp_state(CWS_DUMP);
    } else {
      cli_mgr->notify_cp_dump(type(), ClientHandler::CE_DUMP_END);
      err_log("Start dump transaction error");
      close_on_assert();
      inform_cp_controller(CpStateHandler::CE_DUMP_END);
    }
  } else {
    close_on_assert();
    inform_cp_controller(CpStateHandler::CE_DUMP_END);
  }

  if (m_time_sync_notifier) {
    m_time_sync_notifier->clear_time_sync_info();
  }
}

int WanModemLogHandler::start() {
  if (!start_logging()) {
    storage()->subscribe_media_change_event(notify_media_change, this);
    // Start WCDMA I/Q if enabled
    if (m_save_wcdma_iq) {
      if (m_wcdma_iq_mgr.start(storage())) {
        err_log("start saving WCDMA I/Q error");
      }
    }

    // Start version query
    if (MVS_NOT_BEGIN == m_ver_state || MVS_FAILED == m_ver_state) {
      if (!start_ver_query()) {
        m_ver_state = MVS_QUERY;
      } else {
        err_log("start MODEM version query error");

        m_ver_state = MVS_FAILED;
      }
    }

    try_save_parsing_lib();

    return 0;
  } else {  // Failed to start logging
    return -1;
  }
}

int WanModemLogHandler::stop() {
  storage()->unsubscribe_media_change_event(this);
  // If WCDMA I/Q is started, stop it.
  if (m_wcdma_iq_mgr.is_started()) {
    if (m_wcdma_iq_mgr.stop()) {
      err_log("stop saving WCDMA I/Q error");
    }
  }
  // Stop MODEM version query
  if (MVS_QUERY == m_ver_state) {
    WanModemVerQuery* query = static_cast<WanModemVerQuery*>(consumer());

    query->stop();
    stop_transaction(CWS_WORKING);
  }

  return stop_logging();
}

int WanModemLogHandler::enable_wcdma_iq() {
  // We need to reboot to turn on the /dev/iq_mem device, so we
  // only set the m_save_wcdma_iq member here.
  m_save_wcdma_iq = true;
  return LCR_SUCCESS;
}

int WanModemLogHandler::disable_wcdma_iq() {
  if (!m_save_wcdma_iq) {
    return LCR_SUCCESS;
  }

  int ret = LCR_SUCCESS;

  if (enabled()) {
    if (!m_wcdma_iq_mgr.stop()) {
      m_save_wcdma_iq = false;
    } else {
      ret = LCR_ERROR;
    }
  } else {
    m_save_wcdma_iq = false;
  }
  return ret;
}

int WanModemLogHandler::pause() {
  if (m_wcdma_iq_mgr.is_started()) {
    m_wcdma_iq_mgr.pause();
    m_wcdma_iq_mgr.pre_clear();
  }

  return 0;
}

int WanModemLogHandler::resume() {
  if (m_wcdma_iq_mgr.is_started()) {
    m_wcdma_iq_mgr.resume();
  }

  return 0;
}

void WanModemLogHandler::process_alive() {
  // Call base class first
  LogPipeHandler::process_alive();

  if (enabled() &&
      (MVS_NOT_BEGIN == m_ver_state || MVS_FAILED == m_ver_state)) {
    if (!start_ver_query()) {
      m_ver_state = MVS_QUERY;
    } else {
      err_log("start MODEM version query error");

      m_ver_state = MVS_FAILED;
    }
  }
}

void WanModemLogHandler::stop_transaction(CpWorkState state) {
  if (MVS_QUERY == m_ver_state) {
    WanModemVerQuery* query = static_cast<WanModemVerQuery*>(consumer());
    query->stop();

    m_ver_state = MVS_FAILED;
  }

  LogPipeHandler::stop_transaction(state);
}

void WanModemLogHandler::notify_media_change(void* client,
                                             StorageManager::MediaType type) {
  WanModemLogHandler* wan = static_cast<WanModemLogHandler*>(client);
  wan->try_save_parsing_lib();
}

void WanModemLogHandler::try_save_parsing_lib() {
  bool need_save{};

  if (parse_lib_save_) {
    switch (parse_lib_save_->state()) {
      case ModemParseLibrarySave::NOT_INITED:
        if (!parse_lib_save_->init()) {
          need_save = true;
        }
        break;
      case ModemParseLibrarySave::IDLE:
        need_save = true;
        break;
      default:
        info_log("it is saving parsing lib now, can not save again");
        break;
    }
  } else {  // No ModemParseLibrarySave object
    parse_lib_save_ = new ModemParseLibrarySave{controller(),
                                                multiplexer()};
    if (!parse_lib_save_->init()) {
      need_save = true;
    }
  }
  if (need_save) {
    LogString dir;
    if (!storage()->get_cur_top_dir(dir)) {
      parse_lib_save_->trigger_parse_lib(ls2cstring(dir));
    }
  }
}
