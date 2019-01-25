/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include <sys/cdefs.h>
#include <sys/types.h>

#include <cutils/log.h>

#include "InputEventReader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*****************************************************************************/

InputEventCircularReader::InputEventCircularReader(size_t numEvents)
    : mBuffer(new pseudo_event[numEvents * 2]),
      mBufferEnd(mBuffer + numEvents),
      mHead(mBuffer),
      mCurr(mBuffer),
      mBufferSize(sizeof(pseudo_event)),
      mTotalSpace(numEvents),
      mFreeSpace(numEvents) {
}

InputEventCircularReader::InputEventCircularReader(size_t numEvents, size_t eventSize)
    : mBuffer(new pseudo_event[numEvents * eventSize * 2]),
      mBufferEnd(mBuffer + numEvents * eventSize),
      mHead(mBuffer),
      mCurr(mBuffer),
      mBufferSize(eventSize),
      mTotalSpace(numEvents),
      mFreeSpace(numEvents) {
}

InputEventCircularReader::~InputEventCircularReader() {
    delete [] mBuffer;
}

ssize_t InputEventCircularReader::fill(int fd) {
    size_t numEventsRead = 0;
    if (mFreeSpace) {
        const ssize_t nread = read(fd, mHead, mFreeSpace * mBufferSize);
        if (nread < 0 || nread % mBufferSize) {
            // we got a partial event!!
            return nread < 0 ? -errno : -EINVAL;
        }

        numEventsRead = nread / mBufferSize;
        if (numEventsRead) {
        #ifdef EVENT_DYNAMIC_ADAPT
            mHead += nread;
        #else
            mHead += numEventsRead;
        #endif
            mFreeSpace -= numEventsRead;
            if (mHead > mBufferEnd) {
                size_t s = mHead - mBufferEnd;
            #ifdef EVENT_DYNAMIC_ADAPT
                memcpy(mBuffer, mBufferEnd, s);
            #else
                memcpy(mBuffer, mBufferEnd, s * mBufferSize);
            #endif
                mHead = mBuffer + s;
            }
        }
    }
    return numEventsRead;
}

ssize_t InputEventCircularReader::readEvent(pseudo_event const** events) {
    *events = mCurr;
    ssize_t available = mTotalSpace - mFreeSpace;
    return available ? 1 : 0;
}

void InputEventCircularReader::next() {
#ifdef EVENT_DYNAMIC_ADAPT
    mCurr += mBufferSize;
#else
    mCurr++;
#endif
    mFreeSpace++;
    if (mCurr >= mBufferEnd) {
        mCurr = mBuffer;
    }
}
