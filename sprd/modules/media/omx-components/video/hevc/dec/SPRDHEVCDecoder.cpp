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
#define LOG_TAG "SPRDHEVCDecoder"
#include <utils/Log.h>

#include "SPRDHEVCDecoder.h"
#include "SimpleSoftOMXComponent.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AUtils.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/IOMX.h>
#include "Rect.h"

#include <dlfcn.h>
#include <media/hardware/HardwareAPI.h>
#include <ui/GraphicBufferMapper.h>
#include "sprd_ion.h"

#include "gralloc_public.h"
#include "hevc_dec_api.h"

//#define VIDEODEC_CURRENT_OPT  /*only open for SAMSUNG currently*/


namespace android {

#define MAX_INSTANCES 20

static int instances = 0;

const static int64_t kConditionEventTimeOutNs = 3000000000LL;

static const CodecProfileLevel kProfileLevels[] = {
    { OMX_VIDEO_HEVCProfileMain, OMX_VIDEO_HEVCMainTierLevel1  },
    { OMX_VIDEO_HEVCProfileMain, OMX_VIDEO_HEVCMainTierLevel2  },
    { OMX_VIDEO_HEVCProfileMain, OMX_VIDEO_HEVCMainTierLevel21 },
    { OMX_VIDEO_HEVCProfileMain, OMX_VIDEO_HEVCMainTierLevel3  },
    { OMX_VIDEO_HEVCProfileMain, OMX_VIDEO_HEVCMainTierLevel31 },
    { OMX_VIDEO_HEVCProfileMain, OMX_VIDEO_HEVCMainTierLevel4  },
    { OMX_VIDEO_HEVCProfileMain, OMX_VIDEO_HEVCMainTierLevel41 },
    { OMX_VIDEO_HEVCProfileMain, OMX_VIDEO_HEVCMainTierLevel5  },
    { OMX_VIDEO_HEVCProfileMain, OMX_VIDEO_HEVCMainTierLevel51 },
};

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

#if 0
typedef struct LevelConversion {
    OMX_U32 omxLevel;
    AVCLevel avcLevel;
} LevelConcersion;

static LevelConversion ConversionTable[] = {
    { OMX_VIDEO_AVCLevel1,  AVC_LEVEL1_B },
    { OMX_VIDEO_AVCLevel1b, AVC_LEVEL1   },
    { OMX_VIDEO_AVCLevel11, AVC_LEVEL1_1 },
    { OMX_VIDEO_AVCLevel12, AVC_LEVEL1_2 },
    { OMX_VIDEO_AVCLevel13, AVC_LEVEL1_3 },
    { OMX_VIDEO_AVCLevel2,  AVC_LEVEL2 },
#if 1
    // encoding speed is very poor if video
    // resolution is higher than CIF
    { OMX_VIDEO_AVCLevel21, AVC_LEVEL2_1 },
    { OMX_VIDEO_AVCLevel22, AVC_LEVEL2_2 },
    { OMX_VIDEO_AVCLevel3,  AVC_LEVEL3   },
    { OMX_VIDEO_AVCLevel31, AVC_LEVEL3_1 },
    { OMX_VIDEO_AVCLevel32, AVC_LEVEL3_2 },
    { OMX_VIDEO_AVCLevel4,  AVC_LEVEL4   },
    { OMX_VIDEO_AVCLevel41, AVC_LEVEL4_1 },
    { OMX_VIDEO_AVCLevel42, AVC_LEVEL4_2 },
    { OMX_VIDEO_AVCLevel5,  AVC_LEVEL5   },
    { OMX_VIDEO_AVCLevel51, AVC_LEVEL5_1 },
#endif
};
#endif

OMX_ERRORTYPE SPRDHEVCDecoder::allocateStreamBuffer(OMX_U32 bufferSize)
{
    unsigned long phy_addr = 0;
    size_t size = 0;

    if (mDecoderSwFlag) {
        mPbuf_stream_v = (uint8_t*)malloc(bufferSize * sizeof(unsigned char));
        mPbuf_stream_p = 0;
        mPbuf_stream_size = bufferSize;
    } else {
        if (mIOMMUEnabled) {
            mPmem_stream = new MemIon(SPRD_ION_DEV, bufferSize, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
        } else {
            mPmem_stream = new MemIon(SPRD_ION_DEV, bufferSize, MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
        }

        int fd = mPmem_stream->getHeapID();
        if (fd < 0) {
            ALOGE("Failed to alloc bitstream pmem buffer, getHeapID failed");
            return OMX_ErrorInsufficientResources;
        } else {
            int ret;
            if (mIOMMUEnabled) {
                ret = (*mH265DecGetIOVA)(mHandle, fd, &phy_addr, &size);
            } else {
                ret = mPmem_stream->get_phy_addr_from_ion(&phy_addr, &size);
            }
            if (ret < 0) {
                ALOGE("Failed to alloc bitstream pmem buffer, get phy addr failed");
                return OMX_ErrorInsufficientResources;
            } else {
                mPbuf_stream_v = (uint8_t*)mPmem_stream->getBase();
                mPbuf_stream_p = phy_addr;
                mPbuf_stream_size = size;
            }
        }
    }

    ALOGI("%s, 0x%lx - %p - %zd", __FUNCTION__, mPbuf_stream_p,
            mPbuf_stream_v, mPbuf_stream_size);

    return OMX_ErrorNone;
}

void SPRDHEVCDecoder::releaseStreamBuffer()
{
    ALOGI("%s, 0x%lx - %p - %zd", __FUNCTION__,mPbuf_stream_p,
            mPbuf_stream_v, mPbuf_stream_size);

    if (mPbuf_stream_v != NULL) {
        if (mDecoderSwFlag) {
            free(mPbuf_stream_v);
            mPbuf_stream_v = NULL;
        } else {
            if (mIOMMUEnabled) {
                (*mH265DecFreeIOVA)(mHandle, mPbuf_stream_p, mPbuf_stream_size);
            }
            mPmem_stream.clear();
            mPbuf_stream_v = NULL;
            mPbuf_stream_p = 0;
            mPbuf_stream_size = 0;
        }
    }
}

bool SPRDHEVCDecoder::outputBuffersNotEnough(const H265SwDecInfo *info, OMX_U32 bufferCountMin,
                                   OMX_U32 bufferCountActual, OMX_BOOL useNativeBuffer) {
    if(useNativeBuffer) {
        if (info->numFrames + 1 > bufferCountMin) {
            return true;
        }
    } else {
        if (info->numFrames + 1 + 4 > bufferCountActual) {
            return true;
        }
    }

    return false;
}

SPRDHEVCDecoder::SPRDHEVCDecoder(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mHandle(new tagHEVCHandle),
      mInputBufferCount(0),
      mPicId(0),
      mSetFreqCount(0),
      mMinCompressionRatio(8),
      mFrameWidth(320),
      mFrameHeight(240),
      mStride(mFrameWidth),
      mSliceHeight(mFrameHeight),
      mPictureSize(mStride * mSliceHeight * 3 / 2),
      mCropWidth(mFrameWidth),
      mCropHeight(mFrameHeight),
      mGettingPortFormat(OMX_FALSE),
      mEOSStatus(INPUT_DATA_AVAILABLE),
      mOutputPortSettingsChange(NONE),
      mHeadersDecoded(false),
      mSignalledError(false),
      mDecoderSwFlag(false),
      mChangeToSwDec(false),
      mAllocateBuffers(false),
      mNeedIVOP(true),
      mIOMMUEnabled(false),
      mIOMMUID(-1),
      mStopDecode(false),
      mThumbnailMode(OMX_FALSE),
      mCodecInterBuffer(NULL),
      mCodecExtraBuffer(NULL),
      mPmem_stream(NULL),
      mPbuf_stream_v(NULL),
      mPbuf_stream_p(0),
      mPbuf_stream_size(0),
      mPmem_extra(NULL),
      mPbuf_extra_v(NULL),
      mPbuf_extra_p(0),
      mPbuf_extra_size(0),
      mPbuf_mbinfo_idx(0),
      mLibHandle(NULL),
      mH265DecInit(NULL),
      mH265DecGetInfo(NULL),
      mH265DecDecode(NULL),
      mH265DecRelease(NULL),
      mH265Dec_SetCurRecPic(NULL),
      mH265Dec_GetLastDspFrm(NULL),
      mH265Dec_ReleaseRefBuffers(NULL),
      mH265DecMemInit(NULL),
      mH265DecSetparam(NULL),
      mH265DecGetIOVA(NULL),
      mH265DecFreeIOVA(NULL),
      mH265DecGetIOMMUStatus(NULL) {

    ALOGI("Construct SPRDHEVCDecoder, this: %p, instances: %d", (void *)this, instances);

    mInitCheck = OMX_ErrorNone;

    if (!openDecoder("libomx_hevcdec_hw_sprd.so")) {
        mInitCheck = OMX_ErrorInsufficientResources;
    }

    if (initDecoder() != OMX_ErrorNone) {
        mInitCheck = OMX_ErrorInsufficientResources;
    }

    for (int i = 0; i < 17; i++) {
        mPmem_mbinfo[i] = NULL;
        mPbuf_mbinfo_v[i] = NULL;
        mPbuf_mbinfo_p[i] = 0;
        mPbuf_mbinfo_size[i] = 0;
    }

    initPorts();

    iUseAndroidNativeBuffer[OMX_DirInput] = OMX_FALSE;
    iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_FALSE;

    instances++;
    if (instances > MAX_INSTANCES) {
        ALOGE("instances(%d) are too much, return OMX_ErrorInsufficientResources", instances);
        mInitCheck = OMX_ErrorInsufficientResources;
    }
}

SPRDHEVCDecoder::~SPRDHEVCDecoder() {
    ALOGI("Destruct SPRDHEVCDecoder, this: %p, instances: %d", (void *)this, instances);

    releaseDecoder();

    delete mHandle;
    mHandle = NULL;

    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);
    List<BufferInfo *> &inQueue = getPortQueue(kInputPortIndex);
    CHECK(outQueue.empty());
    CHECK(inQueue.empty());

    instances--;
}

OMX_ERRORTYPE SPRDHEVCDecoder::initCheck() const{
    ALOGI("%s, mInitCheck: 0x%x", __FUNCTION__, mInitCheck);
    return mInitCheck;
}

void SPRDHEVCDecoder::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = kInputPortIndex;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumInputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = 4096*2160*3/2/8;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_HEVC);
    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mFrameWidth;
    def.format.video.nFrameHeight = mFrameHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingHEVC;
    def.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    def.format.video.pNativeWindow = NULL;

    addPort(def);

    def.nPortIndex = kOutputPortIndex;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumOutputBuffers;
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
    def.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    def.format.video.pNativeWindow = NULL;

    def.nBufferSize =
        (def.format.video.nFrameWidth * def.format.video.nFrameHeight * 3) / 2;

    addPort(def);
}

status_t SPRDHEVCDecoder::initDecoder() {

    memset(mHandle, 0, sizeof(tagHEVCHandle));

    mHandle->userdata = (void *)this;
    mHandle->VSP_bindCb = BindFrameWrapper;
    mHandle->VSP_unbindCb = UnbindFrameWrapper;
    mHandle->VSP_extMemCb = ExtMemAllocWrapper;
    mHandle->VSP_ctuinfoMemCb = CtuInfoMemAllocWrapper;

    uint32_t size_inter = H265_DECODER_INTERNAL_BUFFER_SIZE;
    mCodecInterBuffer = (uint8_t *)malloc(size_inter);
    CHECK(mCodecInterBuffer != NULL);

    MMCodecBuffer codec_buf;
    MMDecVideoFormat video_format;

    codec_buf.common_buffer_ptr = mCodecInterBuffer;
    codec_buf.common_buffer_ptr_phy = 0;
    codec_buf.size = size_inter;
    codec_buf.int_buffer_ptr = NULL;
    codec_buf.int_size = 0;

    video_format.video_std = H265;
    video_format.frame_width = 0;
    video_format.frame_height = 0;
    video_format.p_extra = NULL;
    video_format.p_extra_phy = 0;
    video_format.i_extra = 0;
    //video_format.uv_interleaved = 1;
    video_format.yuv_format = YUV420SP_NV12;

    if ((*mH265DecInit)(mHandle, &codec_buf,&video_format) != MMDEC_OK) {
        ALOGE("Failed to init HEVCDEC");
        return OMX_ErrorUndefined;
    }

    //int32 codec_capabilty;
    if ((*mH265GetCodecCapability)(mHandle, &mCapability) != MMDEC_OK) {
        ALOGE("Failed to mH265GetCodecCapability");
    }

    ALOGI("initDecoder, Capability: profile %d, level %d, max wh=%d %d",
          mCapability.profile, mCapability.level, mCapability.max_width, mCapability.max_height);

    int ret = (*mH265DecGetIOMMUStatus)(mHandle);
    ALOGI("Get IOMMU Status: %d(%s)", ret, strerror(errno));
    if (ret < 0) {
        mIOMMUEnabled = false;
    } else {
        mIOMMUEnabled = true;
    }

    return allocateStreamBuffer(H265_DECODER_STREAM_BUFFER_SIZE);
}

void SPRDHEVCDecoder::releaseDecoder() {
    releaseStreamBuffer();

    if (mPbuf_extra_v != NULL) {
        if (mIOMMUEnabled) {
            (*mH265DecFreeIOVA)(mHandle, mPbuf_extra_p, mPbuf_extra_size);
        }
        mPmem_extra.clear();
        mPbuf_extra_v = NULL;
        mPbuf_extra_p = 0;
        mPbuf_extra_size = 0;
    }

    for (int i = 0; i < 17; i++) {
        if (mPbuf_mbinfo_v[i]) {
            if (mIOMMUEnabled) {
                (*mH265DecFreeIOVA)(mHandle, mPbuf_mbinfo_p[i], mPbuf_mbinfo_size[i]);
            }
            mPmem_mbinfo[i].clear();
            mPbuf_mbinfo_v[i] = NULL;
            mPbuf_mbinfo_p[i] = 0;
            mPbuf_mbinfo_size[i] = 0;
        }
    }
    mPbuf_mbinfo_idx = 0;

    if (mH265DecRelease!=NULL)
        (*mH265DecRelease)(mHandle);

    if (mCodecInterBuffer != NULL) {
        free(mCodecInterBuffer);
        mCodecInterBuffer = NULL;
    }

    if (mCodecExtraBuffer != NULL) {
        free(mCodecExtraBuffer);
        mCodecExtraBuffer = NULL;
    }

    if(mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
        mH265Dec_ReleaseRefBuffers = NULL;
        mH265DecRelease = NULL;
    }
}

OMX_ERRORTYPE SPRDHEVCDecoder::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > kOutputPortIndex) {
            return OMX_ErrorUndefined;
        }

        if (formatParams->nIndex != 0) {
            return OMX_ErrorNoMore;
        }

        if (formatParams->nPortIndex == kInputPortIndex) {
            formatParams->eCompressionFormat = OMX_VIDEO_CodingHEVC;
            formatParams->eColorFormat = OMX_COLOR_FormatUnused;
            formatParams->xFramerate = 0;
        } else {
            CHECK(formatParams->nPortIndex == kOutputPortIndex);

            PortInfo *pOutPort = editPortInfo(kOutputPortIndex);
            ALOGI("internalGetParameter, OMX_IndexParamVideoPortFormat, eColorFormat: 0x%x",pOutPort->mDef.format.video.eColorFormat);
            formatParams->eCompressionFormat = OMX_VIDEO_CodingUnused;
            formatParams->eColorFormat = pOutPort->mDef.format.video.eColorFormat;
            formatParams->xFramerate = 0;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel =
            (OMX_VIDEO_PARAM_PROFILELEVELTYPE *) params;

        if (profileLevel->nPortIndex != kInputPortIndex) {
            ALOGE("Invalid port index: %d", profileLevel->nPortIndex);
            return OMX_ErrorUnsupportedIndex;
        }

        size_t index = profileLevel->nProfileIndex;
        size_t nProfileLevels =
            sizeof(kProfileLevels) / sizeof(kProfileLevels[0]);
        if (index >= nProfileLevels) {
            return OMX_ErrorNoMore;
        }

        profileLevel->eProfile = kProfileLevels[index].mProfile;
        profileLevel->eLevel = kProfileLevels[index].mLevel;

#if 0
        if (profileLevel->eProfile == OMX_VIDEO_AVCProfileHigh) {
            if (mCapability.profile < AVC_HIGH) {
                profileLevel->eProfile = OMX_VIDEO_AVCProfileMain;
            }
        }

        if (profileLevel->eProfile == OMX_VIDEO_AVCProfileMain) {
            if (mCapability.profile < AVC_MAIN) {
                profileLevel->eProfile = OMX_VIDEO_AVCProfileBaseline;
            }
        }

        const size_t size =
            sizeof(ConversionTable) / sizeof(ConversionTable[0]);

        for (index = 0; index < size; index++) {
            if (ConversionTable[index].avcLevel > mCapability.level) {
                index--;
                break;
            }
        }

        if (profileLevel->eLevel > ConversionTable[index].omxLevel) {
            profileLevel->eLevel = ConversionTable[index].omxLevel;
        }
#endif
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
        if(mDecoderSwFlag || mIOMMUEnabled) {
            pganbp->nUsage = GRALLOC_USAGE_SW_READ_OFTEN |GRALLOC_USAGE_SW_WRITE_OFTEN;
        } else {
            pganbp->nUsage = GRALLOC_USAGE_VIDEO_BUFFER | GRALLOC_USAGE_SW_READ_OFTEN |GRALLOC_USAGE_SW_WRITE_OFTEN;
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

OMX_ERRORTYPE SPRDHEVCDecoder::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamStandardComponentRole:
    {
        const OMX_PARAM_COMPONENTROLETYPE *roleParams =
            (const OMX_PARAM_COMPONENTROLETYPE *)params;

        if (strncmp((const char *)roleParams->cRole,
                    "video_decoder.hevc",
                    OMX_MAX_STRINGNAME_SIZE - 1)) {
            return OMX_ErrorUndefined;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > kOutputPortIndex) {
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
            /*FIXME: when NativeWindow is null, we can't use nBufferCountMin to calculate to
            * nBufferCountActual in Acodec.cpp&OMXCodec.cpp. So we need set nBufferCountActual
            * manually.
            * 4: reserved buffers by SurfaceFlinger(according to Acodec.cpp&OMXCodec.cpp)*/
            pOutPort->mDef.nBufferCountActual = pOutPort->mDef.nBufferCountMin + 4;
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

        if (defParams->nPortIndex > 1) {
            return OMX_ErrorBadPortIndex;
        }
        if (defParams->nSize != sizeof(OMX_PARAM_PORTDEFINITIONTYPE)) {
            return OMX_ErrorUnsupportedSetting;
        }

        PortInfo *port = editPortInfo(defParams->nPortIndex);

        // default behavior is that we only allow buffer size to increase
        if (defParams->nBufferSize > port->mDef.nBufferSize) {
            port->mDef.nBufferSize = defParams->nBufferSize;
        }

        if (defParams->nBufferCountActual < port->mDef.nBufferCountMin) {
            ALOGW("component requires at least %u buffers (%u requested)",
                    port->mDef.nBufferCountMin, defParams->nBufferCountActual);
            return OMX_ErrorUnsupportedSetting;
        }

        port->mDef.nBufferCountActual = defParams->nBufferCountActual;

        uint32_t oldWidth = port->mDef.format.video.nFrameWidth;
        uint32_t oldHeight = port->mDef.format.video.nFrameHeight;
        uint32_t newWidth = defParams->format.video.nFrameWidth;
        uint32_t newHeight = defParams->format.video.nFrameHeight;

        ALOGI("%s, port:%d, old wh:%d %d, new wh:%d %d", __FUNCTION__, defParams->nPortIndex,
              oldWidth, oldHeight, newWidth, newHeight);

        memcpy(&port->mDef.format.video, &defParams->format.video, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));

        if((oldWidth != newWidth || oldHeight != newHeight)) {
            if (defParams->nPortIndex == kOutputPortIndex) {
                mFrameWidth = newWidth;
                mFrameHeight = newHeight;
                mStride = ((newWidth + 31) & -32);
                mSliceHeight = ((newHeight + 31) & -32);
                mPictureSize = mStride* mSliceHeight * 3 / 2;

                ALOGI("%s, mFrameWidth %d, mFrameHeight %d, mStride %d, mSliceHeight %d", __FUNCTION__,
                      mFrameWidth, mFrameHeight, mStride, mSliceHeight);

                updatePortDefinitions(true, true);
            } else {
                port->mDef.format.video.nFrameWidth = newWidth;
                port->mDef.format.video.nFrameHeight = newHeight;
            }
        }

        PortInfo *inPort = editPortInfo(kInputPortIndex);
        if (inPort->mDef.nBufferSize > mPbuf_stream_size) {
            releaseStreamBuffer();
            allocateStreamBuffer(inPort->mDef.nBufferSize + 4);//additional 4 for startcode
        }

        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDHEVCDecoder::internalUseBuffer(
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

    if (portIndex == OMX_DirOutput) {
        (*header)->pOutputPortPrivate = new BufferCtrlStruct;
        CHECK((*header)->pOutputPortPrivate != NULL);
        BufferCtrlStruct* pBufCtrl= (BufferCtrlStruct*)((*header)->pOutputPortPrivate);
        pBufCtrl->iRefCount = 1; //init by1
        pBufCtrl->id = mIOMMUID;
        if (mAllocateBuffers) {
            if(bufferPrivate != NULL) {
                pBufCtrl->pMem = ((BufferPrivateStruct*)bufferPrivate)->pMem;
                pBufCtrl->bufferFd = ((BufferPrivateStruct*)bufferPrivate)->bufferFd;
                pBufCtrl->phyAddr = ((BufferPrivateStruct*)bufferPrivate)->phyAddr;
                pBufCtrl->bufferSize = ((BufferPrivateStruct*)bufferPrivate)->bufferSize;
            } else {
                pBufCtrl->pMem = NULL;
                pBufCtrl->bufferFd = 0;
                pBufCtrl->phyAddr = 0;
                pBufCtrl->bufferSize = 0;
            }
        } else {
                pBufCtrl->pMem = NULL;
                pBufCtrl->bufferFd = 0;
                pBufCtrl->phyAddr = 0;
                pBufCtrl->bufferSize = 0;
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

OMX_ERRORTYPE SPRDHEVCDecoder::allocateBuffer(
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
        if(mDecoderSwFlag  || mChangeToSwDec) {
            return SprdSimpleOMXComponent::allocateBuffer(header, portIndex, appPrivate, size);
        } else {
            MemIon* pMem = NULL;
            unsigned long phyAddr = 0;
            size_t bufferSize = 0;//don't use OMX_U32
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

            pBuffer = (OMX_U8 *)(pMem->getBase());
            BufferPrivateStruct* bufferPrivate = new BufferPrivateStruct();
            bufferPrivate->pMem = pMem;
            bufferPrivate->bufferFd = fd;
            bufferPrivate->phyAddr = 0;
            bufferPrivate->bufferSize = size64word;
            ALOGI("allocateBuffer, allocate buffer from pmem, pBuffer: %p", pBuffer);

            SprdSimpleOMXComponent::useBuffer(header, portIndex, appPrivate, (OMX_U32)size64word, pBuffer, bufferPrivate);
            delete bufferPrivate;

            return OMX_ErrorNone;
        }
    }

    default:
        return OMX_ErrorUnsupportedIndex;

    }
}

OMX_ERRORTYPE SPRDHEVCDecoder::freeBuffer(
    OMX_U32 portIndex,
    OMX_BUFFERHEADERTYPE *header) {
    switch (portIndex) {
    case OMX_DirInput:
        return SprdSimpleOMXComponent::freeBuffer(portIndex, header);

    case OMX_DirOutput:
    {
        BufferCtrlStruct* pBufCtrl= (BufferCtrlStruct*)(header->pOutputPortPrivate);
        if(pBufCtrl != NULL) {
            ALOGV("freeBuffer, phyAddr: 0x%lx", pBufCtrl->phyAddr);
            if (mIOMMUEnabled && pBufCtrl->phyAddr > 0) {
                (*mH265DecFreeIOVA)(mHandle, pBufCtrl->phyAddr, pBufCtrl->bufferSize);
            }
            if(pBufCtrl->pMem != NULL) {
                pBufCtrl->pMem.clear();
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

OMX_ERRORTYPE SPRDHEVCDecoder::getConfig(
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

OMX_ERRORTYPE SPRDHEVCDecoder::setConfig(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexConfigThumbnailMode:
    {
         OMX_BOOL *pEnable = (OMX_BOOL *)params;
         if (*pEnable == OMX_TRUE) {
              mThumbnailMode = OMX_TRUE;
         }

         ALOGI("setConfig, mThumbnailMode = %d", mThumbnailMode);

        return OMX_ErrorNone;
     }
    default:
        return SprdSimpleOMXComponent::setConfig(index, params);
    }
}

void SPRDHEVCDecoder::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return;
    }

    if (mEOSStatus == OUTPUT_FRAMES_FLUSHED) {
        return;
    }

    List<BufferInfo *> &inQueue = getPortQueue(kInputPortIndex);
    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);

    while (!mStopDecode && (mEOSStatus != INPUT_DATA_AVAILABLE || !inQueue.empty())
            && outQueue.size() != 0) {

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
                ALOGI("onQueueFilled, get outQueue buffer, return, count=%zd, queue_size=%d",count, outQueue.size());
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

//        ALOGI("%s, %d, mBuffer=0x%x, outHeader=0x%x, iRefCount=%d", __FUNCTION__, __LINE__, *itBuffer, outHeader, pBufCtrl->iRefCount);
        ALOGI("%s, %d, outHeader:%p, inHeader: %p, len: %d, nOffset: %d, time: %lld, EOS: %d",
              __FUNCTION__, __LINE__,outHeader,inHeader, inHeader->nFilledLen,inHeader->nOffset, inHeader->nTimeStamp,inHeader->nFlags & OMX_BUFFERFLAG_EOS);

        ++mPicId;
        if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
//bug253058 , the last frame size may be not zero, it need to be decoded.
//            inQueue.erase(inQueue.begin());
//           inInfo->mOwnedByUs = false;
//            notifyEmptyBufferDone(inHeader);
            mEOSStatus = INPUT_EOS_SEEN;
//            continue;
        }

        if(inHeader->nFilledLen == 0) {
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
            continue;
        }

        MMDecInput dec_in;
        MMDecOutput dec_out;

        uint8_t *bitstream = inHeader->pBuffer + inHeader->nOffset;
        uint32_t bufferSize = inHeader->nFilledLen;
        uint32_t copyLen = 0;
        uint32_t add_startcode_len = 0;

        dec_in.pStream = mPbuf_stream_v;
        dec_in.pStream_phy = mPbuf_stream_p;
        dec_in.beLastFrm = 0;
        dec_in.expected_IVOP = mNeedIVOP;
        dec_in.beDisplayed = 1;
        dec_in.err_pkt_num = 0;

        dec_out.frameEffective = 0;

        if (mAllocateBuffers)
            (pBufCtrl->pMem)->flush_ion_buffer((void*)(outHeader->pBuffer + outHeader->nOffset), (void*)(pBufCtrl->phyAddr), pBufCtrl->bufferSize);

        if(mThumbnailMode) {
            if((bitstream[0] != 0x0) || (bitstream[1] != 0x0) || (bitstream[2] != 0x0) || (bitstream[3] != 0x1)) {
                ALOGI("%s, %d, p[0]: %x, p[1]: %x, p[2]: %x, p[3]: %x", __FUNCTION__, __LINE__,
                        bitstream[0], bitstream[1], bitstream[2], bitstream[3]);

                ((uint8_t *) mPbuf_stream_v)[0] = 0x0;
                ((uint8_t *) mPbuf_stream_v)[1] = 0x0;
                ((uint8_t *) mPbuf_stream_v)[2] = 0x0;
                ((uint8_t *) mPbuf_stream_v)[3] = 0x1;

                add_startcode_len = 4;
            }

            if (bufferSize <= mPbuf_stream_size - add_startcode_len)
                copyLen = bufferSize;
            else
                copyLen = mPbuf_stream_size - add_startcode_len;
        } else {
            if (bufferSize <= mPbuf_stream_size)
                copyLen = bufferSize;
            else
                copyLen = mPbuf_stream_size;
        }

        if (mPbuf_stream_v != NULL) {
            memcpy(mPbuf_stream_v + add_startcode_len, bitstream, copyLen);
        }

        dec_in.dataLen = copyLen + add_startcode_len;
#if 0
        {
            ALOGI("%s, %d, mPbuf_stream_v[0x40...]: %x, %x, %x, %x, %x, %x, %x, %x",
                  __FUNCTION__, __LINE__, mPbuf_stream_v[0x40], mPbuf_stream_v[0x41], mPbuf_stream_v[0x42], mPbuf_stream_v[0x43],
                  mPbuf_stream_v[0x44], mPbuf_stream_v[0x45], mPbuf_stream_v[0x46], mPbuf_stream_v[0x47]);

            mPbuf_stream_v[0x43] = mPbuf_stream_v[0x44];
            mPbuf_stream_v[0x44] = mPbuf_stream_v[0x45];
            mPbuf_stream_v[0x45] = mPbuf_stream_v[0x46];
            mPbuf_stream_v[0x46] = mPbuf_stream_v[0x47];
            mPbuf_stream_v[0x47] = mPbuf_stream_v[0x48];
            mPbuf_stream_v[0x48] = mPbuf_stream_v[0x49];
            mPbuf_stream_v[0x49] = mPbuf_stream_v[0x50];

            ALOGI("%s, %d, mPbuf_stream_v[0x40...]: %x, %x, %x, %x, %x, %x, %x, %x",
                  __FUNCTION__, __LINE__, mPbuf_stream_v[0x40], mPbuf_stream_v[0x41], mPbuf_stream_v[0x42], mPbuf_stream_v[0x43],
                  mPbuf_stream_v[0x44], mPbuf_stream_v[0x45], mPbuf_stream_v[0x46], mPbuf_stream_v[0x47]);

            dec_in.dataLen -= 1;
        }
#endif
        ALOGI("%s, %d, dec_in.dataLen: %d, mPicId: %d", __FUNCTION__, __LINE__, dec_in.dataLen, mPicId);

        outHeader->nTimeStamp = inHeader->nTimeStamp;
        outHeader->nFlags = inHeader->nFlags;


        ALOGV("%s, %d, outHeader: %p, pBuffer: %p",__FUNCTION__, __LINE__, outHeader, outHeader->pBuffer);
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
                ALOGE("onQueueFilled, mapper.lock fail %p",outHeader->pBuffer);
                return ;
            }
            ALOGV("%s, %d, pBuffer: 0x%p, vaddr: %p", __FUNCTION__, __LINE__, outHeader->pBuffer,vaddr);
            uint8_t *yuv = (uint8_t *)((uint8_t *)vaddr + outHeader->nOffset);
            ALOGV("%s, %d, yuv: %p, mPicId: %d, outHeader: %p, outHeader->pBuffer: %p, outHeader->nTimeStamp: %lld",
                  __FUNCTION__, __LINE__, yuv, mPicId,outHeader, outHeader->pBuffer, outHeader->nTimeStamp);
            (*mH265Dec_SetCurRecPic)(mHandle, yuv, (void *)outHeader, mPicId);
        } else {
            uint8 *yuv = (uint8 *)(outHeader->pBuffer + outHeader->nOffset);
            (*mH265Dec_SetCurRecPic)(mHandle, yuv, (void *)outHeader, mPicId);
        }
//       dump_bs( mPbuf_stream_v, dec_in.dataLen);

        int64_t start_decode = systemTime();
        MMDecRet decRet = (*mH265DecDecode)(mHandle, &dec_in,&dec_out);
        int64_t end_decode = systemTime();
        ALOGI("%s, %d, decRet: %d, %dms, dec_out.frameEffective: %d, needIVOP: %d", __FUNCTION__, __LINE__, decRet, (unsigned int)((end_decode-start_decode) / 1000000L), dec_out.frameEffective, mNeedIVOP);

        if(iUseAndroidNativeBuffer[OMX_DirOutput]) {
            if(mapper.unlock((const native_handle_t*)outHeader->pBuffer)) {
                ALOGE("onQueueFilled, mapper.unlock fail %p",outHeader->pBuffer);
            }
        }

        if( decRet == MMDEC_OK) {
            mNeedIVOP = false;
        } else {
            mNeedIVOP = true;
            if (decRet == MMDEC_MEMORY_ERROR) {
                ALOGE("failed to allocate memory.");
                notify(OMX_EventError, OMX_ErrorInsufficientResources, 0, NULL);
                mSignalledError = true;
                return;
            } else if (decRet == MMDEC_NOT_SUPPORTED) {
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
#if 1
        H265SwDecInfo decoderInfo;
        MMDecRet ret;
        memset(&decoderInfo, 0, sizeof(H265SwDecInfo));
        ret = (*mH265DecGetInfo)(mHandle, &decoderInfo);
        if(ret == MMDEC_OK && decoderInfo.picWidth != 0) {
            if (!((decoderInfo.picWidth<= mCapability.max_width&& decoderInfo.picHeight<= mCapability.max_height)
                    || (decoderInfo.picWidth <= mCapability.max_height && decoderInfo.picHeight <= mCapability.max_width))) {
                ALOGE("[%d,%d] is out of range [%d, %d], failed to support this format.",
                      decoderInfo.picWidth, decoderInfo.picHeight, mCapability.max_width, mCapability.max_height);
                notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, NULL);
                mSignalledError = true;
                return;
            }

            if (handlePortSettingChangeEvent(&decoderInfo)) {
                return;
            }
        } else {
            ALOGE("failed to get decoder information.");
        }
#endif
        bufferSize = dec_in.dataLen;
        CHECK_LE(bufferSize, inHeader->nFilledLen);
        inHeader->nOffset += bufferSize;
        inHeader->nFilledLen -= bufferSize;

        if (inHeader->nFilledLen <= 0) {
            inHeader->nOffset = 0;
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
        }

        while (!outQueue.empty() &&
                mHeadersDecoded &&
                dec_out.frameEffective) {
            ALOGI("%s, %d, dec_out.pBufferHeader: %p, dec_out.mPicId: %d", __FUNCTION__, __LINE__, dec_out.pBufferHeader, dec_out.mPicId);
            int32_t picId = dec_out.mPicId;//decodedPicture.picId;
//            dump_yuv(dec_out.pOutFrameY, mPictureSize);
            drainOneOutputBuffer(picId, dec_out.pBufferHeader);
            dec_out.frameEffective = false;
            if(mThumbnailMode) {
                mStopDecode = true;
            }
        }
    }
}
bool SPRDHEVCDecoder::handlePortSettingChangeEvent(const H265SwDecInfo *info) {
    OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(kOutputPortIndex)->mDef;
    OMX_BOOL useNativeBuffer = iUseAndroidNativeBuffer[OMX_DirOutput];
    bool needFlushBuffer = outputBuffersNotEnough(info, def->nBufferCountMin, def->nBufferCountActual, useNativeBuffer);
    bool cropChanged = handleCropRectEvent(&info->cropParams);

    if ((mStride != info->picWidth) || (mSliceHeight != info->picHeight) || cropChanged || (!mThumbnailMode && needFlushBuffer)) {
        Mutex::Autolock autoLock(mLock);
        int32_t picId;
        void* pBufferHeader;

        while (MMDEC_OK == (*mH265Dec_GetLastDspFrm)(mHandle, &pBufferHeader, &picId)) {
            drainOneOutputBuffer(picId, pBufferHeader);
        }

        if (mGettingPortFormat == OMX_TRUE) {
            ALOGI("%s, waiting for get crop parameter done", __FUNCTION__);
            status_t err = mCondition.waitRelative(mLock, kConditionEventTimeOutNs);
            if (err != OK) {
                ALOGE("Timed out waiting for mCondition signal!");
            }
        }
        ALOGI("%s, %d, mStride: %d, mSliceHeight: %d, info->picWidth: %d, info->picHeight: %d, mGettingPortFormat:%d",
              __FUNCTION__, __LINE__,mStride, mSliceHeight, info->picWidth, info->picHeight, mGettingPortFormat);

        mFrameWidth = info->cropParams.cropOutWidth;
        mFrameHeight = info->cropParams.cropOutHeight;
        mStride  = info->picWidth;
        mSliceHeight = info->picHeight;
        mPictureSize = mStride * mSliceHeight * 3 / 2;

        if (!mThumbnailMode && needFlushBuffer) {
            if (useNativeBuffer) {
                ALOGI("%s, %d, info->numFrames: %d, def->nBufferCountMin: %d",
                      __FUNCTION__, __LINE__, info->numFrames, def->nBufferCountMin);

                /*FIXME:plus additional one buffer for avoiding timed out,
                *because the number of native window reserved buffer is not sure.*/
                def->nBufferCountMin = info->numFrames + 1 + 1;
            } else {
                ALOGI("%s, %d, info->numFrames: %d, def->nBufferCountActual: %d",
                      __FUNCTION__, __LINE__, info->numFrames, def->nBufferCountActual);

                /*FIXME: When NativeWindow is null, We need calc actual buffer count manually.
                * 1: avoiding timed out, 1:reconstructed frame, 4:reserved buffers by SurfaceFlinger.*/
                def->nBufferCountActual = info->numFrames + 1 + 1 + 4;

                /*fix Bug 375771 testCodecResetsH264WithSurface fail*/
                def->bPopulated = OMX_FALSE;
            }
        }

        updatePortDefinitions(true, true);
        (*mH265Dec_ReleaseRefBuffers)(mHandle);
        notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
        mOutputPortSettingsChange = AWAITING_DISABLED;
        return true;
    }

    return false;
}

bool SPRDHEVCDecoder::handleCropRectEvent(const CropParams *crop) {
    if (mCropWidth != crop->cropOutWidth ||
            mCropHeight != crop->cropOutHeight) {
        ALOGI("%s, crop w h: %d %d", __FUNCTION__, crop->cropOutWidth, crop->cropOutHeight);
        return true;
    }
    return false;
}

void SPRDHEVCDecoder::drainOneOutputBuffer(int32_t picId, void* pBufferHeader) {

    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);

    List<BufferInfo *>::iterator it = outQueue.begin();
    while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != outQueue.end()) {
        ++it;
    }
    CHECK((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)pBufferHeader);

    BufferInfo *outInfo = *it;
    OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;

    outHeader->nFilledLen = mPictureSize;

    ALOGI("%s, %d, outHeader: %p, outHeader->pBuffer: %p, outHeader->nOffset: %d, outHeader->nFlags: %d, outHeader->nTimeStamp: %lld",
          __FUNCTION__, __LINE__, outHeader , outHeader->pBuffer, outHeader->nOffset, outHeader->nFlags, outHeader->nTimeStamp);

//    LOGI("%s, %d, outHeader->nTimeStamp: %d, outHeader->nFlags: %d, mPictureSize: %d", __FUNCTION__, __LINE__, outHeader->nTimeStamp, outHeader->nFlags, mPictureSize);
//   LOGI("%s, %d, out: %0x", __FUNCTION__, __LINE__, outHeader->pBuffer + outHeader->nOffset);

//    dump_yuv(data, mPictureSize);
    outInfo->mOwnedByUs = false;
    outQueue.erase(it);
    outInfo = NULL;

    BufferCtrlStruct* pOutBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
    pOutBufCtrl->iRefCount++;
    notifyFillBufferDone(outHeader);
}

bool SPRDHEVCDecoder::drainAllOutputBuffers() {
    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);
    BufferInfo *outInfo;
    OMX_BUFFERHEADERTYPE *outHeader;

    int32_t picId;
    void* pBufferHeader;

    while (!outQueue.empty() && mEOSStatus != OUTPUT_FRAMES_FLUSHED) {

        if (mHeadersDecoded &&
                MMDEC_OK == (*mH265Dec_GetLastDspFrm)(mHandle, &pBufferHeader, &picId) ) {
            List<BufferInfo *>::iterator it = outQueue.begin();
            while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != outQueue.end()) {
                ++it;
            }
            CHECK((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)pBufferHeader);
            outInfo = *it;
            outQueue.erase(it);
            outHeader = outInfo->mHeader;
            outHeader->nFilledLen = mPictureSize;
        } else {
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

void SPRDHEVCDecoder::onPortFlushCompleted(OMX_U32 portIndex) {
    if (portIndex == kInputPortIndex) {
        mEOSStatus = INPUT_DATA_AVAILABLE;
        mNeedIVOP = true;
    }
}

void SPRDHEVCDecoder::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
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

void SPRDHEVCDecoder::onPortFlushPrepare(OMX_U32 portIndex) {
    if(portIndex == OMX_DirOutput) {
        if( NULL!=mH265Dec_ReleaseRefBuffers )
            (*mH265Dec_ReleaseRefBuffers)(mHandle);
    }
}

void SPRDHEVCDecoder::onReset() {
    mSignalledError = false;

    //avoid process error after stop codec and restart codec when port settings changing.
    mOutputPortSettingsChange = NONE;
}

void SPRDHEVCDecoder::updatePortDefinitions(bool updateCrop, bool updateInputSize) {
    OMX_PARAM_PORTDEFINITIONTYPE *outDef = &editPortInfo(kOutputPortIndex)->mDef;

    if (updateCrop) {
        mCropWidth = mFrameWidth;
        mCropHeight = mFrameHeight;
    }
    outDef->format.video.nFrameWidth = mStride;
    outDef->format.video.nFrameHeight = mSliceHeight;
    outDef->format.video.nStride = mStride;
    outDef->format.video.nSliceHeight = mSliceHeight;
    outDef->nBufferSize = mPictureSize;

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

    // when output format changes, input buffer size does not actually change
    if (updateInputSize) {
        inDef->nBufferSize = max(
                                 outDef->nBufferSize / mMinCompressionRatio,
                                 inDef->nBufferSize);
    }
}

// static
int32_t SPRDHEVCDecoder::ExtMemAllocWrapper(
    void* aUserData, unsigned int size_extra) {
    return static_cast<SPRDHEVCDecoder *>(aUserData)->VSP_malloc_cb(size_extra);
}

// static
int32_t SPRDHEVCDecoder::CtuInfoMemAllocWrapper(
    void* aUserData, unsigned int size_mbinfo, unsigned long *pPhyAddr) {
    return static_cast<SPRDHEVCDecoder *>(aUserData)->VSP_malloc_ctuinfo_cb(size_mbinfo, pPhyAddr);
}

// static
int32_t SPRDHEVCDecoder::BindFrameWrapper(void *aUserData, void *pHeader, unsigned long *pPhyAddr) {
    return static_cast<SPRDHEVCDecoder *>(aUserData)->VSP_bind_cb(pHeader, pPhyAddr);
}

// static
int32_t SPRDHEVCDecoder::UnbindFrameWrapper(void *aUserData, void *pHeader, unsigned long *pPhyAddr) {
    return static_cast<SPRDHEVCDecoder *>(aUserData)->VSP_unbind_cb(pHeader, pPhyAddr);
}

int SPRDHEVCDecoder::VSP_malloc_cb(unsigned int size_extra) {

    ALOGI("%s, %d, mDecoderSwFlag: %d, mPictureSize: %d, size_extra: %d", __FUNCTION__, __LINE__, mDecoderSwFlag, mPictureSize, size_extra);

    int32_t picId;
    void* pBufferHeader;

    /*fix Bug 381332 Whatsapp Recorded Video Getting Not Forward*/
    //while (MMDEC_OK == (*mH265Dec_GetLastDspFrm)(mHandle, &pBufferHeader, &picId)) {
    //    drainOneOutputBuffer(picId, pBufferHeader);
    //}

    MMCodecBuffer extra_mem[MAX_MEM_TYPE];

    if (mDecoderSwFlag) {
        if (mCodecExtraBuffer != NULL) {
            free(mCodecExtraBuffer);
            mCodecExtraBuffer = NULL;
        }
        mCodecExtraBuffer = (uint8_t *)malloc(size_extra);
        if (mCodecExtraBuffer == NULL) {
            return -1;
        }
        extra_mem[SW_CACHABLE].common_buffer_ptr = mCodecExtraBuffer;
        extra_mem[SW_CACHABLE].common_buffer_ptr_phy = 0;
        extra_mem[SW_CACHABLE].size = size_extra;
    } else {
        mPbuf_mbinfo_idx = 0;

        if (mPbuf_extra_v != NULL) {
            if (mIOMMUEnabled) {
                (*mH265DecFreeIOVA)(mHandle, mPbuf_extra_p, mPbuf_extra_size);
            }
            mPmem_extra.clear();
            mPbuf_extra_v = NULL;
            mPbuf_extra_p = 0;
            mPbuf_extra_size = 0;
        }

        if (mIOMMUEnabled) {
            mPmem_extra = new MemIon(SPRD_ION_DEV, size_extra, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
        } else {
            mPmem_extra = new MemIon(SPRD_ION_DEV, size_extra, MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
        }
        int fd = mPmem_extra->getHeapID();
        if(fd >= 0) {
            int ret;
            unsigned long phy_addr;
            size_t buffer_size;

            if (mIOMMUEnabled) {
                ret = (*mH265DecGetIOVA)(mHandle, fd, &phy_addr, &buffer_size);
            } else {
                ret = mPmem_extra->get_phy_addr_from_ion(&phy_addr, &buffer_size);
            }
            if(ret < 0) {
                ALOGE ("mPmem_extra: get phy addr fail %d(%s)", ret, strerror(errno));
                return -1;
            }

            mPbuf_extra_p =phy_addr;
            mPbuf_extra_size = buffer_size;
            mPbuf_extra_v = (uint8_t *)mPmem_extra->getBase();
            ALOGI("pmem 0x%lx - %p - %zd", mPbuf_extra_p, mPbuf_extra_v, mPbuf_extra_size);

            extra_mem[HW_NO_CACHABLE].common_buffer_ptr = mPbuf_extra_v;
            extra_mem[HW_NO_CACHABLE].common_buffer_ptr_phy = mPbuf_extra_p;
            extra_mem[HW_NO_CACHABLE].size = size_extra;
        } else {
            ALOGE ("mPmem_extra: getHeapID fail %d", fd);
            return -1;
        }
    }

    (*mH265DecMemInit)(((SPRDHEVCDecoder *)this)->mHandle, extra_mem);

    mHeadersDecoded = true;

    return 0;
}

int SPRDHEVCDecoder::VSP_malloc_ctuinfo_cb(unsigned int size_mbinfo, unsigned long *pPhyAddr) {

    int idx = mPbuf_mbinfo_idx;

    ALOGI("%s, %d, idx: %d, size_mbinfo: %d", __FUNCTION__, __LINE__, idx, size_mbinfo);

    if (mPbuf_mbinfo_v[idx] != NULL) {
        if (mIOMMUEnabled) {
            (*mH265DecFreeIOVA)(mHandle, mPbuf_mbinfo_p[idx], mPbuf_mbinfo_size[idx]);
        }
        mPmem_mbinfo[idx].clear();
        mPbuf_mbinfo_v[idx] = NULL;
        mPbuf_mbinfo_p[idx] = 0;
        mPbuf_mbinfo_size[idx] = 0;
    }

    if (mIOMMUEnabled) {
        mPmem_mbinfo[idx] = new MemIon(SPRD_ION_DEV, size_mbinfo, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        mPmem_mbinfo[idx] = new MemIon(SPRD_ION_DEV, size_mbinfo, MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }
    int fd = mPmem_mbinfo[idx]->getHeapID();
    if(fd >= 0) {
        int ret;
        unsigned long phy_addr;
        size_t buffer_size;

        if (mIOMMUEnabled) {
            ret = (*mH265DecGetIOVA)(mHandle, fd, &phy_addr, &buffer_size);
        } else {
            ret = mPmem_mbinfo[idx]->get_phy_addr_from_ion(&phy_addr, &buffer_size);
        }
        if(ret < 0) {
            ALOGE ("mPmem_mbinfo[%d]: get phy addr fail %d", idx, ret);
            return -1;
        }

        mPbuf_mbinfo_p[idx] =phy_addr;
        mPbuf_mbinfo_size[idx] = buffer_size;
        mPbuf_mbinfo_v[idx] = (uint8_t *)mPmem_mbinfo[idx]->getBase();
        ALOGI("pmem 0x%lx - %p - %zd", mPbuf_mbinfo_p[idx], mPbuf_mbinfo_v[idx], mPbuf_mbinfo_size[idx]);

        *pPhyAddr = phy_addr;
    } else {
        ALOGE ("mPmem_mbinfo[%d]: getHeapID fail %d", idx, fd);
        return -1;
    }

    mPbuf_mbinfo_idx++;

    return 0;
}

int SPRDHEVCDecoder::VSP_bind_cb(void *pHeader, unsigned long *pPhyAddr) {
    int fd;
    size_t bufferSize = 0;
    int ret = 0;

    BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

    if (mAllocateBuffers) {
        fd = pBufCtrl->bufferFd;
    } else {
        buffer_handle_t pNativeHandle = (buffer_handle_t)(((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer);
        fd = ADP_BUFFD(pNativeHandle);
        pBufCtrl->bufferFd = fd;
    }

    pBufCtrl->iRefCount++;

    if (mIOMMUEnabled) {
        ret = (*mH265DecGetIOVA)(mHandle, fd, pPhyAddr, &bufferSize);
    } else {
        ret = MemIon::Get_phy_addr_from_ion(fd, pPhyAddr, &bufferSize);
    }

    if (ret < 0) {
        ALOGE("%s, failed to map buffer, get phy addr failed", __FUNCTION__);
        return -1;
    }

    pBufCtrl->phyAddr = *pPhyAddr;
    pBufCtrl->bufferSize = bufferSize;

    ALOGI("VSP_bind_cb, pBuffer: %p, pHeader: %p; phyAddr=0x%lx, iRefCount=%d",
          ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader, *pPhyAddr, pBufCtrl->iRefCount);

    return 0;
}

int SPRDHEVCDecoder::VSP_unbind_cb(void *pHeader, unsigned long *pPhyAddr) {
    BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

    ALOGI("VSP_unbind_cb, pBuffer: %p, pHeader: %p; phyAddr=0x%lx, iRefCount=%d",
          ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader, pBufCtrl->phyAddr, pBufCtrl->iRefCount);

    if (pBufCtrl->iRefCount  > 0) {
        pBufCtrl->iRefCount--;

        if (mIOMMUEnabled) {
            (*mH265DecFreeIOVA)(mHandle, pBufCtrl->phyAddr, pBufCtrl->bufferSize);
        }

        pBufCtrl->phyAddr = 0;
        pBufCtrl->bufferSize = 0;
    }

    return 0;
}

OMX_ERRORTYPE SPRDHEVCDecoder::getExtensionIndex(
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
    }  else if (strcmp(name, SPRD_INDEX_CONFIG_THUMBNAIL_MODE) == 0) {
        ALOGI("getExtensionIndex:%s",SPRD_INDEX_CONFIG_THUMBNAIL_MODE);
        *index = OMX_IndexConfigThumbnailMode;
        return OMX_ErrorNone;
    }

    return OMX_ErrorNotImplemented;
}

bool SPRDHEVCDecoder::openDecoder(const char* libName) {
    if(mLibHandle) {
        dlclose(mLibHandle);
    }

    ALOGI("openDecoder, lib: %s", libName);

    mLibHandle = dlopen(libName, RTLD_NOW);
    if(mLibHandle == NULL) {
        ALOGE("openDecoder, can't open lib: %s",libName);
        return false;
    }

    mH265DecInit = (FT_H265DecInit)dlsym(mLibHandle, "H265DecInit");
    if(mH265DecInit == NULL) {
        ALOGE("Can't find H265DecInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265DecGetInfo = (FT_H265DecGetInfo)dlsym(mLibHandle, "H265DecGetInfo");
    if(mH265DecGetInfo == NULL) {
        ALOGE("Can't find H265DecGetInfo in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265DecDecode = (FT_H265DecDecode)dlsym(mLibHandle, "H265DecDecode");
    if(mH265DecDecode == NULL) {
        ALOGE("Can't find H265DecDecode in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265DecRelease = (FT_H265DecRelease)dlsym(mLibHandle, "H265DecRelease");
    if(mH265DecRelease == NULL) {
        ALOGE("Can't find H265DecRelease in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265Dec_SetCurRecPic = (FT_H265Dec_SetCurRecPic)dlsym(mLibHandle, "H265Dec_SetCurRecPic");
    if(mH265Dec_SetCurRecPic == NULL) {
        ALOGE("Can't find H265Dec_SetCurRecPic in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265Dec_GetLastDspFrm = (FT_H265Dec_GetLastDspFrm)dlsym(mLibHandle, "H265Dec_GetLastDspFrm");
    if(mH265Dec_GetLastDspFrm == NULL) {
        ALOGE("Can't find H265Dec_GetLastDspFrm in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265Dec_ReleaseRefBuffers = (FT_H265Dec_ReleaseRefBuffers)dlsym(mLibHandle, "H265Dec_ReleaseRefBuffers");
    if(mH265Dec_ReleaseRefBuffers == NULL) {
        ALOGE("Can't find H265Dec_ReleaseRefBuffers in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265DecMemInit = (FT_H265DecMemInit)dlsym(mLibHandle, "H265DecMemInit");
    if(mH265DecMemInit == NULL) {
        ALOGE("Can't find H265DecMemInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265GetCodecCapability = (FT_H265GetCodecCapability)dlsym(mLibHandle, "H265GetCodecCapability");
    if(mH265GetCodecCapability == NULL) {
        ALOGE("Can't find H265GetCodecCapability in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265DecSetparam = (FT_H265DecSetparam)dlsym(mLibHandle, "H265DecSetParameter");
    if(mH265DecSetparam == NULL) {
        ALOGE("Can't find H265DecSetParameter in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265DecGetIOVA = (FT_H265Dec_get_iova)dlsym(mLibHandle, "H265Dec_get_iova");
    if(mH265DecGetIOVA == NULL) {
        ALOGE("Can't find H265Dec_get_iova in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265DecFreeIOVA = (FT_H265Dec_free_iova)dlsym(mLibHandle, "H265Dec_free_iova");
    if(mH265DecFreeIOVA == NULL) {
        ALOGE("Can't find H265Dec_free_iova in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265DecGetIOMMUStatus = (FT_H265Dec_get_IOMMU_status)dlsym(mLibHandle, "H265Dec_get_IOMMU_status");
    if(mH265DecGetIOMMUStatus == NULL) {
        ALOGE("Can't find H265Dec_get_IOMMU_status in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    return true;
}

}  // namespace android

android::SprdOMXComponent *createSprdOMXComponent(
    const char *name, const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    return new android::SPRDHEVCDecoder(name, callbacks, appData, component);
}
