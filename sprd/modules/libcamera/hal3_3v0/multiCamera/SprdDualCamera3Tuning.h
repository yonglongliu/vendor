/* Copyright (c) 2017, The Linux Foundataion. All rights reserved.
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
#ifndef SPRDCAMERAREEALTUNING_H_HEADER
#define SPRDCAMERAREEALTUNING_H_HEADER

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
#else
#include <gralloc_priv.h>
#endif
#include <ui/GraphicBuffer.h>
#include "../SprdCamera3HWI.h"
#include "SprdMultiCam3Common.h"
#include <ui/GraphicBufferAllocator.h>
#include "SprdCamera3MultiBase.h"

namespace sprdcamera {

#define LOCAL_TPREVIEW_NUM (8)
#define MAX_NUM_TUNING_STREAMS 2

typedef enum { CAM_TYPE_TMAIN = 0, CAM_TYPE_TAUX } TCameraDeviceType;

typedef enum {
    /* Main camera device id*/
    CAM_TMAIN_ID = 0,
    /* Aux camera device id*/
    CAM_TAUX_ID = 2
} TCameraID;

class SprdDualCamera3Tuning : SprdCamera3MultiBase {
  public:
    static void getTCamera(SprdDualCamera3Tuning **pTuningCam);
    static int camera_device_open(__unused const struct hw_module_t *module,
                                  const char *id,
                                  struct hw_device_t **hw_device);
    static int close_camera_device(struct hw_device_t *device);
    static int get_camera_info(int camera_id, struct camera_info *info);
    static int initialize(const struct camera3_device *device,
                          const camera3_callback_ops_t *ops);
    static int configure_streams(const struct camera3_device *device,
                                 camera3_stream_configuration_t *stream_list);
    static const camera_metadata_t *
    construct_default_request_settings(const struct camera3_device *, int type);
    static int process_capture_request(const struct camera3_device *device,
                                       camera3_capture_request_t *request);
    static void notifyMain(const struct camera3_callback_ops *ops,
                           const camera3_notify_msg_t *msg);
    static void
    process_capture_result_main(const struct camera3_callback_ops *ops,
                                const camera3_capture_result_t *result);
    static void
    process_capture_result_aux(const struct camera3_callback_ops *ops,
                               const camera3_capture_result_t *result);
    static void notifyAux(const struct camera3_callback_ops *ops,
                          const camera3_notify_msg_t *msg);
    static void dump(const struct camera3_device *device, int fd);
    static int flush(const struct camera3_device *device);

    static camera3_device_ops_t mCameraCaptureOps;
    static camera3_callback_ops callback_ops_main;
    static camera3_callback_ops callback_ops_aux;

  private:
    sprdcamera_physical_descriptor_t *m_pPhyCamera;
    sprd_virtual_camera_t m_VirtualCamera;
    uint8_t m_nPhyCameras;
    Mutex mLock;
    camera_metadata_t *mStaticMetadata;

    multi_request_saved_t mThumbReq;
    int mCaptureStreamsNum;
    buffer_handle_t *mCallbackBuffer;
    List<new_mem_t *> mLocalBufferList;
    camera3_stream_t mMainStreams[MAX_NUM_TUNING_STREAMS];
    camera3_stream_t mAuxStreams[MAX_NUM_TUNING_STREAMS];
    int mCaptureWidth;
    int mCaptureHeight;
    int mPreviewWidth;
    int mPreviewHeight;
    uint8_t mPreviewStreamsNum;
    uint32_t mCapFrameNumber;
    int mLocalBufferNumber;
    const camera3_callback_ops_t *mCallbackOps;
    bool mhasCallbackStream;
    Mutex mRequestLock;
    new_mem_t mLocalBuffer[LOCAL_TPREVIEW_NUM + 1];
    List<multi_request_saved_t> mSavedRequestList;
    camera3_stream_t *mSaveSnapStream;
    int cameraDeviceOpen(int camera_id, struct hw_device_t **hw_device);
    int setupPhysicalCameras();
    int getCameraInfo(struct camera_info *info);
    void freeLocalBuffer();
    void saveRequest(camera3_capture_request_t *request);
    int allocateBuff();
    void CallBackSnapResult();

  public:
    SprdDualCamera3Tuning();
    virtual ~SprdDualCamera3Tuning();

    int initialize(const camera3_callback_ops_t *callback_ops);
    int configureStreams(const struct camera3_device *device,
                         camera3_stream_configuration_t *stream_list);
    int processCaptureRequest(const struct camera3_device *device,
                              camera3_capture_request_t *request);
    void notifyMain(const camera3_notify_msg_t *msg);
    void processCaptureResultMain(const camera3_capture_result_t *result);

    void processCaptureResultAux(const camera3_capture_result_t *result);
    const camera_metadata_t *
    constructDefaultRequestSettings(const struct camera3_device *device,
                                    int type);
    void _dump(const struct camera3_device *device, int fd);
    int _flush(const struct camera3_device *device);
    int closeCameraDevice();
};
};

#endif /* SPRDCAMERAMU*/
