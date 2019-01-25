/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef SPRD_DEINTERLACE_H_
#define SPRD_DEINTERLACE_H_

#include "vpp_drv_interface.h"

namespace android {

#define  T_BUFFER_INFO SprdSimpleOMXComponent::BufferInfo

struct SPRDDeinterlace {

public:
    explicit SPRDDeinterlace(void* Component);
    virtual ~SPRDDeinterlace();

    bool mDone;
    bool mUseNativeBuffer;
    uint32_t mDeintlFrameNum;
    Condition mDeinterReadyCondition;   // Signal that de-interlace queue are available

    void startDeinterlaceThread();
    int32 VspFreeIova(unsigned long iova, size_t size);
    int32 VspGetIova(int fd, unsigned long *iova, size_t *size);
private:

    bool mIOMMU_VPP_Enabled;
    int32_t  mIOMMU_VPP_ID;

    pthread_t   mThread;                // Thread id for deinterlace

    VPPObject* mVPPHeader;
    void* mComponent;

    BufferCtrlStruct * mDispBufferCtrl;

    KeyedVector<T_BUFFER_INFO *, BufferCtrlStruct *> mBufferCtrlbyBufferInfo;

    void deintlThreadFunc();
    static void * ThreadWrapper(void *me);
    int remapDispBuffer(OMX_BUFFERHEADERTYPE * header, unsigned long* phyAddr);
    int remapDeintlSrcBuffer(SprdSimpleOMXComponent::BufferInfo * itBuffer,  unsigned long* phyAddr);
    void unmapDeintlSrcBuffer(SprdSimpleOMXComponent::BufferInfo * itBuffer);
    void fillDeintlSrcBuffer(SprdSimpleOMXComponent* component, T_BUFFER_INFO * Pre, T_BUFFER_INFO * Cur);

    DISALLOW_EVIL_CONSTRUCTORS(SPRDDeinterlace);
};

}  // namespace android

#endif  // SPRD_AVC_DECODER_H_

