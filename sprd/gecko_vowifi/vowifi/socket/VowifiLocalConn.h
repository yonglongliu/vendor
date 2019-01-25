/************************************************************************

 Copyright (c) 2005-2017 by Spreadtrum, Inc.
 All rights reserved.

 This software is confidential and proprietary to Spreadtrum,
 Inc. No part of this software may be reproduced, stored, transmitted,
 disclosed or used in any form or by any means other than as expressly
 provided by the written license agreement between Spreadtrum and its
 licensee.

 ************************************************************************/
#ifndef _VOWIFI_LOCAL_CONN_H__
#define _VOWIFI_LOCAL_CONN_H__

#include <string>
#include <sys/types.h>
#include <map>
#include <sys/time.h>
#include <pthread.h>

using namespace std;

namespace mozilla {
namespace dom {
namespace vowifiadapter {

class VowifiMsgHandler;


class VowifiLocalConn{

    static const int ZMAXINT  = 0x7FFFFFFF;

public:
    typedef enum {
        eIDLE=0,
        eLISTENING,
        eCONNECTING,
        eCONNECTED
        //eCLOSED
    }ConnState_T;


protected:
    string mSockName_;
    ConnState_T mConnStatus_;
    int mListenRef_;
    int mRxTxRef_;
    VowifiMsgHandler *mPHander_;

    void SetConnState(ConnState_T state);
    bool PushBuffer(const char* buff, size_t len);
    bool PushBuffer(string& msg);

    void StartLoop();

public:
    VowifiLocalConn();
    virtual ~VowifiLocalConn();

    bool IsConnected();
    ConnState_T GetConnState();
    virtual void CloseConn(){};

    void LogInfo(stringstream &str);
    const char* GetStateText(ConnState_T state);

    void SetMsgHandler(VowifiMsgHandler* pMsgHand);

private:
    VowifiLocalConn(const VowifiLocalConn&);
    VowifiLocalConn& operator=(const VowifiLocalConn&);

    pthread_t mTid_;
    pthread_cond_t mCond_;
    pthread_mutex_t mLockPush_;

    map<int, string> mSendQueue_;
    struct timespec mTimeout_;
    int mLclInd_;
    int mIsWaiting_;

    int GetIndex();
    static void* StartSendSrv(void* arg);
    void Run();
    bool SendAll();
    bool SendBuffer(string& msg);

    void SetSignal();
    void WaitSignal();

};

} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla
#endif
