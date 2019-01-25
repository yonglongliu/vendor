/************************************************************************

 Copyright (c) 2005-2017 by Spreadtrum, Inc.
 All rights reserved.

 This software is confidential and proprietary to Spreadtrum,
 Inc. No part of this software may be reproduced, stored, transmitted,
 disclosed or used in any form or by any means other than as expressly
 provided by the written license agreement between Spreadtrum and its
 licensee.

 ************************************************************************/
#include "VowifiMsgHandler.h"
#include "VowifiUtilLog.h"

#include <cstring>

using namespace std;

namespace mozilla {
namespace dom {
namespace vowifiadapter {

    VowifiMsgHandler::VowifiMsgHandler()
    :mCurrPos_(0)
    ,mSubStart_(0)
    ,mSubEnd_(0)
    ,mTailPos_(0)
    ,mReadPos_(0)
    {
        memset(mBuffer_, 0, BUFF_SIZE);
        memset(mRecvBuff_, 0, BUFF_SIZE);
        pthread_mutex_init(&mBufLock_, NULL);
    }

    VowifiMsgHandler::~VowifiMsgHandler()
    {
        pthread_mutex_destroy(&mBufLock_);
    }

    char* VowifiMsgHandler::GetMsgBuffer()
    {
        return mBuffer_ + mTailPos_;
    }

    size_t VowifiMsgHandler::GetBuffSize()
    {
        return BUFF_SIZE - mTailPos_ - 1;
    }

    void VowifiMsgHandler::ClearBuffer()
    {
        // clear buffer will continue keep unused command
        // segment in buffer

        pthread_mutex_lock(&mBufLock_);

        /* move command segment to buffer header */
        size_t num = strlen(mBuffer_) - mCurrPos_ - 1;
        memcpy(mBuffer_, mBuffer_ + mCurrPos_ + 1, num);
        mTailPos_ = num;

        /* reset the rest part buffer to empty */
        memset(mBuffer_+num, 0, BUFF_SIZE - num);

        /* re-calcu buffer positions */
        mCurrPos_ = 0;
        mSubStart_ = 0;
        mSubEnd_ = 0;

        pthread_mutex_unlock(&mBufLock_);
    }


    bool VowifiMsgHandler::GetNextCmd(string& subCmd)
    {
        int i, istack = 0;
        size_t full = strlen(mBuffer_);
        bool ishead = false, isCmd = false;

        for (i = mCurrPos_; i < (int)full; ++i)
        {
            if (!ishead && mBuffer_[i] == '{')
            {
                mSubStart_ = i;
                istack++;       // indicate the bracket pair to be matched
                ishead = true;
                continue;
            }

            if (ishead){
                if (mBuffer_[i] == '{')
                {
                    istack++;
                }else if (mBuffer_[i] == '}'){
                    istack--;
                }
            }

            // find one command pair
            if(ishead && istack == 0)
            {
                mSubEnd_ = i;
                isCmd = true;
                break;
            }
        }

        if (isCmd)
        {
            subCmd.assign((const char*)(mBuffer_ + mSubStart_), mSubEnd_ - mSubStart_ +1);
            mCurrPos_ = i;
        }

        return isCmd;
    }

    char* VowifiMsgHandler::GetRecvBuffer()
    {
        return mRecvBuff_ + mReadPos_;
    }

    size_t VowifiMsgHandler::GetRecvMaxSize()
    {
        mReadPos_ = strlen(mRecvBuff_);
        return BUFF_SIZE - mReadPos_ - 1;
    }

    /* append recv buffer to handler buffer */
    bool VowifiMsgHandler::AppendBuffer()
    {
        pthread_mutex_lock(&mBufLock_);

        /* decide the size move */
        size_t isize = GetBuffSize();
        size_t irSize = strlen(mRecvBuff_);
        if (isize < 25 || irSize <= 0)
        {
            // available buffer is too small
            pthread_mutex_unlock(&mBufLock_);
            return false;
        }
        if (irSize < isize) isize = irSize;
        //ALOGD("->Recv Buffer Rsz[%d]: \n%s, \nMsg Buffer Bsz[%d]: \n%s",
        //     irSize, mRecvBuff_, isize, mBuffer_);

        /* copy buffer and shift it */
        memcpy(GetMsgBuffer(), mRecvBuff_, isize);
        mTailPos_ += isize;
        memcpy(mRecvBuff_, mRecvBuff_ + isize, irSize - isize);

        /* reset the rest part buffer to empty */
        memset(mRecvBuff_+irSize-isize, 0, isize);

        mReadPos_ = irSize - isize;
        //ALOGD("<-Recv Buffer: \n%s, \nMsg Buffer: \n%s", mRecvBuff_, mBuffer_);

        pthread_mutex_unlock(&mBufLock_);
        return true;
    }

    bool VowifiMsgHandler::BufferLock()
    {
        pthread_mutex_lock(&mBufLock_);
        return true;
    }

    bool VowifiMsgHandler::BufferUnlock()
    {
        pthread_mutex_unlock(&mBufLock_);
        return true;
    }
} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla

