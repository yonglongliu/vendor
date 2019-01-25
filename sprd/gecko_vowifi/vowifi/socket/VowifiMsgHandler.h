/************************************************************************

 Copyright (c) 2005-2017 by Spreadtrum, Inc.
 All rights reserved.

 This software is confidential and proprietary to Spreadtrum,
 Inc. No part of this software may be reproduced, stored, transmitted,
 disclosed or used in any form or by any means other than as expressly
 provided by the written license agreement between Spreadtrum and its
 licensee.

 ************************************************************************/
#ifndef _VOWIFI_MSG_HANDLER_H__
#define _VOWIFI_MSG_HANDLER_H__

#include <sys/types.h>
#include <string>
#include <pthread.h>

namespace mozilla {
namespace dom {
namespace vowifiadapter {


class VowifiMsgHandler{

    static const int BUFF_SIZE = 640;

protected:
    char mBuffer_[BUFF_SIZE];    // the buffer of command execution
    char mRecvBuff_[BUFF_SIZE];  // the buffer of read socket

public:
    VowifiMsgHandler();
    virtual ~VowifiMsgHandler();

    virtual void HandlerMsg()=0;

    void ClearBuffer();
    bool GetNextCmd(std::string& subCmd);

    char* GetRecvBuffer();
    size_t GetRecvMaxSize();
    bool AppendBuffer();  // append recv data from mRecvBuff_ to mBuffer_

    bool BufferLock();
    bool BufferUnlock();

private:
    VowifiMsgHandler(const VowifiMsgHandler&);
    VowifiMsgHandler& operator=(const VowifiMsgHandler&);

    char* GetMsgBuffer();
    size_t GetBuffSize();

    /*  the index used by mBuffer_ */
    int mCurrPos_;    // current position of buffer handle
    int mSubStart_;   // the sub-command begin position
    int mSubEnd_;     // the sub-command end position
    int mTailPos_;    // the position to attach received data

    /*  the index used by mRecvBuffer_ */
    int mReadPos_;    // the position to save socket data

    pthread_mutex_t mBufLock_;

};

} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla
#endif
