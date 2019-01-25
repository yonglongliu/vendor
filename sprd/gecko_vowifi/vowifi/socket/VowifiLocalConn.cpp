/************************************************************************

 Copyright (c) 2005-2017 by Spreadtrum, Inc.
 All rights reserved.

 This software is confidential and proprietary to Spreadtrum,
 Inc. No part of this software may be reproduced, stored, transmitted,
 disclosed or used in any form or by any means other than as expressly
 provided by the written license agreement between Spreadtrum and its
 licensee.

 ************************************************************************/
#include "VowifiLocalConn.h"
#include "VowifiMsgHandler.h"

#include "VowifiUtilLog.h"

#include <sstream>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <poll.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>

using namespace std;

namespace mozilla {
namespace dom {
namespace vowifiadapter {

    VowifiLocalConn::VowifiLocalConn()
    :mSockName_("vowifi_server_adapter_local")
    ,mConnStatus_(eIDLE)
    ,mListenRef_(-1)
    ,mRxTxRef_(-1)
    ,mLclInd_(0)
    ,mIsWaiting_(0)
    {
        pthread_cond_init(&mCond_, NULL);
        pthread_mutex_init(&mLockPush_, NULL);
    }

    VowifiLocalConn::~VowifiLocalConn()
    {
        pthread_cond_destroy(&mCond_);
        pthread_mutex_destroy(&mLockPush_);
    }

    void VowifiLocalConn::SetConnState(VowifiLocalConn::ConnState_T state)
    {
        mConnStatus_ = state;
    }

    bool VowifiLocalConn::IsConnected()
    {
        return (mConnStatus_ == VowifiLocalConn::eCONNECTED);
    }

    VowifiLocalConn::ConnState_T VowifiLocalConn::GetConnState()
    {
        return mConnStatus_;
    }

    void VowifiLocalConn::LogInfo(stringstream &infoStr)
    {
        infoStr << "socket  name  :" << mSockName_ << endl;
        infoStr << "connect state :" << GetStateText(mConnStatus_) << endl;
        infoStr << "listen  fd    :" << mListenRef_ << endl;
        infoStr << "sendrcv fd    :" << mRxTxRef_ << endl;
    }

    const char* VowifiLocalConn::GetStateText(ConnState_T state)
    {
        const char *name = 0;
        switch(state)
        {
            case VowifiLocalConn::eIDLE:
                name = "IDLE";
                break;
            case VowifiLocalConn::eLISTENING:
                name = "LISTENING";
                break;
            case VowifiLocalConn::eCONNECTED:
                name = "CONNECTED";
                break;
            //case VowifiLocalConn::eCLOSED:
            //    name = "CLOSED";
            //    break;
            default:
                name = "Unknown";
        }
        return name;
    }

    void VowifiLocalConn::SetMsgHandler(VowifiMsgHandler* pMsgHand)
    {
        pthread_create(&mTid_, 0, StartSendSrv, this);
        clock_gettime(CLOCK_REALTIME, &mTimeout_);

        if (pMsgHand) mPHander_ = pMsgHand;

    }

    bool VowifiLocalConn::PushBuffer(const char* buff, size_t len)
    {
        if (!buff) {
            ALOGW("PushBuffer buff pointer is NULL!");
            return false;
        }
        string info(buff, len);
        return PushBuffer(info);
    }

    bool VowifiLocalConn::PushBuffer(string& msg)
    {
        pthread_mutex_lock(&mLockPush_);
        int idx = GetIndex();

        std::pair<map<int,string>::iterator, bool> ret;
        ret = mSendQueue_.insert(pair<int, string>(idx, msg));

        if (!ret.second) ALOGW("PushBuffer failed to push <%s>", msg.c_str());
        else SetSignal();

        pthread_mutex_unlock(&mLockPush_);
        return ret.second;
    }

    void VowifiLocalConn::StartLoop()
    {
        struct pollfd fdc;
        fdc.fd      = mRxTxRef_;
        fdc.events  = POLLIN;
        fdc.revents = 0;
        char* pBter = NULL;
        size_t buffSize = 0;
        int ret;

        while(mConnStatus_ == VowifiLocalConn::eCONNECTED)
        {
            poll(&fdc, 1, -1);
            if(fdc.revents > 0)
            {
                fdc.revents = 0;
                /* assume it will recv all the data which is less than 1024 */

                //while(1)
                //{
                    mPHander_->BufferLock();
                    buffSize = mPHander_->GetRecvMaxSize();
                    pBter = mPHander_->GetRecvBuffer();

                    ret = read(mRxTxRef_, pBter, buffSize);
                    mPHander_->BufferUnlock();

                    /* in current, the maximum handle buffer size is 640 */
                    if(ret > 0) {
                        while(mPHander_->AppendBuffer()){
                            mPHander_->HandlerMsg();
                        }
                    }else{
                        ALOGE("Failed to read from socket, closing it...");
                        break;
                    }

                    /* no more data in recv buffer */
                    //if(ret < sizeof(pBter))
                    //{
                    //  break;
                    //}
                //}
            }
        }
        CloseConn();
    }

    /* generate the index to keep buffer msg in order */
    int VowifiLocalConn::GetIndex()
    {
        if (mLclInd_ > 65535) {
            mLclInd_ = 0;
            clock_gettime(CLOCK_REALTIME, &mTimeout_);
        }

        mLclInd_++;
        return (((mTimeout_.tv_sec << 8) + mLclInd_) & ZMAXINT);
    }

    void* VowifiLocalConn::StartSendSrv(void* arg)
    {
        VowifiLocalConn* pSelf = (VowifiLocalConn*) arg;
        pSelf->Run();
        return (void*)0;
    }

    void VowifiLocalConn::Run()
    {
        while(1){
            if(mSendQueue_.empty())
            {
                WaitSignal();
            }else{
                SendAll();
            }
        }
    }

    bool VowifiLocalConn::SendAll()
    {
        if (mConnStatus_ != VowifiLocalConn::eCONNECTED)
        {
            return false;
        }

        pthread_mutex_lock(&mLockPush_);
        bool res = true;
        map<int, string>::iterator iter = mSendQueue_.begin();

        for (; iter != mSendQueue_.end(); ++iter)
        {
            res &= SendBuffer(iter->second);
        }
        mSendQueue_.clear();
        pthread_mutex_unlock(&mLockPush_);
        return res;
    }

    bool VowifiLocalConn::SendBuffer(string& msg)
    {
        size_t write_offset = 0;
        const char *to_write;

        size_t len = msg.length();
        to_write = (const char *)msg.c_str();

        while (write_offset < len) {
            ssize_t written;
            do {
                written = write(mRxTxRef_, to_write + write_offset, len - write_offset);
            }while (written < 0 && errno == EINTR);

            if (written >= 0) {
                write_offset += written;
            }else{
                ALOGE("RIL Response: unexpected error on write errno:%d", errno);
                return false;
            }
        }
        return true;
    }

    void VowifiLocalConn::SetSignal()
    {
        if(mIsWaiting_ > 0){
            // creating the signal
            pthread_cond_signal(&mCond_);
        }
    }

    void VowifiLocalConn::WaitSignal()
    {
        int end = 0;

        pthread_mutex_lock(&mLockPush_);
        mIsWaiting_++;

        while(end==0) {
            // waiting the signal
            pthread_cond_wait(&mCond_,&mLockPush_);
            end = 1;
        }

        mIsWaiting_--;
        pthread_mutex_unlock(&mLockPush_);
    }
} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla

