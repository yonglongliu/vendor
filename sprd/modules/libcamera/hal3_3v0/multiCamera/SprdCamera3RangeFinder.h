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
#ifndef SPRDCAMERARANGEFINDER_H_HEADER
#define SPRDCAMERARANGEFINDER_H_HEADER

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
#include "../SprdCamera3HWI.h"
#include "SprdMultiCam3Common.h"
#include <cmr_sensor_info.h>
#include "SprdCamera3MultiBase.h"

namespace sprdcamera {

#define MAX_NUM_CAMERAS 3
#ifndef MAX_NUM_STREAMS
#define MAX_NUM_STREAMS 2
#endif
#ifndef MAX_FINDER_QEQUEST_BUF
#define MAX_FINDER_QEQUEST_BUF 16
#endif
#define MAX_UNMATCHED_QUEUE_SIZE 3
#define FINDER_TIME_DIFF (1000e6)
#define DEPTH_ENGINE_PATH "libdepthengine.so"
#ifndef MAX_NOTIFY_QUEUE_SIZE
#define MAX_NOTIFY_QUEUE_SIZE 100
#endif
#define CLEAR_NOTIFY_QUEUE 50
#define SFACTOR 100
#define AR4_3 133
#define AR16_9 177
#define UWINY1_MAX 240
#define UWINY2_MAX 240

typedef int alDE_ERR_CODE;

typedef enum {
    /* Main camera of the related cam subsystem which controls*/
    RANGE_CAM_TYPE_MAIN = 0,
    /* Aux camera of the related cam subsystem */
    RANGE_CAM_TYPE_AUX
} rangeCameraType;

typedef enum { MSG_DATA_PROC = 1, MSG_EXIT } threadMsgType;

typedef enum {
    /* Main camera device id*/
    RANGE_CAM_MAIN_ID = 0,
    /* Aux camera device id*/
    RANGE_CAM_AUX_ID = 2
} rangeCameraID;

typedef struct {
    threadMsgType msg_type;
    frame_matched_info_t combo_frame;
} frame_matched_msg_t;

typedef struct {
    unsigned short uwInX1;
    unsigned short uwInY1;
    unsigned short uwInX2;
    unsigned short uwInY2;
} uw_Coordinate;

typedef struct {
    void *handle;
    alDE_ERR_CODE (*alSDE2_VersionInfo_Get)(char a_acOutRetbuf[256],
                                            unsigned int a_udInSize);
    alDE_ERR_CODE (*alSDE2_Init)(void *a_pInData, int a_dInSize, int a_dInImgW,
                                 int a_dInImgH, void *a_pInOTPBuf,
                                 int a_dInOTPSize);
    alDE_ERR_CODE (*alSDE2_Run)(void *a_pOutDisparity,
                                void *a_pInSub_YCC420NV21,
                                void *a_pInMain_YCC420NV21, int a_dInVCMStep);
    alDE_ERR_CODE (*alSDE2_DistanceMeasurement)(
        double *a_peOutDistance, unsigned short *a_puwInDisparity,
        int a_dInDisparityW, int a_dInDisparityH, unsigned short a_uwInX1,
        unsigned short a_uwInY1, unsigned short a_uwInX2,
        unsigned short a_uwInY2, int a_dInVCM);
    alDE_ERR_CODE (*alSDE2_HolelessPolish)(unsigned short *a_puwInOutDisparity,
                                           unsigned short a_uwInImageWidth,
                                           unsigned short a_uwInImageHeight);
    alDE_ERR_CODE (*alSDE2_Abort)();
    void (*alSDE2_Close)();
} depth_engine_api_t;

class SprdCamera3RangeFinder : SprdCamera3MultiBase {
  public:
    static void getCameraRangeFinder(SprdCamera3RangeFinder **pFinder);
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

    static camera3_device_ops_t mCameraFinderOps;
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
    int mDepthWidth;
    int mDepthHeight;
    camera3_stream_t mAuxStreams[3];
    camera3_stream_t *m_pMainDepthStream;
    uint8_t mConfigStreamNum;
    uint8_t mPreviewStreamsNum;
    // when notify callback ,push hw notify into notify_list, with lock
    List<camera3_notify_msg_t> mNotifyListMain;
    Mutex mNotifyLockMain;
    List<camera3_notify_msg_t> mNotifyListAux;
    Mutex mNotifyLockAux;
    Mutex mVcmLockAux;
    // This queue stores unmatched buffer for each hwi, accessed with lock
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListMain;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListAux;
    Mutex mUnmatchedQueueLock;
    Mutex mMergequeueMutex;
    Condition mMergequeueSignal;
    currentStatus mCurrentState;
    const camera3_callback_ops_t *mCallbackOps;
    int mVcmSteps;
    int checkOtpInfo();
    int cameraDeviceOpen(int camera_id, struct hw_device_t **hw_device);
    int setupPhysicalCameras();
    int getCameraInfo(struct camera_info *info);
    void getDepthImageSize(int srcWidth, int srcHeight, int *camWidth,
                           int *camHeight);
    void savePreviewRequest(camera3_capture_request_t *request);
    void clearFrameNeverMatched(int whichCamera);

  public:
    SprdCamera3RangeFinder();
    virtual ~SprdCamera3RangeFinder();

    class SyncThread : public Thread {
      public:
        SyncThread();
        ~SyncThread();
        virtual bool threadLoop();
        virtual void requestExit();
        void unLoadGpuApi();
        void initGpuData(int w, int h, int);
        bool mGetNewestFrameForMeasure;
        // This queue stores matched buffer as frame_matched_info_t
        List<frame_matched_msg_t> mSyncMsgList;

        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;

      private:
        void waitMsgAvailable();
    };

    class MeasureThread : public Thread {
      public:
        MeasureThread();
        ~MeasureThread();
        virtual bool threadLoop();
        void waitMsgAvailable();
        int loadDepthEngine();
        int depthAlgoSanityCheck();
        void convertCoordinate(int oldWidth, int oldHeight, int newWidth,
                               int newHeight, uw_Coordinate newCoords);
        int calculateDepthValue(frame_matched_info_t *combDepthResult);
        int depthEngineInit();
        virtual void requestExit();
        bool measurePointChanged(camera3_capture_request_t *request);
        List<frame_matched_msg_t> mMeasureMsgList;
        Mutex mMeasureQueueMutex;
        Condition mMeasureQueueSignal;
        depth_engine_api_t *mDepthEngineApi;

        uint8_t mOtpData[SPRD_DUAL_OTP_SIZE];

        new_mem_t *mLocalBuffer;
        uint8_t mMaxLocalBufferNum;
        Mutex mLocalBufferLock;
        List<buffer_handle_t *> mLocalBufferList;
        const native_handle_t *mNativeBuffer[MAX_FINDER_QEQUEST_BUF];

      private:
        uw_Coordinate mCurUwcoods;
        uw_Coordinate mLastPreCoods;
    };
    sp<MeasureThread> mMeasureThread;
    sp<SyncThread> mSyncThread;
    double mUwDepth;
    double mUwDepthAccuracy;
    Mutex mDepthVauleLock;
    int initialize(const camera3_callback_ops_t *callback_ops);
    int configureStreams(const struct camera3_device *device,
                         camera3_stream_configuration_t *stream_list);
    int processCaptureRequest(const struct camera3_device *device,
                              camera3_capture_request_t *request);
    void notifyMain(const camera3_notify_msg_t *msg);
    void processCaptureResultMain(camera3_capture_result_t *result);
    void notifyAux(const camera3_notify_msg_t *msg);
    void processCaptureResultAux(const camera3_capture_result_t *result);
    const camera_metadata_t *
    constructDefaultRequestSettings(const struct camera3_device *device,
                                    int type);
    void _dump(const struct camera3_device *device, int fd);
    int _flush(const struct camera3_device *device);
    int closeCameraDevice();
    void freeLocalBuffer();
};
};

#endif /* SPRDCAMERAMU*/
