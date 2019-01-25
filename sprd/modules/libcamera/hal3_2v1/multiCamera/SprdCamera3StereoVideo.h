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
#ifndef SPRDCAMERA3DVIDEO_H_HEADER
#define SPRDCAMERA3DVIDEO_H_HEADER

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
#include "SprdCamera3MultiBase.h"
//#include "ts_makeup_api.h"

namespace sprdcamera {

#ifndef MAX_NUM_STREAMS
#define MAX_NUM_STREAMS 3
#endif
#define MAX_VIDEO_QEQUEST_BUF 20
#ifndef MAX_UNMATCHED_QUEUE_SIZE
#define MAX_UNMATCHED_QUEUE_SIZE 3
#endif
#ifndef MAX_SAVE_REQUEST_QUEUE_SIZE
#define MAX_SAVE_REQUEST_QUEUE_SIZE 10
#endif
#define VIDEO_TIME_DIFF (15e6)
#ifndef MAX_NOTIFY_QUEUE_SIZE
#define MAX_NOTIFY_QUEUE_SIZE 10
#endif

typedef struct {
    int srcWidth;
    int srcHeight;
    int stereoVideoWidth;
    int stereoVideoHeight;
} video_size;

class SprdCamera3StereoVideo : SprdCamera3MultiBase {
  public:
    static void getCameraMuxer(SprdCamera3StereoVideo **pMuxer);
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

    static camera3_device_ops_t mCameraMuxerOps;
    static camera3_callback_ops callback_ops_main;
    static camera3_callback_ops callback_ops_aux;

  private:
    sprdcamera_physical_descriptor_t *m_pPhyCamera;
    sprd_virtual_camera_t m_VirtualCamera;
    uint8_t m_nPhyCameras;
    Mutex mLock;
    camera_metadata_t *mStaticMetadata;
    camera3_stream_t mAuxStreams[MAX_VIDEO_QEQUEST_BUF];
    uint8_t mVideoStreamsNum;
    uint8_t mPreviewStreamsNum;
    bool mIsRecording;
    int mShowPreviewDeviceId;
    int mLastShowPreviewDeviceId;
    int mFirstShowPreviewDeviceId;
    bool mFirstConfig;
    uint32_t mFirstFrameNum;
    int mLastLowpowerFlag;
    // when notify callback ,push hw notify into notify_list, with lock
    List<camera3_notify_msg_t> mNotifyListMain;
    Mutex mNotifyLockMain;
    List<camera3_notify_msg_t> mNotifyListAux;
    Mutex mNotifyLockAux;
    uint32_t mWaitFrameNumber;
    uint32_t mHasSendFrameNumber;
    // This queue stores unmatched buffer for each hwi, accessed with lock
    Mutex mUnmatchedQueueLock;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListMain;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListAux;

    Mutex mRequest;
    Mutex mMergequeueMutex;
    Condition mMergequeueSignal;
    bool mIommuEnabled;
    Mutex mWaitFrameMutex;
    Condition mWaitFrameSignal;
    static const int64_t kPendingTime = 1000000;       // 1ms
    static const int64_t kPendingTimeOut = 5000000000; // 5s

    int cameraDeviceOpen(int camera_id, struct hw_device_t **hw_device);
    int setupPhysicalCameras();
    int getCameraInfo(struct camera_info *info);
    void get3DVideoSize(int *pWidth, int *pHeight);
    int validateCaptureRequest(camera3_capture_request_t *request);
    void saveRequest(camera3_capture_request_t *request,
                     int showPreviewDeviceId, int32_t perfectskinlevel);
    int pushRequestList(buffer_handle_t *request, List<buffer_handle_t *> &);
    buffer_handle_t *popRequestList(List<buffer_handle_t *> &list);
    bool matchTwoFrame(hwi_frame_buffer_info_t result1,
                       List<hwi_frame_buffer_info_t> &list,
                       hwi_frame_buffer_info_t *result2);
    int getStreamType(camera3_stream_t *new_stream);
    hwi_frame_buffer_info_t *
    pushToUnmatchedQueue(hwi_frame_buffer_info_t new_buffer_info,
                         List<hwi_frame_buffer_info_t> &queue);

  public:
    SprdCamera3StereoVideo();
    virtual ~SprdCamera3StereoVideo();

    class ReProcessThread : public Thread {
      public:
        ReProcessThread();
        ~ReProcessThread();
        virtual bool threadLoop();
        virtual void requestExit();
        // This queue stores muxer buffer as frame_matched_info_t
        List<muxer_queue_msg_t> mReprocessMsgList;
        Mutex mMutex;
        Condition mSignal;

      private:
        int mVFrameCount;
        int mVLastFrameCount;
        nsecs_t mVLastFpsTime;
        double mVFps;
        void dumpFps();
        void waitMsgAvailable();
        int reProcessFrame(const buffer_handle_t *frame_buffer,
                           uint32_t cur_frameid);
        void videoCallBackResult(frame_matched_info_t *combVideoResult);
        void
        video_3d_convert_face_info_from_preview2video(int *ptr_cam_face_inf,
                                                      int width, int height);
        void video_3d_doFaceMakeup(buffer_handle_t *buffer_handle,
                                   int perfect_level, int *face_info);
    };

    class MuxerThread : public Thread {
      public:
        MuxerThread();
        ~MuxerThread();
        virtual bool threadLoop();
        virtual void requestExit();
        void videoErrorCallback(uint32_t frame_number);
        int loadGpuApi();
        void unLoadGpuApi();
        void initGpuData();
        GPUAPI_t *mGpuApi;
        // This queue stores matched buffer as frame_matched_info_t
        List<muxer_queue_msg_t> mMuxerMsgList;
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        line_buf_t pt_line_buf;
        struct stream_info_s pt_stream_info;
        bool isInitRenderContest;
        sp<ReProcessThread> mReProcessThread;

      private:
        void waitMsgAvailable();
        int muxerTwoFrame(/*out*/ buffer_handle_t *&output_buf,
                          frame_matched_info_t *combVideoResult);
    };
    sp<MuxerThread> mMuxerThread;

    List<old_request> mOldVideoRequestList;
    List<old_request> mOldPreviewRequestList;

    List<buffer_handle_t *> mVideoLocalBufferList;
    Mutex mVideoLocalBufferListLock;

    new_mem_t *mVideoLocalBuffer;
    new_mem_t mPreviewLocalBuffer;
    int32_t mPerfectskinlevel;
    int mrotation;
    uint8_t mMaxLocalBufferNum;
    const camera3_callback_ops_t *mCallbackOps;
    video_size mVideoSize;
    int mMaxPendingCount;
    int mPendingRequest;
    Mutex mRequestLock;
    Condition mRequestSignal;
    void freeLocalBuffer(new_mem_t *LocalBuffer,
                         List<buffer_handle_t *> &bufferList, int bufferNum);
    int initialize(const camera3_callback_ops_t *callback_ops);
    int configureStreams(const struct camera3_device *device,
                         camera3_stream_configuration_t *stream_list);
    void clearFrameNeverMatched(int whichCamera);
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
    void dumpImg(void *addr, int size, int w, int h, int fd, int flag);
    int _flush(const struct camera3_device *device);
    int closeCameraDevice();
};
};

#endif /* SPRDCAMERAMU*/
