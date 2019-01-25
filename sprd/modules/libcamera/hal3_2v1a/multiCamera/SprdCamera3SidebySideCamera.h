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
#ifndef SPRDCAMERA3SIDEBYSIDE_H_HEADER
#define SPRDCAMERA3SIDEBYSIDE_H_HEADER

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
#include "SprdCamera3MultiBase.h"
#include "SGM_SPRD.h"
#include "sprd_depth_configurable_param_sbs.h"
#include "sprdbokeh.h"
#include <unistd.h>
#include <cutils/ashmem.h>
#include "gralloc_buffer_priv.h"

namespace sprdcamera {

#define SBS_LOCAL_CAPBUFF_NUM 14
#define SBS_MAX_NUM_STREAMS 3
#define SBS_THREAD_TIMEOUT 50e6
#define SBS_REFOCUS_COMMON_PARAM_NUM (18)
#define SBS_REFOCUS_2_PARAM_NUM (17)
#define SBS_AF_WINDOW_NUM (9)
#define SBS_MAX_ROI (10)
#define SBS_CALI_SEQ_LEN (20)
#define SBS_GET_SEQ_FROM_AF (0)
#define SBS_REFOCUS_PARAM_NUM                                                  \
    (SBS_AF_WINDOW_NUM + SBS_REFOCUS_2_PARAM_NUM +                             \
     SBS_REFOCUS_COMMON_PARAM_NUM + SBS_MAX_ROI * 5 + SBS_CALI_SEQ_LEN)

#define SBS_CIRCLE_SIZE_SCALE (3)
#define SBS_SMOOTH_SIZE_SCALE (8)
#define SBS_CIRCLE_VALUE_MIN (20)

#define OTP_DUALCAM_DATA_SIZE 500

#define SBS_RAW_DATA_WIDTH 3200
#define SBS_RAW_DATA_HEIGHT 1200

typedef struct {
    uint32_t frame_number;
    buffer_handle_t *buffer;
    camera3_stream_t *stream;
    camera3_stream_buffer_t *input_buffer;
} request_saved_sidebyside_t;

typedef enum {
    SBS_MSG_PREV_PROC = 0,
    SBS_MSG_MAIN_PROC,
    SBS_MSG_BOKEH_PROC,
    SBS_MSG_FLUSHING_PROC,
    SBS_MSG_EXIT
} SideBySideMsgType;

typedef enum {
    SBS_FILE_NV21 = 1,
    SBS_FILE_YUV422,
    SBS_FILE_DEPTH,
    SBS_FILE_RAW,
    SBS_FILE_JPEG,
    SBS_FILE_OTP,
    SBS_FILE_ISPINFO,
    SBS_FILE_MAX
} SideBySideFileType;

typedef struct {
    uint32_t frame_number;
    const camera3_stream_buffer_t *input_buffer;
    buffer_handle_t *main_buffer;
    buffer_handle_t *sub_buffer;
} buffer_combination_sidebyside_t;

typedef struct {
    uint32_t frame_number;
    const camera3_stream_buffer_t *input_buffer;
    buffer_handle_t *prev_buffer;
} preview_combination_sidebyside_t;

typedef struct {
    SideBySideMsgType msg_type;
    buffer_combination_sidebyside_t combo_buff;
    preview_combination_sidebyside_t prev_combo_buff;
    cmr_s64 timestamp;
} sidebyside_queue_msg_t;

class SprdCamera3SideBySideCamera : SprdCamera3MultiBase {
  public:
    static void getCameraSidebyside(SprdCamera3SideBySideCamera **pSidebyside);
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
    static void dump(const struct camera3_device *device, int fd);
    static int flush(const struct camera3_device *device);
    static camera3_device_ops_t mCameraCaptureOps;
    static camera3_callback_ops callback_ops_main;
    static void
    process_capture_result_aux(const struct camera3_callback_ops *ops,
                               const camera3_capture_result_t *result);
    static void notifyAux(const struct camera3_callback_ops *ops,
                          const camera3_notify_msg_t *msg);
    static camera3_callback_ops callback_ops_aux;

  private:
    sprdcamera_physical_descriptor_t *m_pPhyCamera;
    sprd_virtual_camera_t m_VirtualCamera;
    uint8_t m_nPhyCameras;
    Mutex mLock;
    camera_metadata_t *mStaticMetadata;
    int mPreviewWidth;
    int mPreviewHeight;
    int mCaptureWidth;
    int mCaptureHeight;
    int mCapDepthWidth;
    int mCapDepthHeight;
    int mMainCaptureWidth;
    int mMainCaptureHeight;
    int mScalingUpWidth;
    int mScalingUpHeight;
    int mSubCaptureWidth;
    int mSubCaptureHeight;
    int mDepInputWidth;
    int mDepInputHeight;
    int mDepthBufferSize;
    int mFnumber;
    int mCoveredValue;
    int mBokehLevel;
    int mIsSrProcessed;
    int mRealtimeBokeh;
    int mBokehPointX;
    int mBokehPointY;
    int mIspdebuginfosize;
    bool mIommuEnabled;
    new_mem_t mLocalCapBuffer[SBS_LOCAL_CAPBUFF_NUM];
    uint32_t mRotation;
    int mJpegOrientation;
    bool mFlushing;
    List<request_saved_sidebyside_t> mSavedRequestList;
    List<camera3_notify_msg_t> mNotifyListMain;
    Mutex mNotifyLockMain;
    Mutex mResultLock;
    camera3_stream_t *mSavedReqStreams[SBS_MAX_NUM_STREAMS];
    request_saved_sidebyside_t mPrevOutReq;
    int mPreviewStreamsNum;
    Mutex mRequestLock;
    bool mIsCapturing;
    int mjpegSize;
    bool mIsWaitSnapYuv;
    uint8_t mCameraId;
    int32_t mPerfectskinlevel;
    bool mhasCallbackStream;
    request_saved_sidebyside_t mThumbReq;
    void *mDepthHandle;
    int mDepthState;
    int cameraDeviceOpen(int camera_id, struct hw_device_t **hw_device);
    int setupPhysicalCameras();
    int getCameraInfo(int blur_camera_id, struct camera_info *info);
    void freeLocalCapBuffer();
    int allocateBuffer(int w, int h, uint32_t is_cache, int format,
                       new_mem_t *new_mem);
    void saveRequest(camera3_capture_request_t *request);
    virtual void dumpFile(unsigned char *addr, int type, int size, int width,
                          int height, int frame_num, int param, int ismaincam);
    int thumbYuvProc(buffer_handle_t *src_buffer);

  public:
    SprdCamera3SideBySideCamera();
    virtual ~SprdCamera3SideBySideCamera();

    class CaptureThread : public Thread {
      public:
        CaptureThread();
        ~CaptureThread();
        virtual bool threadLoop();
        virtual void requestExit();
        void ProcessDepthImage(unsigned char *subbuffer,
                               unsigned char *mainbuffer,
                               unsigned char *Depthoutbuffer);
        void ProcessBokehImage(unsigned char *inputbuffer,
                               unsigned char *outputbuffer,
                               unsigned char *Depthoutbuffer);
        void saveCaptureBokehParams(buffer_handle_t *mSavedResultBuff,
                                    unsigned char *buffer,
                                    unsigned char *disparity_bufer);
        void Save3dVerificationParam(buffer_handle_t *mSavedResultBuff,
                                     unsigned char *main_yuv,
                                     unsigned char *sub_yuv);
        // This queue stores matched buffer as frame_matched_info_t
        List<sidebyside_queue_msg_t> mCaptureMsgList;
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        const camera3_callback_ops_t *mCallbackOps;
        sprdcamera_physical_descriptor_t *mDevMain;
        sprdcamera_physical_descriptor_t *mDevAux;
        buffer_handle_t *mSavedResultBuff;
        camera3_capture_request_t mSavedCapRequest;
        camera3_stream_buffer_t mSavedCapReqstreambuff;
        camera_metadata_t *mSavedCapReqsettings;
        camera_metadata_t *mSavedPreReqsettings;
        camera3_stream_t mMainStreams[SBS_MAX_NUM_STREAMS];
        camera3_stream_t mAuxStreams[SBS_MAX_NUM_STREAMS];
        uint8_t mCaptureStreamsNum;
        bool mReprocessing;
        bool mBokehConfigParamState;
        int32_t m3dVerificationEnable;
        int mRealtimeBokehNum;
        int32_t mFaceInfo[4];
        int8_t mOtpData[OTP_DUALCAM_DATA_SIZE];

      private:
        void waitMsgAvailable();
        void SideBySideFaceMakeup(private_handle_t *private_handle);
    };
    sp<CaptureThread> mCaptureThread;
    Mutex mMergequeueFinishMutex;
    Condition mMergequeueFinishSignal;

    int initialize(const camera3_callback_ops_t *callback_ops);
    int configureStreams(const struct camera3_device *device,
                         camera3_stream_configuration_t *stream_list);
    int processCaptureRequest(const struct camera3_device *device,
                              camera3_capture_request_t *request);
    int processAuxCaptureRequest(camera3_capture_request_t *request);
    void notifyMain(const camera3_notify_msg_t *msg);
    void processCaptureResultMain(camera3_capture_result_t *result);
    void processCaptureResultAux(camera3_capture_result_t *result);
    void CallBackResult(uint32_t frame_number,
                        camera3_buffer_status_t buffer_status);
    const camera_metadata_t *
    constructDefaultRequestSettings(const struct camera3_device *device,
                                    int type);
    void CallBackSnapResult();
    void _dump(const struct camera3_device *device, int fd);

    int _flush(const struct camera3_device *device);
    int closeCameraDevice();
};
};

#endif /* SPRDCAMERAMU*/
