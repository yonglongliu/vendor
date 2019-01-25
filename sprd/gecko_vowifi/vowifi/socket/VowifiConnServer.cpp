/************************************************************************

 Copyright (c) 2005-2017 by Spreadtrum, Inc.
 All rights reserved.

 This software is confidential and proprietary to Spreadtrum,
 Inc. No part of this software may be reproduced, stored, transmitted,
 disclosed or used in any form or by any means other than as expressly
 provided by the written license agreement between Spreadtrum and its
 licensee.

 ************************************************************************/
#include "VowifiConnServer.h"
#include "VowifiMsgHandler.h"

#include "VowifiUtilLog.h"

#include <sstream>

#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <cutils/sockets.h>

using namespace std;

namespace mozilla {
namespace dom {
namespace vowifiadapter {

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "[VowifiService]"
#endif

    VowifiConnServer::VowifiConnServer()
    {
        pthread_mutex_init(&mMuxConn_, NULL);
    }

    VowifiConnServer::~VowifiConnServer()
    {
        pthread_mutex_destroy(&mMuxConn_);
    }

    bool VowifiConnServer::InitConnect()
    {
        mListenRef_ = socket_local_server(
                             mSockName_.c_str(),
                             ANDROID_SOCKET_NAMESPACE_RESERVED,
                             SOCK_STREAM);
        if (mListenRef_ < 0) {
            LOGE("Could not listen socket: %s, error: %s\n",
                  mSockName_.c_str(), strerror(errno));
            return false;
        }else{
            mConnStatus_ = VowifiConnServer::eLISTENING;
            AcceptConnect();
        }
        return true;
    }

    void VowifiConnServer::ReConnect()
    {
        pthread_mutex_lock(&mMuxConn_);

        if (mConnStatus_ == VowifiConnServer::eCONNECTED) {
            pthread_mutex_unlock(&mMuxConn_);
            return;
        }else{
            //CloseConn();
            if (mConnStatus_ == VowifiConnServer::eLISTENING) {
                AcceptConnect();
            }else if(mConnStatus_ == VowifiConnServer::eIDLE){
                InitConnect();
            }
        }

        pthread_mutex_unlock(&mMuxConn_);
    }

    void VowifiConnServer::CloseConn()
    {
        if (mConnStatus_ == VowifiConnServer::eCONNECTED) {
            if (mRxTxRef_ != -1){
                mConnStatus_ = VowifiConnServer::eLISTENING;
                close(mRxTxRef_);
                mRxTxRef_ = -1;
            }
        }else if(mConnStatus_ > VowifiConnServer::eIDLE){
            if (mListenRef_ != -1){
                mConnStatus_ = VowifiConnServer::eIDLE;
                close(mListenRef_);
                mListenRef_ = -1;
            }
        }
        stringstream strs;
        LogInfo(strs);
        LOGD("Close connection is done!\n==========\n  SocketServer Infor:\n%s\n",strs.str().c_str());
    }

    void VowifiConnServer::ReadLoop()
    {
        /* start the read loop */
        StartLoop();
    }

    void VowifiConnServer::AcceptConnect()
    {
        if (mConnStatus_ == VowifiConnServer::eIDLE) {
            return;
        }else{
            LOGD("Waiting on accept a socket connection!");
            int ret;
            struct pollfd connect_fds;
            struct sockaddr_un peeraddr;
            socklen_t socklen = sizeof(peeraddr);

            connect_fds.fd = mListenRef_;
            connect_fds.events = POLLIN;
            connect_fds.revents = 0;
            stringstream strs;

            while(mConnStatus_ != eCONNECTED) {
                strs.str("");
                LogInfo(strs);
                LOGD("Waiting accept connection!\n==========\n  SocketServer Infor:\n%s\n",strs.str().c_str());

                poll(&connect_fds, 1, -1);
                mRxTxRef_ = accept(mListenRef_, (struct sockaddr*)&peeraddr, &socklen);

                if (mRxTxRef_ < 0 ) {
                    LOGE("Error on accept() errno:%d", errno);
                    /* start listening for new connections again */
                    continue;
                }
                ret = fcntl(mRxTxRef_, F_SETFL, O_NONBLOCK);
                if (ret < 0) {
                    LOGE ("Error setting O_NONBLOCK errno:%d", errno);
                }
                mConnStatus_ = VowifiConnServer::eCONNECTED;
                strs.str("");
                LogInfo(strs);
                LOGD("Setup connection is done!\n==========\n  SocketServer Infor:\n%s\n",strs.str().c_str());
            }
        }
    }

    bool VowifiConnServer::Send(const char *pBuff, size_t len)
    {
        /* in case the connection broken when it is going to send
        * data, the re-connetion and read loop will be started in here */

        //if(!IsConnected()){
        //    ReConnect();
        //    ReadLoop();
        //}

        return PushBuffer(pBuff, len);
    }

    bool VowifiConnServer::Send(string& msg)
    {
        /* in case the connection broken when it is going to send
        * data, the re-connetion and read loop will be started in here */

        //if(!IsConnected()){
        //    ReConnect();
        //    ReadLoop();
        //}

        return PushBuffer(msg);
    }
} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla
