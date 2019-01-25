/*
 *  ext_wcn_log.h - The external WCN log and dump handler class declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-7-13 Zhang Ziyi
 *  Initial version.
 */

#include <poll.h>

#include "client_mgr.h"
#include "diag_dev_hdl.h"
#include "ext_wcn_dump.h"
#include "ext_wcn_log.h"
#include "log_ctrl.h"

ExtWcnLogHandler::ExtWcnLogHandler(LogController* ctrl, Multiplexer* multi,
                                   const LogConfig::ConfigEntry* conf,
                                   StorageManager& stor_mgr)
    : LogPipeHandler(ctrl, multi, conf, stor_mgr) {}

int ExtWcnLogHandler::start_dump(const struct tm& lt) {
  // Create the data consumer first
  int err = 0;
  ExtWcnDumpConsumer* dump;
  DiagDeviceHandler* diag;

  dump = new ExtWcnDumpConsumer(name(), *storage(), lt);

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

void ExtWcnLogHandler::diag_transaction_notify(
    void* client, DataConsumer::LogProcResult res) {
  ExtWcnLogHandler* cp = static_cast<ExtWcnLogHandler*>(client);

  if (CWS_DUMP == cp->cp_state()) {
    set_wcn_dump_prop();
    cp->end_dump();
    ClientManager* cli_mgr = cp->controller()->cli_mgr();
    cli_mgr->notify_cp_dump(cp->type(), ClientHandler::CE_DUMP_END);
  } else {
    err_log("Receive diag notify %d under state %d, ignore", res,
            cp->cp_state());
  }
}

int ExtWcnLogHandler::start() { return start_logging(); }

int ExtWcnLogHandler::stop() { return stop_logging(); }

void ExtWcnLogHandler::process_assert(const struct tm& lt) {
  if (CWS_NOT_WORKING == cp_state() || CWS_DUMP == cp_state()) {
    inform_cp_controller(CpStateHandler::CE_DUMP_END);
    return;
  }

  bool will_reset_cp = will_be_reset();

  info_log("external wcn %s, cp state is %d, cp log is %s.",
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

    if (!start_dump(lt)) {
      inform_cp_controller(CpStateHandler::CE_DUMP_START);
      if (log_diag_dev_same()) {
        del_events(POLLIN);
      }
      set_cp_state(CWS_DUMP);
    } else {
      // wcnd requires the property to be set
      set_wcn_dump_prop();

      cli_mgr->notify_cp_dump(CT_WCN, ClientHandler::CE_DUMP_END);
      err_log("Start dump transaction error");
      close_on_assert();
      inform_cp_controller(CpStateHandler::CE_DUMP_END);
    }
  } else {
    close_on_assert();
    inform_cp_controller(CpStateHandler::CE_DUMP_END);
  }
}
