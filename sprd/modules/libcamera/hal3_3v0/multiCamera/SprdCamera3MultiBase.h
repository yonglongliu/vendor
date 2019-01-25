/* Copyright (c) 2016, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef SPRDCAMERAMULTIBASE_H_HEADER
#define SPRDCAMERAMULTIBASE_H_HEADER

#include <stdlib.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <cutils/properties.h>
#include <hardware/camera3.h>
#include <hardware/camera.h>
#include <system/camera.h>
#include <sys/mman.h>
#include <sprd_ion.h>
#ifdef CONFIG_GPU_PLATFORM_ROGUE
#include <gralloc_public.h>
#include <ui/GraphicBufferMapper.h>
#else
#include <gralloc_priv.h>
#endif
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include "../SprdCamera3HWI.h"
//#include "ts_makeup_api.h"
#include "SprdMultiCam3Common.h"

namespace sprdcamera {

class SprdCamera3MultiBase {
  public:
    SprdCamera3MultiBase();
    virtual ~SprdCamera3MultiBase();

    virtual int allocateOne(int w, int h, uint32_t is_cache,
                            new_mem_t *new_mem);
    virtual void freeOneBuffer(new_mem_t *LocalBuffer);
    virtual int validateCaptureRequest(camera3_capture_request_t *request);
    virtual buffer_handle_t *popRequestList(List<buffer_handle_t *> &list);
    virtual buffer_handle_t *popBufferList(List<new_mem_t *> &list,
                                           camera_buffer_type_t type);
    virtual void pushBufferList(new_mem_t *localbuffer,
                                buffer_handle_t *backbuf, int localbuffer_num,
                                List<new_mem_t *> &list);
    virtual int getStreamType(camera3_stream_t *new_stream);
    virtual bool matchTwoFrame(hwi_frame_buffer_info_t result1,
                               List<hwi_frame_buffer_info_t> &list,
                               hwi_frame_buffer_info_t *result2);
    virtual hwi_frame_buffer_info_t *
    pushToUnmatchedQueue(hwi_frame_buffer_info_t new_buffer_info,
                         List<hwi_frame_buffer_info_t> &queue);
    virtual void *getBufferAddr(buffer_handle_t *buffer);
    virtual int getBufferSize(buffer_handle_t *buffer);
    virtual void dumpImg(void *addr, int size, int frameId, int flag);
    virtual void dumpFps();
    virtual int initialize(const camera3_callback_ops_t *callback_ops);
    /*#ifdef CONFIG_FACE_BEAUTY
        virtual void doFaceMakeup(struct camera_frame_type *frame,
                                  int perfect_level, int *face_info);
        virtual void convert_face_info(int *ptr_cam_face_inf, int width,
                                       int height);

    #endif */
    bool DepthRotateCCW90(uint16_t *a_uwDstBuf, uint16_t *a_uwSrcBuf,
                          uint16_t a_uwSrcWidth, uint16_t a_uwSrcHeight,
                          uint32_t a_udFileSize);
    bool DepthRotateCCW180(uint16_t *a_uwDstBuf, uint16_t *a_uwSrcBuf,
                           uint16_t a_uwSrcWidth, uint16_t a_uwSrcHeight,
                           uint32_t a_udFileSize);
    bool NV21Rotate90(uint8_t *a_ucDstBuf, uint8_t *a_ucSrcBuf,
                      uint16_t a_uwSrcWidth, uint16_t a_uwSrcHeight,
                      uint32_t a_udFileSize);
    bool NV21Rotate180(uint8_t *a_ucDstBuf, uint8_t *a_ucSrcBuf,
                       uint16_t a_uwSrcWidth, uint16_t a_uwSrcHeight,
                       uint32_t a_udFileSize);

  private:
    Mutex mBufferListLock;
    bool mIommuEnabled;
    int mVFrameCount;
    int mVLastFrameCount;
    nsecs_t mVLastFpsTime;
    double mVFps;
};
}
#endif
