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
#ifndef SPRDCAMERA3DCAPTURE_H_HEADER
#define SPRDCAMERA3DCAPTURE_H_HEADER

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
#include "SprdMultiCam3Common.h"
//#include "ts_makeup_api.h"

namespace sprdcamera {

#define LOCAL_CAPBUFF_NUM 3
#define MAX_NUM_CAMERAS 3
#ifndef MAX_NUM_STREAMS
#define MAX_NUM_STREAMS 3
#endif
#define MAX_CAP_QEQUEST_BUF 5

typedef struct {
    uint32_t frame_number;
    uint32_t preview_id;
    buffer_handle_t *buffer;
    camera3_stream_t *stream;
    camera3_stream_buffer_t *input_buffer;
} request_saved_t;

typedef enum {
    CAPTURE_MSG_DATA_PROC = 1,
    CAPTURE_MSG_COMBAIN_PROC,
    CAPTURE_MSG_EXIT
} captureMsgType;

typedef struct {
    uint32_t frame_number;
    const camera3_stream_buffer_t *input_buffer;
    buffer_handle_t *buffer1;
    buffer_handle_t *buffer2;
} buffer_combination_t;

typedef struct {
    captureMsgType msg_type;
    buffer_combination_t combo_buff;
} capture_queue_msg_t;

class SprdCamera3Capture {
  public:
    static void getCameraCapture(SprdCamera3Capture **pCapture);
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
    Mutex mLock1;
    camera_metadata_t *mStaticMetadata;
    int mLastWidth;
    int mLastHeight;
    int mCaptureWidth;
    int mCaptureHeight;
    bool mIommuEnabled;
    new_mem_t *mLocalBuffer;
    new_mem_t *mLocalCapBuffer;
    const native_handle_t *mNativeBuffer[MAX_CAP_QEQUEST_BUF];
    const native_handle_t *mNativeCapBuffer[LOCAL_CAPBUFF_NUM];
    List<buffer_handle_t *> mLocalBufferList;
    List<request_saved_t> mSavedRequestList;
    camera3_stream_t *mSavedReqStreams[MAX_NUM_STREAMS];
    uint8_t mPreviewStreamsNum;
    int mPreviewID;
    Mutex mRequestLock;
    int mLastShowPreviewDeviceId;
    uint32_t mHasSendFrameNumber;
    uint32_t mWaitFrameNumber;
    Mutex mWaitFrameMutex;
    Condition mWaitFrameSignal;
    bool mIsCapturing;
    int32_t mPerfectskinlevel;
    int32_t g_face_info[4];
    int cameraDeviceOpen(int camera_id, struct hw_device_t **hw_device);
    int setupPhysicalCameras();
    int getCameraInfo(struct camera_info *info);
    void get3DCaptureSize(int *pWidth, int *pHeight);
    void set3DCaptureMode();
    int getStreamType(camera3_stream_t *new_stream);
    void freeLocalBuffer(new_mem_t *pLocalBuffer);
    void freeLocalCapBuffer(new_mem_t *pLocalCapBuffer);
    int allocateOne(int w, int h, uint32_t is_cache, new_mem_t *new_mem);
    int validateCaptureRequest(camera3_capture_request_t *request);
    void saveRequest(camera3_capture_request_t *request,
                     uint32_t showPreviewDeviceId);
    int pushRequestList(buffer_handle_t *request, List<buffer_handle_t *> &);
    buffer_handle_t *popRequestList(List<buffer_handle_t *> &list);

  public:
    SprdCamera3Capture();
    virtual ~SprdCamera3Capture();

    class CaptureThread : public Thread {
      public:
        CaptureThread();
        ~CaptureThread();
        virtual bool threadLoop();
        virtual void requestExit();
        void videoErrorCallback(uint32_t frame_number);
        int loadGpuApi();
        void unLoadGpuApi();
        void initGpuData();
        // This queue stores matched buffer as frame_matched_info_t
        List<capture_queue_msg_t> mCaptureMsgList;
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        const camera3_callback_ops_t *mCallbackOps;
        sprdcamera_physical_descriptor_t *mDevMain;
        buffer_handle_t *mSavedResultBuff;
        camera3_capture_request_t mSavedCapRequest;
        camera3_stream_buffer_t mSavedCapReqstreambuff;
        camera_metadata_t *mSavedCapReqsettings;
        camera3_stream_t mMainStreams[MAX_NUM_STREAMS];
        camera3_stream_t mAuxStreams[MAX_NUM_STREAMS];
        uint8_t mCaptureStreamsNum;
        int m3DCaptureWidth;
        int m3DCaptureHeight;
        bool mReprocessing;
        GPUAPI_t *mGpuApi;
        line_buf_t pt_line_buf;
        struct stream_info_s pt_stream_info;
        bool isInitRenderContest;

      private:
        int mVFrameCount;
        int mVLastFrameCount;
        nsecs_t mVLastFpsTime;
        double mVFps;
        void dumpFps();
        void waitMsgAvailable();
        int combineTwoPicture(/*out*/ buffer_handle_t *&output_buf,
                              buffer_handle_t *inputbuff1,
                              buffer_handle_t *inputbuff2);
        void reProcessFrame(const buffer_handle_t *frame_buffer,
                            uint32_t cur_frameid);
        void cap_3d_convert_face_info_from_preview2cap(int *ptr_cam_face_inf,
                                                       int width, int height);
        void cap_3d_doFaceMakeup(private_handle_t *private_handle,
                                 int perfect_level, int *face_info);
    };
    sp<CaptureThread> mCaptureThread;

    int initialize(const camera3_callback_ops_t *callback_ops);
    int configureStreams(const struct camera3_device *device,
                         camera3_stream_configuration_t *stream_list);
    int processCaptureRequest(const struct camera3_device *device,
                              camera3_capture_request_t *request);
    void notifyMain(const camera3_notify_msg_t *msg);
    void processCaptureResultMain(const camera3_capture_result_t *result);
    void notifyAux(const camera3_notify_msg_t *msg);
    void processCaptureResultAux(const camera3_capture_result_t *result);
    const camera_metadata_t *
    constructDefaultRequestSettings(const struct camera3_device *device,
                                    int type);
    void _dump(const struct camera3_device *device, int fd);
    void dumpImg(void *addr, int size, int fd);
    int _flush(const struct camera3_device *device);
    int closeCameraDevice();
};
};

#endif /* SPRDCAMERAMU*/
