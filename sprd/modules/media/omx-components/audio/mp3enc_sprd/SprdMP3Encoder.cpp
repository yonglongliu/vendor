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

//#define LOG_NDEBUG 0
#define LOG_TAG "SprdMP3Encoder"
#include <utils/Log.h>

#include "SprdMP3Encoder.h"

#include <dlfcn.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/hexdump.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace android {

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

bool SprdMP3Encoder::openDecoder(const char* libName) {
    if(mLibHandle) {
        dlclose(mLibHandle);
    }

    ALOGI("openDecoder, lib: %s", libName);

    mLibHandle = dlopen(libName, RTLD_NOW);
    if(mLibHandle == NULL) {
        ALOGE("openDecoder, can't open lib: %s",libName);
        return false;
    }

    mMP3_ENC_MemoryAlloc = (FT_MP3_ENC_MemoryAlloc)dlsym(mLibHandle, "MP3_ENC_MemoryAlloc");
    if(mMP3_ENC_MemoryAlloc == NULL) {
        ALOGE("Can't find MP3_ENC_MemoryAlloc in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP3_ENC_InitDecoder = (FT_MP3_ENC_InitDecoder)dlsym(mLibHandle, "MP3_ENC_Init");
    if(mMP3_ENC_InitDecoder == NULL) {
        ALOGE("Can't find MP3_ENC_Init in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP3_ENC_EncoderProcess = (FT_MP3_ENC_EncoderProcess)dlsym(mLibHandle, "MP3_ENC_EncoderProcess");
    if(mMP3_ENC_EncoderProcess == NULL) {
        ALOGE("Can't find MP3_ENC_EncoderProcess in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP3_ENC_ExitEncoder = (FT_MP3_ENC_ExitEncoder)dlsym(mLibHandle, "MP3_ENC_ExitEncoder");
    if(mMP3_ENC_ExitEncoder == NULL) {
        ALOGE("Can't find MP3_ENC_ExitEncoder in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP3_ENC_MemoryFree = (FT_MP3_ENC_MemoryFree)dlsym(mLibHandle, "MP3_ENC_MemoryFree");
    if(mMP3_ENC_MemoryFree == NULL) {
        ALOGE("Can't find MP3_ENC_MemoryFree in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    return true;
}

SprdMP3Encoder::SprdMP3Encoder(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mEncState(NULL),
      mSidState(NULL),
      mMp3_enc_handel_ptr(NULL),
      mMp3_enc_init(false),
      mNumChannels(1),
      mSamplingRate(44100),
      mFramelen(1152),
      mStreamlen(0),
      mBitRate(0),
      mInputSize(0),
      mInputTimeUs(-1ll),
      mSawInputEOS(false),
      mSignalledError(false),
      mLibHandle(NULL),
      mMP3_ENC_MemoryAlloc(NULL),
      mMP3_ENC_InitDecoder(NULL),
      mMP3_ENC_EncoderProcess(NULL),
      mMP3_ENC_ExitEncoder(NULL),
      mMP3_ENC_MemoryFree(NULL) {
    bool ret = false;
    ret = openDecoder("libomx_mp3enc_sprd.so");
    CHECK_EQ(ret, true);
    initPorts();
    CHECK_EQ(initEncoder(), (status_t)OK);
}

SprdMP3Encoder::~SprdMP3Encoder() {
    if (mEncState != NULL) {
        mEncState = mSidState = NULL;
    }
    if (mMp3_enc_handel_ptr != NULL) {
            mMP3_ENC_ExitEncoder(mMp3_enc_handel_ptr);
            mMP3_ENC_MemoryFree(&mMp3_enc_handel_ptr);
       }
}

void SprdMP3Encoder::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = 4086*2;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainAudio;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.audio.cMIMEType = const_cast<char *>("audio/raw");
    def.format.audio.pNativeRender = NULL;
    def.format.audio.bFlagErrorConcealment = OMX_FALSE;
    def.format.audio.eEncoding = OMX_AUDIO_CodingPCM;

    addPort(def);

    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = 8192;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainAudio;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;

    def.format.audio.cMIMEType = const_cast<char *>("audio/mpeg");
    def.format.audio.pNativeRender = NULL;
    def.format.audio.bFlagErrorConcealment = OMX_FALSE;
    def.format.audio.eEncoding = OMX_AUDIO_CodingMP3;

    addPort(def);
}

status_t SprdMP3Encoder::initEncoder() {
    return OK;
}

OMX_ERRORTYPE SprdMP3Encoder::internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
        case OMX_IndexParamAudioPortFormat:
        {
            OMX_AUDIO_PARAM_PORTFORMATTYPE *formatParams =
                (OMX_AUDIO_PARAM_PORTFORMATTYPE *)params;

            if (formatParams->nPortIndex > 1) {
                return OMX_ErrorUndefined;
            }

            if (formatParams->nIndex > 0) {
                return OMX_ErrorNoMore;
            }

            formatParams->eEncoding =
                (formatParams->nPortIndex == 0)
                    ? OMX_AUDIO_CodingPCM : OMX_AUDIO_CodingMP3;

            return OMX_ErrorNone;
        }

        case OMX_IndexParamAudioMp3:
        {
            OMX_AUDIO_PARAM_MP3TYPE *mp3Params =
                (OMX_AUDIO_PARAM_MP3TYPE *)params;


            if (mp3Params->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }


            mp3Params->nChannels = mNumChannels;
            mp3Params->nSampleRate = mSamplingRate;

            return OMX_ErrorNone;
        }

        case OMX_IndexParamAudioPcm:
        {
            OMX_AUDIO_PARAM_PCMMODETYPE *pcmParams =
                (OMX_AUDIO_PARAM_PCMMODETYPE *)params;


            if (pcmParams->nPortIndex != 0) {
                return OMX_ErrorUndefined;
            }

            pcmParams->nChannels = mNumChannels;
            pcmParams->eNumData = OMX_NumericalDataSigned;
            pcmParams->eEndian = OMX_EndianBig;
            pcmParams->bInterleaved = OMX_TRUE;
            pcmParams->nBitPerSample = 16;

            pcmParams->nSamplingRate = mSamplingRate ;

            pcmParams->ePCMMode = OMX_AUDIO_PCMModeLinear;
            pcmParams->eChannelMapping[0] = OMX_AUDIO_ChannelLF;
            pcmParams->eChannelMapping[1] = OMX_AUDIO_ChannelRF;

            return OMX_ErrorNone;
        }

        default:
            return SprdSimpleOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SprdMP3Encoder::internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
        case OMX_IndexParamStandardComponentRole:
        {
            const OMX_PARAM_COMPONENTROLETYPE *roleParams =
                (const OMX_PARAM_COMPONENTROLETYPE *)params;


            if (strncmp((const char *)roleParams->cRole,
                        "audio_encoder.mp3",
                        OMX_MAX_STRINGNAME_SIZE - 1)) {
                return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamAudioPortFormat:
        {
            const OMX_AUDIO_PARAM_PORTFORMATTYPE *formatParams =
                (const OMX_AUDIO_PARAM_PORTFORMATTYPE *)params;


            if (formatParams->nPortIndex > 1) {
                return OMX_ErrorUndefined;
            }

            if (formatParams->nIndex > 0) {
                return OMX_ErrorNoMore;
            }

            if ((formatParams->nPortIndex == 0
                        && formatParams->eEncoding != OMX_AUDIO_CodingPCM)
                || (formatParams->nPortIndex == 1
                        && formatParams->eEncoding != OMX_AUDIO_CodingAMR)) {
                return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamAudioMp3:
        {
            OMX_AUDIO_PARAM_MP3TYPE *mp3Params =
                (OMX_AUDIO_PARAM_MP3TYPE *)params;

            return OMX_ErrorNone;
        }

        case OMX_IndexParamAudioPcm:
        {
            OMX_AUDIO_PARAM_PCMMODETYPE *pcmParams =
                (OMX_AUDIO_PARAM_PCMMODETYPE *)params;


            if (pcmParams->nPortIndex != 0) {
                return OMX_ErrorUndefined;
            }
            mSamplingRate = pcmParams->nSamplingRate ;
            mNumChannels  = pcmParams->nChannels ;
            ALOGI("neo: OMX_IndexParamAudioPcm  mSamplingRate %d mNumChannels %d",mSamplingRate, mNumChannels);
            return OMX_ErrorNone;
        }


        default:
            return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}
void MP3_ENC_PreProcess(int16* data , int32 samples ,int16 channels) {
        int32 i ;
        int16 *sp,ch,*dptr;
        int16 fix_Buf[2304];
        i=channels*samples*2;	
        memcpy((int8*)fix_Buf ,  data , i );
        sp = fix_Buf ;
    for (ch=0;ch<channels;ch++)
    {
        sp=fix_Buf+ch;
        dptr=data+1152*ch;
        i=samples>>3;
	MIN_LIMIT(i,0);
        while(i--)
        {
            *dptr++=*sp;
                sp+=channels;
            *dptr++=*sp;
                sp+=channels;
            *dptr++=*sp;
                sp+=channels;
            *dptr++=*sp;
                sp+=channels;
            *dptr++=*sp;
                sp+=channels;
            *dptr++=*sp;
                sp+=channels;
            *dptr++=*sp;
                sp+=channels;
            *dptr++=*sp;
                sp+=channels;
        }

   }
}
void SprdMP3Encoder::onQueueFilled(OMX_U32 /* portIndex */) {

    if (mSignalledError) {
        return;
    }
        if (!mMp3_enc_init){
                mBitRate = mSamplingRate * mNumChannels * 16 ;
                MIN_LIMIT(mBitRate,32000);
                MAX_LIMIT(mBitRate,320000);
            in_parameter.sample_rate     = mSamplingRate;
            in_parameter.bit_rate        = mBitRate;
            in_parameter.ch_count        = mNumChannels;
            in_parameter.MS_sign         = 1;
            in_parameter.VBR_sign        = 0;
            in_parameter.cut_off         = 4000;
            ALOGI("neo: mp3_enc mSamplingRate %d , mBitRate %d , mNumChannels %d" , mSamplingRate ,mBitRate , mNumChannels );
            /* MP3 encoder init */
            if(mMp3_enc_handel_ptr == NULL) {
            if (mMP3_ENC_MemoryAlloc(&mMp3_enc_handel_ptr)!= 0) {
            ALOGE("MP3_ENC_MemoryAlloc error");
   }
            mMP3_ENC_InitDecoder( &in_parameter,
                           mMp3_enc_handel_ptr);
            mMp3_enc_handel_ptr->bit_pool_buf.stream_len=&mStreamlen;
            }
            if(mSamplingRate<32000)
                mFramelen=576;
            else
                mFramelen=1152;
                copy = 0;
                copy_tmp = 0 ;
                numBytesPerInputFrame = mFramelen * mNumChannels * 2 ;
                copy_need = numBytesPerInputFrame ;
                mMp3_enc_init = true ;
       }
    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    for (;;) {
        // We do the following until we run out of buffers.
        while (mInputSize < numBytesPerInputFrame) {
            // As long as there's still input data to be read we
            // will drain "kNumSamplesPerFrame" samples
            // into the "mInputFrame" buffer and then encode those
            // as a unit into an output buffer.
            if (mSawInputEOS || inQueue.empty()) {
                return;
            }
            BufferInfo *inInfo = *inQueue.begin();
            OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;

            const void *inData = inHeader->pBuffer + inHeader->nOffset;
            if (copy_need > inHeader->nFilledLen) {
                copy_tmp  = inHeader->nFilledLen;
                copy_need =  copy_need - inHeader->nFilledLen ;
            }else {
                copy_tmp  =  copy_need ;
                copy_need = 0;
            }
            inHeader->nOffset += copy_tmp;
            inHeader->nFilledLen -= copy_tmp;
            if (mInputSize == 0) {
                mInputTimeUs = inHeader->nTimeStamp;
            }

            inHeader->nTimeStamp +=
                (copy * 1000000ll / mSamplingRate / mNumChannels) / sizeof(int16_t);

            if(copy_tmp > 0) {
                memcpy((uint8_t *)mInputFrame+mInputSize , (void*)inData, copy_tmp);
            }
            mInputSize += copy_tmp ;
            if(mInputSize == numBytesPerInputFrame){
                 copy = mInputSize ;

            }
            if (inHeader->nFilledLen == 0) {
                if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
                    ALOGV("saw input EOS");
                    mSawInputEOS = true;

                    // Pad any remaining data with zeroes.
                    memset((uint8_t *)mInputFrame + mInputSize,
                           0,
                           numBytesPerInputFrame - mInputSize);

                    mInputSize = numBytesPerInputFrame;
                }

                inQueue.erase(inQueue.begin());
                inInfo->mOwnedByUs = false;
                notifyEmptyBufferDone(inHeader);

                inData = NULL;
                inHeader = NULL;
                inInfo = NULL;
            }

        }
        // At this  point we have all the input data necessary to encode
        // a single frame, all we need is an output buffer to store the result
        // in.

        if (outQueue.empty()) {
            return;
        }

        BufferInfo *outInfo = *outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;
        uint8_t *outPtr = outHeader->pBuffer + outHeader->nOffset;
        size_t outAvailable = outHeader->nAllocLen - outHeader->nOffset;

        if(mStreamlen!=0)
             mStreamlen=0;
        mMp3_enc_handel_ptr->bit_pool_buf.stream_out=mOutFrame;
        mMp3_enc_handel_ptr->bit_pool_buf.stream_len=&mStreamlen;

        /*  first we need to convert data from interleaved to non-interleaved  */
        MP3_ENC_PreProcess(mInputFrame ,mFramelen , mNumChannels );
        mMP3_ENC_EncoderProcess(mInputFrame,(int16)mFramelen, mMp3_enc_handel_ptr);

        memcpy(outPtr , mOutFrame , mStreamlen);
        outHeader->nFilledLen = mStreamlen;
        outHeader->nFlags = OMX_BUFFERFLAG_ENDOFFRAME;

        if (mSawInputEOS) {
            // We also tag this output buffer with EOS if it corresponds
            // to the final input buffer.
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;
        }
        outHeader->nTimeStamp = mInputTimeUs;

        outQueue.erase(outQueue.begin());
        outInfo->mOwnedByUs = false;
        notifyFillBufferDone(outHeader);

        outHeader = NULL;
        outInfo = NULL;

        mInputSize = 0;
        mStreamlen = 0;
        copy_need  = numBytesPerInputFrame ;
        copy_tmp   = 0;
    }
}
}  // namespace android

android::SprdOMXComponent *createSprdOMXComponent(
        const char *name, const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    return new android::SprdMP3Encoder(name, callbacks, appData, component);
}

