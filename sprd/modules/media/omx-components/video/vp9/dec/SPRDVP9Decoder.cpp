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
#define LOG_TAG "SPRDVP9Decoder"
#include <utils/Log.h>

#include "SPRDVP9Decoder.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/IOMX.h>
#include <media/hardware/HardwareAPI.h>
#include <ui/GraphicBufferMapper.h>
#include "Rect.h"

#include "gralloc_public.h"
#include "vp9_dec_api.h"
#include <dlfcn.h>
#include "sprd_ion.h"

namespace android {

const static int64_t kConditionEventTimeOutNs = 3000000000LL;

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

SPRDVP9Decoder::SPRDVP9Decoder(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mHandle(new tagVP9Handle),
      mInputBufferCount(0),
      mSetFreqCount(0),
      mFrameWidth(320),
      mFrameHeight(240),
      mStride(mFrameWidth),
      mSliceHeight(mFrameHeight),
      mMaxWidth(352),
      mMaxHeight(288),
      mCropWidth(mFrameWidth),
      mCropHeight(mFrameHeight),
      mGettingPortFormat(OMX_FALSE),
      mEOSStatus(INPUT_DATA_AVAILABLE),
      mOutputPortSettingsChange(NONE),
      mHeadersDecoded(false),
      mSignalledError(false),
      mNeedIVOP(true),
      mIOMMUEnabled(false),
      mIOMMUID(-1),
      mAllocateBuffers(false),
      mPbuf_inter(NULL),
      mPmem_stream(NULL),
      mPbuf_stream_v(NULL),
      mPbuf_stream_p(0),
      mPbuf_stream_size(0),
      mPmem_extra(NULL),
      mPbuf_extra_v(NULL),
      mPbuf_extra_p(0),
      mPbuf_extra_size(0),
      mLibHandle(NULL),
      mVP9DecSetCurRecPic(NULL),
      mVP9DecInit(NULL),
      mVP9DecDecode(NULL),
      mVP9DecRelease(NULL),
      mVP9GetVideoDimensions(NULL),
      mVP9GetBufferDimensions(NULL),
      mVP9DecReleaseRefBuffers(NULL),
      mVP9DecGetLastDspFrm(NULL),
      mVP9GetCodecCapability(NULL),
      mVP9DecGetIOVA(NULL),
      mVP9DecFreeIOVA(NULL),
      mVP9DecGetIOMMUStatus(NULL),
      mVP9DecMemInit(NULL) {

    ALOGI("Construct SPRDVP9Decoder, this: %p", (void *)this);

    initPorts();
    CHECK_EQ(openDecoder("libomx_vp9dec_hw_sprd.so"), true);

    CHECK_EQ(initDecoder(), (status_t)OK);

    iUseAndroidNativeBuffer[OMX_DirInput] = OMX_FALSE;
    iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_FALSE;

#ifdef SPRD_DUMP_YUV
    mFile_yuv = fopen("/data/misc/media/video_out.yuv", "wb");
#endif

#ifdef SPRD_DUMP_BS
    mFile_bs = fopen("/data/misc/media/video_es.vp9", "wb");
#endif
}

SPRDVP9Decoder::~SPRDVP9Decoder() {

    ALOGI("Destruct SPRDVP9Decoder, this: %p", (void *)this);

#ifdef SPRD_DUMP_BS
    if (mFile_bs) {
        fclose(mFile_bs);
        mFile_bs = NULL;
    }
#endif

#ifdef SPRD_DUMP_YUV
    if (mFile_yuv) {
        fclose(mFile_yuv);
        mFile_yuv = NULL;
    }
#endif

    releaseDecoder();

    delete mHandle;
    mHandle = NULL;

    if(mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

void SPRDVP9Decoder::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = 1920*1088*3/2/2;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_VP9);
    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mFrameWidth;
    def.format.video.nFrameHeight = mFrameHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingVP9;
    def.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    def.format.video.pNativeWindow = NULL;

    addPort(def);

    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;

    def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_RAW);
    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mFrameWidth;
    def.format.video.nFrameHeight = mFrameHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    def.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    def.format.video.pNativeWindow = NULL;

    def.nBufferSize =
        (def.format.video.nFrameWidth * def.format.video.nFrameHeight * 3) / 2;

    addPort(def);
}

status_t SPRDVP9Decoder::initDecoder() {
    ALOGE("J:initDecoder1");
    memset(mHandle, 0, sizeof(VP9Handle));
    ALOGE("J:initDecoder2");
    mHandle->userdata = (void *)this;
    mHandle->VSP_bindCb = BindFrameWrapper;
    mHandle->VSP_unbindCb = UnbindFrameWrapper;
    mHandle->VSP_extMemCb = extMemoryAllocWrapper;

    MMCodecBuffer InterMemBfr;
    MMDecVideoFormat VideoFormat;

    unsigned long phy_addr = 0;
    size_t size = 0;

    uint32_t size_inter = VP9_DECODER_INTERNAL_BUFFER_SIZE;
    mPbuf_inter = (uint8_t *)malloc(size_inter);
    CHECK(mPbuf_inter != NULL);

    InterMemBfr.common_buffer_ptr = mPbuf_inter;
    InterMemBfr.common_buffer_ptr_phy= 0;
    InterMemBfr.size = size_inter;

    VideoFormat.yuv_format = YUV420SP_NV12;
    ALOGE("J:initDecoder3");
    if((*mVP9DecInit)( mHandle, &InterMemBfr, &VideoFormat) != MMDEC_OK) {
        ALOGE("Failed to init VP9DEC");
        return OMX_ErrorUndefined;
    }

    if ((*mVP9GetCodecCapability)(mHandle, &mMaxWidth, &mMaxHeight) != MMDEC_OK) {
        ALOGE("Failed to mVP9GetCodecCapability\n");
    }

    int ret = (*mVP9DecGetIOMMUStatus)(mHandle);
    ALOGI("Get IOMMU Status: %d(%s)", ret, strerror(errno));
    if (ret < 0) {
        mIOMMUEnabled = false;
    } else {
        mIOMMUEnabled = true;
    }
    mIOMMUEnabled = false;
    //unsigned long phy_addr = 0;
    size_t size_stream;

    size_stream = ONEFRAME_BITSTREAM_BFR_SIZE;
    if (mIOMMUEnabled) {
        mPmem_stream = new MemIon(SPRD_ION_DEV, size_stream, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        mPmem_stream = new MemIon(SPRD_ION_DEV, size_stream, MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }
    int fd = mPmem_stream->getHeapID();
    if (fd < 0) {
        ALOGE("Failed to alloc bitstream pmem buffer, getHeapID failed");
        return OMX_ErrorInsufficientResources;
    } else {
        int ret;
        if (mIOMMUEnabled) {
            ret = (*mVP9DecGetIOVA)(mHandle, fd, &phy_addr, &size);
        } else {
            ret = mPmem_stream->get_phy_addr_from_ion(&phy_addr, &size);
        }
        if (ret < 0) {
            ALOGE("Failed to alloc bitstream pmem buffer, get phy addr failed");
            return OMX_ErrorInsufficientResources;
        } else {
            mPbuf_stream_v = (uint8_t *)mPmem_stream->getBase();
            mPbuf_stream_p = phy_addr;
            mPbuf_stream_size = size;
        }
    }
    ALOGI("mPbuf_stream_p 0x%x",mPbuf_stream_p);
    return OMX_ErrorNone;
}

void SPRDVP9Decoder::releaseDecoder() {
    if (mPbuf_stream_v != NULL) {
        if (mIOMMUEnabled) {
            (*mVP9DecFreeIOVA)(mHandle, mPbuf_stream_p, mPbuf_stream_size);
        }
        mPmem_stream.clear();
        mPbuf_stream_v = NULL;
        mPbuf_stream_p = 0;
        mPbuf_stream_size = 0;
    }

    if(mPbuf_extra_v != NULL) {
        if (mIOMMUEnabled) {
            (*mVP9DecFreeIOVA)(mHandle, mPbuf_extra_p, mPbuf_extra_size);
        }
        mPmem_extra.clear();
        mPbuf_extra_v = NULL;
        mPbuf_extra_p = 0;
        mPbuf_extra_size = 0;
    }

    (*mVP9DecRelease)(mHandle);

    if (mPbuf_inter != NULL) {
        free(mPbuf_inter);
        mPbuf_inter = NULL;
    }
}

OMX_ERRORTYPE SPRDVP9Decoder::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > 1) {
            return OMX_ErrorUndefined;
        }

        if (formatParams->nIndex != 0) {
            return OMX_ErrorNoMore;
        }

        if (formatParams->nPortIndex == 0) {
            formatParams->eCompressionFormat = OMX_VIDEO_CodingVP9;
            formatParams->eColorFormat = OMX_COLOR_FormatUnused;
            formatParams->xFramerate = 0;
        } else {
            CHECK_EQ(formatParams->nPortIndex, 1u);

            PortInfo *pOutPort = editPortInfo(kOutputPortIndex);
            ALOGI("internalGetParameter, OMX_IndexParamVideoPortFormat, eColorFormat: 0x%x",pOutPort->mDef.format.video.eColorFormat);
            formatParams->eCompressionFormat = OMX_VIDEO_CodingUnused;
            formatParams->eColorFormat = pOutPort->mDef.format.video.eColorFormat;
            formatParams->xFramerate = 0;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamEnableAndroidBuffers:
    {
        EnableAndroidNativeBuffersParams *peanbp = (EnableAndroidNativeBuffersParams *)params;
        peanbp->enable = iUseAndroidNativeBuffer[OMX_DirOutput];
        ALOGI("internalGetParameter, OMX_IndexParamEnableAndroidBuffers %d",peanbp->enable);
        return OMX_ErrorNone;
    }

    case OMX_IndexParamGetAndroidNativeBuffer:
    {
        GetAndroidNativeBufferUsageParams *pganbp;

        pganbp = (GetAndroidNativeBufferUsageParams *)params;
        if(mIOMMUEnabled) {
            pganbp->nUsage = GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_OFTEN;
        } else {
            pganbp->nUsage = GRALLOC_USAGE_VIDEO_BUFFER|GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_OFTEN;
        }
        ALOGI("internalGetParameter, OMX_IndexParamGetAndroidNativeBuffer 0x%x",pganbp->nUsage);
        return OMX_ErrorNone;
    }

    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *defParams =
            (OMX_PARAM_PORTDEFINITIONTYPE *)params;

        if (defParams->nPortIndex > 1
                || defParams->nSize
                != sizeof(OMX_PARAM_PORTDEFINITIONTYPE)) {
            return OMX_ErrorUndefined;
        }

        PortInfo *port = editPortInfo(defParams->nPortIndex);

        {
            Mutex::Autolock autoLock(mLock);
            memcpy(defParams, &port->mDef, sizeof(port->mDef));
            if (isExecuting() && mOutputPortSettingsChange == NONE) {
                mGettingPortFormat = OMX_TRUE;
            } else {
                mGettingPortFormat = OMX_FALSE;
            }
        }
        return OMX_ErrorNone;
    }

    default:
        return OMX_ErrorUnsupportedIndex;
    }
}

OMX_ERRORTYPE SPRDVP9Decoder::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamStandardComponentRole:
    {
        const OMX_PARAM_COMPONENTROLETYPE *roleParams =
            (const OMX_PARAM_COMPONENTROLETYPE *)params;

        if (strncmp((const char *)roleParams->cRole,
                    "video_decoder.vp9",
                    OMX_MAX_STRINGNAME_SIZE - 1)) {
            return OMX_ErrorUndefined;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > 1) {
            return OMX_ErrorUndefined;
        }

        if (formatParams->nIndex != 0) {
            return OMX_ErrorNoMore;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamEnableAndroidBuffers:
    {
        EnableAndroidNativeBuffersParams *peanbp = (EnableAndroidNativeBuffersParams *)params;
        PortInfo *pOutPort = editPortInfo(kOutputPortIndex);
        if (peanbp->enable == OMX_FALSE) {
            ALOGI("internalSetParameter, disable AndroidNativeBuffer");
            iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_FALSE;

            pOutPort->mDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        } else {
            ALOGI("internalSetParameter, enable AndroidNativeBuffer");
            iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_TRUE;

            pOutPort->mDef.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCbCr_420_SP;
        }
        return OMX_ErrorNone;
    }

    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *defParams =
            (OMX_PARAM_PORTDEFINITIONTYPE *)params;

        if (defParams->nPortIndex > 1
                || defParams->nSize
                != sizeof(OMX_PARAM_PORTDEFINITIONTYPE)) {
            return OMX_ErrorUndefined;
        }

        PortInfo *port = editPortInfo(defParams->nPortIndex);

        if (defParams->nBufferSize != port->mDef.nBufferSize) {
            CHECK_GE(defParams->nBufferSize, port->mDef.nBufferSize);
            port->mDef.nBufferSize = defParams->nBufferSize;
        }

        if (defParams->nBufferCountActual
                != port->mDef.nBufferCountActual) {
            CHECK_GE(defParams->nBufferCountActual,
                     port->mDef.nBufferCountMin);

            port->mDef.nBufferCountActual = defParams->nBufferCountActual;
        }

        uint32_t oldWidth = port->mDef.format.video.nFrameWidth;
        uint32_t oldHeight = port->mDef.format.video.nFrameHeight;
        uint32_t newWidth = defParams->format.video.nFrameWidth;
        uint32_t newHeight = defParams->format.video.nFrameHeight;

        ALOGI("%s, old wh:%d %d, new wh:%d %d", __FUNCTION__, oldWidth, oldHeight, newWidth, newHeight);

        memcpy(&port->mDef.format.video, &defParams->format.video, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));

        if((oldWidth != newWidth || oldHeight != newHeight)) {
            if (defParams->nPortIndex == kOutputPortIndex) {
                mFrameWidth = newWidth;
                mFrameHeight = newHeight;
                mStride = ((newWidth + 63) & ~63);
                mSliceHeight = ((newHeight + 63) & ~63);

                ALOGI("%s, mFrameWidth %d, mFrameHeight %d, mStride %d, mSliceHeight %d", __FUNCTION__,
                      mFrameWidth, mFrameHeight, mStride, mSliceHeight);

                updatePortDefinitions(true, true);
            } else {
                port->mDef.format.video.nFrameWidth = newWidth;
                port->mDef.format.video.nFrameHeight = newHeight;
            }
        }

        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDVP9Decoder::internalUseBuffer(
    OMX_BUFFERHEADERTYPE **header,
    OMX_U32 portIndex,
    OMX_PTR appPrivate,
    OMX_U32 size,
    OMX_U8 *ptr,
    BufferPrivateStruct* bufferPrivate) {

    *header = new OMX_BUFFERHEADERTYPE;
    (*header)->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    (*header)->nVersion.s.nVersionMajor = 1;
    (*header)->nVersion.s.nVersionMinor = 0;
    (*header)->nVersion.s.nRevision = 0;
    (*header)->nVersion.s.nStep = 0;
    (*header)->pBuffer = ptr;
    (*header)->nAllocLen = size;
    (*header)->nFilledLen = 0;
    (*header)->nOffset = 0;
    (*header)->pAppPrivate = appPrivate;
    (*header)->pPlatformPrivate = NULL;
    (*header)->pInputPortPrivate = NULL;
    (*header)->pOutputPortPrivate = NULL;
    (*header)->hMarkTargetComponent = NULL;
    (*header)->pMarkData = NULL;
    (*header)->nTickCount = 0;
    (*header)->nTimeStamp = 0;
    (*header)->nFlags = 0;
    (*header)->nOutputPortIndex = portIndex;
    (*header)->nInputPortIndex = portIndex;

    if(portIndex == OMX_DirOutput) {
        (*header)->pOutputPortPrivate = new BufferCtrlStruct;
        CHECK((*header)->pOutputPortPrivate != NULL);
        BufferCtrlStruct* pBufCtrl= (BufferCtrlStruct*)((*header)->pOutputPortPrivate);
        pBufCtrl->iRefCount = 1; //init by1
        pBufCtrl->id = mIOMMUID;
        if(bufferPrivate != NULL) {
            pBufCtrl->pMem = ((BufferPrivateStruct*)bufferPrivate)->pMem;
            pBufCtrl->phyAddr = ((BufferPrivateStruct*)bufferPrivate)->phyAddr;
            pBufCtrl->bufferSize = ((BufferPrivateStruct*)bufferPrivate)->bufferSize;
            pBufCtrl->bufferFd = 0;
        } else {
            if (mIOMMUEnabled) {
                unsigned long picPhyAddr = 0;
                size_t bufferSize = 0;
                buffer_handle_t pNativeHandle = (buffer_handle_t)((*header)->pBuffer);
                int fd = ADP_BUFFD(pNativeHandle);

                (*mVP9DecGetIOVA)(mHandle, fd, &picPhyAddr, &bufferSize);
                ALOGI(" mVP9DecGetIOVA %d fd :%d 0x%x",__LINE__,fd,picPhyAddr);
                pBufCtrl->pMem = NULL;
                pBufCtrl->bufferFd = fd;
                pBufCtrl->phyAddr = picPhyAddr;
                pBufCtrl->bufferSize = bufferSize;
            } else {
                pBufCtrl->pMem = NULL;
                pBufCtrl->bufferFd = 0;
                pBufCtrl->phyAddr = 0;
                pBufCtrl->bufferSize = 0;
            }
        }
    }

    PortInfo *port = editPortInfo(portIndex);

    port->mBuffers.push();

    BufferInfo *buffer =
        &port->mBuffers.editItemAt(port->mBuffers.size() - 1);
    ALOGI("internalUseBuffer, header=%p, pBuffer=%p, size=%d",*header, ptr, size);
    buffer->mHeader = *header;
    buffer->mOwnedByUs = false;

    if (port->mBuffers.size() == port->mDef.nBufferCountActual) {
        port->mDef.bPopulated = OMX_TRUE;
        checkTransitions();
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDVP9Decoder::allocateBuffer(
    OMX_BUFFERHEADERTYPE **header,
    OMX_U32 portIndex,
    OMX_PTR appPrivate,
    OMX_U32 size) {
    switch (portIndex) {
    case OMX_DirInput:
        return SprdSimpleOMXComponent::allocateBuffer(header, portIndex, appPrivate, size);

    case OMX_DirOutput:
    {
        mAllocateBuffers = true;
        MemIon* pMem = NULL;
        unsigned long phyAddr = 0;
        size_t bufferSize = 0;
        OMX_U8* pBuffer = NULL;
        size_t size64word = (size + 1024*4 - 1) & ~(1024*4 - 1);
        int fd, ret;

        if (mIOMMUEnabled) {
            pMem = new MemIon(SPRD_ION_DEV, size64word, 0 , (1<<31) | ION_HEAP_ID_MASK_SYSTEM);
        } else {
            pMem = new MemIon(SPRD_ION_DEV, size64word, 0 , (1<<31) | ION_HEAP_ID_MASK_MM);
        }

        fd = pMem->getHeapID();
        if(fd < 0) {
            ALOGE("Failed to alloc outport pmem buffer");
            return OMX_ErrorInsufficientResources;
        }

        if (mIOMMUEnabled) {
            ret = (*mVP9DecGetIOVA)(mHandle, fd, &phyAddr, &bufferSize);
            if(ret) {
                ALOGE("VP9DecGetIOVA Failed: %d(%s)", ret, strerror(errno));
                return OMX_ErrorInsufficientResources;
            }
        } else {
            if(pMem->get_phy_addr_from_ion(&phyAddr, &bufferSize)) {
                ALOGE("get_phy_addr_from_ion fail");
                return OMX_ErrorInsufficientResources;
            }
        }

        pBuffer = (OMX_U8 *)(pMem->getBase());
        BufferPrivateStruct* bufferPrivate = new BufferPrivateStruct();
        bufferPrivate->pMem = pMem;
        bufferPrivate->phyAddr = phyAddr;
        bufferPrivate->bufferSize = bufferSize;
        ALOGI("allocateBuffer, allocate buffer from pmem, pBuffer: %p, phyAddr: 0x%lx, size: %zd", pBuffer, phyAddr, bufferSize);

        SprdSimpleOMXComponent::useBuffer(header, portIndex, appPrivate, (OMX_U32)bufferSize, pBuffer, bufferPrivate);
        delete bufferPrivate;

        return OMX_ErrorNone;
    }

    default:
        return OMX_ErrorUnsupportedIndex;

    }
}

OMX_ERRORTYPE SPRDVP9Decoder::freeBuffer(
    OMX_U32 portIndex,
    OMX_BUFFERHEADERTYPE *header) {
    switch (portIndex) {
    case OMX_DirInput:
        return SprdSimpleOMXComponent::freeBuffer(portIndex, header);

    case OMX_DirOutput:
    {
        BufferCtrlStruct* pBufCtrl= (BufferCtrlStruct*)(header->pOutputPortPrivate);
        if(pBufCtrl != NULL) {
            if(pBufCtrl->pMem != NULL) {
                ALOGI("freeBuffer, phyAddr: 0x%lx", pBufCtrl->phyAddr);
                if (mIOMMUEnabled) {
                    (*mVP9DecFreeIOVA)(mHandle, pBufCtrl->phyAddr, pBufCtrl->bufferSize);
                }
                pBufCtrl->pMem.clear();
            } else {
                if (mIOMMUEnabled && pBufCtrl->bufferFd > 0) {
                    (*mVP9DecFreeIOVA)(mHandle, pBufCtrl->phyAddr, pBufCtrl->bufferSize);
                }
            }
            return SprdSimpleOMXComponent::freeBuffer(portIndex, header);
        } else {
            ALOGE("freeBuffer, pBufCtrl==NULL");
            return OMX_ErrorUndefined;
        }
    }

    default:
        return OMX_ErrorUnsupportedIndex;
    }
}

void SPRDVP9Decoder::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return;
    }

    if (mEOSStatus == OUTPUT_FRAMES_FLUSHED) {
        return;
    }

    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    while ((mEOSStatus != INPUT_DATA_AVAILABLE || !inQueue.empty())
            && !outQueue.empty()) {

        if (mEOSStatus == INPUT_EOS_SEEN) {
            drainAllOutputBuffers();
            return;
        }

        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;

        List<BufferInfo *>::iterator itBuffer = outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = NULL;
        BufferCtrlStruct *pBufCtrl = NULL;
        size_t count = 0;
        do {
            if(count >= outQueue.size()) {
                ALOGI("%s, %d, get outQueue buffer fail, return, count=%zd, queue_size=%d",__FUNCTION__, __LINE__, count, outQueue.size());
                return;
            }

            outHeader = (*itBuffer)->mHeader;
            pBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
            if(pBufCtrl == NULL) {
                ALOGE("onQueueFilled, pBufCtrl == NULL, fail");
                notify(OMX_EventError, OMX_ErrorUndefined, 0, NULL);
                mSignalledError = true;
                return;
            }

            itBuffer++;
            count++;
        } while (pBufCtrl->iRefCount > 0);

        ALOGI("%s, %d, outHeader:%p, inHeader: %p, len: %d, nOffset: %d, time: %lld, EOS: %d",
              __FUNCTION__, __LINE__,outHeader,inHeader, inHeader->nFilledLen,inHeader->nOffset, inHeader->nTimeStamp,inHeader->nFlags & OMX_BUFFERFLAG_EOS);

        if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
            mEOSStatus = INPUT_EOS_SEEN; //the last frame size may be not zero, it need to be decoded.
        }

        if(inHeader->nFilledLen == 0) {
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
            continue;
        }

        outHeader->nTimeStamp = inHeader->nTimeStamp;

        MMDecInput dec_in;
        MMDecOutput dec_out;

        uint8_t *bitstream = inHeader->pBuffer + inHeader->nOffset;
        uint32_t bufferSize = inHeader->nFilledLen;

        if (mPbuf_stream_v != NULL) {
            memcpy(mPbuf_stream_v, bitstream, bufferSize);
        }
        dec_in.pStream=  mPbuf_stream_v;
        dec_in.pStream_phy= mPbuf_stream_p;
        dec_in.dataLen = bufferSize;
        dec_in.beLastFrm = 0;
        dec_in.expected_IVOP = mNeedIVOP;
        dec_in.beDisplayed = 1;
        dec_in.err_pkt_num = 0;

        dec_out.frameEffective = 0;

        unsigned long picPhyAddr = 0;

        pBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
        if(pBufCtrl->phyAddr != 0) {
            picPhyAddr = pBufCtrl->phyAddr;
        } else {
            if (mIOMMUEnabled) {
                ALOGE("onQueueFilled, pBufCtrl->phyAddr == 0, fail");
                notify(OMX_EventError, OMX_ErrorUndefined, 0, NULL);
                mSignalledError = true;
                return;
            } else {
                buffer_handle_t pNativeHandle = (buffer_handle_t)outHeader->pBuffer;
                size_t bufferSize = 0;
                MemIon::Get_phy_addr_from_ion(ADP_BUFFD(pNativeHandle), &picPhyAddr, &bufferSize);
                pBufCtrl->phyAddr = picPhyAddr;
            }
        }

        if (mAllocateBuffers)
            (pBufCtrl->pMem)->flush_ion_buffer((void*)(outHeader->pBuffer + outHeader->nOffset), (void*)(pBufCtrl->phyAddr), pBufCtrl->bufferSize);

        GraphicBufferMapper &mapper = GraphicBufferMapper::get();
        if(iUseAndroidNativeBuffer[OMX_DirOutput]) {
            OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(kOutputPortIndex)->mDef;
            int width = def->format.video.nStride;
            int height = def->format.video.nSliceHeight;
            Rect bounds(width, height);
            void *vaddr;
            int usage;

            usage = GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_OFTEN;

            if(mapper.lock((const native_handle_t*)outHeader->pBuffer, usage, bounds, &vaddr)) {
                ALOGI("onQueueFilled, mapper.lock fail %p",outHeader->pBuffer);
                return ;
            }
            uint8_t *yuv = (uint8_t *)((uint8_t *)vaddr + outHeader->nOffset);
            ALOGV("%s, %d, yuv: %p,outHeader->pBuffer: %p, outHeader->nOffset: %d, outHeader->nFlags: %d, outHeader->nTimeStamp: %lld",
                  __FUNCTION__, __LINE__, yuv, outHeader->pBuffer, outHeader->nOffset, outHeader->nFlags, outHeader->nTimeStamp);
            (*mVP9DecSetCurRecPic)(mHandle, yuv, (uint8_t *)picPhyAddr, (void *)outHeader);
        } else {
            (*mVP9DecSetCurRecPic)(mHandle, outHeader->pBuffer, (uint8_t *)picPhyAddr, (void *)outHeader);
        }
#ifdef SPRD_DUMP_BS
        if (mFile_bs != NULL) {
            fwrite(mPbuf_stream_v, 1, dec_in.dataLen, mFile_bs);
        }
#endif
        MMDecRet decRet = (*mVP9DecDecode)(mHandle, &dec_in,&dec_out);
        ALOGI("%s, %d, decRet: %d, dec_out.frameEffective: %d", __FUNCTION__, __LINE__, decRet, dec_out.frameEffective);

        if(iUseAndroidNativeBuffer[OMX_DirOutput]) {
            if(mapper.unlock((const native_handle_t*)outHeader->pBuffer)) {
                ALOGI("onQueueFilled, mapper.unlock fail %p",outHeader->pBuffer);
            }
        }

        if (decRet == MMDEC_OK || decRet == MMDEC_MEMORY_ALLOCED) {
            int32_t disp_width, disp_height;
            int32_t buf_width, buf_height;

            (*mVP9GetBufferDimensions)(mHandle, &buf_width, &buf_height);

            if (!((buf_width<= mMaxWidth&& buf_height<= mMaxHeight)
                    || (buf_width <= mMaxHeight && buf_height <= mMaxWidth))) {
                ALOGE("[%d,%d] is out of range [%d, %d], failed to support this format.",
                      buf_width, buf_height, mMaxWidth, mMaxHeight);
                notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, NULL);
                mSignalledError = true;
                return;
            }

            (*mVP9GetVideoDimensions)(mHandle, &disp_width, &disp_height);

            if (disp_width != mFrameWidth ||
                    disp_height != mFrameHeight ||
                    buf_width != mStride ||
                    buf_height != mSliceHeight) {
                Mutex::Autolock autoLock(mLock);
                if (mGettingPortFormat == OMX_TRUE) {
                    ALOGI("%s, waiting for get crop parameter done", __FUNCTION__);
                    status_t err = mCondition.waitRelative(mLock, kConditionEventTimeOutNs);
                    if (err != OK) {
                        ALOGE("Timed out waiting for mCondition signal!");
                    }
                }
                ALOGI("%s, OutputPortSettingsChange, disp wh %dx%d -> %dx%d, buf wh %dx%d -> %dx%d",
                    __FUNCTION__, mFrameWidth, mFrameHeight, disp_width, disp_height,
                    mStride, mSliceHeight, buf_width, buf_height);
                mFrameWidth = disp_width;
                mFrameHeight = disp_height;
                mStride = buf_width;
                mSliceHeight = buf_height;
                updatePortDefinitions(true, true);
                (*mVP9DecReleaseRefBuffers)(mHandle);

                notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
                mOutputPortSettingsChange = AWAITING_DISABLED;
                return;
            } else if(decRet == MMDEC_MEMORY_ALLOCED) {
                continue;
            }
            mNeedIVOP = false;
        } else if (decRet == MMDEC_MEMORY_ERROR) {
            ALOGE("failed to allocate memory, signal error!");
            notify(OMX_EventError, OMX_ErrorInsufficientResources, 0, NULL);
            mSignalledError = true;
            return;
        } else {
            ALOGE("failed to decode video frame.");
            mNeedIVOP = true;
            if (decRet == MMDEC_NOT_SUPPORTED) {
                ALOGE("failed to support this format.");
                notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, NULL);
                mSignalledError = true;
               return;
            } else if (decRet == MMDEC_STREAM_ERROR) {
                ALOGE("failed to decode video frame, stream error");
            } else if (decRet == MMDEC_HW_ERROR) {
                ALOGE("failed to decode video frame, hardware error");
            } else {
                ALOGI("now, we don't take care of the decoder return: %d", decRet);
            }
        }

        bufferSize = dec_in.dataLen;
        inHeader->nOffset += bufferSize;


        ALOGI("bufferSize %d, %d",bufferSize,inHeader->nFilledLen);
        CHECK_LE(bufferSize, inHeader->nFilledLen);
        inHeader->nFilledLen -= bufferSize;

        if (inHeader->nFilledLen <= 0) {
            inHeader->nOffset = 0;
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
        }
        ++mInputBufferCount;

        if (dec_out.frameEffective) {
            outHeader = (OMX_BUFFERHEADERTYPE*)(dec_out.pBufferHeader);
            outHeader->nOffset = 0;
            outHeader->nFilledLen = (mStride * mSliceHeight * 3) / 2;
            outHeader->nFlags = 0;
            ALOGI("%s, %d, drainOneOutputBuffer, dec_out.pBufferHeader: %p, nTimeStamp: %lld mStride: %d, mSliceHeight: %d",
                __FUNCTION__, __LINE__, outHeader, outHeader->nTimeStamp,mStride, mSliceHeight);
#ifdef SPRD_DUMP_YUV
            if (mFile_yuv != NULL) {
                fwrite(dec_out.pOutFrameY, 1, outHeader->nFilledLen, mFile_yuv);
            }
#endif
        } else {
            continue;
        }

        List<BufferInfo *>::iterator it = outQueue.begin();
        while ((*it)->mHeader != outHeader) {
            ++it;
        }

        BufferInfo *outInfo = *it;
        outInfo->mOwnedByUs = false;
        outQueue.erase(it);
        outInfo = NULL;

        BufferCtrlStruct* pOutBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
        pOutBufCtrl->iRefCount++;

        notifyFillBufferDone(outHeader);
        outHeader = NULL;
    }
}

bool SPRDVP9Decoder::drainAllOutputBuffers() {
    ALOGI("%s, %d", __FUNCTION__, __LINE__);

    List<BufferInfo *> &outQueue = getPortQueue(1);
    BufferInfo *outInfo;
    OMX_BUFFERHEADERTYPE *outHeader;
    void *pBufferHeader;

    while (!outQueue.empty() && mEOSStatus != OUTPUT_FRAMES_FLUSHED) {

        if (mHeadersDecoded && (*mVP9DecGetLastDspFrm)(mHandle, &pBufferHeader) ) {
            ALOGI("%s, %d, VP9DecGetLastDspFrm, pBufferHeader: %p", __FUNCTION__, __LINE__, pBufferHeader);
            List<BufferInfo *>::iterator it = outQueue.begin();
            while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != outQueue.end()) {
                ++it;
            }
            CHECK((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)pBufferHeader);
            outInfo = *it;
            outQueue.erase(it);
            outHeader = outInfo->mHeader;
            outHeader->nFilledLen = (mStride * mSliceHeight * 3) / 2;
        } else {

            ALOGI("%s, %d, output EOS", __FUNCTION__, __LINE__);
            outInfo = *outQueue.begin();
            outQueue.erase(outQueue.begin());
            outHeader = outInfo->mHeader;
            outHeader->nTimeStamp = 0;
            outHeader->nFilledLen = 0;
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;
            mEOSStatus = OUTPUT_FRAMES_FLUSHED;
        }

        outInfo->mOwnedByUs = false;
        BufferCtrlStruct* pOutBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
        pOutBufCtrl->iRefCount++;
        notifyFillBufferDone(outHeader);
    }

    return true;
}

void SPRDVP9Decoder::onPortFlushCompleted(OMX_U32 portIndex) {
    ALOGI("onPortFlushCompleted, portIndex:%d",portIndex);

    if (portIndex == OMX_DirInput) {
        mEOSStatus = INPUT_DATA_AVAILABLE;
        mNeedIVOP = true;
    }
}

void SPRDVP9Decoder::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
    if (portIndex != 1) {
        return;
    }

    switch (mOutputPortSettingsChange) {
    case NONE:
        break;

    case AWAITING_DISABLED:
    {
        CHECK(!enabled);
        mOutputPortSettingsChange = AWAITING_ENABLED;
        break;
    }

    default:
    {
        CHECK_EQ((int)mOutputPortSettingsChange, (int)AWAITING_ENABLED);
        CHECK(enabled);
        mOutputPortSettingsChange = NONE;
        break;
    }
    }
}

OMX_ERRORTYPE SPRDVP9Decoder::getConfig(
    OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
    case OMX_IndexConfigCommonOutputCrop:
    {
        OMX_CONFIG_RECTTYPE *rectParams = (OMX_CONFIG_RECTTYPE *)params;

        if (rectParams->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        rectParams->nLeft = 0;
        rectParams->nTop = 0;
        {
            ALOGI("%s, mCropWidth:%d, mCropHeight:%d, mGettingPortFormat:%d",
                __FUNCTION__, mCropWidth, mCropHeight, mGettingPortFormat);
            Mutex::Autolock autoLock(mLock);
            rectParams->nWidth = mCropWidth;
            rectParams->nHeight = mCropHeight;
            mGettingPortFormat = OMX_FALSE;
            mCondition.signal();
        }
        return OMX_ErrorNone;
    }

    default:
        return OMX_ErrorUnsupportedIndex;
    }
}

void SPRDVP9Decoder::onPortFlushPrepare(OMX_U32 portIndex) {
    ALOGI("onPortFlushPrepare, portIndex:%d",portIndex);

    if (portIndex == OMX_DirOutput) {
        if (NULL != mVP9DecReleaseRefBuffers)
            (*mVP9DecReleaseRefBuffers)(mHandle);
    }
}

void SPRDVP9Decoder::onReset() {
    mGettingPortFormat = OMX_FALSE;
    mSignalledError = false;

    //avoid process error after stop codec and restart codec when port settings changing.
    mOutputPortSettingsChange = NONE;
}

void SPRDVP9Decoder::updatePortDefinitions(bool updateCrop, bool updateInputSize) {
    OMX_PARAM_PORTDEFINITIONTYPE *outDef = &editPortInfo(kOutputPortIndex)->mDef;

    if (updateCrop) {
        mCropWidth = mFrameWidth;
        mCropHeight = mFrameHeight;
    }
    outDef->format.video.nFrameWidth = mStride;
    outDef->format.video.nFrameHeight = mSliceHeight;
    outDef->format.video.nStride = mStride;
    outDef->format.video.nSliceHeight = mSliceHeight;
    outDef->nBufferSize = mStride * mSliceHeight * 3 / 2;

    ALOGI("%s, %d %d %d %d", __FUNCTION__, outDef->format.video.nFrameWidth,
          outDef->format.video.nFrameHeight,
          outDef->format.video.nStride,
          outDef->format.video.nSliceHeight);

    OMX_PARAM_PORTDEFINITIONTYPE *inDef = &editPortInfo(kInputPortIndex)->mDef;

    inDef->format.video.nFrameWidth = mFrameWidth;
    inDef->format.video.nFrameHeight = mFrameHeight;
    // input port is compressed, hence it has no stride
    inDef->format.video.nStride = 0;
    inDef->format.video.nSliceHeight = 0;
}

OMX_ERRORTYPE SPRDVP9Decoder::getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index) {

    ALOGI("getExtensionIndex, name: %s",name);
    if(strcmp(name, SPRD_INDEX_PARAM_ENABLE_ANB) == 0) {
        ALOGI("getExtensionIndex:%s",SPRD_INDEX_PARAM_ENABLE_ANB);
        *index = (OMX_INDEXTYPE) OMX_IndexParamEnableAndroidBuffers;
        return OMX_ErrorNone;
    } else if (strcmp(name, SPRD_INDEX_PARAM_GET_ANB) == 0) {
        ALOGI("getExtensionIndex:%s",SPRD_INDEX_PARAM_GET_ANB);
        *index = (OMX_INDEXTYPE) OMX_IndexParamGetAndroidNativeBuffer;
        return OMX_ErrorNone;
    } else if (strcmp(name, SPRD_INDEX_PARAM_USE_ANB) == 0) {
        ALOGI("getExtensionIndex:%s",SPRD_INDEX_PARAM_USE_ANB);
        *index = OMX_IndexParamUseAndroidNativeBuffer2;
        return OMX_ErrorNone;
    }

    return OMX_ErrorNotImplemented;
}

// static
int32_t SPRDVP9Decoder::BindFrameWrapper(
    void *aUserData, void *pHeader, int flag) {
    return static_cast<SPRDVP9Decoder *>(aUserData)->VSP_bind_cb(pHeader, flag);
}

// static
int32_t SPRDVP9Decoder::UnbindFrameWrapper(
    void *aUserData, void *pHeader, int flag) {
    return static_cast<SPRDVP9Decoder *>(aUserData)->VSP_unbind_cb(pHeader, flag);
}

int32_t SPRDVP9Decoder::extMemoryAllocWrapper(
    void *aUserData, unsigned int extra_mem_size) {
    return static_cast<SPRDVP9Decoder *>(aUserData)->extMemoryAlloc(extra_mem_size);
}

int SPRDVP9Decoder::VSP_bind_cb(void *pHeader,int flag) {
    BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

    ALOGI("VSP_bind_cb, pBuffer: %p, pHeader: %p; iRefCount=%d",
          ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader,pBufCtrl->iRefCount);

    pBufCtrl->iRefCount++;
    return 0;
}

int SPRDVP9Decoder::VSP_unbind_cb(void *pHeader,int flag) {
    BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

    ALOGI("VSP_unbind_cb, pBuffer: %p, pHeader: %p; iRefCount=%d",
          ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader,pBufCtrl->iRefCount);

    if (pBufCtrl->iRefCount  > 0) {
        pBufCtrl->iRefCount--;
    }

    return 0;
}

int SPRDVP9Decoder::extMemoryAlloc(unsigned int extra_mem_size) {

    MMCodecBuffer extra_mem[MAX_MEM_TYPE];
    ALOGI("%s, %d, extra_mem_size: %d", __FUNCTION__, __LINE__, extra_mem_size);

    if (mPbuf_extra_v != NULL) {
        if (mIOMMUEnabled) {
            (*mVP9DecFreeIOVA)(mHandle, mPbuf_extra_p, mPbuf_extra_size);
        }
        mPmem_extra.clear();
        mPbuf_extra_v = NULL;
        mPbuf_extra_p = 0;
        mPbuf_extra_size = 0;
    }

    if (mIOMMUEnabled) {
        mPmem_extra = new MemIon(SPRD_ION_DEV, extra_mem_size, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        mPmem_extra = new MemIon(SPRD_ION_DEV, extra_mem_size, MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }
    int fd = mPmem_extra->getHeapID();
    if(fd >= 0) {
        int ret;
        unsigned long phy_addr;
        size_t buffer_size;

        if (mIOMMUEnabled) {
            ret = (*mVP9DecGetIOVA)(mHandle, fd, &phy_addr, &buffer_size);
        } else {
            ret = mPmem_extra->get_phy_addr_from_ion(&phy_addr, &buffer_size);
        }
        if(ret < 0) {
            ALOGE ("mPmem_extra: get phy addr fail %d",ret);
            return -1;
        }

        mPbuf_extra_p = phy_addr;
        mPbuf_extra_size = buffer_size;
        mPbuf_extra_v = (uint8_t *)mPmem_extra->getBase();
        ALOGI("pmem 0x%lx - %p - %zd", mPbuf_extra_p, mPbuf_extra_v, mPbuf_extra_size);
        extra_mem[HW_NO_CACHABLE].common_buffer_ptr = mPbuf_extra_v;
        extra_mem[HW_NO_CACHABLE].common_buffer_ptr_phy = mPbuf_extra_p;
        extra_mem[HW_NO_CACHABLE].size = extra_mem_size;
    } else {
        ALOGE ("mPmem_extra: getHeapID fail %d", fd);
        return -1;
    }

    (*mVP9DecMemInit)(((SPRDVP9Decoder *)this)->mHandle, extra_mem);

    mHeadersDecoded = true;

    return 0;
}

bool SPRDVP9Decoder::openDecoder(const char* libName) {
    if(mLibHandle) {
        dlclose(mLibHandle);
    }

    ALOGI("openDecoder, lib: %s", libName);

    mLibHandle = dlopen(libName, RTLD_NOW);
    if(mLibHandle == NULL) {
        ALOGE("openDecoder, can't open lib: %s",libName);
        return false;
    }

    mVP9DecSetCurRecPic = (FT_VP9DecSetCurRecPic)dlsym(mLibHandle, "VP9DecSetCurRecPic");
    if(mVP9DecSetCurRecPic == NULL) {
        ALOGE("Can't find VP9DecSetCurRecPic in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVP9DecInit = (FT_VP9DecInit)dlsym(mLibHandle, "VP9DecInit");
    if(mVP9DecInit == NULL) {
        ALOGE("Can't find VP9DecInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVP9DecDecode = (FT_VP9DecDecode)dlsym(mLibHandle, "VP9DecDecode");
    if(mVP9DecDecode == NULL) {
        ALOGE("Can't find VP9DecDecode in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVP9DecRelease = (FT_VP9DecRelease)dlsym(mLibHandle, "VP9DecRelease");
    if(mVP9DecRelease == NULL) {
        ALOGE("Can't find VP9DecRelease in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVP9GetVideoDimensions = (FT_VP9GetVideoDimensions)dlsym(mLibHandle, "VP9GetVideoDimensions");
    if(mVP9GetVideoDimensions == NULL) {
        ALOGE("Can't find VP9GetVideoDimensions in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVP9GetBufferDimensions = (FT_VP9GetBufferDimensions)dlsym(mLibHandle, "VP9GetBufferDimensions");
    if(mVP9GetBufferDimensions == NULL) {
        ALOGE("Can't find VP9GetBufferDimensions in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVP9DecReleaseRefBuffers = (FT_VP9DecReleaseRefBuffers)dlsym(mLibHandle, "VP9DecReleaseRefBuffers");
    if(mVP9DecReleaseRefBuffers == NULL) {
        ALOGE("Can't find VP9DecReleaseRefBuffers in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVP9DecGetLastDspFrm = (FT_VP9DecGetLastDspFrm)dlsym(mLibHandle, "VP9DecGetLastDspFrm");
    if(mVP9DecGetLastDspFrm == NULL) {
        ALOGE("Can't find VP9DecGetLastDspFrm in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVP9GetCodecCapability = (FT_VP9GetCodecCapability)dlsym(mLibHandle, "VP9GetCodecCapability");
    if(mVP9GetCodecCapability == NULL) {
        ALOGE("Can't find VP9GetCodecCapability in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVP9DecGetIOVA = (FT_VP9Dec_get_iova)dlsym(mLibHandle, "VP9Dec_get_iova");
    if(mVP9DecGetIOVA == NULL) {
        ALOGE("Can't find VP9Dec_get_iova in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVP9DecFreeIOVA = (FT_VP9Dec_free_iova)dlsym(mLibHandle, "VP9Dec_free_iova");
    if(mVP9DecFreeIOVA == NULL) {
        ALOGE("Can't find VP9Dec_free_iova in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVP9DecGetIOMMUStatus = (FT_VP9Dec_get_IOMMU_status)dlsym(mLibHandle, "VP9Dec_get_IOMMU_status");
    if(mVP9DecGetIOMMUStatus == NULL) {
        ALOGE("Can't find VP9Dec_get_IOMMU_status in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVP9DecMemInit = (FT_VP9DecMemInit)dlsym(mLibHandle, "VP9DecMemInit");
    if(mVP9DecMemInit == NULL) {
        ALOGE("Can't find VP9DecMemInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    return true;
}

void SPRDVP9Decoder::freeOutputBufferIOVA() {
    PortInfo *port = editPortInfo(OMX_DirOutput);

    for (size_t i = 0; i < port->mBuffers.size(); ++i) {
        BufferInfo *buffer = &port->mBuffers.editItemAt(i);
        OMX_BUFFERHEADERTYPE *header = buffer->mHeader;

        if(header->pOutputPortPrivate != NULL) {
                BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct*)(header->pOutputPortPrivate);
                if (mIOMMUEnabled && pBufCtrl->bufferFd > 0) {
                    ALOGI("%s, fd: %d, iova: 0x%lx", __FUNCTION__, pBufCtrl->bufferFd, pBufCtrl->phyAddr);
                    (*mVP9DecFreeIOVA)(mHandle, pBufCtrl->phyAddr, pBufCtrl->bufferSize);
                    pBufCtrl->bufferFd = -1;
            }
        }
    }
}

}  // namespace android

android::SprdOMXComponent *createSprdOMXComponent(
    const char *name, const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    return new android::SPRDVP9Decoder(name, callbacks, appData, component);
}

