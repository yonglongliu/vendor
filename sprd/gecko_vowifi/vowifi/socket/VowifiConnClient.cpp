/************************************************************************

 Copyright (c) 2005-2017 by Spreadtrum, Inc.
 All rights reserved.

 This software is confidential and proprietary to Spreadtrum,
 Inc. No part of this software may be reproduced, stored, transmitted,
 disclosed or used in any form or by any means other than as expressly
 provided by the written license agreement between Spreadtrum and its
 licensee.

 ************************************************************************/
#include "VowifiConnClient.h"
#include "VowifiMsgHandler.h"

#include "VowifiUtilLog.h"

#include <sstream>

#include <unistd.h>
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
#define LOG_TAG "[VowifiAdapter]"
#endif

#define INIT_WAIT_TIME 500

    VowifiConnClient::VowifiConnClient()
    :mWaitTmr_(INIT_WAIT_TIME)
    {
        pthread_mutex_init(&mMuxConn_, NULL);
    }

    VowifiConnClient::~VowifiConnClient()
    {
        pthread_mutex_destroy(&mMuxConn_);
    }

    bool VowifiConnClient::InitConnect()
    {
        while (mRxTxRef_ < 0) {
            mRxTxRef_ = socket_local_client(
                             mSockName_.c_str(),
                             ANDROID_SOCKET_NAMESPACE_RESERVED,
                             SOCK_STREAM);

            mConnStatus_ = eCONNECTING;
            if (mRxTxRef_ < 0) {
                LOGE("Could not open client socket: %s, error: %s\n", mSockName_.c_str(), strerror(errno));
                Wait();
            }
        }
        mConnStatus_ = eCONNECTED;
        mWaitTmr_ = INIT_WAIT_TIME;
        stringstream strs;
        LogInfo(strs);
        LOGD("Setup connection is done!\n==========\n  SocketClient Infor:\n%s\n",strs.str().c_str());
        return true;
    }

    void VowifiConnClient::ReConnect()
    {
        if (mConnStatus_ == eCONNECTED) {
            return;
        }else{
            //CloseConn();
            InitConnect();
        }
    }

    void VowifiConnClient::CloseConn()
    {
        if (mRxTxRef_ != -1){
            close(mRxTxRef_);
            mRxTxRef_ = -1;
        }
        mConnStatus_ = eIDLE;
        mWaitTmr_ = INIT_WAIT_TIME;
        stringstream strs;
        LogInfo(strs);
        LOGD("Close connection is done!\n==========\n  SocketClient Infor:\n%s\n",strs.str().c_str());
    }

    void VowifiConnClient::ReadLoop()
    {
        /* start the read loop */
        StartLoop();
    }

    bool VowifiConnClient::Send(const char *pBuff, size_t len)
    {
        //if(!IsConnected()){
        //    ReConnect();
        //}
        return PushBuffer(pBuff, len);
    }

    bool VowifiConnClient::Send(string& msg)
    {
        //if(!IsConnected()){
        //    ReConnect();
        //}
        return PushBuffer(msg);
    }

    void VowifiConnClient::Wait()
    {
        if (mWaitTmr_ < 16000)
            mWaitTmr_ = mWaitTmr_ * 2;

        usleep(mWaitTmr_*1000);
    }

} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla
