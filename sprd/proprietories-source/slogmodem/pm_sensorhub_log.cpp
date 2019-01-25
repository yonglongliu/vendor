/*
 *  pm_sensorhub_log.cpp - The power management, sensorhub log
 *                         and dump handler class declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-3-2 YAN Zhihang
 *  Initial version.
 */

#include <poll.h>

#include "client_mgr.h"
#include "diag_dev_hdl.h"
#include "log_config.h"
#include "log_ctrl.h"
#include "pm_modem_dump.h"
#include "pm_sensorhub_log.h"

PmSensorhubLogHandler::PmSensorhubLogHandler(LogController* ctrl,
                                             Multiplexer* multi,
                                             const LogConfig::ConfigEntry* conf,
                                             StorageManager& stor_mgr)
    : LogPipeHandler(ctrl, multi, conf, stor_mgr) {}

int PmSensorhubLogHandler::start_dump(const struct tm& lt) {
  // Create the data consumer first
  int err = 0;
  PmModemDumpConsumer* dump;
  DiagDeviceHandler* diag;

  dump = new PmModemDumpConsumer(name(), *storage(), lt, nullptr,
                                 "\nPM_memdump_finish", 0x31);

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

void PmSensorhubLogHandler::diag_transaction_notify(
    void* client, DataConsumer::LogProcResult res) {
  PmSensorhubLogHandler* cp = static_cast<PmSensorhubLogHandler*>(client);

  if (CWS_DUMP == cp->cp_state()) {
    cp->end_dump();
    ClientManager* cli_mgr = cp->controller()->cli_mgr();
    cli_mgr->notify_cp_dump(cp->type(), ClientHandler::CE_DUMP_END);
  } else {
    err_log("Receive diag notify %d under state %d, ignore", res,
            cp->cp_state());
  }
}

int PmSensorhubLogHandler::start() { return start_logging(); }

int PmSensorhubLogHandler::stop() { return stop_logging(); }

void PmSensorhubLogHandler::process_assert(const struct tm& lt) {
  if (CWS_NOT_WORKING == cp_state() || CWS_DUMP == cp_state()) {
    inform_cp_controller(CpStateHandler::CE_DUMP_END);
    return;
  }

  bool will_reset_cp = will_be_reset();

  info_log("power management and sensorhub %s, cp state is %d, cp log is %s.",
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
}
