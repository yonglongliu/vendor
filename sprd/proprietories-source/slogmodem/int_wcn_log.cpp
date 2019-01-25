/*
 *  int_wcn_log.cpp - The internal WCN log and dump handler class
 *                    implementation declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-8-7 Zhang Ziyi
 *  Initial version.
 */

#include "client_mgr.h"
#include "cp_dir.h"
#include "int_wcn_log.h"
#include "log_ctrl.h"
#include "log_file.h"

IntWcnLogHandler::IntWcnLogHandler(LogController* ctrl, Multiplexer* multi,
                                   const LogConfig::ConfigEntry* conf,
                                   StorageManager& stor_mgr,
                                   const char* dump_path)
    : LogPipeHandler(ctrl, multi, conf, stor_mgr),
      m_dump_path(dump_path) {}

int IntWcnLogHandler::start_dump(const struct tm& lt) {
  // Save to dump file directly.
  int err;

  LogFile* mem_file = open_dump_mem_file(lt);
  if (mem_file) {
    err = mem_file->copy(m_dump_path);
    mem_file->close();
    if (err) {
      mem_file->dir()->remove(mem_file);
      err_log("save dump error");
    }
  } else {
    err_log("create dump mem file failed");
    err = -1;
  }

  return err;
}

int IntWcnLogHandler::start() { return start_logging(); }

int IntWcnLogHandler::stop() { return stop_logging(); }

void IntWcnLogHandler::process_assert(const struct tm& lt) {
  if (CWS_NOT_WORKING == cp_state() || CWS_DUMP == cp_state()) {
    inform_cp_controller(CpStateHandler::CE_DUMP_END);
    return;
  }

  bool will_reset_cp = will_be_reset();

  info_log("internal wcn %s, cp state is %d, cp log is %s.",
           (will_reset_cp ? "will be reset" : "is asserted"),
           cp_state(), (enabled() ? "enabled" : "disabled"));

  if (will_reset_cp) {
    set_cp_state(CWS_NOT_WORKING);
    inform_cp_controller(CpStateHandler::CE_DUMP_END);
    return;
  }

  // Save wcn dump if log is turned on
  if (enabled()) {
    ClientManager* cli_mgr = controller()->cli_mgr();
    cli_mgr->notify_cp_dump(CT_WCN, ClientHandler::CE_DUMP_START);
    inform_cp_controller(CpStateHandler::CE_DUMP_START);
    set_cp_state(CWS_DUMP);
    // wcnd requires the property to be set
    set_wcn_dump_prop();

    if (start_dump(lt)) {
      err_log("internal wcn dump error");
    }
    cli_mgr->notify_cp_dump(CT_WCN, ClientHandler::CE_DUMP_END);
  }

  close_on_assert();
  inform_cp_controller(CpStateHandler::CE_DUMP_END);
}
