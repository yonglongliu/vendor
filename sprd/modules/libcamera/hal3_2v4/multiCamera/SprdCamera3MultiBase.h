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
#include <gralloc_priv.h>
#include <ui/GraphicBuffer.h>
#include "../SprdCamera3HWI.h"
//#include "ts_makeup_api.h"
#include "SprdMultiCam3Common.h"

namespace sprdcamera {

typedef enum {
    PREVIEW_REQUEST_STATE = 0,
    WAIT_FIRST_YUV_STATE,  // wait first yuv frame
    WAIT_SECOND_YUV_STATE, // wait second yuv frame
    WAIT_THIRD_YUV_STATE,  // wait third yuv frame
    REPROCESS_STATE,       // encode yuv to jpeg
} request_state;

class SprdCamera3MultiBase {
  public:
    request_state mReqState;
    SprdCamera3MultiBase();
    virtual ~SprdCamera3MultiBase();
    int initialize(multiCameraMode mode);
    virtual int allocateOne(int w, int h, new_mem_t *new_mem, int type);
    virtual void freeOneBuffer(new_mem_t *buffer);
    virtual int validateCaptureRequest(camera3_capture_request_t *request);
    virtual void convertToRegions(int32_t *rect, int32_t *region, int weight);
    virtual uint8_t getCoveredValue(CameraMetadata &frame_settings,
                                    SprdCamera3HWI *hwiSub,
                                    SprdCamera3HWI *hwiMin,
                                    int convered_camera_id);
    virtual buffer_handle_t *popRequestList(List<buffer_handle_t *> &list);
    virtual int getStreamType(camera3_stream_t *new_stream);
    virtual void dumpFps();
    virtual void dumpData(unsigned char *addr, int type, int size, int param1,
                          int param2, int param3, const char param4[20]);
    virtual bool matchTwoFrame(hwi_frame_buffer_info_t result1,
                               List<hwi_frame_buffer_info_t> &list,
                               hwi_frame_buffer_info_t *result2);
    virtual hwi_frame_buffer_info_t *
    pushToUnmatchedQueue(hwi_frame_buffer_info_t new_buffer_info,
                         List<hwi_frame_buffer_info_t> &queue);
    virtual int convertToImg_frm(private_handle_t *in, img_frm *out,
                                 cmr_u32 format);
    virtual int convertToImg_frm(void *phy_addr, void *vir_addr, int width,
                                 int height, int fd, cmr_u32 format,
                                 img_frm *out);
    /*
#ifdef CONFIG_FACE_BEAUTY
virtual void doFaceMakeup(struct camera_frame_type *frame,
                          int perfect_level, int *face_info);

virtual void convert_face_info(int *ptr_cam_face_inf, int width,
                               int height);
#endif
*/
    bool ScaleNV21(uint8_t *a_ucDstBuf, uint16_t a_uwDstWidth,
                   uint16_t a_uwDstHeight, uint8_t *a_ucSrcBuf,
                   uint16_t a_uwSrcWidth, uint16_t a_uwSrcHeight,
                   uint32_t a_udFileSize);
    int flushIonBuffer(int buffer_fd, void *v_addr, size_t size);
    uint32_t getJpegSize(uint8_t *jpegBuffer, uint32_t maxSize);
    void setJpegSize(char *jpeg_base, uint32_t max_jpeg_size,
                     uint32_t jpeg_size);
    int jpeg_encode_exif_simplify(img_frm *src_img, img_frm *pic_enc_img,
                                  struct img_frm *dst_img, SprdCamera3HWI *hwi);
    int jpeg_encode_exif_simplify(private_handle_t *src_private_handle,
                                  private_handle_t *pic_enc_private_handle,
                                  private_handle_t *dst_private_handle,
                                  SprdCamera3HWI *hwi);

  private:
    bool mIommuEnabled;
    int mVFrameCount;
    int mVLastFrameCount;
    nsecs_t mVLastFpsTime;
    // for convered feature
    List<int> mLumaList;
    multiCameraMode mCameraMode;
    int mLowLumaConut;
    int mconut;
    uint32_t mCurScene;
    uint8_t mBrightConut;
    uint8_t mLowConut;
    uint8_t mDarkConut;
    //
};
}
#endif
