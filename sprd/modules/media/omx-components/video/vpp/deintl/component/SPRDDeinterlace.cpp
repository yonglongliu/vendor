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

//#define LOG_NDEBUG 0
#define LOG_TAG "SPRDDeinterlace"
#include <utils/Log.h>
#include <media/IOMX.h>
#include <sys/prctl.h>
#include <ui/GraphicBufferMapper.h>
#include <media/stagefright/foundation/ADebug.h>
#include "sprd_ion.h"
#include "Rect.h"

#include "gralloc_public.h"

#include "SPRDAVCDecoder.h"
#include "SPRDDeinterlace.h"

namespace android {

SPRDDeinterlace::SPRDDeinterlace(void* component):
    mDone(false) ,
    mDeintlFrameNum(0),
    mIOMMU_VPP_Enabled(false),
    mIOMMU_VPP_ID(-1){
    ALOGI("Construct SPRDDeinterlace, this: %p", (void *)this);
    mComponent = component;
    mDispBufferCtrl = new BufferCtrlStruct;
    mVPPHeader = new VPPObject;

    int ret = vpp_deint_init(mVPPHeader);
    if (ret!=0) {
        ALOGE("failed to init deint module");
        //return OMX_ErrorUndefined;
    }

    ret = vpp_deint_get_IOMMU_status(mVPPHeader);
    if (ret < 0) {
        ALOGE("Failed to get IOMMU status, ret: %d(%s)", ret, strerror(errno));
        mIOMMU_VPP_Enabled = false;
    } else {
        ALOGI("VSP IOMMU is enabled");
        mIOMMU_VPP_Enabled = true;
    }
    ALOGI("%s, is mIOMMU_VPP_Enabled : %d, ID: %d", __FUNCTION__, mIOMMU_VPP_Enabled, mIOMMU_VPP_ID);

}

SPRDDeinterlace::~SPRDDeinterlace() {
    ALOGI("Destruct SPRDDeinterlace, this: %p", (void *)this);

    void *dummy;
    pthread_join(mThread, &dummy);
    ALOGI("Deintl thread finished\n");

    delete mDispBufferCtrl;

    vpp_deint_release(mVPPHeader);
    if (mVPPHeader) {
        delete mVPPHeader;
        mVPPHeader = NULL;
    }
}

int32 SPRDDeinterlace::VspFreeIova(unsigned long iova, size_t size) {
    int32 ret ;
    ret = vpp_deint_free_iova(mVPPHeader, iova, size);
    ALOGV("VspFreeIova : %d\n", ret);
    return  ret;
}

int32 SPRDDeinterlace::VspGetIova(int fd, unsigned long *iova, size_t *size) {
    int32 ret ;
    ret = vpp_deint_get_iova(mVPPHeader, fd, iova, size);
    ALOGV("VspGetIova : %d\n", ret);
    return  ret;
}

int SPRDDeinterlace::remapDispBuffer(OMX_BUFFERHEADERTYPE * header,  unsigned long* phyAddr) {
    size_t bufferSize = 0;

    if (mIOMMU_VPP_Enabled) {
        //MemIon::Get_iova(mIOMMU_VPP_ID, private_h->share_fd, phyAddr, &bufferSize);
    } else {
        //MemIon::Get_phy_addr_from_ion(private_h->share_fd, phyAddr, &bufferSize);
    }

    mDispBufferCtrl->pMem = NULL;
    mDispBufferCtrl->bufferFd = ADP_BUFFD((buffer_handle_t)header->pBuffer);
    mDispBufferCtrl->phyAddr = *phyAddr;
    mDispBufferCtrl->bufferSize = bufferSize;

    return 0;
}

int SPRDDeinterlace::remapDeintlSrcBuffer(T_BUFFER_INFO * itBuffer,  unsigned long* phyAddr) {
    if (mIOMMU_VPP_Enabled) {

        BufferCtrlStruct* pBufCtrl= (BufferCtrlStruct*)(itBuffer->mHeader->pOutputPortPrivate);
        size_t bufferSize = 0;
		/*
		if(pBufCtrl->pMem->get_iova(mIOMMU_VPP_ID, phyAddr, &bufferSize)) {
		    ALOGE("get_mm_iova fail");
		    *phyAddr = 0;
		    return OMX_ErrorInsufficientResources;
		}
		*/
        BufferCtrlStruct * pVPPBufCtrl= new BufferCtrlStruct;
        pVPPBufCtrl->pMem = pBufCtrl->pMem;
        pVPPBufCtrl->bufferFd = 0;
        pVPPBufCtrl->phyAddr = *phyAddr;
        pVPPBufCtrl->bufferSize = bufferSize;

        mBufferCtrlbyBufferInfo.add(itBuffer, pVPPBufCtrl);
    } else {
        BufferCtrlStruct * pBufCtrl= (BufferCtrlStruct*)(itBuffer->mHeader->pOutputPortPrivate);
        if(pBufCtrl->phyAddr != 0) {
            *phyAddr= pBufCtrl->phyAddr;
        } else {
            ALOGE("%s, %d, deint frame buffer is null\n", __FUNCTION__, __LINE__);
            *phyAddr = 0;
            return  -1;
        }
    }

    return 0;
}

void SPRDDeinterlace::unmapDeintlSrcBuffer(T_BUFFER_INFO * itBuffer) {
    ssize_t index;

    if (mIOMMU_VPP_Enabled) {
        index = mBufferCtrlbyBufferInfo.indexOfKey(itBuffer);
        if (index < 0) {
            ALOGE("%s, %d: The index(%d) of itBufferNodePre(%p) in mBufferCtrlbyBufferInfo error\n",
                  __FUNCTION__, __LINE__, index, itBuffer);
            return;
        }

        BufferCtrlStruct* pBufCtrl = mBufferCtrlbyBufferInfo.valueAt(index);

        if(pBufCtrl != NULL && pBufCtrl->pMem != NULL) {
            ALOGI("freeBuffer, phyAddr: 0x%lx", pBufCtrl->phyAddr);
                //pBufCtrl->pMem->free_iova(mIOMMU_VPP_ID, pBufCtrl->phyAddr, pBufCtrl->bufferSize);
                //pBufCtrl->pMem.clear();
        }

        mBufferCtrlbyBufferInfo.removeItemsAt(index);
        delete pBufCtrl;
    }
}

void SPRDDeinterlace::fillDeintlSrcBuffer(SprdSimpleOMXComponent* component,
        T_BUFFER_INFO* itBufferNodePre, T_BUFFER_INFO * itBufferNodeCur) {
    ALOGV("%s, %d: mDeinterInputBufQueue size: %d, mDecOutputBufQueue size: %d, mDeintlFrameNum: %d\n",
          __FUNCTION__, __LINE__, component->mDeinterInputBufQueue.size(), component->mDecOutputBufQueue.size(), mDeintlFrameNum);

    if(mDeintlFrameNum > 1) {
        //unmapDeintlSrcBuffer(itBufferNodePre);
        component->mDecOutputBufQueue.push_back(itBufferNodePre);
        component->mDeinterInputBufQueue.erase(component->mDeinterInputBufQueue.begin());

        if ((itBufferNodeCur->mHeader->nFlags == OMX_BUFFERFLAG_EOS)) {
            mDone = true;
            //unmapDeintlSrcBuffer(itBufferNodeCur);
            component->mDecOutputBufQueue.push_back(itBufferNodeCur);
            component->mDeinterInputBufQueue.erase(component->mDeinterInputBufQueue.begin());
            ALOGE("%s, %d: Deinterlace meet EOS, mDeinterInputBufQueue size: %d, mDecOutputBufQueue size: %d\n",
                  __FUNCTION__, __LINE__, component->mDeinterInputBufQueue.size(), component->mDecOutputBufQueue.size());
        }
    }

    return;
}

//deinterlace thread
void SPRDDeinterlace::deintlThreadFunc() {
    prctl(PR_SET_NAME, (unsigned long)"Deinterlace", 0, 0, 0);

    SprdSimpleOMXComponent* component = reinterpret_cast<SprdSimpleOMXComponent*>(mComponent);
    List<SprdSimpleOMXComponent::BufferInfo *> &outQueue = component->getPortQueue(kOutputPortIndex);

    SprdSimpleOMXComponent::BufferInfo * itBufferNodePre =NULL;
    SprdSimpleOMXComponent::BufferInfo * itBufferNodeCur = NULL;
    unsigned long DstPhyAddr = 0;
    unsigned long SrcPhyAddr = 0;
    unsigned long RefPhyAddr = 0;
    BufferCtrlStruct* pBufCtrl = NULL;
    int32_t ret = 0;

    while (!mDone) {
        Mutex::Autolock autoLock(component->mThreadLock);

        while ( outQueue.empty() || mDone
                ||component->mDeinterInputBufQueue.size() < 2) {
            if (mDone) {
                ALOGI("%s, %d: deintl thread done\n", __FUNCTION__, __LINE__);
                return;
            } else {
                ALOGI("%s, %d, mDeinterReadyCondition wait \n", __FUNCTION__, __LINE__);
                mDeinterReadyCondition.wait(component->mThreadLock);
            }
        }

        ALOGI("%s, %d, start to do deinterlace, mDeintlFrameNum = %d\n", __FUNCTION__, __LINE__, mDeintlFrameNum);
        List<SprdSimpleOMXComponent::BufferInfo *>::iterator itor = component->mDeinterInputBufQueue.begin();
        if (mDeintlFrameNum == 0) {
            itBufferNodePre = NULL;   //previous frame, as reference for deinterlace
            itBufferNodeCur = *itor;   //current frame
        } else {
            itBufferNodePre = *itor;   //previous frame, as reference for deinterlace
            itBufferNodeCur = *(++itor);   //current frame

            //remapDeintlSrcBuffer(itBufferNodePre, &RefPhyAddr);
            pBufCtrl= (BufferCtrlStruct*)(itBufferNodePre->mHeader->pOutputPortPrivate);
            RefPhyAddr = pBufCtrl->phyAddr;
            if (!RefPhyAddr) {
                ALOGE("%s, %d, obtain deint processing buffer failed\n", __FUNCTION__, __LINE__);
                return;
            }
        }

        //remapDeintlSrcBuffer(itBufferNodeCur, &SrcPhyAddr);
        pBufCtrl= (BufferCtrlStruct*)(itBufferNodeCur->mHeader->pOutputPortPrivate);
        SrcPhyAddr = pBufCtrl->phyAddr;
        if (!SrcPhyAddr) {
            ALOGE("%s, %d, obtain deint processing buffer failed\n", __FUNCTION__, __LINE__);
            return;
        }

        //find an available display buffer from native window buffer queue
        List<SPRDAVCDecoder::BufferInfo *>::iterator itBuffer = outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = (*itBuffer)->mHeader;
        pBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);

        if(pBufCtrl->phyAddr != 0) {
            DstPhyAddr = pBufCtrl->phyAddr;
            ALOGV("%s, %d, pBufCtrl %p, DstPhyAddr 0x%lx",__FUNCTION__,__LINE__,pBufCtrl,DstPhyAddr);
        } else {
            if (mIOMMU_VPP_Enabled) {
                ret = vpp_deint_get_iova(mVPPHeader, pBufCtrl->bufferFd, &(pBufCtrl->phyAddr), &(pBufCtrl->bufferSize));
                ALOGV("%s, %d, bufferFd : %d, phy_addr : 0x%lx, ret : %d", __FUNCTION__,__LINE__,
                              pBufCtrl->bufferFd, pBufCtrl->phyAddr, ret);
                DstPhyAddr = pBufCtrl->phyAddr;
                if (DstPhyAddr == 0){
                    return;
                }
            } else {
                buffer_handle_t pNativeHandle = (buffer_handle_t)outHeader->pBuffer;
                size_t bufferSize = 0;
                MemIon::Get_phy_addr_from_ion(ADP_BUFFD(pNativeHandle), &DstPhyAddr, &bufferSize);
                pBufCtrl->phyAddr = DstPhyAddr;
                ALOGV("%s, %d, pBufCtrl %p, DstPhyAddr 0x%lx",__FUNCTION__,__LINE__,pBufCtrl,DstPhyAddr);
            }
        }

        uint8 *yuv = NULL;
        //get yuv address of native buffer to store display data
        GraphicBufferMapper &mapper = GraphicBufferMapper::get();
        if(mUseNativeBuffer) {
            OMX_PARAM_PORTDEFINITIONTYPE *def = &component->editPortInfo(OMX_DirOutput)->mDef;
            int width = def->format.video.nStride;
            int height = def->format.video.nSliceHeight;
            Rect bounds(width, height);
            void *vaddr;
            int usage;

            usage = GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_OFTEN;
            if(mapper.lock((const native_handle_t*)outHeader->pBuffer, usage, bounds, &vaddr)) {
                ALOGE("threadFunc, mapper.lock fail %p",outHeader->pBuffer);
                continue;
            }

            yuv = (uint8 *)vaddr + outHeader->nOffset;
        }

        //if the flag of next buffer node is eos, then not call de-interlace
        if (itBufferNodeCur->mHeader->nFilledLen== 0) {
            if (itBufferNodeCur->mHeader->nFlags != OMX_BUFFERFLAG_EOS) {
                ALOGE("%s, %d, nFilledLen is 0 !!!\n", __FUNCTION__, __LINE__);
                return;
            } else {
                ALOGI("%s, %d, see eos flag from decoder output buffer, mNodeId: %d\n",
                      __FUNCTION__, __LINE__, itBufferNodeCur->mNodeId);
            }
        } else {
#ifdef DEINT_NULL
            if (yuv) {
                ALOGI("threadFunc() copy one frame start");
                memcpy(yuv, itBufferNodeCur->mHeader->pBuffer, itBufferNodeCur->mHeader->nFilledLen);
                ALOGI("threadFunc() copy one frame end");
            }
#else

            DEINT_PARAMS_T params;

            params.width  = itBufferNodeCur->DeintlWidth;
            params.height = itBufferNodeCur->DeintlHeight;
            params.interleave = INTERLEAVE;
            params.threshold = THRESHOLD;

            params.y_len = params.width*params.height;
            params.c_len = params.y_len >> 2;

            ret = vpp_deint_process(mVPPHeader, SrcPhyAddr, RefPhyAddr, DstPhyAddr, mDeintlFrameNum, &params);
            if (ret != 0) {
                ALOGE("%s, %d, deint process error, the current frame will not be displayed\n", __FUNCTION__, __LINE__);
                //return;
            }
#endif
        }


        if(mUseNativeBuffer) {
            if(mapper.unlock((const native_handle_t*)outHeader->pBuffer)) {
                ALOGE("threadFunc() threadFunc, mapper.unlock fail %p",outHeader->pBuffer);
            }
        }

        if(ret == 0) {
            mDeintlFrameNum++;

            if (mIOMMU_VPP_Enabled) {
                //vpp_deint_free_iova(mVPPHeader, mDispBufferCtrl->phyAddr, mDispBufferCtrl->bufferSize);
            }

            outHeader->nTimeStamp = itBufferNodeCur->mHeader->nTimeStamp;
            outHeader->nFlags = itBufferNodeCur->mHeader->nFlags;
            outHeader->nFilledLen = itBufferNodeCur->mHeader->nFilledLen;

            component->drainOneOutputBuffer(mDeintlFrameNum, outHeader, (uint64)outHeader->nTimeStamp);
        }

        fillDeintlSrcBuffer(component, itBufferNodePre, itBufferNodeCur);
    }
    ALOGI("%s, %d, deintl thread exit\n", __FUNCTION__, __LINE__);
}

void *SPRDDeinterlace::ThreadWrapper(void *me) {
    ALOGV("ThreadWrapper: %p", me);
    SPRDDeinterlace * deintl = reinterpret_cast<SPRDDeinterlace *>(me);
    deintl->deintlThreadFunc();
    return NULL;
}

void SPRDDeinterlace::startDeinterlaceThread() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&mThread, &attr, ThreadWrapper, this);
    pthread_attr_destroy(&attr);
}

}// namespace android
