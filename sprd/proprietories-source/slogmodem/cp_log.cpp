/*
 *  cp_log.cpp - The main function for the CP log and dump program.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 *
 *  2018-6-22 Zhang Ziyi
 *  Minor modified to adapt to SC9820E platform.
 */

/*  This is the service program for CP log on phone storage.
 *  Usage:
 *    slogmodem [–-test-ic <initial_conf>] [--test-conf <working_conf>]
 *
 *    –-test-ic <initial_conf>
 *    Specify test mode initial configuration file path.
 *    /system/etc/test_mode.conf is used in test mode (calibration or autotest)
 *    /system/etc/slog_mode.conf is used by default.
 *
 *    --test-conf <working_conf>
 *    Specify working configuration file in test mode (calibration mode or
 *    autotest mode). <working conf> shall only include the file name.
 */

#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "def_config.h"
#include "log_ctrl.h"
#include "log_config.h"

static void join_to_cgroup_blockio(void) {
    char path[96];
    if ((access("/sys/fs/cgroup/blkio/other/tasks", 0) == 0) && (access("/data/local/cgroup_en", 0) == 0)) {
        snprintf(path, sizeof path, "echo %lu > /sys/fs/cgroup/blkio/other/tasks", static_cast<unsigned long>(getpid()));
        info_log( "slogmodem join to cgroup: [%s]\n", path);
        system(path);
    }
}

int main(int argc, char** argv) {
  umask(0);
  LogController log_controller;
  LogConfig log_config(CP_LOG_TMP_CONFIG_FILE);

  info_log("slogmodem start ID=%u/%u, GID=%u/%u",
           static_cast<unsigned>(getuid()), static_cast<unsigned>(geteuid()),
           static_cast<unsigned>(getgid()), static_cast<unsigned>(getegid()));

  int err = log_config.read_config(argc, argv);
  if (err < 0) {
    return err;
  }

  // Ignore SIGPIPE to avoid to be killed by the kernel
  // when writing to a socket which is closed by the peer.
  struct sigaction siga;

  memset(&siga, 0, sizeof siga);
  siga.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &siga, 0);

  if (log_controller.init(&log_config) < 0) {
    return 2;
  }
  join_to_cgroup_blockio();
  return log_controller.run();
}
