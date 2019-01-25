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

#ifndef SPRD_MP3_ENCODER_H_

#define SPRD_MP3_ENCODER_H_

#include "SprdSimpleOMXComponent.h"

#include <time.h>
#include <assert.h>
#include "typedefs.h"
#include "mp3_enc_typeDefs.h"

#include "mp3_enc_encoder.h"
namespace android {

struct SprdMP3Encoder : public SprdSimpleOMXComponent {
    SprdMP3Encoder(
            const char *name,
            const OMX_CALLBACKTYPE *callbacks,
            OMX_PTR appData,
            OMX_COMPONENTTYPE **component);

protected:
    virtual ~SprdMP3Encoder();

    virtual OMX_ERRORTYPE internalGetParameter(
            OMX_INDEXTYPE index, OMX_PTR params);

    virtual OMX_ERRORTYPE internalSetParameter(
            OMX_INDEXTYPE index, const OMX_PTR params);

    virtual void onQueueFilled(OMX_U32 portIndex);

private:
    enum {
        kNumBuffers             = 4,
        kNumSamplesPerFrame     = 160,
    };

    void *mEncState;
    void *mSidState;
	MP3_ENC_INPUT_T in_parameter;
	MP3_ENC_DATA_T  *mMp3_enc_handel_ptr;
	bool mMp3_enc_init ;
    int32_t mNumChannels;
    int32_t mSamplingRate;
	int32_t mFramelen;
	int32 mStreamlen;
    OMX_U32 mBitRate;

    size_t mInputSize;
    int16 mInputFrame[2304];
	uint8 mOutFrame[360*4];
    int64_t mInputTimeUs;

    bool mSawInputEOS;
    bool mSignalledError;
    // used to calculate the input bytes into the muliple of framelen  for encode , 
    int16 copy ;
    int16 copy_need ;
    int16 copy_tmp;
    int16 numBytesPerInputFrame ;


    void* mLibHandle;
    FT_MP3_ENC_MemoryAlloc     mMP3_ENC_MemoryAlloc;
    FT_MP3_ENC_InitDecoder     mMP3_ENC_InitDecoder;
    FT_MP3_ENC_EncoderProcess  mMP3_ENC_EncoderProcess;
    FT_MP3_ENC_ExitEncoder     mMP3_ENC_ExitEncoder;
    FT_MP3_ENC_MemoryFree	   mMP3_ENC_MemoryFree;


    void initPorts();
    status_t initEncoder();
    bool openDecoder(const char* libName);

    status_t setAudioParams();

    DISALLOW_EVIL_CONSTRUCTORS(SprdMP3Encoder);
};

}  // namespace android

#endif  // SOFT_MP3_ENCODER_H_
