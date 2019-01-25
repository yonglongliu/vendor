/*
 *  client_mgr.h - The client manager class declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#ifndef CLIENT_MGR_H_
#define CLIENT_MGR_H_

#include "cp_log_cmn.h"
#include "client_hdl.h"

class ClientManager : public FdHandler {
 public:
  ClientManager(LogController* ctrl, Multiplexer* multi, size_t max_cli = 0);
  ~ClientManager();

  int init(const char* server_addr, unsigned short port);

  void process(int events);

  void process_client_disconn(ClientHandler* client);

  void notify_cp_dump(CpType cpt, ClientHandler::CpEvent evt);

 private:
  size_t m_max_clients;
  LogList<ClientHandler*> m_clients;

  int create_listen_tcp_socket(const char* server_addr,
                               unsigned short port);
};

#endif  // !CLIENT_MGR_H_
