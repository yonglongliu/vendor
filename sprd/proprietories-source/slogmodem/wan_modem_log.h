/*
 *  wan_modem_log.h - The WAN MODEM log and dump handler class declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-7-13 Zhang Ziyi
 *  Initial version.
 */
#ifndef WAN_MODEM_LOG_H_
#define WAN_MODEM_LOG_H_

#include "log_pipe_hdl.h"
#include "stor_mgr.h"
#include "wcdma_iq_mgr.h"
#include "wan_modem_time_sync_hdl.h"

class LogFile;
class ModemParseLibrarySave;

class WanModemLogHandler : public LogPipeHandler {
 public:
  WanModemLogHandler(LogController* ctrl, Multiplexer* multi,
                     const LogConfig::ConfigEntry* conf,
                     StorageManager& stor_mgr, const char* dump_path);
  ~WanModemLogHandler();

  // Override base class function
  void process_alive();

  /*  pause - pause logging and I/Q before log clearing.
   *
   *  This function assume m_enable is true.
   *
   *  Return Value:
   *    0
   */
  int pause() override;
  /*  resume - resume logging and I/Q after log clearing.
   *
   *  This function assume m_enable is true.
   *
   *  Return Value:
   *    0
   */
  int resume() override;
  /*  enable_wcdma_iq - enable WCDMA I/Q saving.
   *
   *  Return LCR_SUCCESS on success, LCR_xxx on error.
   */
  int enable_wcdma_iq();
  /*  disable_wcdma_iq - disable WCDMA I/Q saving.
   *
   *  Return LCR_SUCCESS on success, LCR_xxx on error.
   */
  int disable_wcdma_iq();
  /*  set_md_save - enable minidump save or nor according to parameter
   *  @enable - true if enable minidump
   */
  void set_md_save(bool enable) { m_save_md = enable; }
  /*  set_md_internal - set minidump storage position
   *  @int_pos - true if save position is internal storage
   */
  void set_md_internal(bool int_pos) { m_md_to_int = int_pos; }

 private:
  // number of bytes predicated for modem version when not available
  static const size_t PRE_MODEM_VERSION_LEN = 200;

  enum ModemVerState {
    MVS_NOT_BEGIN,
    MVS_QUERY,
    MVS_GOT,
    MVS_FAILED
  };

  enum ModemVerStoreState {
    MVSS_NOT_SAVED,
    MVSS_SAVED
  };

  const char* m_dump_path;
  // WCDMA I/Q manager
  WcdmaIqManager m_wcdma_iq_mgr;
  // Save WCDMA I/Q
  bool m_save_wcdma_iq;
  // Save minidump or not
  bool m_save_md;
  // Save minidump to internal storage
  bool m_md_to_int;
  WanModemTimeSync* m_time_sync_notifier;
  bool m_timestamp_miss;
  ModemVerState m_ver_state;
  ModemVerStoreState m_ver_stor_state;
  LogString m_modem_ver;
  size_t m_reserved_version_len;
  ModemParseLibrarySave* parse_lib_save_;

  // Override base class function
  int create_storage() override;

  // Override base class function
  void stop_transaction(CpWorkState state) override;

  /*  save_minidump - Save the mini dump.
   *
   *  @t: the time to be used as the file name
   */
  void save_minidump(const struct tm& t);
  /*  save_sipc_and_minidump - save sipc dump memory and
   *                           minidump if minidump enabled
   *  @lt: system time for file name generation
   */
  void save_sipc_and_minidump(const struct tm& lt);

  void on_new_log_file(LogFile* f) override;

  /*  start_dump - WAN MODEM memory dump.
   *  Return Value:
   *    Return 0 if the dump transaction is started successfully,
   *    return -1 if the dump transaction can not be started,
   */
  int start_dump(const struct tm& lt);

  /*  start - override the virtual function for starting WAN log
   *
   *  Return Value:
   *    Return 0 if WAN log start successfully,
   */
  int start();

  /*
   *    stop - Stop logging.
   *
   *    This function put the LogPipeHandler in an adminitrative
   *    disable state and close the log device.
   *
   *    Return Value:
   *      0
   */
  int stop();

  int start_ver_query();
  void correct_ver_info();
  /* frame_noversion_and_reserve - frame the unavailable information
   *                               and reserve spaces for the the modem
   *                               version to come.
   * @length - reserved length to be written to log file
   *
   * Return pointer to the framed info.
   */
  uint8_t* frame_noversion_and_reserve(size_t& length);
  /* frame_version_info - frame the version info
   * @payload - payload of version info
   * @pl_len - payload length
   * @len_reserved - spaces reserved for the resulted frame if not equal 0
   * @frame_len - length of resulted frame
   *
   * Return pointer to the framed info.
   */
  uint8_t* frame_version_info(const uint8_t* payload, size_t pl_len,
                              size_t len_reserved, size_t& frame_len);

  bool save_timestamp(LogFile* f);

  void process_assert(const struct tm& lt) override;

  static void transaction_modem_ver_notify(void* client,
                                           DataConsumer::LogProcResult res);

  /*  diag_transaction_notify - Diagnosis port transaction result
   *                            notification function.
   *  @client: client parameter. It's the WanModemLogHandler* pointer.
   *  @result: the transaction result.
   *
   *  This function is called by current DataConsumer object.
   */
  static void diag_transaction_notify(void* client,
                                      DataConsumer::LogProcResult res);

  /* calculate_modem_timestamp - calculate wan modem timestamp with the
   *                             system count and uptime received from refnotify.
   * @ts: time sync info consist of cp boot system count and uptime
   * @modem_timestamp: value result
   */
  static void calculate_modem_timestamp(const time_sync& ts,
                                        modem_timestamp& mp);

  /*  notify_media_change - callback function to trigger the copy of
   *                        modem log library from internal storage to
   *                        external storage.
   *  @client: the log pipe handler client(wan, wcn or gnss).
   *  @type: internal storage or external storage.
   */
  static void notify_media_change(void* client, StorageManager::MediaType type);

  static void notify_modem_time_update(void* client, const time_sync& ts);

  static void fill_smp_header(uint8_t* buf, size_t len,
                              unsigned lcn, unsigned type);

  static void fill_diag_header(uint8_t* buf, uint32_t sn, unsigned len,
                               unsigned cmd, unsigned subcmd);

  void try_save_parsing_lib();
};

#endif  // !WAN_MODEM_LOG_H_
