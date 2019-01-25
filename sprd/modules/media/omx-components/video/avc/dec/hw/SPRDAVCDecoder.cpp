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
#define LOG_TAG "SPRDAVCDecoder"
#include <utils/Log.h>

#include "SPRDAVCDecoder.h"
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
#include <sys/prctl.h>
#include <cutils/properties.h>
#include "avc_utils_sprd.h"

#include "gralloc_public.h"
#include "sprd_ion.h"
#include "avc_dec_api.h"

namespace android {

#define MAX_INSTANCES 20

static int instances = 0;

const static int64_t kConditionEventTimeOutNs = 3000000000LL;

static const CodecProfileLevel kProfileLevels[] = {
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel41 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel42 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel5  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51 },

    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel32 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel4  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel41 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel42 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel5  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51 },

    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel32 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel4  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel41 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel42 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel5  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel51 },
};

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

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
};

SPRDAVCDecoder::SPRDAVCDecoder(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mHandle(new tagAVCHandle),
      mInputBufferCount(0),
      mPicId(0),
      mSetFreqCount(0),
      mMinCompressionRatio(2),
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
      mDumpYUVEnabled(false),
      mDumpStrmEnabled(false),
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
      mDecoderSawSPS(false),
      mDecoderSawPPS(false),
      mSPSData(NULL),
      mSPSDataSize(0),
      mPPSData(NULL),
      mPPSDataSize(0),
      mIsResume(false),
      mSecureFlag(false),
      mAllocInput(false),
      mDeintl(NULL),
      mDeintInitialized(false),
      mNeedDeinterlace(false),
      mLibHandle(NULL),
      mH264DecInit(NULL),
      mH264DecGetInfo(NULL),
      mH264DecDecode(NULL),
      mH264DecRelease(NULL),
      mH264Dec_SetCurRecPic(NULL),
      mH264Dec_GetLastDspFrm(NULL),
      mH264Dec_ReleaseRefBuffers(NULL),
      mH264DecMemInit(NULL),
      mH264GetCodecCapability(NULL),
      mH264DecGetNALType(NULL),
      mH264DecSetparam(NULL),
      mH264DecGetIOVA(NULL),
      mH264DecFreeIOVA(NULL),
      mH264DecGetIOMMUStatus(NULL),
      mFrameDecoded(false),
      mIsInterlacedSequence(false) {

    ALOGI("Construct SPRDAVCDecoder, this: %p, instances: %d", (void *)this, instances);

    mInitCheck = OMX_ErrorNone;

    size_t nameLen = strlen(name);
    size_t suffixLen = strlen(".secure");

    if (!strcmp(name + nameLen - suffixLen, ".secure")) {
        mSecureFlag = true;
    }

    //read config flag
#define USE_SW_DECODER 0x01
#define USE_HW_DECODER 0x00

    uint8_t video_cfg = USE_HW_DECODER;
    FILE *fp = fopen("/data/data/com.sprd.test.videoplayer/app_decode/flag", "rb");
    if (fp != NULL) {
        fread(&video_cfg, sizeof(uint8_t), 1, fp);
        fclose(fp);
    }
    ALOGI("%s, video_cfg: %d, SecureFlag: %d", __FUNCTION__, video_cfg, mSecureFlag);

    bool ret = false;
    if (USE_HW_DECODER == video_cfg) {
        ret = openDecoder("libomx_avcdec_hw_sprd.so");
    }

    if(ret == false) {
        ret = openDecoder("libomx_avcdec_sw_sprd.so");
        mDecoderSwFlag = true;
    }

    CHECK_EQ(ret, true);

    char value_dump[PROPERTY_VALUE_MAX];

    property_get("h264dec.yuv.dump", value_dump, "false");
    mDumpYUVEnabled = !strcmp(value_dump, "true");

    property_get("h264dec.strm.dump", value_dump, "false");
    mDumpStrmEnabled = !strcmp(value_dump, "true");
    ALOGI("%s, mDumpYUVEnabled: %d, mDumpStrmEnabled: %d", __FUNCTION__, mDumpYUVEnabled, mDumpStrmEnabled);

    if(mDecoderSwFlag) {
        CHECK_EQ(initDecoder(), (status_t)OK);
    } else {
        if (initDecoder() != OK) {
            if (openDecoder("libomx_avcdec_sw_sprd.so")) {
                mDecoderSwFlag = true;
                if(initDecoder() != OK) {
                    mInitCheck = OMX_ErrorInsufficientResources;
                }
            } else {
                mInitCheck = OMX_ErrorInsufficientResources;
            }
        }
    }

    mSPSData = (uint8_t *)malloc(H264_HEADER_SIZE);
    mPPSData = (uint8_t *)malloc(H264_HEADER_SIZE);
    if (mSPSData == NULL || mPPSData == NULL) {
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

SPRDAVCDecoder::~SPRDAVCDecoder() {
    ALOGI("Destruct SPRDAVCDecoder, this: %p, instances: %d", (void *)this, instances);


    if (mDeintl) {

        CHECK(mDeinterInputBufQueue.empty());

        freeDecOutputBuffer();
        mDeintl->mDone = true;
        mDeintl->mDeinterReadyCondition.signal();

        delete mDeintl;
        mDeintl = NULL;
    }

    releaseDecoder();

    if (mSPSData != NULL) {
        free(mSPSData);
        mSPSData = NULL;
    }

    if (mPPSData != NULL) {
        free(mPPSData);
        mPPSData = NULL;
    }

    delete mHandle;
    mHandle = NULL;

    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);
    List<BufferInfo *> &inQueue = getPortQueue(kInputPortIndex);
    CHECK(outQueue.empty());
    CHECK(inQueue.empty());

    instances--;
}

OMX_ERRORTYPE SPRDAVCDecoder::initCheck() const{
    ALOGI("%s, mInitCheck: 0x%x", __FUNCTION__, mInitCheck);
    return mInitCheck;
}

OMX_ERRORTYPE SPRDAVCDecoder::allocateStreamBuffer(OMX_U32 bufferSize)
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
            unsigned int memory_type = mSecureFlag ? ION_HEAP_ID_MASK_MM | ION_FLAG_SECURE : ION_HEAP_ID_MASK_MM;
            mPmem_stream = new MemIon(SPRD_ION_DEV, bufferSize, MemIon::NO_CACHING, memory_type);
        }
        int fd = mPmem_stream->getHeapID();
        if (fd < 0) {
            ALOGE("Failed to alloc bitstream pmem buffer, getHeapID failed");
            return OMX_ErrorInsufficientResources;
        } else {
            int ret;
            if (mIOMMUEnabled) {
                ret = (*mH264DecGetIOVA)(mHandle, fd, &phy_addr, &size);
            } else {
                ret = mPmem_stream->get_phy_addr_from_ion(&phy_addr, &size);
            }
            if (ret < 0) {
                ALOGE("Failed to alloc bitstream pmem buffer, get phy addr failed: %d(%s)", ret, strerror(errno));
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

void SPRDAVCDecoder::releaseStreamBuffer()
{
    ALOGI("%s, 0x%lx - %p - %zd", __FUNCTION__,mPbuf_stream_p,
            mPbuf_stream_v, mPbuf_stream_size);

    if (mPbuf_stream_v != NULL) {
        if (mDecoderSwFlag) {
            free(mPbuf_stream_v);
            mPbuf_stream_v = NULL;
        } else {
            if (mIOMMUEnabled) {
                (*mH264DecFreeIOVA)(mHandle, mPbuf_stream_p, mPbuf_stream_size);
            }
            mPmem_stream.clear();
            mPbuf_stream_v = NULL;
            mPbuf_stream_p = 0;
            mPbuf_stream_size = 0;
        }
    }
}

bool SPRDAVCDecoder::outputBuffersNotEnough(const H264SwDecInfo *info, OMX_U32 bufferCountMin,
        OMX_U32 bufferCountActual, OMX_BOOL useNativeBuffer) {
    if (mNeedDeinterlace && !mIsInterlacedSequence) {
        return false;
    }

    if(useNativeBuffer) {
        if (info->numRefFrames + info->has_b_frames + 1 > bufferCountMin) {
            return true;
        }
    } else {
        if (info->numRefFrames + info->has_b_frames + 1 + 4 > bufferCountActual) {
            return true;
        }
    }

    return false;
}

void SPRDAVCDecoder::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = kInputPortIndex;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumInputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = 1920*1088*3/2/2;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_AVC);
    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mFrameWidth;
    def.format.video.nFrameHeight = mFrameHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
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

status_t SPRDAVCDecoder::initDecoder() {

    memset(mHandle, 0, sizeof(tagAVCHandle));

    mHandle->userdata = (void *)this;
    mHandle->VSP_bindCb = BindFrameWrapper;
    mHandle->VSP_unbindCb = UnbindFrameWrapper;
    mHandle->VSP_extMemCb = ExtMemAllocWrapper;
    mHandle->VSP_mbinfoMemCb = MbinfoMemAllocWrapper;
    mHandle->VSP_clearDpbCb = ClearDpbWrapper;

    uint32_t size_inter = H264_DECODER_INTERNAL_BUFFER_SIZE;
    mCodecInterBuffer = (uint8_t *)malloc(size_inter);
    CHECK(mCodecInterBuffer != NULL);

    MMCodecBuffer codec_buf;
    MMDecVideoFormat video_format;

    codec_buf.common_buffer_ptr = mCodecInterBuffer;
    codec_buf.common_buffer_ptr_phy = 0;
    codec_buf.size = size_inter;
    codec_buf.int_buffer_ptr = NULL;
    codec_buf.int_size = 0;

    video_format.video_std = H264;
    video_format.frame_width = 0;
    video_format.frame_height = 0;
    video_format.p_extra = NULL;
    video_format.p_extra_phy = 0;
    video_format.i_extra = 0;
    video_format.yuv_format = YUV420SP_NV12;

    if ((*mH264DecInit)(mHandle, &codec_buf,&video_format) != MMDEC_OK) {
        ALOGE("Failed to init AVCDEC");
        return OMX_ErrorUndefined;
    }

    //int32 codec_capabilty;
    if ((*mH264GetCodecCapability)(mHandle, &mCapability) != MMDEC_OK) {
        ALOGE("Failed to mH264GetCodecCapability");
    }

    ALOGI("initDecoder, Capability: profile %d, level %d, max wh=%d %d",
          mCapability.profile, mCapability.level, mCapability.max_width, mCapability.max_height);

    int ret = (*mH264DecGetIOMMUStatus)(mHandle);
    ALOGI("Get IOMMU Status: %d", ret);
    if (ret < 0) {
        mIOMMUEnabled = false;
    } else {
        mIOMMUEnabled = mSecureFlag ? false : true;
    }

    return allocateStreamBuffer(H264_DECODER_STREAM_BUFFER_SIZE);
}

void SPRDAVCDecoder::releaseDecoder() {
    releaseStreamBuffer();

    if (mPbuf_extra_v != NULL) {
        if (mIOMMUEnabled) {
            (*mH264DecFreeIOVA)(mHandle, mPbuf_extra_p, mPbuf_extra_size);
        }
        mPmem_extra.clear();
        mPbuf_extra_v = NULL;
        mPbuf_extra_p = 0;
        mPbuf_extra_size = 0;
    }

    for (int i = 0; i < 17; i++) {
        if (mPbuf_mbinfo_v[i]) {
            if (mIOMMUEnabled) {
                (*mH264DecFreeIOVA)(mHandle, mPbuf_mbinfo_p[i], mPbuf_mbinfo_size[i]);
            }
            mPmem_mbinfo[i].clear();
            mPbuf_mbinfo_v[i] = NULL;
            mPbuf_mbinfo_p[i] = 0;
            mPbuf_mbinfo_size[i] = 0;
        }
    }
    mPbuf_mbinfo_idx = 0;

    if( mH264DecRelease!=NULL )
        (*mH264DecRelease)(mHandle);

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
        mH264Dec_ReleaseRefBuffers = NULL;
        mH264DecRelease = NULL;
    }
}

OMX_ERRORTYPE SPRDAVCDecoder::internalGetParameter(
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
            formatParams->eCompressionFormat = OMX_VIDEO_CodingAVC;
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

        for (index = 1; index < (size-1); index++) {
            if (ConversionTable[index].avcLevel > mCapability.level) {
                index--;
                break;
            }
        }

        if (profileLevel->eLevel > ConversionTable[index].omxLevel) {
            profileLevel->eLevel = ConversionTable[index].omxLevel;
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
        if (mDecoderSwFlag)
            pganbp->nUsage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
        else if (mIOMMUEnabled)
            pganbp->nUsage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_NEVER;
        else
            pganbp->nUsage = GRALLOC_USAGE_VIDEO_BUFFER | GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_NEVER;

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

OMX_ERRORTYPE SPRDAVCDecoder::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamStandardComponentRole:
    {
        const OMX_PARAM_COMPONENTROLETYPE *roleParams =
            (const OMX_PARAM_COMPONENTROLETYPE *)params;

        if (strncmp((const char *)roleParams->cRole,
                    "video_decoder.avc",
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
            //pOutPort->mDef.nBufferCountActual = pOutPort->mDef.nBufferCountMin + 4;
        } else {
            ALOGI("internalSetParameter, enable AndroidNativeBuffer");
            iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_TRUE;

            pOutPort->mDef.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCbCr_420_SP;
        }
        return OMX_ErrorNone;
    }

    case OMX_IndexParamAllocNativeHandle:
    {
        EnableAndroidNativeBuffersParams *peanbp = (EnableAndroidNativeBuffersParams *)params;
        PortInfo *pOutPort = editPortInfo(kInputPortIndex);
        if (peanbp->enable == OMX_FALSE) {
            ALOGI("internalSetParameter, disable AllocNativeHandle");
            iUseAndroidNativeBuffer[OMX_DirInput] = OMX_FALSE;
        } else {
            ALOGI("internalSetParameter, enable AllocNativeHandle");
            iUseAndroidNativeBuffer[OMX_DirInput] = OMX_TRUE;
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
                mStride = ((newWidth + 15) & -16);
                mSliceHeight = ((newHeight + 15) & -16);
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

OMX_ERRORTYPE SPRDAVCDecoder::internalUseBuffer(
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
        if(mAllocateBuffers) {
            if(bufferPrivate != NULL) {
                pBufCtrl->pMem = ((BufferPrivateStruct*)bufferPrivate)->pMem;
                pBufCtrl->phyAddr = ((BufferPrivateStruct*)bufferPrivate)->phyAddr;
                pBufCtrl->bufferSize = ((BufferPrivateStruct*)bufferPrivate)->bufferSize;
                pBufCtrl->bufferFd = ((BufferPrivateStruct*)bufferPrivate)->bufferFd;
            } else {
                pBufCtrl->pMem = NULL;
                pBufCtrl->phyAddr = 0;
                pBufCtrl->bufferSize = 0;
                pBufCtrl->bufferFd = 0;
            }
        } else {
            if (mIOMMUEnabled) {
                unsigned long picPhyAddr = 0;
                size_t bufferSize = 0;
                buffer_handle_t pNativeHandle = (buffer_handle_t)((*header)->pBuffer);
                int fd = ADP_BUFFD(pNativeHandle);

                (*mH264DecGetIOVA)(mHandle, fd, &picPhyAddr, &bufferSize);

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
    } else if (portIndex == OMX_DirInput) {
        if (mSecureFlag || mAllocInput) {
            (*header)->pInputPortPrivate = new BufferCtrlStruct;
            CHECK((*header)->pInputPortPrivate != NULL);
            BufferCtrlStruct* pBufCtrl= (BufferCtrlStruct*)((*header)->pInputPortPrivate);
            if(bufferPrivate != NULL) {
                pBufCtrl->pMem = ((BufferPrivateStruct*)bufferPrivate)->pMem;
                pBufCtrl->phyAddr = ((BufferPrivateStruct*)bufferPrivate)->phyAddr;
                pBufCtrl->bufferSize = ((BufferPrivateStruct*)bufferPrivate)->bufferSize;
                pBufCtrl->bufferFd = 0;
            } else {
                pBufCtrl->pMem = NULL;
                pBufCtrl->phyAddr = 0;
                pBufCtrl->bufferSize = 0;
                pBufCtrl->bufferFd = 0;
            }
        }
    }

    PortInfo *port = editPortInfo(portIndex);

    port->mBuffers.push();

    BufferInfo *buffer =
        &port->mBuffers.editItemAt(port->mBuffers.size() - 1);
    ALOGI("internalUseBuffer, portIndex= %d, header=%p, pBuffer=%p, size=%d", portIndex, *header, ptr, size);
    buffer->mHeader = *header;
    buffer->mOwnedByUs = false;

    if (port->mBuffers.size() == port->mDef.nBufferCountActual) {
        port->mDef.bPopulated = OMX_TRUE;
        checkTransitions();
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoder::allocateBuffer(
    OMX_BUFFERHEADERTYPE **header,
    OMX_U32 portIndex,
    OMX_PTR appPrivate,
    OMX_U32 size) {
    switch (portIndex) {
    case OMX_DirInput:
    {
        if (mSecureFlag || mAllocInput) {
            MemIon* pMem = NULL;
            unsigned long phyAddr = 0;
            size_t bufferSize = 0;
            void *pBuffer = NULL;
            size_t size64word = (size + 1024*4 - 1) & ~(1024*4 - 1);
            int fd;

            if (mAllocInput)
                pMem = new MemIon(SPRD_ION_DEV, size64word, MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
            else
                pMem = new MemIon(SPRD_ION_DEV, size64word, MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM | ION_FLAG_SECURE);
            fd = pMem->getHeapID();
            if(fd < 0) {
                ALOGE("%s, Failed to alloc input pmem buffer", __FUNCTION__);
                return OMX_ErrorInsufficientResources;
            }

            if(pMem->get_phy_addr_from_ion(&phyAddr, &bufferSize)) {
                ALOGE("%s, input get_phy_addr_from_ion fail", __FUNCTION__);
                return OMX_ErrorInsufficientResources;
            }

            pBuffer = pMem->getBase();
            BufferPrivateStruct* bufferPrivate = new BufferPrivateStruct();
            bufferPrivate->pMem = pMem;
            bufferPrivate->phyAddr = phyAddr;
            bufferPrivate->bufferSize = bufferSize;

            ALOGI("%s, allocate input buffer from MemIon, virAddr: %p, phyAddr: 0x%lx, size: %zd",
                    __FUNCTION__, pBuffer, phyAddr, bufferSize);

            if (mAllocInput)
                SprdSimpleOMXComponent::useBuffer(header, portIndex, appPrivate, (OMX_U32)bufferSize,(OMX_U8 *)pBuffer, bufferPrivate);
            else
                SprdSimpleOMXComponent::useBuffer(header, portIndex, appPrivate, (OMX_U32)bufferSize, (OMX_U8 *)phyAddr, bufferPrivate);

            delete bufferPrivate;
            return OMX_ErrorNone;
        } else {
            return SprdSimpleOMXComponent::allocateBuffer(header, portIndex, appPrivate, size);
        }
    }

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
                pMem = new MemIon(SPRD_ION_DEV, size64word, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
            } else {
                unsigned int memory_type = mSecureFlag ? ION_HEAP_ID_MASK_MM | ION_FLAG_SECURE : ION_HEAP_ID_MASK_MM;
                pMem = new MemIon(SPRD_ION_DEV, size64word, MemIon::NO_CACHING, memory_type);
            }

            fd = pMem->getHeapID();
            if(fd < 0) {
                ALOGE("Failed to alloc outport pmem buffer");
                return OMX_ErrorInsufficientResources;
            }

            if (mIOMMUEnabled) {
                ret = (*mH264DecGetIOVA)(mHandle, fd, &phyAddr, &bufferSize);
                if(ret) {
                    ALOGE("H264DecGetIOVA Failed: %d(%s)", ret, strerror(errno));
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
            bufferPrivate->bufferFd = fd;
            ALOGI("allocateBuffer, allocate buffer from pmem, pBuffer: %p, phyAddr: 0x%lx, size: %zd", pBuffer, phyAddr, bufferSize);

            SprdSimpleOMXComponent::useBuffer(header, portIndex, appPrivate, (OMX_U32)bufferSize, pBuffer, bufferPrivate);
            delete bufferPrivate;

            return OMX_ErrorNone;
        }
    }

    default:
        return OMX_ErrorUnsupportedIndex;

    }
}

OMX_ERRORTYPE SPRDAVCDecoder::freeBuffer(
    OMX_U32 portIndex,
    OMX_BUFFERHEADERTYPE *header) {
    switch (portIndex) {
    case OMX_DirInput:
    {
        if (mSecureFlag || mAllocInput) {
            BufferCtrlStruct* pBufCtrl= (BufferCtrlStruct*)(header->pInputPortPrivate);
            if(pBufCtrl != NULL && pBufCtrl->pMem != NULL) {
                ALOGI("freeBuffer, intput phyAddr: 0x%lx", pBufCtrl->phyAddr);
                pBufCtrl->pMem.clear();
                return SprdSimpleOMXComponent::freeBuffer(portIndex, header);
            } else {
                ALOGE("freeBuffer, intput pBufCtrl==NULL or pBufCtrl->pMem==NULL");
                return OMX_ErrorUndefined;
            }
        } else {
            return SprdSimpleOMXComponent::freeBuffer(portIndex, header);
        }
    }

    case OMX_DirOutput:
    {
        BufferCtrlStruct* pBufCtrl= (BufferCtrlStruct*)(header->pOutputPortPrivate);
        if(pBufCtrl != NULL) {
            if (mIOMMUEnabled && pBufCtrl->phyAddr > 0) {
                ALOGI("freeBuffer,phyAddr: 0x%lx", pBufCtrl->phyAddr);
                if (mDeintl && mDecoderSwFlag){
                    mDeintl->VspFreeIova(pBufCtrl->phyAddr, pBufCtrl->bufferSize);
                }else{
                    (*mH264DecFreeIOVA)(mHandle, pBufCtrl->phyAddr, pBufCtrl->bufferSize);
                }
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

OMX_ERRORTYPE SPRDAVCDecoder::getConfig(
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

OMX_ERRORTYPE SPRDAVCDecoder::setConfig(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexConfigThumbnailMode:
    {
        OMX_BOOL *pEnable = (OMX_BOOL *)params;

        if (*pEnable == OMX_TRUE) {
            mThumbnailMode = OMX_TRUE;
        }

        ALOGI("setConfig, mThumbnailMode = %d", mThumbnailMode);

        if (mThumbnailMode) {
            PortInfo *pInPort = editPortInfo(OMX_DirInput);
            PortInfo *pOutPort = editPortInfo(OMX_DirOutput);
            pInPort->mDef.nBufferCountActual = 2;
            pOutPort->mDef.nBufferCountActual = 2;
#if 0
            pOutPort->mDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
            if(!mDecoderSwFlag) {
                mChangeToSwDec = true;
            }
#endif
        }
        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::setConfig(index, params);
    }
}

void SPRDAVCDecoder::dump_strm(uint8 *pBuffer, int32 aInBufSize) {
    if(mDumpStrmEnabled) {
        FILE *fp = fopen("/data/misc/media/video_es.m4v","ab");
        fwrite(pBuffer,1,aInBufSize,fp);
        fclose(fp);
    }
}

void SPRDAVCDecoder::dump_yuv(uint8 *pBuffer, int32 aInBufSize) {
    if(mDumpYUVEnabled) {
        FILE *fp = fopen("/data/misc/media/video_out.yuv","ab");
        fwrite(pBuffer,1,aInBufSize,fp);
        fclose(fp);
    }
}

void SPRDAVCDecoder::onDecodePrepare(OMX_BUFFERHEADERTYPE *header) {
    if (!mSecureFlag && header->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
        findCodecConfigData(header);
}

void SPRDAVCDecoder::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return;
    }

    if (mDeintl && portIndex == kOutputPortIndex) {
        signalDeintlThread();
    }

    if (mEOSStatus == OUTPUT_FRAMES_FLUSHED) {
        return;
    }

    if(mChangeToSwDec) {

        mChangeToSwDec = false;

        ALOGI("%s, %d, change to sw decoder, mThumbnailMode: %d",
              __FUNCTION__, __LINE__, mThumbnailMode);

        if (mIOMMUEnabled && (!mIsInterlacedSequence || !mNeedDeinterlace || mThumbnailMode)) {
            freeOutputBufferIOVA();
        }

        releaseDecoder();

        if(!openDecoder("libomx_avcdec_sw_sprd.so")) {
            ALOGE("onQueueFilled, open  libomx_avcdec_sw_sprd.so failed.");
            notify(OMX_EventError, OMX_ErrorDynamicResourcesUnavailable, 0, NULL);
            mSignalledError = true;
            mDecoderSwFlag = false;
            return;
        }

        mDecoderSwFlag = true;

        if(initDecoder() != OK) {
            ALOGE("onQueueFilled, init sw decoder failed.");
            notify(OMX_EventError, OMX_ErrorDynamicResourcesUnavailable, 0, NULL);
            mSignalledError = true;
            return;
        }
        if (mThumbnailMode) {
            MMDecVideoFormat video_format;
            video_format.thumbnail = 1;
            (*mH264DecSetparam)(mHandle, &video_format);
        }
    }

    List<BufferInfo *> &inQueue = getPortQueue(kInputPortIndex);
    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);

    while (!mStopDecode && (mEOSStatus != INPUT_DATA_AVAILABLE || !inQueue.empty())
            && (outQueue.size() != 0 || (mDeintl && !mDecOutputBufQueue.empty()))) {

        if (mEOSStatus == INPUT_EOS_SEEN && mFrameDecoded) {
            drainAllOutputBuffers();
            return;
        }

        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;

        OMX_BUFFERHEADERTYPE *outHeader = NULL;
        List<BufferInfo *>::iterator itBuffer;
        BufferCtrlStruct *pBufCtrl = NULL;
        size_t count = 0;
        uint32_t queueSize = 0;

        if (mDeintl) {
            itBuffer = mDecOutputBufQueue.begin();
        } else {
            itBuffer = outQueue.begin();
        }

        do {
            if (mDeintl) {
                queueSize = mDecOutputBufQueue.size();
            } else {
                queueSize = outQueue.size();
            }

            if(count >= queueSize) {
#ifdef DUMP_DEBUG
                ALOGI("onQueueFilled, get outQueue buffer, return, count=%zd, queue_size=%d",count, queueSize);
#endif
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

#ifdef DUMP_DEBUG
        ALOGI("%s, %d, outHeader:%p, inHeader: %p, len: %d, nOffset: %d, time: %lld, EOS: %d",
              __FUNCTION__, __LINE__,outHeader,inHeader, inHeader->nFilledLen,
              inHeader->nOffset, inHeader->nTimeStamp,inHeader->nFlags & OMX_BUFFERFLAG_EOS);
#endif
        ++mPicId;
        mFrameDecoded = false;
        if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
            mEOSStatus = INPUT_EOS_SEEN;
        }

        if(inHeader->nFilledLen == 0) {
            mFrameDecoded = true;
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

        if (!mSecureFlag && !(inHeader->nFlags & OMX_BUFFERFLAG_CODECCONFIG)) {
            if (!mDecoderSawSPS) {
                if (mSPSDataSize > 0) {
                    ALOGI("%s, drain SPSData", __FUNCTION__);
                    bitstream = mSPSData;
                    bufferSize = mSPSDataSize;
                    mIsResume = true;
                }
            } else if (!mDecoderSawPPS) {
                if (mPPSDataSize > 0) {
                    ALOGI("%s, drain PPSData", __FUNCTION__);
                    bitstream = mPPSData;
                    bufferSize = mPPSDataSize;
                    mIsResume = true;
                }
            }
        }

        if (!mSecureFlag) {
            dec_in.pStream = mPbuf_stream_v;
            dec_in.pStream_phy = mPbuf_stream_p;
        } else {
            dec_in.pStream = NULL;
            dec_in.pStream_phy = (unsigned long)bitstream;
        }
        dec_in.beLastFrm = 0;
        dec_in.expected_IVOP = mNeedIVOP;
        dec_in.beDisplayed = 1;
        dec_in.err_pkt_num = 0;
        dec_in.nTimeStamp = (uint64)(inHeader->nTimeStamp);

        dec_out.frameEffective = 0;

        if (!mSecureFlag) {
            if(mThumbnailMode) {
                if(((bitstream[0] != 0x0) || (bitstream[1] != 0x0) || (bitstream[2] != 0x0) || (bitstream[3] != 0x1))
                    &&((bitstream[0] != 0x0) || (bitstream[1] != 0x0) || (bitstream[2] != 0x1))) {
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
        } else {
            dec_in.dataLen = bufferSize;
        }

        ALOGV("%s, %d, dec_in.dataLen: %d, mPicId: %d", __FUNCTION__, __LINE__, dec_in.dataLen, mPicId);

        outHeader->nTimeStamp = inHeader->nTimeStamp;
        outHeader->nFlags = inHeader->nFlags;

        unsigned long picPhyAddr = 0;
        if(!mDecoderSwFlag) {
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
        }

        ALOGV("%s, %d, outHeader: %p, pBuffer: %p, phyAddr: 0x%lx",
                __FUNCTION__, __LINE__, outHeader, outHeader->pBuffer, picPhyAddr);
        GraphicBufferMapper &mapper = GraphicBufferMapper::get();
        if (mDeintl) {
            (*mH264Dec_SetCurRecPic)(mHandle, (uint8 *)outHeader->pBuffer, (uint8 *)picPhyAddr, (void *)outHeader, mPicId);
        } else {
            if(iUseAndroidNativeBuffer[OMX_DirOutput]) {
                OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(kOutputPortIndex)->mDef;
                int width = def->format.video.nStride;
                int height = def->format.video.nSliceHeight;
                Rect bounds(width, height);
                void *vaddr;
                int usage;
                if (mDecoderSwFlag)
                    usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
                else if (mIOMMUEnabled)
                    usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_NEVER;
                else
                    usage = GRALLOC_USAGE_VIDEO_BUFFER | GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_NEVER;

                if(mapper.lock((const native_handle_t*)outHeader->pBuffer, usage, bounds, &vaddr)) {
                    ALOGE("onQueueFilled, mapper.lock fail %p",outHeader->pBuffer);
                    return ;
                }
                ALOGV("%s, %d, pBuffer: 0x%p, vaddr: %p", __FUNCTION__, __LINE__, outHeader->pBuffer,vaddr);
                uint8_t *yuv = (uint8_t *)((uint8_t *)vaddr + outHeader->nOffset);
                ALOGV("%s, %d, yuv: %p, mPicId: %d, outHeader: %p, outHeader->pBuffer: %p, outHeader->nTimeStamp: %lld",
                __FUNCTION__, __LINE__, yuv, mPicId,outHeader, outHeader->pBuffer, outHeader->nTimeStamp);
                (*mH264Dec_SetCurRecPic)(mHandle, yuv, (uint8 *)picPhyAddr, (void *)outHeader, mPicId);
            } else {
                uint8 *yuv = (uint8 *)(outHeader->pBuffer + outHeader->nOffset);
                (*mH264Dec_SetCurRecPic)(mHandle, yuv, (uint8 *)picPhyAddr, (void *)outHeader, mPicId);
            }
        }

        dump_strm(mPbuf_stream_v, dec_in.dataLen);

        int64_t start_decode = systemTime();
        MMDecRet decRet = (*mH264DecDecode)(mHandle, &dec_in,&dec_out);
        int64_t end_decode = systemTime();
#ifdef DUMP_DEBUG
        ALOGI("%s, %d, decRet: %d, %dms, dec_out.frameEffective: %d, needIVOP: %d, in {%p, 0x%lx}, consume byte: %u, flag:0x%x, SPS:%d, PPS:%d, pts:%lld",
              __FUNCTION__, __LINE__, decRet, (unsigned int)((end_decode-start_decode) / 1000000L),
              dec_out.frameEffective, mNeedIVOP, dec_in.pStream, dec_in.pStream_phy, dec_in.dataLen, inHeader->nFlags,dec_out.sawSPS,dec_out.sawPPS, dec_out.pts);
#endif
        mDecoderSawSPS = dec_out.sawSPS;
        mDecoderSawPPS = dec_out.sawPPS;

        if(iUseAndroidNativeBuffer[OMX_DirOutput] && !mDeintl) {
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
                if (mDecoderSwFlag) {
                    notify(OMX_EventError, OMX_ErrorInsufficientResources, 0, NULL);
                    mSignalledError = true;
                } else {
                    ALOGI("change to sw decoder.");
                    mChangeToSwDec = true;
                    mDecoderSawSPS = false;
                    mDecoderSawPPS = false;
                    mIsResume = false;
                }
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

        MMDecRet ret;
        ret = (*mH264DecGetInfo)(mHandle, &mDecoderInfo);
        if(ret == MMDEC_OK) {
            if (!((mDecoderInfo.picWidth<= mCapability.max_width&& mDecoderInfo.picHeight<= mCapability.max_height)
                    || (mDecoderInfo.picWidth <= mCapability.max_height && mDecoderInfo.picHeight <= mCapability.max_width))) {
                ALOGE("[%d,%d] is out of range [%d, %d], failed to support this format.",
                      mDecoderInfo.picWidth, mDecoderInfo.picHeight, mCapability.max_width, mCapability.max_height);
                notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, NULL);
                mSignalledError = true;
                return;
            }

            if (mDecoderInfo.need_deinterlace) {
                mNeedDeinterlace = true;
            }

            if (!mDeintInitialized && !mThumbnailMode && mNeedDeinterlace) {
                void* component = reinterpret_cast<void*>(this);
                mDeintInitialized = true;
                mDeintl = new SPRDDeinterlace(component);
                mDeintl->mUseNativeBuffer = iUseAndroidNativeBuffer[OMX_DirOutput];
                mDeintl->startDeinterlaceThread();
                allocateDecOutputBuffer(&mDecoderInfo);
            }

            if (handlePortSettingChangeEvent(&mDecoderInfo)) {
                return;
            } else if(mChangeToSwDec == true) {
                return;
            }
        } else {
            ALOGE("failed to get decoder information.");
        }

        if (mIsResume) {
            if ((decRet != MMDEC_OK && decRet != MMDEC_FRAME_SEEK_IVOP) || ret != MMDEC_OK) {
                ALOGI("SPS or PPS data error, reset backup data size.");
                mSPSDataSize = 0;
                mPPSDataSize = 0;
            }
            mIsResume = false;
        } else {
            CHECK_LE(dec_in.dataLen, inHeader->nFilledLen + add_startcode_len);

            bufferSize = dec_in.dataLen;
            inHeader->nOffset += bufferSize;
            inHeader->nFilledLen -= bufferSize;
            if (inHeader->nFilledLen <= 0) {
                mFrameDecoded = true;
                inHeader->nOffset = 0;
                inInfo->mOwnedByUs = false;
                inQueue.erase(inQueue.begin());
                inInfo = NULL;
                notifyEmptyBufferDone(inHeader);
                inHeader = NULL;
            }
        }

        if (mDeintl) {
            while (!mDecOutputBufQueue.empty() && mHeadersDecoded && dec_out.frameEffective) {
                drainOneDeintlBuffer(dec_out.pBufferHeader, dec_out.pts);
                dec_out.frameEffective = false;
            }
        } else {
            while (!outQueue.empty() &&
                        mHeadersDecoded &&
                        dec_out.frameEffective) {
#ifdef DUMP_DEBUG
                ALOGI("%s, %d, dec_out.pBufferHeader: %p, dec_out.mPicId: %d, dec_out.pts: %lld",
                    __FUNCTION__, __LINE__, dec_out.pBufferHeader, dec_out.mPicId, dec_out.pts);
#endif
                drainOneOutputBuffer(dec_out.mPicId, dec_out.pBufferHeader, dec_out.pts);
                dump_yuv(dec_out.pOutFrameY, mPictureSize);
                dec_out.frameEffective = false;
                if(mThumbnailMode) {
                    mStopDecode = true;
                }
            }
        }
    }
}

bool SPRDAVCDecoder::handlePortSettingChangeEvent(const H264SwDecInfo *info) {
    OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(kOutputPortIndex)->mDef;
    OMX_BOOL useNativeBuffer = iUseAndroidNativeBuffer[OMX_DirOutput];
    bool needFlushBuffer = outputBuffersNotEnough(info, def->nBufferCountMin, def->nBufferCountActual, useNativeBuffer);
    bool cropChanged = handleCropRectEvent(&info->cropParams);

    if ((mStride != info->picWidth) || (mSliceHeight != info->picHeight) || cropChanged || (!mThumbnailMode && needFlushBuffer)) {
        Mutex::Autolock autoLock(mLock);
        int32_t picId;
        void* pBufferHeader;
        uint64 pts;

        while (MMDEC_OK == (*mH264Dec_GetLastDspFrm)(mHandle, &pBufferHeader, &picId, &pts)) {
            if (mDeintl) {
                drainOneDeintlBuffer(pBufferHeader, pts);
            } else {
                drainOneOutputBuffer(picId, pBufferHeader, pts);
            }
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
                ALOGI("%s, %d, info->numRefFrames: %d, info->has_b_frames: %d, def->nBufferCountMin: %d",
                      __FUNCTION__, __LINE__, info->numRefFrames, info->has_b_frames, def->nBufferCountMin);

                /*FIXME:plus additional one buffer for avoiding timed out,
                *because the number of native window reserved buffer is not sure.*/
                def->nBufferCountMin = info->numRefFrames + info->has_b_frames + 1 + 1;
            } else {
                ALOGI("%s, %d, info->numRefFrames: %d, info->has_b_frames: %d, def->nBufferCountActual: %d",
                      __FUNCTION__, __LINE__, info->numRefFrames, info->has_b_frames, def->nBufferCountActual);

                /*FIXME: When NativeWindow is null, We need calc actual buffer count manually.
                * 1: avoiding timed out, 1:reconstructed frame, 4:reserved buffers by SurfaceFlinger.*/
                def->nBufferCountActual = info->numRefFrames + info->has_b_frames + 1 + 1 + 4;

                /*fix Bug 375771 testCodecResetsH264WithSurface fail*/
                def->bPopulated = OMX_FALSE;
            }
        }

        updatePortDefinitions(true, true);
        (*mH264Dec_ReleaseRefBuffers)(mHandle);
        notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
        mOutputPortSettingsChange = AWAITING_DISABLED;
        return true;
    }

    return false;
}

bool SPRDAVCDecoder::handleCropRectEvent(const CropParams *crop) {
    if (mCropWidth != crop->cropOutWidth ||
            mCropHeight != crop->cropOutHeight) {
        ALOGI("%s, crop w h: %d %d", __FUNCTION__, crop->cropOutWidth, crop->cropOutHeight);
        return true;
    }
    return false;
}

void SPRDAVCDecoder::drainOneOutputBuffer(int32_t picId, void* pBufferHeader, uint64 pts) {

    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);

    List<BufferInfo *>::iterator it = outQueue.begin();
    while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != outQueue.end()) {
        ++it;
    }
    CHECK((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)pBufferHeader);

    BufferInfo *outInfo = *it;
    OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;

    outHeader->nFilledLen = mPictureSize;
    outHeader->nTimeStamp = (OMX_TICKS)pts;

    ALOGI("%s, %d, outHeader: %p, outHeader->pBuffer: %p, outHeader->nOffset: %d, outHeader->nFlags: %d, outHeader->nTimeStamp: %lld",
          __FUNCTION__, __LINE__, outHeader , outHeader->pBuffer, outHeader->nOffset, outHeader->nFlags, outHeader->nTimeStamp);

    outInfo->mOwnedByUs = false;
    outQueue.erase(it);
    outInfo = NULL;

    BufferCtrlStruct* pOutBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
    pOutBufCtrl->iRefCount++;
    notifyFillBufferDone(outHeader);
}

bool SPRDAVCDecoder::drainAllOutputBuffers() {
    ALOGI("%s, %d", __FUNCTION__, __LINE__);

    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);
    BufferInfo *outInfo;
    List<BufferInfo *>::iterator it;
    List<BufferInfo *>::iterator it_end;
    OMX_BUFFERHEADERTYPE *outHeader;

    int32_t picId;
    void* pBufferHeader;
    uint64 pts;

    while (mEOSStatus != OUTPUT_FRAMES_FLUSHED) {

        if (mDeintl && mDecOutputBufQueue.empty()) {
            ALOGW("%s, %d, There is no more  decoder output buffer\n", __FUNCTION__, __LINE__);
            return false;
        } else if(outQueue.empty()) {
            ALOGW("%s, %d, There is no more  display buffer\n", __FUNCTION__, __LINE__);
            return false;
        }

        if(mDeintl) {
            it = mDecOutputBufQueue.begin();
            it_end = mDecOutputBufQueue.end();
        } else {
            it = outQueue.begin();
            it_end = outQueue.end();
        }

        if (mHeadersDecoded && MMDEC_OK == (*mH264Dec_GetLastDspFrm)(mHandle, &pBufferHeader, &picId, &pts) ) {
            while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != it_end) {
                ++it;
            }
            CHECK((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)pBufferHeader);
            outInfo = *it;
            outHeader = outInfo->mHeader;
            outHeader->nFilledLen = mPictureSize;

        } else {
            outInfo = *it;
            outHeader = outInfo->mHeader;
            outHeader->nTimeStamp = 0;
            outHeader->nFilledLen = 0;
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;
            mEOSStatus = OUTPUT_FRAMES_FLUSHED;
        }

        if(mDeintl) {
            Mutex::Autolock autoLock(mThreadLock);
            mDeinterInputBufQueue.push_back(*it);
            mDecOutputBufQueue.erase(it);
        } else {
            outQueue.erase(it);
            outInfo->mOwnedByUs = false;
            BufferCtrlStruct* pOutBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
            pOutBufCtrl->iRefCount++;
            notifyFillBufferDone(outHeader);
        }
    }

    if(mDeintl) {
        signalDeintlThread();
    }
    return true;
}

void SPRDAVCDecoder::onPortFlushCompleted(OMX_U32 portIndex) {
    if (portIndex == kInputPortIndex) {
        mEOSStatus = INPUT_DATA_AVAILABLE;
        mNeedIVOP = true;
    }

    if (portIndex == kOutputPortIndex && mDeintl) {
        flushDeintlBuffer();
    }
}

void SPRDAVCDecoder::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
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

void SPRDAVCDecoder::onPortFlushPrepare(OMX_U32 portIndex) {
    if(portIndex == OMX_DirInput)
        ALOGI("%s", __FUNCTION__);

    if(portIndex == OMX_DirOutput) {
        if( NULL!=mH264Dec_ReleaseRefBuffers )
            (*mH264Dec_ReleaseRefBuffers)(mHandle);
    }
}

void SPRDAVCDecoder::onReset() {
    mGettingPortFormat = OMX_FALSE;
    mSignalledError = false;

    //avoid process error after stop codec and restart codec when port settings changing.
    mOutputPortSettingsChange = NONE;
    mDecoderSawSPS = false;
    mDecoderSawPPS = false;
    mSPSDataSize = 0;
    mPPSDataSize = 0;
    mIsResume = false;
}

void SPRDAVCDecoder::updatePortDefinitions(bool updateCrop, bool updateInputSize) {
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
int32_t SPRDAVCDecoder::ExtMemAllocWrapper(
    void* aUserData, unsigned int size_extra) {
    return static_cast<SPRDAVCDecoder *>(aUserData)->VSP_malloc_cb(size_extra);
}
int32_t SPRDAVCDecoder::ClearDpbWrapper(
    void* aUserData) {
    return static_cast<SPRDAVCDecoder *>(aUserData)->VSP_clear_dpb_cb();
}

// static
int32_t SPRDAVCDecoder::MbinfoMemAllocWrapper(
    void* aUserData, unsigned int size_mbinfo, unsigned long *pPhyAddr) {

    return static_cast<SPRDAVCDecoder *>(aUserData)->VSP_malloc_mbinfo_cb(size_mbinfo, pPhyAddr);
}

// static
int32_t SPRDAVCDecoder::BindFrameWrapper(void *aUserData, void *pHeader) {
    return static_cast<SPRDAVCDecoder *>(aUserData)->VSP_bind_cb(pHeader);
}

// static
int32_t SPRDAVCDecoder::UnbindFrameWrapper(void *aUserData, void *pHeader) {
    return static_cast<SPRDAVCDecoder *>(aUserData)->VSP_unbind_cb(pHeader);
}

int SPRDAVCDecoder::VSP_malloc_cb(unsigned int size_extra) {

    ALOGI("%s, %d, mDecoderSwFlag: %d, mPictureSize: %d, size_extra: %d",
            __FUNCTION__, __LINE__, mDecoderSwFlag, mPictureSize, size_extra);

    int32_t picId;
    void* pBufferHeader;
    uint64 pts;

    /*fix Bug 381332 Whatsapp Recorded Video Getting Not Forward*/
    while (MMDEC_OK == (*mH264Dec_GetLastDspFrm)(mHandle, &pBufferHeader, &picId, &pts)) {
        if (mDeintl) {
            drainOneDeintlBuffer(pBufferHeader, pts);
        } else {
            drainOneOutputBuffer(picId, pBufferHeader, pts);
        }
    }

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
                (*mH264DecFreeIOVA)(mHandle, mPbuf_extra_p, mPbuf_extra_size);
            }
            mPmem_extra.clear();
            mPbuf_extra_v = NULL;
            mPbuf_extra_p = 0;
            mPbuf_extra_size = 0;
        }

        if (mIOMMUEnabled) {
            mPmem_extra = new MemIon(SPRD_ION_DEV, size_extra, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
        } else {
            unsigned int memory_type = mSecureFlag ? ION_HEAP_ID_MASK_MM | ION_FLAG_SECURE : ION_HEAP_ID_MASK_MM;
            mPmem_extra = new MemIon(SPRD_ION_DEV, size_extra, MemIon::NO_CACHING, memory_type);
        }
        int fd = mPmem_extra->getHeapID();
        if(fd >= 0) {
            int ret;
            unsigned long phy_addr;
            size_t buffer_size;

            if (mIOMMUEnabled) {
                ret = (*mH264DecGetIOVA)(mHandle, fd, &phy_addr, &buffer_size);
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

    (*mH264DecMemInit)(((SPRDAVCDecoder *)this)->mHandle, extra_mem);

    mHeadersDecoded = true;

    return 0;
}
int SPRDAVCDecoder::VSP_clear_dpb_cb() {
    int32_t picId;
    void* pBufferHeader;
    uint64 pts;
    ALOGI("%s, %d", __FUNCTION__, __LINE__);
    while (MMDEC_OK == (*mH264Dec_GetLastDspFrm)(mHandle, &pBufferHeader, &picId, &pts)) {
        if (mDeintl) {
            drainOneDeintlBuffer(pBufferHeader, pts);
        } else {
            drainOneOutputBuffer(picId, pBufferHeader, pts);
        }
    }
    mPbuf_mbinfo_idx = 0;
    return 0;
}


int SPRDAVCDecoder::VSP_malloc_mbinfo_cb(unsigned int size_mbinfo, unsigned long *pPhyAddr) {

    int idx = mPbuf_mbinfo_idx;

    ALOGI("%s, %d, idx: %d, size_mbinfo: %d", __FUNCTION__, __LINE__, idx, size_mbinfo);

    if (mPbuf_mbinfo_v[idx] != NULL) {
        if (mIOMMUEnabled) {
            (*mH264DecFreeIOVA)(mHandle, mPbuf_mbinfo_p[idx], mPbuf_mbinfo_size[idx]);
        }
        mPmem_mbinfo[idx].clear();
        mPbuf_mbinfo_v[idx] = NULL;
        mPbuf_mbinfo_p[idx] = 0;
        mPbuf_mbinfo_size[idx] = 0;
    }

    if (mIOMMUEnabled) {
        mPmem_mbinfo[idx] = new MemIon(SPRD_ION_DEV, size_mbinfo, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        unsigned int memory_type = mSecureFlag ? ION_HEAP_ID_MASK_MM | ION_FLAG_SECURE : ION_HEAP_ID_MASK_MM;
        mPmem_mbinfo[idx] = new MemIon(SPRD_ION_DEV, size_mbinfo, MemIon::NO_CACHING, memory_type);
    }
    int fd = mPmem_mbinfo[idx]->getHeapID();
    if(fd >= 0) {
        int ret;
        unsigned long phy_addr;
        size_t buffer_size;

        if (mIOMMUEnabled) {
            ret = (*mH264DecGetIOVA)(mHandle, fd, &phy_addr, &buffer_size);
        } else {
            ret = mPmem_mbinfo[idx]->get_phy_addr_from_ion(&phy_addr, &buffer_size);
        }
        if(ret < 0) {
            ALOGE ("mPmem_mbinfo[%d]: get phy addr fail %d(%s)", idx, ret, strerror(errno));
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

int SPRDAVCDecoder::VSP_bind_cb(void *pHeader) {
    BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

#ifdef DUMP_DEBUG
    ALOGI("VSP_bind_cb, pBuffer: %p, pHeader: %p; iRefCount=%d",
          ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader,pBufCtrl->iRefCount);
#endif
    pBufCtrl->iRefCount++;
    return 0;
}

int SPRDAVCDecoder::VSP_unbind_cb(void *pHeader) {
    BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);
#ifdef DUMP_DEBUG
    ALOGI("VSP_unbind_cb, pBuffer: %p, pHeader: %p; iRefCount=%d",
          ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader,pBufCtrl->iRefCount);
#endif
    if (pBufCtrl->iRefCount  > 0) {
        pBufCtrl->iRefCount--;
    }

    return 0;
}

OMX_ERRORTYPE SPRDAVCDecoder::getExtensionIndex(
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
    } else if (strcmp(name, SPRD_INDEX_CONFIG_THUMBNAIL_MODE) == 0) {
        ALOGI("getExtensionIndex:%s",SPRD_INDEX_CONFIG_THUMBNAIL_MODE);
        *index = OMX_IndexConfigThumbnailMode;
        return OMX_ErrorNone;
    } else if (strcmp(name, SPRD_INDEX_PARAM_ALLOC_NATIVE_HANDLE) == 0) {
        ALOGI("getExtensionIndex:%s",SPRD_INDEX_PARAM_ALLOC_NATIVE_HANDLE);
        *index = OMX_IndexParamAllocNativeHandle;
        return OMX_ErrorNone;
    }

    return OMX_ErrorNotImplemented;
}

bool SPRDAVCDecoder::openDecoder(const char* libName) {
    if(mLibHandle) {
        dlclose(mLibHandle);
    }

    ALOGI("openDecoder, lib: %s", libName);

    mLibHandle = dlopen(libName, RTLD_NOW);
    if(mLibHandle == NULL) {
        ALOGE("openDecoder, can't open lib: %s",libName);
        return false;
    }

    mH264DecInit = (FT_H264DecInit)dlsym(mLibHandle, "H264DecInit");
    if(mH264DecInit == NULL) {
        ALOGE("Can't find H264DecInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecGetInfo = (FT_H264DecGetInfo)dlsym(mLibHandle, "H264DecGetInfo");
    if(mH264DecGetInfo == NULL) {
        ALOGE("Can't find H264DecGetInfo in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecDecode = (FT_H264DecDecode)dlsym(mLibHandle, "H264DecDecode");
    if(mH264DecDecode == NULL) {
        ALOGE("Can't find H264DecDecode in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecRelease = (FT_H264DecRelease)dlsym(mLibHandle, "H264DecRelease");
    if(mH264DecRelease == NULL) {
        ALOGE("Can't find H264DecRelease in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264Dec_SetCurRecPic = (FT_H264Dec_SetCurRecPic)dlsym(mLibHandle, "H264Dec_SetCurRecPic");
    if(mH264Dec_SetCurRecPic == NULL) {
        ALOGE("Can't find H264Dec_SetCurRecPic in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264Dec_GetLastDspFrm = (FT_H264Dec_GetLastDspFrm)dlsym(mLibHandle, "H264Dec_GetLastDspFrm");
    if(mH264Dec_GetLastDspFrm == NULL) {
        ALOGE("Can't find H264Dec_GetLastDspFrm in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264Dec_ReleaseRefBuffers = (FT_H264Dec_ReleaseRefBuffers)dlsym(mLibHandle, "H264Dec_ReleaseRefBuffers");
    if(mH264Dec_ReleaseRefBuffers == NULL) {
        ALOGE("Can't find H264Dec_ReleaseRefBuffers in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecMemInit = (FT_H264DecMemInit)dlsym(mLibHandle, "H264DecMemInit");
    if(mH264DecMemInit == NULL) {
        ALOGE("Can't find H264DecMemInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264GetCodecCapability = (FT_H264GetCodecCapability)dlsym(mLibHandle, "H264GetCodecCapability");
    if(mH264GetCodecCapability == NULL) {
        ALOGE("Can't find H264GetCodecCapability in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecGetNALType = (FT_H264DecGetNALType)dlsym(mLibHandle, "H264DecGetNALType");
    if(mH264DecGetNALType == NULL) {
        ALOGE("Can't find H264DecGetNALType in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecSetparam = (FT_H264DecSetparam)dlsym(mLibHandle, "H264DecSetParameter");
    if(mH264DecSetparam == NULL) {
        ALOGE("Can't find H264DecSetParameter in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecGetIOVA = (FT_H264Dec_get_iova)dlsym(mLibHandle, "H264Dec_get_iova");
    if(mH264DecGetIOVA == NULL) {
        ALOGE("Can't find H264Dec_get_iova in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecFreeIOVA = (FT_H264Dec_free_iova)dlsym(mLibHandle, "H264Dec_free_iova");
    if(mH264DecFreeIOVA == NULL) {
        ALOGE("Can't find H264Dec_free_iova in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecGetIOMMUStatus = (FT_H264Dec_get_IOMMU_status)dlsym(mLibHandle, "H264Dec_get_IOMMU_status");
    if(mH264DecGetIOMMUStatus == NULL) {
        ALOGE("Can't find H264Dec_get_IOMMU_status in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    return true;
}

void SPRDAVCDecoder::findCodecConfigData(OMX_BUFFERHEADERTYPE *header) {
        int nal_type, nal_ref_idc;
        int ret;
        uint8 *p = header->pBuffer + header->nOffset;

        MMDecRet decRet = (*mH264DecGetNALType)(mHandle, p, header->nFilledLen, &nal_type, &nal_ref_idc);

        ALOGI("%s, header:%p, nal_type:%d, nal_ref_idc:%d, mSPSDataSize:%u, mPPSDataSize:%u",
              __FUNCTION__, header, nal_type, nal_ref_idc, mSPSDataSize, mPPSDataSize);

        if (decRet == MMDEC_OK) {
            if (nal_type == 0x7/*SPS*/ &&
                header->nFilledLen <= H264_HEADER_SIZE) {
                mSPSDataSize = header->nFilledLen;
                memcpy(mSPSData, p, mSPSDataSize);
                if (!mCapability.support_1080i) {
                    if ((ret = isInterlacedSequence(mSPSData, mSPSDataSize)) != 0) {
                        if(!mDecoderSwFlag) {
                            mChangeToSwDec = true;
                            mIsInterlacedSequence = true;
                            if (ret == 1){
                                mNeedDeinterlace = true;
                            }
                        }
                    }
                }
            } else if (nal_type == 0x8/*PPS*/ &&
                           header->nFilledLen <= H264_HEADER_SIZE) {
                mPPSDataSize = header->nFilledLen;
                memcpy(mPPSData, p, mPPSDataSize);
            }
        }
}

void SPRDAVCDecoder::freeOutputBufferIOVA() {
    PortInfo *port = editPortInfo(OMX_DirOutput);

    for (size_t i = 0; i < port->mBuffers.size(); ++i) {
        BufferInfo *buffer = &port->mBuffers.editItemAt(i);
        OMX_BUFFERHEADERTYPE *header = buffer->mHeader;

        if(header->pOutputPortPrivate != NULL) {
            BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct*)(header->pOutputPortPrivate);
            if (pBufCtrl->phyAddr > 0) {
                ALOGI("%s, fd: %d, iova: 0x%lx", __FUNCTION__, pBufCtrl->bufferFd, pBufCtrl->phyAddr);
                (*mH264DecFreeIOVA)(mHandle, pBufCtrl->phyAddr, pBufCtrl->bufferSize);
                pBufCtrl->bufferFd = 0;
                pBufCtrl->phyAddr = 0;
            }
        }
    }
}

OMX_ERRORTYPE SPRDAVCDecoder::allocateDecOutputBuffer(H264SwDecInfo* pDecoderInfo) {
    // decoder buffer(numRefFrames+has_b_frames) + deinterlace buffer(3)
    OMX_U32 DecBufferCount = mDecoderInfo.numRefFrames + mDecoderInfo.has_b_frames + 3;

    for (OMX_U32 i = 0; i < DecBufferCount; ++i) {
        OMX_BUFFERHEADERTYPE * header = new OMX_BUFFERHEADERTYPE;
        header->pBuffer = NULL;
        header->nAllocLen = mPictureSize;
        header->nFilledLen = 0;
        header->nOffset = 0;
        header->pOutputPortPrivate = NULL;
        header->pMarkData = NULL;
        header->nTickCount = 0;
        header->nTimeStamp = 0;
        header->nFlags = 0;

        MemIon* pMem = NULL;
        unsigned long phyAddr = 0;
        size_t bufferSize = 0;//don't use OMX_U32
        size_t size64word = pDecoderInfo->picWidth * pDecoderInfo->picHeight *3 / 2;
        int ret;

        if (mIOMMUEnabled) {
            pMem = new MemIon(SPRD_ION_DEV, size64word, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
        } else {
            unsigned int memory_type = mSecureFlag ? ION_HEAP_ID_MASK_MM | ION_FLAG_SECURE : ION_HEAP_ID_MASK_MM;
            pMem = new MemIon(SPRD_ION_DEV, size64word, MemIon::NO_CACHING, memory_type);
        }

        int fd = pMem->getHeapID();
        if(fd < 0) {
            delete header;
            header = NULL;
            ALOGE("Failed to alloc outport pmem buffer");
            return OMX_ErrorInsufficientResources;
        }

        if (mIOMMUEnabled) {
            if (mDeintl && mDecoderSwFlag){
                ret = mDeintl->VspGetIova(fd, &phyAddr, &bufferSize);
            }else{
                ret = (*mH264DecGetIOVA)(mHandle, fd, &phyAddr, &bufferSize);
            }
        } else {
            ret = pMem->get_phy_addr_from_ion(&phyAddr, &bufferSize);
        }

        if (ret < 0) {
            delete header;
            header = NULL;
            ALOGE("%s, get phy addr Failed: %d(%s)", __FUNCTION__, ret, strerror(errno));
            return OMX_ErrorInsufficientResources;
        }

        header->pBuffer = (OMX_U8 *)(pMem->getBase());

        header->pOutputPortPrivate = new BufferCtrlStruct;
        CHECK(header->pOutputPortPrivate != NULL);
        BufferCtrlStruct* pBufCtrl= (BufferCtrlStruct*)(header->pOutputPortPrivate);
        pBufCtrl->iRefCount = 0; //init by 0
        pBufCtrl->pMem = pMem;
        pBufCtrl->bufferFd = fd;
        pBufCtrl->phyAddr = phyAddr;
        pBufCtrl->bufferSize = bufferSize;
        ALOGV("%s: allocate buffer from pmem, pBuffer: %p, bufferFd: %d,  phyAddr: 0x%lx, size: %zd",
                __FUNCTION__, header->pBuffer, pBufCtrl->bufferFd, pBufCtrl->phyAddr, bufferSize);


        BufferInfo* bufNode = new BufferInfo;
        bufNode->mHeader = header;
        bufNode->mNodeId = i;

        mDecOutputBufQueue.push_back(bufNode);
        mBufferNodes.push_back(bufNode);
    }

    ALOGI("%s, %d, sucessful allocated buffer count: %d, buffer size: %d, total buffer count: %d, mIOMMUEnabled : %d",
          __FUNCTION__, __LINE__, mDecOutputBufQueue.size(), mPictureSize, DecBufferCount, mIOMMUEnabled);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDAVCDecoder::freeDecOutputBuffer() {
    ALOGI("freeBufferNodes\n");

    mDecOutputBufQueue.clear();
    mDeinterInputBufQueue.clear();

    for (size_t i = mBufferNodes.size(); i-- > 0;) {
        BufferInfo* pBufferNode = mBufferNodes.editItemAt(i);

        if (pBufferNode != NULL && pBufferNode->mHeader != NULL) {
            ALOGV("free buffer node id: %d, i = %d\n", pBufferNode->mNodeId, i);

            BufferCtrlStruct* pBufCtrl= (BufferCtrlStruct*)(pBufferNode->mHeader->pOutputPortPrivate);
            if(pBufCtrl != NULL && pBufCtrl->pMem != NULL) {
                ALOGV("freeBuffer, phyAddr: 0x%lx\n", pBufCtrl->phyAddr);
                if (mIOMMUEnabled) {
                    if (mDeintl && mDecoderSwFlag){
                        mDeintl->VspFreeIova(pBufCtrl->phyAddr, pBufCtrl->bufferSize);
                    }else{
                        (*mH264DecFreeIOVA)(mHandle, pBufCtrl->phyAddr, pBufCtrl->bufferSize);
                    }
                }
                pBufCtrl->pMem.clear();
            } else {
                ALOGE("freeBuffer, pMem==NULL");
                return OMX_ErrorUndefined;
            }

            if (pBufferNode->mHeader->pOutputPortPrivate != NULL) {
                delete (BufferCtrlStruct*)(pBufferNode->mHeader->pOutputPortPrivate);
                pBufferNode->mHeader->pOutputPortPrivate = NULL;
            }

            delete pBufferNode->mHeader;
            delete pBufferNode;
        }
        mBufferNodes.removeAt(i);
    }
    return OMX_ErrorNone;
}

void SPRDAVCDecoder::drainOneDeintlBuffer(void* pBufferHeader, uint64 pts) {
    List<BufferInfo *>::iterator it = mDecOutputBufQueue.begin();
    while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != mDecOutputBufQueue.end()) {
        ++it;
    }
    CHECK((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)pBufferHeader);

    BufferInfo * bufferNode = *it;
    OMX_BUFFERHEADERTYPE *outHeader = bufferNode->mHeader;

    bufferNode->DeintlWidth = mDecoderInfo.picWidth;
    bufferNode->DeintlHeight = mDecoderInfo.picHeight;

    outHeader->nFilledLen = mPictureSize;
    outHeader->nTimeStamp = (OMX_TICKS)pts;

    {
        Mutex::Autolock autoLock(mThreadLock);
        mDeinterInputBufQueue.push_back(*it);
        mDecOutputBufQueue.erase(it);
    }

    signalDeintlThread();

    ALOGI("%s, %d, mDeinterInputBufQueue.size = %d, mNodeId = %d\n",
            __FUNCTION__, __LINE__, mDeinterInputBufQueue.size(), bufferNode->mNodeId);
}

bool SPRDAVCDecoder::drainAllDecOutputBufferforDeintl() {
    OMX_BUFFERHEADERTYPE *outHeader;
    BufferInfo *outBufferNode;

    int32_t picId;
    void* pBufferHeader;
    uint64 pts;

    while (!mDecOutputBufQueue.empty() && mEOSStatus != OUTPUT_FRAMES_FLUSHED) {
        //Mutex::Autolock nodeQueLock(mNodeQueLock);

        if (MMDEC_OK == (*mH264Dec_GetLastDspFrm)(mHandle, &pBufferHeader, &picId, &pts) ) {
            List<BufferInfo *>::iterator it = mDecOutputBufQueue.begin();
            while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != mDecOutputBufQueue.end()) {
                ++it;
            }
            CHECK((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)pBufferHeader);
            outBufferNode = *it;
            outHeader = outBufferNode->mHeader;
            outHeader->nFilledLen = mPictureSize;
            ALOGI("drainAllDecOutputBufferNodes, node id = %d", outBufferNode->mNodeId);

            {
                Mutex::Autolock autoLock(mThreadLock);
                mDeinterInputBufQueue.push_back(*it);
                mDecOutputBufQueue.erase(it);
            }
        } else {
            List<BufferInfo *>::iterator it = mDecOutputBufQueue.begin();
            outBufferNode = *it;
            outHeader = outBufferNode->mHeader;
            outHeader->nTimeStamp = 0;
            outHeader->nFilledLen = 0;
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;
            mEOSStatus = OUTPUT_FRAMES_FLUSHED;

            ALOGI("drainAllDecOutputBufferNodes, the last buffer node id = %d", outBufferNode->mNodeId);
            //the last buffer, use the eos flag
            {
                Mutex::Autolock autoLock(mThreadLock);
                mDeinterInputBufQueue.push_back(*it);
                mDecOutputBufQueue.erase(it);
            }
        }
    }

    ALOGI("drainAllDecOutputBufferNodes() mDeinterInputBufQueue.size() = %d", mDeinterInputBufQueue.size());

    return true;
}

void SPRDAVCDecoder::signalDeintlThread() {
    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);
    if (mDeinterInputBufQueue.size() > 1 && !outQueue.empty()) {
        ALOGI("%s, %d,  send mDeinterReadyCondition signal\n", __FUNCTION__, __LINE__);
        mDeintl->mDeinterReadyCondition.signal();
    }
}

void SPRDAVCDecoder::flushDeintlBuffer() {
    ALOGI("%s, %d, mDecOutputBufQueue size: %d, mDeinterInputBufQueue size: %d",
          __FUNCTION__, __LINE__, mDecOutputBufQueue.size(), mDeinterInputBufQueue.size());

    if (mDeinterInputBufQueue.empty()) {
        return;
    }

    for (size_t i = mDeinterInputBufQueue.size(); i-- > 0;) {
        List<BufferInfo *>::iterator itBufferNode = mDeinterInputBufQueue.begin();
        mDecOutputBufQueue.push_back(*itBufferNode);
        mDeinterInputBufQueue.erase(itBufferNode);
    }

    mDeintl->mDeintlFrameNum = 0;
}

}  // namespace android

android::SprdOMXComponent *createSprdOMXComponent(
    const char *name, const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    return new android::SPRDAVCDecoder(name, callbacks, appData, component);
}
