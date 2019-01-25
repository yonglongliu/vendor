/*
 *  modem_parse_library_save.h - save modem log parse library
 *
 *  Copyright (C) 2017,2018 Spreadtrum Communications Inc.
 *
 *  History:
 *  2017-4-1 Yan Zhihang
 *  Initial version.
 *
 *  2018-6-22 Zhang Ziyi
 *  Adapt to SC9820E platform.
 */

#ifndef _MODEM_PARSE_LIBRARY_SAVE_H_
#define _MODEM_PARSE_LIBRARY_SAVE_H_

#include "fd_hdl.h"

class ModemParseLibrarySave : public FdHandler {
 public:
  enum State {
    NOT_INITED,
    IDLE,
    WORKING
  };

  ModemParseLibrarySave(LogController* ctrl, Multiplexer* multiplexer);
  ~ModemParseLibrarySave();

  State state() const { return state_; }

  int init();

  /*  trigger_parse_lib - request to start to save the parsing lib.
   *  @dir: the path under which to save the parsing lib.
   */
  int trigger_parse_lib(const char* dir);

  // FdHandler::process
  void process(int events) override;

 private:
  struct LibSave {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    CpDirectory* dir;
    bool run;
  };

  void proc_save_req();

  /*  save_log_parsing_lib - save the MODEM log parsing lib
   *
   *  Return Value:
   *    Return 0 on success, -1 otherwise.
   */
  int save_log_parsing_lib();

  static int save_log_lib(const LogString& lib_name, const LogString& sha1_name,
                          int modem_part, size_t offset, size_t len,
                          const uint8_t* sha1);

  /*  get_image_path - get the device file path of the MODEM image partition.
   *  @ct: the WAN MODEM type
   *  @img_path: the buffer to hold the device file path.
   *  @len: length of the buffer.
   *
   *  Return Value:
   *    Return 0 on success, -1 otherwise.
   *
   */
  static int get_image_path(CpType ct, char* img_path, size_t len);

  static int open_modem_img();

  static int get_modem_img(int img_fd, uint8_t* new_sha1, size_t& lib_offset,
                           size_t& lib_len);

  static int check_cur_parsing_lib(const LogString& log_name,
                                   const LogString& sha1_name,
                                   uint8_t* old_sha1, size_t& old_len);

  static void* save_parse_lib(void* arg);

  /*  wait_result - wait for saving result.
   *
   *  Return 0 on success, -1 on error.
   */
  int wait_result();

 private:
  // Requests
  static const uint8_t kQuit;
  static const uint8_t kSave;
  // Results
  static const uint8_t kSuccess;
  static const uint8_t kFailure;

  State state_;
  pthread_t save_thread_;
  pthread_mutex_t mutex_;
  int thrd_sock_;
  // Directory under which to save the parsing lib.
  LogString path_;
};

#endif  //!_MODEM_PARSE_LIBRARY_SAVE_H_
