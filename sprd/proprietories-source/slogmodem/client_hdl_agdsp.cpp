/*
 *  client_hdl_agdsp.cpp - AGDSP command handler.
 *
 *  Copyright (C) 2018 UniSoC Inc.
 *
 *  History:
 *  2018-6-25 Zhang Ziyi
 *  AGDSP requests isolated.
 */

#include <cstring>
#include <poll.h>
#include <unistd.h>

#include "client_hdl.h"
#include "client_mgr.h"
#include "client_req.h"
#include "log_ctrl.h"
#include "parse_utils.h"
#include "req_err.h"

void ClientHandler::proc_get_agdsp_log_dest(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (tok) {  // Unexpected parameter
    send_response(fd(), REC_INVAL_PARAM);

    LogString sd;

    str_assign(sd, reinterpret_cast<const char*>(req), len);
    err_log("GET_AGDSP_LOG_OUTPUT invalid param %s",
            ls2cstring(sd));

    return;
  }

  LogConfig::AgDspLogDestination dest = controller()->get_agdsp_log_dest();
  const char* dest_str;

  switch (dest) {
    case LogConfig::AGDSP_LOG_DEST_OFF:
      dest_str = "OFF";
      break;
    case LogConfig::AGDSP_LOG_DEST_UART:
      dest_str = "UART";
      break;
    default:
      dest_str = "USB";
      break;
  }

  send_text_response(fd(), dest_str);
}

void ClientHandler::proc_set_agdsp_log_dest(const uint8_t* req, size_t len) {
  const uint8_t* endp = req + len;
  const uint8_t* tok;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (!tok) {  // Parameter missing
    send_response(fd(), REC_INVAL_PARAM);

    err_log("SET_AGDSP_LOG_OUTPUT parameter missing");

    return;
  }

  LogConfig::AgDspLogDestination dest;

  if (3 == tlen && !memcmp(tok, "OFF", 3)) {
    dest = LogConfig::AGDSP_LOG_DEST_OFF;
  } else if (4 == tlen && !memcmp(tok, "UART", 4)) {
    dest = LogConfig::AGDSP_LOG_DEST_UART;
  } else if (3 == tlen && !memcmp(tok, "USB", 3)) {
    dest = LogConfig::AGDSP_LOG_DEST_AP;
  } else {
    send_response(fd(), REC_INVAL_PARAM);

    LogString sd;

    str_assign(sd, reinterpret_cast<const char*>(tok), tlen);
    err_log("SET_AGDSP_LOG_OUTPUT invalid destination %s",
            ls2cstring(sd));
    return;
  }

  req = tok + tlen;
  len = endp - req;

  tok = get_token(req, len, tlen);
  if (tok) {  // Unexpected parameter
    send_response(fd(), REC_INVAL_PARAM);

    LogString sd;

    str_assign(sd, reinterpret_cast<const char*>(req), len);
    err_log("SET_AGDSP_LOG_OUTPUT extra parameter %s",
            ls2cstring(sd));

    return;
  }

  int err = controller()->set_agdsp_log_dest(dest);

  if (LCR_SUCCESS != err) {
    err_log("set_agdsp_log_dest error %d", err);
  }
  send_response(fd(), trans_result_to_req_result(err));
}

void ClientHandler::proc_get_agdsp_pcm(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (tok) {  // Unexpected parameter
    send_response(fd(), REC_INVAL_PARAM);

    LogString sd;

    str_assign(sd, reinterpret_cast<const char*>(req), len);
    err_log("GET_AGDSP_PCM_OUTPUT invalid param %s",
            ls2cstring(sd));

    return;
  }

  bool pcm = controller()->get_agdsp_pcm_dump();

  send_text_response(fd(), pcm ? "ON" : "OFF");
}

void ClientHandler::proc_set_agdsp_pcm(const uint8_t* req, size_t len) {
  const uint8_t* endp = req + len;
  const uint8_t* tok;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (!tok) {  // Parameter missing
    send_response(fd(), REC_INVAL_PARAM);

    err_log("SET_AGDSP_PCM_OUTPUT parameter missing");

    return;
  }

  bool pcm;

  if (2 == tlen && !memcmp(tok, "ON", 2)) {
    pcm = true;
  } else if (3 == tlen && !memcmp(tok, "OFF", 3)) {
    pcm = false;
  } else {
    send_response(fd(), REC_INVAL_PARAM);

    LogString sd;

    str_assign(sd, reinterpret_cast<const char*>(tok), tlen);
    err_log("SET_AGDSP_PCM_OUTPUT invalid param %s",
            ls2cstring(sd));
    return;
  }

  req = tok + tlen;
  len = endp - req;

  tok = get_token(req, len, tlen);
  if (tok) {  // Unexpected parameter
    send_response(fd(), REC_INVAL_PARAM);

    LogString sd;

    str_assign(sd, reinterpret_cast<const char*>(req), len);
    err_log("SET_AGDSP_PCM_OUTPUT extra parameter %s",
            ls2cstring(sd));

    return;
  }

  int err = controller()->set_agdsp_pcm_dump(pcm);

  if (LCR_SUCCESS != err) {
    err_log("set_agdsp_pcm_dump error %d", err);
  }
  send_response(fd(), trans_result_to_req_result(err));
}
