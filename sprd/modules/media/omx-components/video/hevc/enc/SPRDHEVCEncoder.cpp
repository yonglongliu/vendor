/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_NDEBUG 0
#define LOG_TAG "SPRDHEVCEncoder"
#include <utils/Log.h>

#include "hevc_enc_api.h"
#ifndef CONFIG_BIA_SUPPORT
#include <arm_neon.h>
#endif
#include "SimpleSoftOMXComponent.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
#include <media/IOMX.h>

#include <MetadataBufferType.h>
#include <HardwareAPI.h>

#include <ui/Rect.h>
#include <ui/GraphicBufferMapper.h>
#include <dlfcn.h>

#include <linux/ion.h>
#include "sprd_ion.h"

#include "MemIon.h"

#include "SPRDHEVCEncoder.h"
#include "gralloc_public.h"
#include <OMX_VideoExt.h>
#include <OMX_IndexExt.h>

#define VIDEOENC_CURRENT_OPT

namespace android {

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
    HEVCLevel hevcLevel;
} LevelConcersion;

static LevelConversion ConversionTable[] = {
    { OMX_VIDEO_AVCLevel1,  AVC_LEVEL1_B },
    { OMX_VIDEO_AVCLevel1b, AVC_LEVEL1   },
    { OMX_VIDEO_AVCLevel11, AVC_LEVEL1_1 },
    { OMX_VIDEO_AVCLevel12, AVC_LEVEL1_2 },
    { OMX_VIDEO_AVCLevel13, AVC_LEVEL1_3 },
//    { OMX_VIDEO_AVCLevel2,  AVC_LEVEL2 },
    { OMX_VIDEO_HEVCMainTierLevel2, AVC_LEVEL2 },
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

static status_t ConvertOmxHevcLevelToHevcSpecLevel(
    OMX_U32 omxLevel, HEVCLevel *hevcLevel) {
    for (size_t i = 0, n = sizeof(ConversionTable)/sizeof(ConversionTable[0]);
            i < n; ++i) {
        if (omxLevel == ConversionTable[i].omxLevel) {
            *hevcLevel = ConversionTable[i].hevcLevel;
            return OK;
        }
    }

    ALOGE("ConvertOmxHevcLevelToHevcSpecLevel: %d level not supported",
          (int32_t)omxLevel);

    return BAD_VALUE;
}

static status_t ConvertHevcSpecLevelToOmxHevcLevel(
    HEVCLevel hevcLevel, OMX_U32 *omxLevel) {
    for (size_t i = 0, n = sizeof(ConversionTable)/sizeof(ConversionTable[0]);
            i < n; ++i) {
        if (hevcLevel == ConversionTable[i].hevcLevel) {
            *omxLevel = ConversionTable[i].omxLevel;
            return OK;
        }
    }

    ALOGE("ConvertHevcSpecLevelToOmxHevcLevel: %d level not supported",
          (int32_t) hevcLevel);

    return BAD_VALUE;
}
#endif
/*
 * In case of orginal input height_org is not 16 aligned, we shoud copy original data to a larger space
 * Example: width_org = 640, height_org = 426,
 * We have to copy this data to a width_dst = 640 height_dst = 432 buffer which is 16 aligned.
 * Be careful, when doing this convert we MUST keep UV in their right position.
 *
 * FIXME: If width_org is not 16 aligned also, this would be much complicate
 *
 */
inline static void ConvertYUV420PlanarToYVU420SemiPlanar(uint8_t *inyuv, uint8_t* outyuv,
        int32_t width_org, int32_t height_org, int32_t width_dst, int32_t height_dst) {

    int32_t inYsize = width_org * height_org;
    uint32_t *outy =  (uint32_t *) outyuv;
    uint16_t *incb = (uint16_t *) (inyuv + inYsize);
    uint16_t *incr = (uint16_t *) (inyuv + inYsize + (inYsize >> 2));

    /* Y copying */
    memcpy(outy, inyuv, inYsize);

    /* U & V copying, Make sure uv data is in their right position*/
    uint32_t *outUV = (uint32_t *) (outyuv + width_dst * height_dst);
    for (int32_t i = height_org >> 1; i > 0; --i) {
        for (int32_t j = width_org >> 2; j > 0; --j) {
            uint32_t tempU = *incb++;
            uint32_t tempV = *incr++;

            tempU = (tempU & 0xFF) | ((tempU & 0xFF00) << 8);
            tempV = (tempV & 0xFF) | ((tempV & 0xFF00) << 8);
            uint32_t temp = tempV | (tempU << 8);

            // Flip U and V
            *outUV++ = temp;
        }
    }
}

#ifdef CONFIG_BIA_SUPPORT
static int RGB_r_y[256];
static int RGB_r_cb[256];
static int RGB_r_cr_b_cb[256];
static int RGB_g_y[256];
static int RGB_g_cb[256];
static int RGB_g_cr[256];
static int RGB_b_y[256];
static int RGB_b_cr[256];
static  bool mConventFlag = false;

//init the convert table, the Transformation matrix is as:
// Y  =  ((66 * (_r)  + 129 * (_g)  + 25    * (_b)) >> 8) + 16
// Cb = ((-38 * (_r) - 74   * (_g)  + 112  * (_b)) >> 8) + 128
// Cr =  ((112 * (_r) - 94   * (_g)  - 18    * (_b)) >> 8) + 128
inline static void inittable() {
    ALOGI("init table");
    int i = 0;
    for(i = 0; i < 256; i++) {
        RGB_r_y[i] =  ((66 * i) >> 8);
        RGB_r_cb[i] = ((38 * i) >> 8);
        RGB_r_cr_b_cb[i] = ((112 * i) >> 8 );
        RGB_g_y[i] = ((129 * i) >> 8) + 16 ;
        RGB_g_cb[i] = ((74 * i) >> 8) + 128 ;
        RGB_g_cr[i] = ((94 * i) >> 8) + 128;
        RGB_b_y[i] =  ((25 * i) >> 8);
        RGB_b_cr[i] = ((18 * i) >> 8);
    }
}
inline static void ConvertARGB888ToYVU420SemiPlanar_c(uint8_t *inrgb, uint8_t* outy,uint8_t* outuv,
        int32_t width_org, int32_t height_org, int32_t width_dst, int32_t height_dst) {
#define RGB2Y(_r, _g, _b)    (  *(RGB_r_y +_r)      +   *(RGB_g_y+_g)   +    *(RGB_b_y+_b))
#define RGB2CB(_r, _g, _b)   ( -*(RGB_r_cb +_r)     -   *(RGB_g_cb+_g)  +    *(RGB_r_cr_b_cb+_b))
#define RGB2CR(_r, _g, _b)   (  *(RGB_r_cr_b_cb +_r)-   *(RGB_g_cr+_g)  -    *(RGB_b_cr+_b))
    uint8_t *argb_ptr = inrgb;
    uint8_t *y_p = outy;
    uint8_t *vu_p = outuv;

    if (NULL == inrgb || NULL ==  outy || NULL ==  outuv)
        return;
    if (0 != (width_org & 1) || 0 != (height_org & 1))
        return;
    if(!mConventFlag) {
        mConventFlag = true;
        inittable();
    }
    ALOGI("rgb2yuv start");
    uint8_t *y_ptr;
    uint8_t *vu_ptr;
    int64_t start_encode = systemTime();
    uint32 i ;
    uint32 j = height_org + 1;
    while(--j) {
        //the width_dst may be bigger than width_org,
        //make start byte in every line of Y and CbCr align
        y_ptr = y_p;
        y_p += width_dst;
        if (!(j & 1))  {
            vu_ptr = vu_p;
            vu_p += width_dst;
            i  = width_org / 2 + 1;
            while(--i) {
                //format abgr, litter endian
                *y_ptr++    = RGB2Y(*argb_ptr, *(argb_ptr+1), *(argb_ptr+2));
                *vu_ptr++ =  RGB2CR(*argb_ptr, *(argb_ptr+1), *(argb_ptr+2));
                *vu_ptr++  = RGB2CB(*argb_ptr, *(argb_ptr+1), *(argb_ptr+2));
                *y_ptr++    = RGB2Y(*(argb_ptr + 4), *(argb_ptr+5), *(argb_ptr+6));
                argb_ptr += 8;
            }
        } else {
            i  = width_org + 1;
            while(--i) {
                //format abgr, litter endian
                *y_ptr++ = RGB2Y(*argb_ptr, *(argb_ptr+1), *(argb_ptr+2));
                argb_ptr += 4;
            }
        }
    }
    int64_t end_encode = systemTime();
    ALOGI("rgb2yuv time: %d",(unsigned int)((end_encode-start_encode) / 1000000L));
}
#else
/*this is neon assemble function.It is need width_org align in 16Bytes.height_org align in 2Bytes*/
/*in cpu not busy status,it deal with 1280*720 rgb data in 5-6ms */
extern "C" void neon_intrinsics_ARGB888ToYVU420Semi(uint8_t *inrgb, uint8_t* outy,uint8_t* outuv,
        int32_t width_org, int32_t height_org, int32_t width_dst, int32_t height_dst);

/*this is neon c function.It is need width_org align in 2Bytes.height_org align in 2Bytes*/
/*like ConvertARGB888ToYVU420SemiPlanar function parameters requirement*/
/*in cpu not busy status,it deal with 1280*720 rgb data in 5-6ms */
void neon_intrinsics_ARGB888ToYVU420Semi_c(uint8_t *inrgb, uint8_t* outy,uint8_t* outuv,
        int32_t width_org, int32_t height_org, int32_t width_dst, int32_t height_dst) {
    int32_t i, j;
    uint8_t *argb_ptr = inrgb;
    uint8_t *y_ptr = outy;
    uint8_t *temp_y_ptr = y_ptr;
    uint8_t *uv_ptr = outuv;
    uint8_t *argb_tmpptr ;
    uint8x8_t r1fac = vdup_n_u8(66);
    uint8x8_t g1fac = vdup_n_u8(129);
    uint8x8_t b1fac = vdup_n_u8(25);
    uint8x8_t r2fac = vdup_n_u8(38);
    uint8x8_t g2fac = vdup_n_u8(74);
    uint8x8_t b2fac = vdup_n_u8(112);
    // int8x8_t r3fac = vdup_n_s16(112);
    uint8x8_t g3fac = vdup_n_u8(94);
    uint8x8_t b3fac = vdup_n_u8(18);

    uint8x8_t y_base = vdup_n_u8(16);
    uint8x8_t uv_base = vdup_n_u8(128);

    bool needadjustPos = true;
    //due to width_dst is align in 16Bytes.so if width_org is align in 16bytes.no need adjust pos.
    if(width_org%16 == 0) {
        needadjustPos = false;
    }
    int32_t linecount=0;
    for (i=height_org; i>0; i--)    /////  line
    {
        for (j=(width_org>>3); j>0; j--)   ///// col
        {
            uint8 y, cb, cr;
            int8 r, g, b;
            uint8 p_r[16],p_g[16],p_b[16];
            uint16x8_t temp;
            uint8x8_t result;
            uint8x8x2_t result_uv;
            uint8x8_t result_u;
            uint8x8_t result_v;

            uint8x8x4_t argb = vld4_u8(argb_ptr);
            temp = vmull_u8(argb.val[0],r1fac);    ///////////////////////y  0,1,2
            temp = vmlal_u8(temp,argb.val[1],g1fac);
            temp = vmlal_u8(temp,argb.val[2],b1fac);
            result = vshrn_n_u16(temp,8);
            result = vadd_u8(result,y_base);
            vst1_u8(y_ptr,result);     ////*y_ptr = y;

            if(linecount%2==0) {
                temp = vmull_u8(argb.val[2],b2fac);    ///////////////////////cb
                temp = vmlsl_u8(temp,argb.val[1],g2fac);
                temp = vmlsl_u8(temp,argb.val[0],r2fac);
                result_u = vshrn_n_u16(temp,8);
                result_u = vadd_u8(result_u,uv_base);

                temp = vmull_u8(argb.val[0],b2fac);    ///////////////////////cr
                temp = vmlsl_u8(temp,argb.val[1],g3fac);
                temp = vmlsl_u8(temp,argb.val[2],b3fac);
                result_v = vshrn_n_u16(temp,8);
                result_v = vadd_u8(result_v,uv_base);

                /////uuuuuuuuvvvvvvvv -->> uvuvuvuvuvuvuvuvuv
                result_uv = vtrn_u8( result_v,result_u );
                vst1_u8(uv_ptr,result_uv.val[0]);
                uv_ptr += 8;
            }
            y_ptr += 8;
            argb_ptr += 32;
        }
        linecount++;
        if(needadjustPos) {
            //note:let y,uv,argb get correct position before operating next line.
            y_ptr = outy + width_dst*linecount;
            uv_ptr = outuv + width_dst*(linecount/2);
            argb_ptr = inrgb + 4*width_org*linecount;
        }
    }
}

inline static void ConvertARGB888ToYVU420SemiPlanar_neon(uint8_t *inrgb, uint8_t* outy,uint8_t* outuv,
        int32_t width_org, int32_t height_org, int32_t width_dst, int32_t height_dst) {
#define RGB2Y(_r, _g, _b) (((66 * (_r) + 129 * (_g) + 25 * (_b)) >> 8) + 16)
#define RGB2CB(_r, _g, _b) (((-38 * (_r) - 74 * (_g) + 112 * (_b)) >> 8) + 128)
#define RGB2CR(_r, _g, _b) (((112 * (_r) - 94 * (_g) - 18 * (_b)) >> 8) + 128)
    uint32_t i, j;
    uint32_t *argb_ptr = (uint32_t *)inrgb;
    uint8_t *y_ptr = outy;
    uint8_t *vu_ptr = outuv;

    if (NULL == inrgb || NULL == outuv || NULL==outy)
        return;

    if (height_org & 0x1 != 0)
        height_org &= ~0x1;

    if (width_org & 0x1 != 0) {
        ALOGE("width_org:%d is not supported", width_org);
        return;
    }

    int64_t start_encode = systemTime();
    neon_intrinsics_ARGB888ToYVU420Semi_c(inrgb,  y_ptr, vu_ptr, //  1280*720  =>  22ms in padv2
                                          width_org,  height_org,  width_dst,  height_dst);
    int64_t end_encode = systemTime();
    ALOGI("wfd: ConvertARGB888ToYVU420SemiPlanar_neon:  rgb2yuv cost time: %d",
          (unsigned int)((end_encode-start_encode) / 1000000L));
}
#endif

inline static void ConvertARGB888ToYVU420SemiPlanar(uint8_t *inrgb, uint8_t* outy,uint8_t* outuv,
                    int32_t width_org, int32_t height_org, int32_t width_dst, int32_t height_dst) {
#ifdef CONFIG_BIA_SUPPORT
ConvertARGB888ToYVU420SemiPlanar_c(inrgb, outy, outuv, width_org, height_org, width_dst, height_dst);
#else
ConvertARGB888ToYVU420SemiPlanar_neon(inrgb, outy, outuv, width_org, height_org, width_dst, height_dst);
#endif
}

#ifdef VIDEOENC_CURRENT_OPT
inline static void set_ddr_freq(const char* freq_in_khz) {
    const char* const set_freq = "/sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_freq";

    FILE* fp = fopen(set_freq, "w");
    if (fp != NULL) {
        fprintf(fp, "%s", freq_in_khz);
        ALOGE("set ddr freq to %skhz", freq_in_khz);
        fclose(fp);
    } else {
        ALOGE("Failed to open %s", set_freq);
    }
}
#endif

SPRDHEVCEncoder::SPRDHEVCEncoder(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mHandle(new tagHEVCHandle),
      mEncParams(new tagHEVCEncParam),
      mEncConfig(new MMEncConfig),
      mSliceGroup(NULL),
      mNumInputFrames(-1),
      mPrevTimestampUs(-1),
      mSetFreqCount(0),
      mEncSceneMode(0),
      mSetEncMode(false),
      mVideoWidth(176),
      mVideoHeight(144),
      mVideoFrameRate(30),
      mVideoBitRate(192000),
      mVideoColorFormat(OMX_SPRD_COLOR_FormatYVU420SemiPlanar),
      mStoreMetaData(OMX_FALSE),
      mPrependSPSPPS(OMX_FALSE),
      mIOMMUEnabled(false),
      mIOMMUID(-1),
      mStarted(false),
      mSpsPpsHeaderReceived(false),
      mReadyForNextFrame(true),
      mSawInputEOS(false),
      mSignalledError(false),
      mKeyFrameRequested(false),
      mPbuf_inter(NULL),
      mPbuf_yuv_v(NULL),
      mPbuf_yuv_p(0),
      mPbuf_yuv_size(0),
      mPbuf_stream_v(NULL),
      mPbuf_stream_p(0),
      mPbuf_stream_size(0),
      mPbuf_extra_v(NULL),
      mPbuf_extra_p(0),
      mPbuf_extra_size(0),
      mHEVCEncProfile(AVC_BASELINE),
      mHEVCEncLevel(AVC_LEVEL2),
      mPFrames(29),
      mNeedAlign(true),
      mLibHandle(NULL),
      mH265EncPreInit(NULL),
      mH265EncInit(NULL),
      mH265EncSetConf(NULL),
      mH265EncGetConf(NULL),
      mH265EncStrmEncode(NULL),
      mH265EncGenHeader(NULL),
      mH265EncRelease(NULL),
      mH265EncGetCodecCapability(NULL),
      mH265EncGetIOVA(NULL),
      mH265EncFreeIOVA(NULL),
      mH265EncGetIOMMUStatus(NULL),
      mH265Enc_NeedAlign(NULL),
      mFrameWidth(176),
      mFrameHeight(144),
      mSupportRGBEnc(false) {

    ALOGI("Construct SPRDHEVCEncoder, this: 0x%p", (void *)this);

    CHECK(mHandle != NULL);
    memset(mHandle, 0, sizeof(tagHEVCHandle));

    mHandle->videoEncoderData = NULL;
    mHandle->userData = this;
    mHandle->VSP_FlushBSCache = FlushCacheWrapper;

    memset(&mEncInfo, 0, sizeof(mEncInfo));
#ifdef CONFIG_SPRD_RECORD_EIS
    mEISMode = false;
#endif

    CHECK_EQ(openEncoder("libomx_hevcenc_hw_sprd.so"), true);

    ALOGI("%s, line:%d, name: %s", __FUNCTION__, __LINE__, name);

    MMCodecBuffer InterMemBfr;
    uint32_t size_inter = H265ENC_INTERNAL_BUFFER_SIZE;

    mPbuf_inter = (uint8_t *)malloc(size_inter);
    CHECK(mPbuf_inter != NULL);
    InterMemBfr.common_buffer_ptr = mPbuf_inter;
    InterMemBfr.common_buffer_ptr_phy= 0;
    InterMemBfr.size = size_inter;

    CHECK_EQ((*mH265EncPreInit)(mHandle, &InterMemBfr), MMENC_OK);

    CHECK_EQ ((*mH265EncGetCodecCapability)(mHandle, &mCapability), MMENC_OK);
#ifdef CONFIG_RGB_ENC_SUPPORT
    mSupportRGBEnc = true;
#endif
    int ret = (*mH265Enc_NeedAlign)(mHandle);
    ALOGI("Enc needAign:%d",ret);
    if (ret == 0) {
        mNeedAlign = false;
    }
    ret = (*mH265EncGetIOMMUStatus)(mHandle);
    ALOGI("Get IOMMU Status: %d(%s)", ret, strerror(errno));
    if (ret < 0) {
        mIOMMUEnabled = false;
    } else {
        mIOMMUEnabled = true;
    }

    initPorts();
    ALOGI("Construct SPRDHEVCEncoder, Capability: profile %d, level %d, max wh=%d %d",
          mCapability.profile, mCapability.level, mCapability.max_width, mCapability.max_height);

#ifdef SPRD_DUMP_YUV
    mFile_yuv = fopen("/data/dump/video_in.yuv", "ab");
#endif

#ifdef SPRD_DUMP_BS
    mFile_bs = fopen("/data/misc/media/video_en.h265", "wb");
#endif
}

SPRDHEVCEncoder::~SPRDHEVCEncoder() {
    ALOGI("Destruct SPRDHEVCEncoder, this: 0x%p", (void *)this);

    releaseResource();

    releaseEncoder();

    List<BufferInfo *> &outQueue = getPortQueue(1);
    List<BufferInfo *> &inQueue = getPortQueue(0);
    CHECK(outQueue.empty());
    CHECK(inQueue.empty());

    if (mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }

#ifdef SPRD_DUMP_YUV
    if (mFile_yuv) {
        fclose(mFile_yuv);
        mFile_yuv = NULL;
    }
#endif

#ifdef SPRD_DUMP_BS
    if (mFile_bs) {
        fclose(mFile_bs);
        mFile_bs = NULL;
    }
#endif
}

OMX_ERRORTYPE SPRDHEVCEncoder::initEncParams() {
    CHECK(mEncConfig != NULL);
    memset(mEncConfig, 0, sizeof(MMEncConfig));

    CHECK(mEncParams != NULL);
    memset(mEncParams, 0, sizeof(tagHEVCEncParam));
    mEncParams->rate_control = HEVC_ON;
    mEncParams->initQP = 0;
    mEncParams->init_CBP_removal_delay = 1600;

    mEncParams->intramb_refresh = 0;
    mEncParams->auto_scd = HEVC_ON;
    mEncParams->out_of_band_param_set = HEVC_ON;
    mEncParams->poc_type = 2;
    mEncParams->log2_max_poc_lsb_minus_4 = 12;
    mEncParams->delta_poc_zero_flag = 0;
    mEncParams->offset_poc_non_ref = 0;
    mEncParams->offset_top_bottom = 0;
    mEncParams->num_ref_in_cycle = 0;
    mEncParams->offset_poc_ref = NULL;

    mEncParams->num_ref_frame = 1;
    mEncParams->num_slice_group = 1;
    mEncParams->fmo_type = 0;

    mEncParams->db_filter = HEVC_ON;
    mEncParams->disable_db_idc = 0;

    mEncParams->alpha_offset = 0;
    mEncParams->beta_offset = 0;
    mEncParams->constrained_intra_pred = HEVC_OFF;

    mEncParams->data_par = HEVC_OFF;
    mEncParams->fullsearch = HEVC_OFF;
    mEncParams->search_range = 16;
    mEncParams->sub_pel = HEVC_OFF;
    mEncParams->submb_pred = HEVC_OFF;
    mEncParams->rdopt_mode = HEVC_OFF;
    mEncParams->bidir_pred = HEVC_OFF;

    mEncParams->use_overrun_buffer = HEVC_OFF;

#ifdef VIDEOENC_CURRENT_OPT
    if (((mVideoWidth <= 720) && (mVideoHeight <= 480)) || ((mVideoWidth <= 480) && (mVideoHeight <= 720))) {
        set_ddr_freq("200000");
        mSetFreqCount ++;
    } else {
        set_ddr_freq("300000");
        mSetFreqCount ++;
    }
#endif

    MMCodecBuffer ExtraMemBfr;
    MMCodecBuffer StreamMemBfr, StreamMemBfrPn;
    unsigned long phy_addr = 0;
    size_t size = 0;
    size_t size_of_yuv = ((mVideoWidth+31)&(~31)) * ((mVideoHeight+31)&(~31)) * 3/2;
    size_t size_stream = size_of_yuv/3;

    //iwhale2
    if ((mVideoWidth >= 1280) && (mVideoHeight >= 720)) {
        int32_t align_width = ((mVideoWidth+31)&(~31));
        int32_t align_height = ((mVideoHeight+31)&(~31));
        int32_t line_word = (align_width > 2048) ? 128 : 64;
        int32_t Y_infor_offset = align_height * line_word;
        int32_t UV_infor_offset = (((align_height/2 +31) >> 5) << 5) * line_word;

        size_of_yuv += Y_infor_offset;
        size_of_yuv += UV_infor_offset;
    }

    size_t size_extra = size_of_yuv << 1;
    size_extra += (406*2*sizeof(uint32));
    size_extra += 100*1024;
#ifdef CONFIG_SPRD_RECORD_EIS
    if (mEISMode) {
        size_extra += 1024*5;
    }
#endif
    if (mIOMMUEnabled) {
        mPmem_extra = new MemIon("/dev/ion", size_extra, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        mPmem_extra = new MemIon("/dev/ion", size_extra, MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }

    int fd = mPmem_extra->getHeapID();
    if (fd < 0) {
        ALOGE("Failed to alloc extra buffer (%zd), getHeapID failed", size_extra);
        return OMX_ErrorInsufficientResources;
    }

    int ret;
    if (mIOMMUEnabled) {
        ret = (*mH265EncGetIOVA)(mHandle, fd, &phy_addr, &size);
    } else {
        ret = mPmem_extra->get_phy_addr_from_ion(&phy_addr, &size);
    }
    if (ret < 0) {
        ALOGE("Failed to alloc extra buffer, get phy addr failed");
        return OMX_ErrorInsufficientResources;
    }

    mPbuf_extra_v = (uint8_t*)mPmem_extra->getBase();
    mPbuf_extra_p = phy_addr;
    mPbuf_extra_size = size;

    if (mIOMMUEnabled) {
        mPmem_stream = new MemIon("/dev/ion", size_stream, 0 , (1<<31) |ION_HEAP_ID_MASK_SYSTEM);
    } else {
        mPmem_stream = new MemIon("/dev/ion", size_stream, 0 , (1<<31) |ION_HEAP_ID_MASK_MM);
    }

    fd = mPmem_stream->getHeapID();
    if (fd < 0) {
        ALOGE("Failed to alloc stream buffer (%zd), getHeapID failed", size_stream);
        return OMX_ErrorInsufficientResources;
    }

    if (mIOMMUEnabled) {
        ret = (*mH265EncGetIOVA)(mHandle, fd, &phy_addr, &size);
    } else {
        ret = mPmem_stream->get_phy_addr_from_ion(&phy_addr, &size);
    }
    if (ret < 0) {
        ALOGE("Failed to alloc stream buffer, get phy addr failed");
        return OMX_ErrorInsufficientResources;
    }

    mPbuf_stream_v = (uint8_t*)mPmem_stream->getBase();
    mPbuf_stream_p = phy_addr;
    mPbuf_stream_size = size;

    if (mIOMMUEnabled) {
        mPmem_streamPn = new MemIon("/dev/ion", size_stream, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        mPmem_streamPn = new MemIon("/dev/ion", size_stream, MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }

    fd = mPmem_streamPn->getHeapID();
    if (fd < 0) {
        ALOGE("Failed to alloc stream buffer (%zd), getHeapID failed", size_stream);
        return OMX_ErrorInsufficientResources;
    }

    if (mIOMMUEnabled) {
        ret = (*mH265EncGetIOVA)(mHandle, fd, &phy_addr, &size);
    } else {
        ret = mPmem_streamPn->get_phy_addr_from_ion(&phy_addr, &size);
    }
    if (ret < 0) {
        ALOGE("Failed to alloc stream buffer, get phy addr failed");
        return OMX_ErrorInsufficientResources;
    }

    mPbuf_stream_v_pn = (uint8_t*)mPmem_streamPn->getBase();
    mPbuf_stream_p_pn = phy_addr;
    mPbuf_stream_size_pn = size;

    ExtraMemBfr.common_buffer_ptr = mPbuf_extra_v;
    ExtraMemBfr.common_buffer_ptr_phy = mPbuf_extra_p;
    ExtraMemBfr.size = size_extra;

    StreamMemBfr.common_buffer_ptr = mPbuf_stream_v;
    StreamMemBfr.common_buffer_ptr_phy = mPbuf_stream_p;
    StreamMemBfr.size = size_stream;

    StreamMemBfrPn.common_buffer_ptr = mPbuf_stream_v_pn;
    StreamMemBfrPn.common_buffer_ptr_phy = mPbuf_stream_p_pn;
    StreamMemBfrPn.size = size_stream;
    mEncInfo.frame_width = mFrameWidth;
    mEncInfo.frame_height = mFrameHeight;
    mEncInfo.EISMode = false;
#ifdef CONFIG_SPRD_RECORD_EIS
    mEncInfo.EISMode = mEISMode;
#endif
    mEncInfo.is_h263 = 0;
    mEncInfo.org_width = mVideoWidth;
    mEncInfo.org_height = mVideoHeight;
    if (mVideoColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) {
        mEncInfo.yuv_format = MMENC_YUV420SP_NV12;
    } else {
        mEncInfo.yuv_format = MMENC_YUV420SP_NV21;
    }

    mEncInfo.time_scale = 1000;

    if ((*mH265EncInit)(mHandle, &ExtraMemBfr,&StreamMemBfr, &StreamMemBfrPn, &mEncInfo)) {
        ALOGE("Failed to init mp4enc");
        return OMX_ErrorUndefined;
    }

    if ((*mH265EncGetConf)(mHandle, mEncConfig)) {
        ALOGE("Failed to get default encoding parameters");
        return OMX_ErrorUndefined;
    }

    mEncConfig->h263En = 0;
    mEncConfig->RateCtrlEnable = 1; //cbr
    mEncConfig->targetBitRate = mVideoBitRate;
    mEncConfig->FrameRate = mVideoFrameRate;
    mEncConfig->PFrames = mPFrames;
    if (mEncSceneMode == 0) {
        mEncConfig->QP_IVOP = 28;//qp
    } else {
        mEncConfig->QP_IVOP = 36;
    }
    mEncConfig->QP_PVOP = 28;//qp
    mEncConfig->vbv_buf_size = mVideoBitRate/2;
    mEncConfig->profileAndLevel = 1;
    mEncConfig->PrependVPSSPSPPSEnalbe = ((mPrependSPSPPS == OMX_FALSE) ? 0 : 1);
    mEncConfig->EncSceneMode = mEncSceneMode;
    mEncConfig->CTUSize_Slice = 0;
    mEncConfig->openPnmode = 0;

    if ((*mH265EncSetConf)(mHandle, mEncConfig)) {
        ALOGE("Failed to set default encoding parameters");
        return OMX_ErrorUndefined;
    }

    mEncParams->width = mVideoWidth;
    mEncParams->height = mVideoHeight;
    mEncParams->bitrate = mVideoBitRate;
    mEncParams->frame_rate = 1000 * mVideoFrameRate;  // In frames/ms!
    mEncParams->CPB_size = (uint32_t) (mVideoBitRate >> 1);

    // Set profile and level
    mEncParams->profile = mHEVCEncProfile;
    mEncParams->level = mHEVCEncLevel;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCEncoder::initEncoder() {
    CHECK(!mStarted);

    OMX_ERRORTYPE errType = OMX_ErrorNone;
    if (OMX_ErrorNone != (errType = initEncParams())) {
        ALOGE("Failed to initialized encoder params");
        mSignalledError = true;
        notify(OMX_EventError, OMX_ErrorUndefined, 0, 0);
        return errType;
    }
    mNumInputFrames = -3;  // 1st two buffers contain SPS and PPS
    mSpsPpsHeaderReceived = false;
    mReadyForNextFrame = true;
    mStarted = true;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCEncoder::releaseEncoder() {

    (*mH265EncRelease)(mHandle);

    if (mPbuf_inter != NULL) {
        free(mPbuf_inter);
        mPbuf_inter = NULL;
    }

    delete mEncParams;
    mEncParams = NULL;

    delete mEncConfig;
    mEncConfig = NULL;

    delete mHandle;
    mHandle = NULL;

    mStarted = false;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDHEVCEncoder::releaseResource() {

    if (mPbuf_extra_v != NULL) {
        if (mIOMMUEnabled) {
            (*mH265EncFreeIOVA)(mHandle, mPbuf_extra_p, mPbuf_extra_size);
        }
        mPmem_extra.clear();
        mPbuf_extra_v = NULL;
        mPbuf_extra_p = 0;
        mPbuf_extra_size = 0;
    }
    if (mPbuf_stream_v_pn != NULL) {
        if (mIOMMUEnabled) {
            (*mH265EncFreeIOVA)(mHandle, mPbuf_stream_p_pn, mPbuf_stream_size_pn);
        }
        mPmem_streamPn.clear();
        mPbuf_stream_v_pn = NULL;
        mPbuf_stream_p_pn = 0;
        mPbuf_stream_size_pn = 0;
    }

    if (mPbuf_stream_v != NULL) {
        if (mIOMMUEnabled) {
            (*mH265EncFreeIOVA)(mHandle, mPbuf_stream_p, mPbuf_stream_size);
        }
        mPmem_stream.clear();
        mPbuf_stream_v = NULL;
        mPbuf_stream_p = 0;
        mPbuf_stream_size = 0;
    }

    if (mPbuf_yuv_v != NULL) {
        if (mIOMMUEnabled) {
            (*mH265EncFreeIOVA)(mHandle, mPbuf_yuv_p, mPbuf_yuv_size);
        }
        mYUVInPmemHeap.clear();
        mPbuf_yuv_v = NULL;
        mPbuf_yuv_p = 0;
        mPbuf_yuv_size = 0;
    }

#ifdef VIDEOENC_CURRENT_OPT
    while (mSetFreqCount > 0) {
        set_ddr_freq("0");
        mSetFreqCount --;
    }
#endif

    return OMX_ErrorNone;
}

void SPRDHEVCEncoder::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    const size_t kInputBufferSize = (mVideoWidth * mVideoHeight * 3) >> 1;

    // 31584 is PV's magic number.  Not sure why.
    const size_t kOutputBufferSize =
        (kInputBufferSize > 31584) ? kInputBufferSize: 31584;

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = kInputBufferSize;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.video.cMIMEType = const_cast<char *>("video/raw");
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    def.format.video.eColorFormat = OMX_SPRD_COLOR_FormatYVU420SemiPlanar;
    def.format.video.xFramerate = (mVideoFrameRate << 16);  // Q16 format
    def.format.video.nBitrate = mVideoBitRate;
    def.format.video.nFrameWidth = mVideoWidth;
    def.format.video.nFrameHeight = mVideoHeight;
    def.format.video.nStride = mVideoWidth;
    def.format.video.nSliceHeight = mVideoHeight;

    addPort(def);

    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = kOutputBufferSize;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;

    def.format.video.cMIMEType = const_cast<char *>("video/hevc");
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingHEVC;
    def.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    def.format.video.xFramerate = (0 << 16);  // Q16 format
    def.format.video.nBitrate = mVideoBitRate;
    def.format.video.nFrameWidth = mVideoWidth;
    def.format.video.nFrameHeight = mVideoHeight;
    def.format.video.nStride = mVideoWidth;
    def.format.video.nSliceHeight = mVideoHeight;

    addPort(def);
}

OMX_ERRORTYPE SPRDHEVCEncoder::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamVideoErrorCorrection:
    {
        return OMX_ErrorNotImplemented;
    }

    case OMX_IndexParamVideoBitrate:
    {
        OMX_VIDEO_PARAM_BITRATETYPE *bitRate =
            (OMX_VIDEO_PARAM_BITRATETYPE *) params;

        if (bitRate->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        bitRate->eControlRate = OMX_Video_ControlRateVariable;
        bitRate->nTargetBitrate = mVideoBitRate;
        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > 1) {
            return OMX_ErrorUndefined;
        }

        if (formatParams->nIndex > 3) {
            return OMX_ErrorNoMore;
        }

        if (formatParams->nPortIndex == 0) {
            formatParams->eCompressionFormat = OMX_VIDEO_CodingUnused;
            if (formatParams->nIndex == 0) {
                formatParams->eColorFormat = OMX_SPRD_COLOR_FormatYVU420SemiPlanar;
            } else if (formatParams->nIndex == 1) {
                formatParams->eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            } else if (formatParams->nIndex == 2) {
                formatParams->eColorFormat = OMX_COLOR_FormatYUV420Planar;
            } else {
                formatParams->eColorFormat = OMX_COLOR_FormatAndroidOpaque;
            }
        } else {
            formatParams->eCompressionFormat = OMX_VIDEO_CodingHEVC;
            formatParams->eColorFormat = OMX_COLOR_FormatUnused;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoHevc:
    {
        OMX_VIDEO_PARAM_HEVCTYPE *hevcParams =
            (OMX_VIDEO_PARAM_HEVCTYPE *)params;

        if (hevcParams->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        hevcParams->eProfile = OMX_VIDEO_HEVCProfileMain;
        hevcParams->eLevel = OMX_VIDEO_HEVCMainTierLevel5;

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel =
            (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)params;

        if (profileLevel->nPortIndex != 1) {
            return OMX_ErrorUndefined;
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
        //ALOGI("Query supported profile level = %d, %d",profileLevel->eProfile, profileLevel->eLevel);
        return OMX_ErrorNone;
    }

    case OMX_IndexParamStoreMetaDataBuffer:
    {
        StoreMetaDataInBuffersParams *pStoreMetaData = (StoreMetaDataInBuffersParams *)params;
        if (pStoreMetaData->nPortIndex != 0) {
            ALOGE("%s: StoreMetadataInBuffersParams.nPortIndex not zero!",
                  __FUNCTION__);
            return OMX_ErrorUndefined;
        }

        pStoreMetaData->bStoreMetaData = mStoreMetaData;
        return OMX_ErrorNone;
    }

    case OMX_IndexParamDescribeColorFormat:
    {
        DescribeColorFormatParams *pDescribeColorFormat = (DescribeColorFormatParams *)params;

        MediaImage &image = pDescribeColorFormat->sMediaImage;
        memset(&image, 0, sizeof(image));

        image.mType = MediaImage::MEDIA_IMAGE_TYPE_UNKNOWN;
        image.mNumPlanes = 0;

        const OMX_COLOR_FORMATTYPE fmt = pDescribeColorFormat->eColorFormat;
        image.mWidth = pDescribeColorFormat->nFrameWidth;
        image.mHeight = pDescribeColorFormat->nFrameHeight;

        ALOGI("%s, DescribeColorFormat: 0x%x, w h = %d %d", __FUNCTION__,
              pDescribeColorFormat->eColorFormat,
              pDescribeColorFormat->nFrameWidth, pDescribeColorFormat->nFrameHeight);

        if (fmt != OMX_SPRD_COLOR_FormatYVU420SemiPlanar &&
                fmt != OMX_COLOR_FormatYUV420SemiPlanar &&
                fmt != OMX_COLOR_FormatYUV420Planar) {
            ALOGW("do not know color format 0x%x = %d", fmt, fmt);
            return OMX_ErrorUnsupportedSetting;
        }

        // TEMPORARY FIX for some vendors that advertise sliceHeight as 0
        if (pDescribeColorFormat->nStride != 0 && pDescribeColorFormat->nSliceHeight == 0) {
            ALOGW("using sliceHeight=%u instead of what codec advertised (=0)",
                  pDescribeColorFormat->nFrameHeight);
            pDescribeColorFormat->nSliceHeight = pDescribeColorFormat->nFrameHeight;
        }

        // we need stride and slice-height to be non-zero
        if (pDescribeColorFormat->nStride == 0 || pDescribeColorFormat->nSliceHeight == 0) {
            ALOGW("cannot describe color format 0x%x = %d with stride=%u and sliceHeight=%u",
                  fmt, fmt, pDescribeColorFormat->nStride, pDescribeColorFormat->nSliceHeight);
            return OMX_ErrorBadParameter;
        }

        // set-up YUV format
        image.mType = MediaImage::MEDIA_IMAGE_TYPE_YUV;
        image.mNumPlanes = 3;
        image.mBitDepth = 8;
        image.mPlane[image.Y].mOffset = 0;
        image.mPlane[image.Y].mColInc = 1;
        image.mPlane[image.Y].mRowInc = pDescribeColorFormat->nStride;
        image.mPlane[image.Y].mHorizSubsampling = 1;
        image.mPlane[image.Y].mVertSubsampling = 1;

        switch (fmt) {
        case OMX_SPRD_COLOR_FormatYVU420SemiPlanar:
            // NV21
            image.mPlane[image.V].mOffset = pDescribeColorFormat->nStride*pDescribeColorFormat->nSliceHeight;
            image.mPlane[image.V].mColInc = 2;
            image.mPlane[image.V].mRowInc = pDescribeColorFormat->nStride;
            image.mPlane[image.V].mHorizSubsampling = 2;
            image.mPlane[image.V].mVertSubsampling = 2;

            image.mPlane[image.U].mOffset = image.mPlane[image.V].mOffset + 1;
            image.mPlane[image.U].mColInc = 2;
            image.mPlane[image.U].mRowInc = pDescribeColorFormat->nStride;
            image.mPlane[image.U].mHorizSubsampling = 2;
            image.mPlane[image.U].mVertSubsampling = 2;
            break;

        case OMX_COLOR_FormatYUV420SemiPlanar:
            // FIXME: NV21 for sw-encoder, NV12 for decoder and hw-encoder
            // NV12
            image.mPlane[image.U].mOffset = pDescribeColorFormat->nStride*pDescribeColorFormat->nSliceHeight;
            image.mPlane[image.U].mColInc = 2;
            image.mPlane[image.U].mRowInc = pDescribeColorFormat->nStride;
            image.mPlane[image.U].mHorizSubsampling = 2;
            image.mPlane[image.U].mVertSubsampling = 2;

            image.mPlane[image.V].mOffset = image.mPlane[image.U].mOffset + 1;
            image.mPlane[image.V].mColInc = 2;
            image.mPlane[image.V].mRowInc = pDescribeColorFormat->nStride;
            image.mPlane[image.V].mHorizSubsampling = 2;
            image.mPlane[image.V].mVertSubsampling = 2;
            break;

        case OMX_COLOR_FormatYUV420Planar: // used for YV12
            image.mPlane[image.U].mOffset = pDescribeColorFormat->nStride*pDescribeColorFormat->nSliceHeight;
            image.mPlane[image.U].mColInc = 1;
            image.mPlane[image.U].mRowInc = pDescribeColorFormat->nStride / 2;
            image.mPlane[image.U].mHorizSubsampling = 2;
            image.mPlane[image.U].mVertSubsampling = 2;

            image.mPlane[image.V].mOffset = image.mPlane[image.U].mOffset
                                            + (pDescribeColorFormat->nStride * pDescribeColorFormat->nSliceHeight / 4);
            image.mPlane[image.V].mColInc = 1;
            image.mPlane[image.V].mRowInc = pDescribeColorFormat->nStride / 2;
            image.mPlane[image.V].mHorizSubsampling = 2;
            image.mPlane[image.V].mVertSubsampling = 2;
            break;

        default:
            TRESPASS();
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoInit:
    {
        OMX_PORT_PARAM_TYPE* pParam = (OMX_PORT_PARAM_TYPE*)params;
        pParam->nPorts = 2;
        pParam->nStartPortNumber = 0;
        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDHEVCEncoder::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamVideoErrorCorrection:
    {
        return OMX_ErrorNotImplemented;
    }

    case OMX_IndexConfigVideoRecordMode:
    {
        OMX_BOOL *pEnable = (OMX_BOOL *)params;
#ifdef CONFIG_SPRD_RECORD_EIS
        if (*pEnable == 1) {
            mEISMode = OMX_TRUE;
        }
        ALOGI("record eismode=%d",mEISMode);
#endif
        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoBitrate:
    {
        OMX_VIDEO_PARAM_BITRATETYPE *bitRate =
            (OMX_VIDEO_PARAM_BITRATETYPE *) params;

        if (bitRate->nPortIndex != 1 ||
                (bitRate->eControlRate != OMX_Video_ControlRateVariable &&
                bitRate->eControlRate != OMX_Video_ControlRateConstant)) {
            return OMX_ErrorUndefined;
        }

        mVideoBitRate = bitRate->nTargetBitrate;
        if(bitRate->eControlRate == OMX_Video_ControlRateConstant) {
            //for samsung, samsung set cbr instead of mEncSceneMode.
            //for sprd, volte set mEncSceneMode, wfd set mEncSceneMode and cbr.
            if (!mSetEncMode) {
                mEncSceneMode = 1;  //encode in volte mode.
            }
        }
        return OMX_ErrorNone;
    }

    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *def =
            (OMX_PARAM_PORTDEFINITIONTYPE *)params;
        if (def->nPortIndex > 1) {
            return OMX_ErrorUndefined;
        }

        if (def->nPortIndex == 0) {
            if (def->format.video.eCompressionFormat != OMX_VIDEO_CodingUnused ||
                    (def->format.video.eColorFormat != OMX_COLOR_FormatYUV420Flexible &&
                     def->format.video.eColorFormat != OMX_SPRD_COLOR_FormatYVU420SemiPlanar &&
                     def->format.video.eColorFormat != OMX_COLOR_FormatYUV420SemiPlanar &&
                     def->format.video.eColorFormat != OMX_COLOR_FormatYUV420Planar &&
                     def->format.video.eColorFormat != OMX_COLOR_FormatAndroidOpaque)) {
                return OMX_ErrorUndefined;
            }
        } else {
            if (def->format.video.eCompressionFormat != OMX_VIDEO_CodingHEVC ||
                    (def->format.video.eColorFormat != OMX_COLOR_FormatUnused)) {
                return OMX_ErrorUndefined;
            }
        }
        int32_t stride, sliceHeight;
        if (mNeedAlign) {
            sliceHeight = (def->format.video.nFrameHeight+15)&(~15);
        } else {
            sliceHeight = def->format.video.nFrameHeight;
        }
        stride = (def->format.video.nFrameWidth+15)&(~15);
        // As encoder we should modify bufferSize on Input port
        // Make sure we have enough input date for input buffer
        if(def->nPortIndex <= 1) {
            uint32_t bufferSize = stride * sliceHeight * 3/2;
            if(bufferSize > def->nBufferSize) {
                def->nBufferSize = bufferSize;
            }
        }
        //translate Flexible 8-bit YUV format to our default YUV format
        if (def->format.video.eColorFormat == OMX_COLOR_FormatYUV420Flexible) {
            ALOGI("internalSetParameter, translate Flexible 8-bit YUV format to SPRD YVU420SemiPlanar");
            def->format.video.eColorFormat = OMX_SPRD_COLOR_FormatYVU420SemiPlanar;
        }

        OMX_ERRORTYPE err = SprdSimpleOMXComponent::internalSetParameter(index, params);
        if (OMX_ErrorNone != err) {
            return err;
        }

        if (def->nPortIndex == 0) {
            mFrameWidth = def->format.video.nFrameWidth;
            mFrameHeight = def->format.video.nFrameHeight;
            mVideoWidth = stride;
            mVideoHeight = sliceHeight;
            mVideoFrameRate = def->format.video.xFramerate >> 16;
            mVideoColorFormat = def->format.video.eColorFormat;
        } else {
            mVideoBitRate = def->format.video.nBitrate;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamStandardComponentRole:
    {
        const OMX_PARAM_COMPONENTROLETYPE *roleParams =
            (const OMX_PARAM_COMPONENTROLETYPE *)params;

        if (strncmp((const char *)roleParams->cRole,
                    "video_encoder.hevc",
                    OMX_MAX_STRINGNAME_SIZE - 1)) {
            return OMX_ErrorUndefined;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoPortFormat:
    {
        const OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (const OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > 1) {
            return OMX_ErrorUndefined;
        }

        if (formatParams->nPortIndex == 0) {
            if (formatParams->eCompressionFormat != OMX_VIDEO_CodingUnused ||
                    (formatParams->eColorFormat != OMX_SPRD_COLOR_FormatYVU420SemiPlanar &&
                     formatParams->eColorFormat != OMX_COLOR_FormatYUV420SemiPlanar &&
                     formatParams->eColorFormat != OMX_COLOR_FormatYUV420Planar &&
                     formatParams->eColorFormat != OMX_COLOR_FormatAndroidOpaque)) {
                return OMX_ErrorUndefined;
            }
            mVideoColorFormat = formatParams->eColorFormat;
        } else {
            if (formatParams->eCompressionFormat != OMX_VIDEO_CodingHEVC ||
                    formatParams->eColorFormat != OMX_COLOR_FormatUnused) {
                return OMX_ErrorUndefined;
            }
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoHevc:
    {
        OMX_VIDEO_PARAM_HEVCTYPE *hevcType =
            (OMX_VIDEO_PARAM_HEVCTYPE *)params;

        if (hevcType->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        if (OMX_VIDEO_HEVCProfileMain != hevcType->eProfile)
            return OMX_ErrorUndefined;

        return OMX_ErrorNone;
    }

    case OMX_IndexParamStoreMetaDataBuffer:
    {
        StoreMetaDataInBuffersParams *pStoreMetaData = (StoreMetaDataInBuffersParams *)params;
        if (pStoreMetaData->nPortIndex != 0) {
            ALOGE("%s: StoreMetadataInBuffersParams.nPortIndex not zero!",
                  __FUNCTION__);
            return OMX_ErrorUndefined;
        }

        mStoreMetaData = pStoreMetaData->bStoreMetaData;
        ALOGV("StoreMetaDataInBuffers set to: %s",
              mStoreMetaData ? " true" : "false");
        return OMX_ErrorNone;
    }

    case OMX_IndexParamPrependSPSPPSToIDR:
    {
        PrependSPSPPSToIDRFramesParams *pPrependSPSPPS = (PrependSPSPPSToIDRFramesParams *)params;
        mPrependSPSPPS = pPrependSPSPPS->bEnable; // currently avcenc driver will PrependSPSPPS defaultly
        ALOGI("OMX_IndexParamPrependSPSPPSToIDR return ok");
        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoIntraRefresh:
    {
        return OMX_ErrorNone;   ///hw encoder may not support this mode
    }

    default:
        return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDHEVCEncoder::setConfig(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexConfigVideoIntraVOPRefresh:
    {
        OMX_CONFIG_INTRAREFRESHVOPTYPE *pConfigIntraRefreshVOP =
            (OMX_CONFIG_INTRAREFRESHVOPTYPE *)params;

        if (pConfigIntraRefreshVOP->nPortIndex != kOutputPortIndex) {
            return OMX_ErrorBadPortIndex;
        }

        mKeyFrameRequested = pConfigIntraRefreshVOP->IntraRefreshVOP;
        return OMX_ErrorNone;
    }

    case OMX_IndexConfigEncSceneMode:
    {
        int *pEncSceneMode = (int *)params;
        mEncSceneMode = *pEncSceneMode;
        mSetEncMode = true;
        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::setConfig(index, params);
    }
}

OMX_ERRORTYPE SPRDHEVCEncoder::getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index) {
    if(strcmp(name, "OMX.google.android.index.storeMetaDataInBuffers") == 0) {
        *index = (OMX_INDEXTYPE) OMX_IndexParamStoreMetaDataBuffer;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.google.android.index.describeColorFormat") == 0) {
        *index = (OMX_INDEXTYPE) OMX_IndexParamDescribeColorFormat;
        return OMX_ErrorNone;
    } else if(strcmp(name, "OMX.google.android.index.prependSPSPPSToIDRFrames") == 0) {
        *index = (OMX_INDEXTYPE) OMX_IndexParamPrependSPSPPSToIDR;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.sprd.index.RecordMode") == 0) {
        *index = (OMX_INDEXTYPE) OMX_IndexConfigVideoRecordMode;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.sprd.index.EncSceneMode") == 0) {
        *index = (OMX_INDEXTYPE) OMX_IndexConfigEncSceneMode;
        return OMX_ErrorNone;
    }

    return SprdSimpleOMXComponent::getExtensionIndex(name, index);
}

// static
void SPRDHEVCEncoder::FlushCacheWrapper(void* aUserData) {
    static_cast<SPRDHEVCEncoder *>(aUserData)->flushCacheforBSBuf();

    return;
}

void SPRDHEVCEncoder::flushCacheforBSBuf() {
    mPmem_stream->flush_ion_buffer(
        (void*)mPbuf_stream_v, (void*)mPbuf_stream_p, mPbuf_stream_size);

    return;
}

void SPRDHEVCEncoder::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mSawInputEOS) {
        return;
    }

    if (!mStarted) {
        if (OMX_ErrorNone != initEncoder()) {
            return;
        }
    }

    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    while (!mSawInputEOS && !inQueue.empty() && !outQueue.empty()) {
        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;
        BufferInfo *outInfo = *outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;

        outHeader->nTimeStamp = 0;
        outHeader->nFlags = 0;
        outHeader->nOffset = 0;
        outHeader->nFilledLen = 0;
        outHeader->nOffset = 0;

        uint8_t *outPtr = (uint8_t *) outHeader->pBuffer;
        uint32_t dataLength = outHeader->nAllocLen;

        // Combine SPS and PPS and place them in the very first output buffer
        // SPS and PPS are separated by start code 0x00000001
        // Assume that we have exactly one SPS and exactly one PPS.
        if (!mSpsPpsHeaderReceived && mNumInputFrames <= 0) {
            MMEncOut vps_header, sps_header, pps_header;
            int ret;

            memset(&vps_header, 0, sizeof(MMEncOut));
            memset(&sps_header, 0, sizeof(MMEncOut));
            memset(&pps_header, 0, sizeof(MMEncOut));
            mPmem_stream->flush_ion_buffer((void*)mPbuf_stream_v, (void*)mPbuf_stream_p, mPbuf_stream_size);
            ++mNumInputFrames;
            ret = (*mH265EncGenHeader)(mHandle, &vps_header, 0);
            outHeader->nFilledLen = vps_header.strmSize;
            ALOGI("%s, %d, vps_header.strmSize: %d", __FUNCTION__, __LINE__, vps_header.strmSize);

            {   //added by xiaowei, 2013.10.08, for bug 220340.
                uint8_t *p = (uint8_t *)(vps_header.pOutBuf);
                ALOGI("vps: %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x,%0x, %0x, %0x, %0x, %0x",
                      p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9],p[10], p[11], p[12], p[13], p[14], p[15]);

                p[0] = p[1] = p[2] = 0x0;
                p[3] = 0x1;
            }

#ifdef SPRD_DUMP_BS
            if (mFile_bs != NULL) {
                fwrite(vps_header.pOutBuf, 1, vps_header.strmSize, mFile_bs);
            }
#endif

            memcpy(outPtr, vps_header.pOutBuf, vps_header.strmSize);
            outPtr+= vps_header.strmSize;
            mPmem_stream->flush_ion_buffer((void*)mPbuf_stream_v, (void*)mPbuf_stream_p, mPbuf_stream_size);

            ++mNumInputFrames;
            ret = (*mH265EncGenHeader)(mHandle, &sps_header, 1);
            outHeader->nFilledLen += sps_header.strmSize;
            ALOGI("%s, %d, sps_header.strmSize: %d", __FUNCTION__, __LINE__, sps_header.strmSize);

            {   //added by xiaowei, 2013.10.08, for bug 220340.
                uint8_t *p = (uint8_t *)(sps_header.pOutBuf);
                ALOGI("sps: %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x,%0x, %0x, %0x, %0x, %0x",
                      p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9],p[10], p[11], p[12], p[13], p[14], p[15]);

                p[0] = p[1] = p[2] = 0x0;
                p[3] = 0x1;
            }

#ifdef SPRD_DUMP_BS
            if (mFile_bs != NULL) {
                fwrite(sps_header.pOutBuf, 1, sps_header.strmSize, mFile_bs);
            }
#endif

            memcpy(outPtr, sps_header.pOutBuf, sps_header.strmSize);
            outPtr+= sps_header.strmSize;
            mPmem_stream->flush_ion_buffer((void*)mPbuf_stream_v, (void*)mPbuf_stream_p, mPbuf_stream_size);
            ++mNumInputFrames;
            ret = (*mH265EncGenHeader)(mHandle, &pps_header, 2);
            ALOGI("%s, %d, pps_header.strmSize: %d", __FUNCTION__, __LINE__, pps_header.strmSize);

            {   //added by xiaowei, 2013.10.08, for bug 220340.
                uint8_t *p = (uint8_t *)(pps_header.pOutBuf);
                ALOGI("pps: %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x,",
                      p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);

                p[0] = p[1] = p[2] = 0x0;
                p[3] = 0x1;
            }

#ifdef SPRD_DUMP_BS
            if (mFile_bs != NULL) {
                fwrite(pps_header.pOutBuf, 1, pps_header.strmSize, mFile_bs);
            }
#endif

            outHeader->nFilledLen += pps_header.strmSize;
            memcpy(outPtr, pps_header.pOutBuf, pps_header.strmSize);

            mSpsPpsHeaderReceived = true;
            CHECK_EQ(0, mNumInputFrames);  // 1st video frame is 0
            outHeader->nFlags = OMX_BUFFERFLAG_CODECCONFIG;
            outQueue.erase(outQueue.begin());
            outInfo->mOwnedByUs = false;
            notifyFillBufferDone(outHeader);
            return;
        }

        ALOGV("%s, line:%d, inHeader->nFilledLen: %d, mStoreMetaData: %d, mVideoColorFormat: 0x%x",
              __FUNCTION__, __LINE__, inHeader->nFilledLen, mStoreMetaData, mVideoColorFormat);

        // Save the input buffer info so that it can be
        // passed to an output buffer
        InputBufferInfo info;
        info.mTimeUs = inHeader->nTimeStamp;
        info.mFlags = inHeader->nFlags;
        mInputBufferInfoVec.push(info);

        if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
            mSawInputEOS = true;
        }

        if (inHeader->nFilledLen > 0) {
            const void *inData = inHeader->pBuffer + inHeader->nOffset;
            uint8_t *inputData = (uint8_t *) inData;
            CHECK(inputData != NULL);

            MMEncIn vid_in;
            MMEncOut vid_out;
            memset(&vid_in, 0, sizeof(MMEncIn));
            memset(&vid_out, 0, sizeof(MMEncOut));
            uint8_t* py = NULL;
            uint8_t* py_phy = NULL;
            uint32_t width = 0;
            uint32_t height = 0;
            uint32_t x = 0;
            uint32_t y = 0;
            bool needUnmap = false;
            int bufFd = -1;
            unsigned long iova = 0;
            size_t iovaLen = 0;

            if (mVideoColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) {
                vid_in.yuv_format = MMENC_YUV420SP_NV12;
            } else {
                vid_in.yuv_format = MMENC_YUV420SP_NV21;
            }

            if (mStoreMetaData) {
                unsigned int *mataData = (unsigned int *)inputData;
                unsigned int type = *mataData++;

                if (type == kMetadataBufferTypeCameraSource) {
                    py_phy = (uint8_t*)(*(unsigned long *)mataData);
                    mataData += sizeof(unsigned long)/sizeof(unsigned int);
                    py = (uint8_t*)(*(unsigned long *)mataData);
                    mataData += sizeof(unsigned long)/sizeof(unsigned int);
                    width = (uint32_t)(*((uint32_t *) mataData++));
                    height = (uint32_t)(*((uint32_t *) mataData++));
                    x = (uint32_t)(*((uint32_t *) mataData++));
                    y = (uint32_t)(*((uint32_t *) mataData++));
                    int fd = (int32)(*((int32*) mataData));
                    unsigned long py_addr=0;
                    size_t buf_size=0;
                    int ret = 0;
                    if (mIOMMUEnabled) {
                        ret = (*mH265EncGetIOVA)(mHandle, fd, &py_addr, &buf_size);
                    } else {
                        ret = MemIon::Get_phy_addr_from_ion(fd, &py_addr, &buf_size);
                    }
                    if(ret) {
                        ALOGE("Failed to Get_iova or Get_phy_addr_from_ion %d", ret);
                        return;
                    }
                    if (mIOMMUEnabled) {
                        needUnmap = true;
                        bufFd = fd;
                        iova = py_addr;
                        iovaLen = buf_size;
                    }
                    py_phy = (uint8_t*)py_addr;
                } else if (type == kMetadataBufferTypeGrallocSource) {
                    buffer_handle_t buf = *((buffer_handle_t *)(inputData + sizeof(void *)));

                    if (mPbuf_yuv_v == NULL) {
                        size_t yuv_size;
                        if ((ADP_FORMAT(buf) == HAL_PIXEL_FORMAT_RGBA_8888) && mSupportRGBEnc) {
                            yuv_size = ((mVideoWidth+15)&(~15)) * ((mVideoHeight+15)&(~15)) * 4;
                            vid_in.yuv_format = MMENC_RGBA32;
                        } else {
                            yuv_size = mVideoWidth * mVideoHeight * 3/2;
                        }
                        if (mIOMMUEnabled) {
                            mYUVInPmemHeap = new MemIon("/dev/ion", yuv_size, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
                        } else {
                            mYUVInPmemHeap = new MemIon("/dev/ion", yuv_size, MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
                        }

                        int fd = mYUVInPmemHeap->getHeapID();
                        if (fd < 0) {
                            ALOGE("Failed to alloc yuv buffer");
                            return;
                        }

                        int ret;
                        unsigned long phy_addr;
                        size_t buffer_size;
                        if(mIOMMUEnabled) {
                            ret = (*mH265EncGetIOVA)(mHandle, fd, &phy_addr, &buffer_size);
                        } else {
                            ret = mYUVInPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
                        }
                        if(ret) {
                            ALOGE("Failed to get_iova or get_phy_addr_from_ion %d", ret);
                            return;
                        }

                        mPbuf_yuv_v =(uint8_t *) mYUVInPmemHeap->getBase();
                        mPbuf_yuv_p = phy_addr;
                        mPbuf_yuv_size = buffer_size;
                    }

                    py = mPbuf_yuv_v;
                    py_phy = (uint8_t*)mPbuf_yuv_p;

                    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
                    Rect bounds(mVideoWidth, mVideoHeight);

                    void* vaddr;
                    struct android_ycbcr ycbcr;
                    if (ADP_FORMAT(buf) == HAL_PIXEL_FORMAT_YCBCR_420_888){
                        if (mapper.lockYCbCr(buf, GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_NEVER, bounds, &ycbcr)) {
                            ALOGE("%s, line:%d, mapper.lockYCbCr failed", __FUNCTION__, __LINE__);
                            return;
                        }
                        void* vaddr = malloc(mVideoWidth * mVideoHeight * 3 / 2);
                        SprdSimpleOMXComponent::ConvertFlexYUVToPlanar((uint8_t*)vaddr, mVideoWidth, mVideoHeight, &ycbcr, mVideoWidth, mVideoHeight);
                    } else {
                        if (mapper.lock(buf, GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_NEVER, bounds, &vaddr)) {
                            ALOGE("%s, line:%d, mapper.lock failed", __FUNCTION__, __LINE__);
                            return;
                        }
                    }

                    if (mVideoColorFormat == OMX_COLOR_FormatYUV420Planar) {
                        ConvertYUV420PlanarToYVU420SemiPlanar((uint8_t*)vaddr, py, mVideoWidth, mVideoHeight,
                                                              mVideoWidth, mVideoHeight);
                    } else if(mVideoColorFormat == OMX_COLOR_FormatAndroidOpaque) {
                        ALOGI("private_h->format:0x%x",ADP_FORMAT(buf));
                        if (ADP_FORMAT(buf) == HAL_PIXEL_FORMAT_YCbCr_420_SP || ADP_FORMAT(buf) == HAL_PIXEL_FORMAT_YCBCR_420_888)
                            vid_in.yuv_format = MMENC_YUV420SP_NV12;
                        else if ((ADP_FORMAT(buf) == HAL_PIXEL_FORMAT_RGBA_8888) && mSupportRGBEnc)
                            vid_in.yuv_format = MMENC_RGBA32;
                        else
                            vid_in.yuv_format = MMENC_YUV420SP_NV21;

                        if(ADP_FORMAT(buf) == HAL_PIXEL_FORMAT_YCrCb_420_SP ||
                             ADP_FORMAT(buf) == HAL_PIXEL_FORMAT_YCbCr_420_SP ||
                             (ADP_FORMAT(buf) == HAL_PIXEL_FORMAT_YCBCR_420_888 && ycbcr.chroma_step == 2) ||
                             ((ADP_FORMAT(buf) == HAL_PIXEL_FORMAT_RGBA_8888) && mSupportRGBEnc)) {
                            if (ADP_USAGE(buf) & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
                                unsigned long py_addr=0;
                                size_t buf_size=0;
                                int fd = ADP_BUFFD(buf);
                                int ret = 0;

                                if (mIOMMUEnabled) {
                                    ret = (*mH265EncGetIOVA)(mHandle, fd, &py_addr, &buf_size);
                                } else {
                                    ret = MemIon::Get_phy_addr_from_ion(fd, &py_addr, &buf_size);
                                }
                                if(ret) {
                                    ALOGE("Failed to Get_iova or Get_phy_addr_from_ion %d", ret);
                                    return;
                                }
                                if (mIOMMUEnabled) {
                                    needUnmap = true;
                                    bufFd = fd;
                                    iova = py_addr;
                                    iovaLen = buf_size;
                                }

                                py = (uint8_t*)vaddr;
                                py_phy = (uint8_t*)py_addr;
                            } else {
                                memcpy(py, vaddr, mVideoWidth * mVideoHeight * 3/2);
                            }
                        } else if (ADP_FORMAT(buf) == HAL_PIXEL_FORMAT_YCbCr_420_P || ycbcr.chroma_step == 1) {
                            ConvertYUV420PlanarToYVU420SemiPlanar((uint8_t*)vaddr, py, mVideoWidth, mVideoHeight, mVideoWidth, mVideoHeight);
                        }else {
#ifdef CONFIG_BIA_SUPPORT
                            ConvertARGB888ToYVU420SemiPlanar_c((uint8_t*)vaddr, py, py+(mVideoWidth * mVideoHeight), mVideoWidth, mVideoHeight, mVideoWidth, mVideoHeight);
#else
                            ConvertARGB888ToYVU420SemiPlanar_neon((uint8_t*)vaddr, py, py+(mVideoWidth * mVideoHeight), mVideoWidth, mVideoHeight, mVideoWidth, mVideoHeight);
#endif
                        }
                    } else if(ADP_USAGE(buf) & GRALLOC_USAGE_HW_VIDEO_ENCODER ||
                              mVideoColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) {
                        unsigned long py_addr=0;
                        size_t buf_size=0;
                        int fd = ADP_BUFFD(buf);
                        int ret = 0;
                        //ALOGD("private_h->format:0x%x",private_h->format);

                        if (mIOMMUEnabled) {
                            ret = (*mH265EncGetIOVA)(mHandle, fd, &py_addr, &buf_size);
                        } else {
                            ret = MemIon::Get_phy_addr_from_ion(fd, &py_addr, &buf_size);
                        }
                        if(ret) {
                            ALOGE("Failed to Get_iova or Get_phy_addr_from_ion %d", ret);
                            return;
                        }

                        py = (uint8_t*)vaddr;
                        py_phy = (uint8_t*)py_addr;
                        if (mIOMMUEnabled) {
                            needUnmap = true;
                            bufFd = fd;
                            iova = py_addr;
                            iovaLen = buf_size;
                        }
                        //ALOGD("%s, mIOMMUEnabled = %d, fd = 0x%lx, vaddr = 0x%lx, ion_addr = 0x%lx",__FUNCTION__, mIOMMUEnabled, fd, vaddr, ion_addr);
                    } else {
                        memcpy(py, vaddr, mVideoWidth * mVideoHeight * 3/2);
                    }

                    if (mapper.unlock(buf)) {
                        ALOGE("%s, line:%d, mapper.unlock failed", __FUNCTION__, __LINE__);
                        return;
                    }

                } else {
                    ALOGE("Error MetadataBufferType %d", type);
                    return;
                }
            } else {
                if (mPbuf_yuv_v == NULL) {
                    int32 yuv_size;
                    if (mVideoColorFormat == OMX_COLOR_FormatAndroidOpaque && mSupportRGBEnc)
                        yuv_size = ((mVideoWidth+15)&(~15)) * ((mVideoHeight+15)&(~15)) * 4;
                    else
                        yuv_size = mVideoWidth * mVideoHeight * 3/2;
                    if(mIOMMUEnabled) {
                        mYUVInPmemHeap = new MemIon("/dev/ion", yuv_size, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
                    } else {
                        mYUVInPmemHeap = new MemIon("/dev/ion", yuv_size, MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
                    }

                    int fd = mYUVInPmemHeap->getHeapID();
                    if (fd < 0) {
                        ALOGE("Failed to alloc yuv buffer");
                        return;
                    }

                    int ret;
                    unsigned long phy_addr;
                    size_t buffer_size;
                    if(mIOMMUEnabled) {
                        ret = (*mH265EncGetIOVA)(mHandle, fd, &phy_addr, &buffer_size);
                    } else {
                        ret = mYUVInPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
                    }
                    if(ret) {
                        ALOGE("Failed to get_iova or get_phy_addr_from_ion %d", ret);
                        return;
                    }

                    mPbuf_yuv_v =(uint8_t *) mYUVInPmemHeap->getBase();
                    mPbuf_yuv_p = phy_addr;
                    mPbuf_yuv_size = buffer_size;
                }

                py = mPbuf_yuv_v;
                py_phy = (uint8_t*)mPbuf_yuv_p;

                if (mVideoColorFormat == OMX_COLOR_FormatYUV420Planar) {
                    ConvertYUV420PlanarToYVU420SemiPlanar(inputData, py, mVideoWidth, mVideoHeight,
                                                          mVideoWidth, mVideoHeight);
                } else if(mVideoColorFormat == OMX_COLOR_FormatAndroidOpaque) {
                    if (mSupportRGBEnc) {
                        memcpy(py, inputData, ((mVideoWidth+15)&(~15)) * ((mVideoHeight+15)&(~15)) * 4);
                        vid_in.yuv_format = MMENC_RGBA32;
                    } else {
                        ConvertARGB888ToYVU420SemiPlanar(inputData, py, py+(mVideoWidth * mVideoHeight), mVideoWidth, mVideoHeight, mVideoWidth, mVideoHeight);
                    }
                } else {
                    memcpy(py, inputData, mVideoWidth * mVideoHeight * 3/2);
                }
            }
#ifdef CONFIG_SPRD_RECORD_EIS
            if (mEISMode) {
                char *parameter = (char *)py+mVideoWidth * mVideoHeight * 3/2;
                double *warp = (double *)parameter;
                if (*warp++ == 16171225) {
                    for (int i=0;i<3;i++) {
                        for (int j=0;j<3;j++) {
                            vid_in.matrix[i][j]=*warp++;
                        }
                    }
                } else {
                    vid_in.matrix[0][0] = vid_in.matrix[1][1] = vid_in.matrix[2][2] = 1.0;
                    vid_in.matrix[0][1] = vid_in.matrix[0][2] = vid_in.matrix[1][0] = 0.0;
                    vid_in.matrix[1][2] = vid_in.matrix[2][0] = vid_in.matrix[2][1] = 0.0;
                }
                ALOGV("EISLib2record:%f %f %f %f %f %f %f %f %f",
                    vid_in.matrix[0][0], vid_in.matrix[0][1], vid_in.matrix[0][2],
                    vid_in.matrix[1][0], vid_in.matrix[1][1], vid_in.matrix[1][2],
                    vid_in.matrix[2][0], vid_in.matrix[2][1], vid_in.matrix[2][2]);
            }
#endif

            // vid_in.time_stamp is not use for now.
            vid_in.time_stamp = (inHeader->nTimeStamp + 500) / 1000;  // in ms;
            vid_in.channel_quality = 1;

            vid_in.needIVOP = false;    // default P frame
            if (mKeyFrameRequested || (mNumInputFrames == 0)) {
                vid_in.needIVOP = true;    // I frame
                ALOGI("Request an IDR frame");
            }

            vid_in.p_src_y = py;
            vid_in.p_src_v = 0;
            vid_in.p_src_y_phy = py_phy;
            vid_in.p_src_v_phy = 0;

            if(width != 0 && height != 0) {
                vid_in.p_src_u = py + width*height;
                vid_in.p_src_u_phy = py_phy + width*height;
            } else {
                vid_in.p_src_u = py + mVideoWidth*mVideoHeight;
                vid_in.p_src_u_phy = py_phy + mVideoWidth*mVideoHeight;
            }

            vid_in.org_img_width = (int32_t)width;
            vid_in.org_img_height = (int32_t)height;
            vid_in.crop_x = (int32_t)x;
            vid_in.crop_y = (int32_t)y;

#ifdef SPRD_DUMP_YUV
            if (mFile_yuv != NULL) {
                fwrite(py, 1, mVideoWidth * mVideoHeight * 3/2, mFile_yuv);
            }
#endif
            mPmem_stream->flush_ion_buffer((void*)mPbuf_stream_v, (void*)mPbuf_stream_p, mPbuf_stream_size);
            int64_t start_encode = systemTime();
            int ret = (*mH265EncStrmEncode)(mHandle, &vid_in, &vid_out);
            int64_t end_encode = systemTime();
            ALOGI("H265EncStrmEncode[%lld] %dms, in {%p-%p, %dx%d, %d}, out {%p-%d, %d}, wh{%d, %d}, xy{%d, %d}",
                  mNumInputFrames, (unsigned int)((end_encode-start_encode) / 1000000L), py, py_phy,
                  mVideoWidth, mVideoHeight, vid_in.yuv_format, vid_out.pOutBuf, vid_out.strmSize, vid_out.vopType, width, height, x, y);
            if (needUnmap) {
                ALOGV("Free_iova, fd: %d, iova: 0x%lx, size: %zd", bufFd, iova, iovaLen);
                (*mH265EncFreeIOVA)(mHandle, iova, iovaLen);
            }
            if ((vid_out.strmSize < 0) || (ret != MMENC_OK)) {
                ALOGE("Failed to encode frame %lld, ret=%d", mNumInputFrames, ret);
#if 0  //removed by xiaowei, 20131017, for cr224544
                mSignalledError = true;
                notify(OMX_EventError, OMX_ErrorUndefined, 0, 0);
#endif
            } else {
                ALOGI("%s, %d, out_stream_ptr: %p", __FUNCTION__, __LINE__, outPtr);

                {   //added by xiaowei, 2013.10.08, for bug 220340.
                    uint8_t *p = (uint8_t *)(vid_out.pOutBuf);
                    if (p) {
                        ALOGI("frame: %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x,",
                              p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);

                        p[0] = p[1] = p[2] = 0x0;
                        p[3] = 0x1;
                    }
                }
            }

#ifdef SPRD_DUMP_BS
            if (mFile_bs != NULL) {
                fwrite(vid_out.pOutBuf, 1, vid_out.strmSize, mFile_bs);
            }
#endif

            if(vid_out.strmSize > 0) {
                dataLength = vid_out.strmSize;
                memcpy(outPtr, vid_out.pOutBuf, dataLength);

                if (vid_out.vopType == 0) { //I VOP
                    mKeyFrameRequested = false;
                    outHeader->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
                }
                ++mNumInputFrames;
            } else {
                dataLength = 0;
            }
        } else {
            dataLength = 0;
        }

        if ((inHeader->nFlags & OMX_BUFFERFLAG_EOS) && (inHeader->nFilledLen == 0)) {
            // We also tag this output buffer with EOS if it corresponds
            // to the final input buffer.
            ALOGI("saw EOS");
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;
        }

        inQueue.erase(inQueue.begin());
        inInfo->mOwnedByUs = false;
        notifyEmptyBufferDone(inHeader);

        CHECK(!mInputBufferInfoVec.empty());
        InputBufferInfo *inputBufInfo = mInputBufferInfoVec.begin();
        if (dataLength > 0 || (inHeader->nFlags & OMX_BUFFERFLAG_EOS)) {  //add this judgement by xiaowei, 20131017, for cr224544
            outQueue.erase(outQueue.begin());
            outHeader->nTimeStamp = inputBufInfo->mTimeUs;
            outHeader->nFlags |= (inputBufInfo->mFlags | OMX_BUFFERFLAG_ENDOFFRAME);
            outHeader->nFilledLen = dataLength;
            outInfo->mOwnedByUs = false;
            notifyFillBufferDone(outHeader);
        }
        mInputBufferInfoVec.erase(mInputBufferInfoVec.begin());
    }
}

bool SPRDHEVCEncoder::openEncoder(const char* libName) {
    if(mLibHandle) {
        dlclose(mLibHandle);
    }

    ALOGI("openEncoder, lib: %s", libName);

    mLibHandle = dlopen(libName, RTLD_NOW);
    if(mLibHandle == NULL) {
        ALOGE("openEncoder, can't open lib: %s",libName);
        return false;
    }

    mH265EncPreInit = (FT_H265EncPreInit)dlsym(mLibHandle, "H265EncPreInit");
    if(mH265EncPreInit == NULL) {
        ALOGE("Can't find mH265EncPreInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265EncInit = (FT_H265EncInit)dlsym(mLibHandle, "H265EncInit");
    if(mH265EncInit == NULL) {
        ALOGE("Can't find H265EncInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265EncSetConf = (FT_H265EncSetConf)dlsym(mLibHandle, "H265EncSetConf");
    if(mH265EncSetConf == NULL) {
        ALOGE("Can't find H265EncSetConf in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265EncGetConf = (FT_H265EncGetConf)dlsym(mLibHandle, "H265EncGetConf");
    if(mH265EncGetConf == NULL) {
        ALOGE("Can't find H265EncGetConf in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265EncStrmEncode = (FT_H265EncStrmEncode)dlsym(mLibHandle, "H265EncStrmEncode");
    if(mH265EncStrmEncode == NULL) {
        ALOGE("Can't find H265EncStrmEncode in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265EncGenHeader = (FT_H265EncGenHeader)dlsym(mLibHandle, "H265EncGenHeader");
    if(mH265EncGenHeader == NULL) {
        ALOGE("Can't find H265EncGenHeader in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265EncRelease = (FT_H265EncRelease)dlsym(mLibHandle, "H265EncRelease");
    if(mH265EncRelease == NULL) {
        ALOGE("Can't find H265EncRelease in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }

    mH265EncGetCodecCapability = (FT_H265EncGetCodecCapability)dlsym(mLibHandle, "H265EncGetCodecCapability");
    if(mH265EncGetCodecCapability == NULL) {
        ALOGE("Can't find H265EncGetCodecCapability in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265EncGetIOVA = (FT_H265Enc_get_iova)dlsym(mLibHandle, "H265Enc_get_iova");
    if(mH265EncGetIOVA == NULL) {
        ALOGE("Can't find H265Enc_get_iova in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265EncFreeIOVA = (FT_H265Enc_free_iova)dlsym(mLibHandle, "H265Enc_free_iova");
    if(mH265EncFreeIOVA == NULL) {
        ALOGE("Can't find H265Enc_free_iova in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265EncGetIOMMUStatus = (FT_H265Enc_get_IOMMU_status)dlsym(mLibHandle, "H265Enc_get_IOMMU_status");
    if(mH265EncGetIOMMUStatus == NULL) {
        ALOGE("Can't find H265Enc_get_IOMMU_status in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH265Enc_NeedAlign = (FT_H265Enc_Need_Align)dlsym(mLibHandle, "H265Enc_NeedAlign");
    if(mH265Enc_NeedAlign == NULL) {
        ALOGE("Can't find H265Enc_NeedAlign in %s",libName);
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
    return new android::SPRDHEVCEncoder(name, callbacks, appData, component);
}
