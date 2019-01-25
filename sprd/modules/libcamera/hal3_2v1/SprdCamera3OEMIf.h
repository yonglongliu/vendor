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
#ifndef _SPRDCAMERA3_OEMIF_H
#define _SPRDCAMERA3_OEMIF_H

#include "MemIon.h"
#include <utils/threads.h>
#include <pthread.h>
#include <semaphore.h>
extern "C" {
//    #include <linux/android_pmem.h>
}
#include <sys/types.h>

#include <utils/threads.h>
#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/BinderService.h>
#ifndef MINICAMERA
#include <binder/MemoryBase.h>
#endif
#include <binder/MemoryHeapBase.h>
#include <hardware/camera.h>
#include <hardware/gralloc.h>
#include <camera/CameraParameters.h>
#include "SprdCameraParameters.h"
#include "cmr_common.h"
#include "cmr_oem.h"
#include "sprd_dma_copy_k.h"

#include "SprdCamera3Setting.h"
#include "SprdCamera3Stream.h"
#include "SprdCamera3Channel.h"
#include "SprdCameraSystemPerformance.h"
#include <hardware/power.h>
#ifdef CONFIG_CAMERA_GYRO
#include <android/sensor.h>
#ifdef CONFIG_SPRD_ANDROID_8
#include <sensor/Sensor.h>
#else
#include <gui/Sensor.h>
#include <gui/SensorManager.h>
#include <gui/SensorEventQueue.h>
#endif
#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/Looper.h>
#endif
#ifdef CONFIG_CAMERA_EIS
#include "sprd_eis.h"
#endif
#ifdef CONFIG_FACE_BEAUTY
#include "camera_face_beauty.h"
#endif

#include <sys/socket.h>
#include <cutils/sockets.h>
#include <ui/GraphicBuffer.h>

using namespace android;

namespace sprdcamera {

#ifndef MINICAMERA
typedef void (*preview_callback)(sp<MemoryBase>, void *);
typedef void (*recording_callback)(sp<MemoryBase>, void *);
typedef void (*shutter_callback)(void *);
typedef void (*raw_callback)(sp<MemoryBase>, void *);
typedef void (*jpeg_callback)(sp<MemoryBase>, void *);
#endif
typedef void (*autofocus_callback)(bool, void *);
typedef void (*channel_callback)(int index, void *user_data);
typedef void (*timer_handle_func)(union sigval arg);

typedef enum {
    SNAPSHOT_DEFAULT_MODE = 0,
    SNAPSHOT_NO_ZSL_MODE,
    SNAPSHOT_ZSL_MODE,
    SNAPSHOT_PREVIEW_MODE,
    SNAPSHOT_ONLY_MODE,
    SNAPSHOT_VIDEO_MODE,
} snapshot_mode_type_t;

typedef enum {
    RATE_STOP = 0,
    RATE_NORMAL,
    RATE_FAST,
    RATE_VERY_FAST,
} sensor_rate_level_t;

/*
*  fd:         ion fd
*  phys_addr:  offset from fd, always set 0
*/
typedef struct sprd_camera_memory {
    MemIon *ion_heap;
    cmr_uint phys_addr;
    cmr_uint phys_size;
    cmr_s32 fd;
    cmr_s32 dev_fd;
    void *handle;
    void *data;
    bool busy_flag;
} sprd_camera_memory_t;

typedef struct sprd_3dnr_memory {
    sp<GraphicBuffer> bufferhandle;
    void *private_handle;
} sprd_3dnr_memory_t;

struct ZslBufferQueue {
    camera_frame_type frame;
    sprd_camera_memory_t *heap_array;
};

typedef struct {
    List<ZslBufferQueue> *cam1_ZSLQueue;
    List<ZslBufferQueue> *cam3_ZSLQueue;
    ZslBufferQueue match_frame1;
    ZslBufferQueue match_frame3;
    int cam1_id;
    int cam3_id;
} multi_camera_zsl_match_frame;

typedef struct {
    int32_t preWidth;
    int32_t preHeight;
    uint32_t *buffPhy;
    uint32_t *buffVir;
    uint32_t heapSize;
    uint32_t heapNum;
} camera_oem_buff_info_t;

#define MAX_SUB_RAWHEAP_NUM 10
#define MAX_LOOP_COLOR_COUNT 3
#define MAX_Y_UV_COUNT 2
#define ISP_TUNING_WAIT_MAX_TIME 4000

#define USE_ONE_RESERVED_BUF 1

// 9820e_4.4.4_NULL
#ifdef ANDROID_VERSION_KK
#define SPRD_NULL (0)
#else
#define SPRD_NULL (void *)0
#endif

class SprdCamera3OEMIf : public virtual RefBase {
  public:
    SprdCamera3OEMIf(int cameraId, SprdCamera3Setting *setting);
    virtual ~SprdCamera3OEMIf();
    inline int getCameraId() const;
    virtual void closeCamera();
    virtual int takePicture();
    virtual int cancelPicture();
    virtual int setTakePictureSize(uint32_t width, uint32_t height);
    virtual status_t faceDectect(bool enable);
    virtual status_t faceDectect_enable(bool enable);
    virtual status_t autoFocus();
    virtual status_t cancelAutoFocus();
    virtual status_t setAePrecaptureSta(uint8_t state);
    virtual int openCamera();
    void initialize();
    void setCaptureRawMode(bool mode);
    void setCallBackYuvMode(bool mode); /**add for 3d capture*/
    void setCaptureReprocessMode(bool mode, uint32_t width,
                                 uint32_t height); /**add for 3d capture*/
    void antiShakeParamSetup();
    int displayCopy(cmr_uint dst_phy_addr, cmr_uint dst_virtual_addr,
                    cmr_uint src_phy_addr, cmr_uint src_virtual_addr,
                    uint32_t src_w, uint32_t src_h);
    bool isFaceBeautyOn(SPRD_DEF_Tag sprddefInfo);

    int flushIonBuffer(int buffer_fd, void *v_addr, void *p_addr, size_t size);
    sprd_camera_memory_t *allocCameraMem(int buf_size, int num_bufs,
                                         uint32_t is_cache);
    sprd_camera_memory_t *allocReservedMem(int buf_size, int num_bufs,
                                           uint32_t is_cache);
    int start(camera_channel_type_t channel_type, uint32_t frame_number);
    int stop(camera_channel_type_t channel_type, uint32_t frame_number);
    int releasePreviewFrame(int i);
    int setCameraConvertCropRegion(void);
    int CameraConvertCropRegion(uint32_t sensorWidth, uint32_t sensorHeight,
                                struct img_rect *cropRegion);
    inline bool isCameraInit();
    int SetCameraParaTag(cmr_int cameraParaTag);
    int SetJpegGpsInfo(bool is_set_gps_location);
    int setCapturePara(camera_capture_mode_t stream_type,
                       uint32_t frame_number);
    int SetChannelHandle(void *regular_chan, void *picture_chan);
    int SetDimensionPreview(cam_dimension_t preview_size);
    int SetDimensionVideo(cam_dimension_t video_size);
    int SetDimensionRaw(cam_dimension_t raw_size);
    int SetDimensionCapture(cam_dimension_t capture_size);
    int CameraConvertCoordinateToFramework(int32_t *rect);
    int CameraConvertCoordinateFromFramework(int32_t *rect);

    int PushPreviewbuff(buffer_handle_t *buff_handle);
    int PushVideobuff(buffer_handle_t *buff_handle);
    int PushZslbuff(buffer_handle_t *buff_handle);
    int PushFirstPreviewbuff();
    int PushFirstVideobuff();
    int PushFirstZslbuff();
    int PushVideoSnapShotbuff(int32_t frame_number, camera_stream_type_t type);
    int PushZslSnapShotbuff();
    snapshot_mode_type_t GetTakePictureMode();
    camera_status_t GetCameraStatus(camera_status_type_t state);
    void bindcoreEnabled();
    int IommuIsEnabled(void);
    void setSensorCloseFlag();
    int checkIfNeedToStopOffLineZsl();
    bool isIspToolMode();
    void ispToolModeInit();
    uint64_t getZslBufferTimestamp(); /**add for 3dcapture, get zsl buffer's
                                         timestamp in zsl query*/
    int getMultiCameraMode(void);
    void setMultiCallBackYuvMode(bool mode);
    void setSprdCameraLowpower(int flag);
    int autoFocusToFaceFocus();
    bool isNeedAfFullscan();
    int camera_ioctrl(int cmd, void *param1, void *param2);
    bool isVideoCopyFromPreview();
    int getCppMaxSize(cam_dimension_t *);
    void setMimeType(int type);

  public:
    static int pre_alloc_cap_mem_thread_init(void *p_data);
    static int pre_alloc_cap_mem_thread_deinit(void *p_data);
    static void *pre_alloc_cap_mem_thread_proc(void *p_data);
    uint32_t isPreAllocCapMem();

    static int ZSLMode_monitor_thread_init(void *p_data);
    static int ZSLMode_monitor_thread_deinit(void *p_data);
    static cmr_int ZSLMode_monitor_thread_proc(struct cmr_msg *message,
                                               void *p_data);

    void setIspFlashMode(uint32_t mode);
    void matchZSLQueue(ZslBufferQueue *frame);
    void setMultiCameraMode(multiCameraMode mode);
#ifdef CONFIG_CAMERA_EIS
    virtual void EisPreview_init();
    virtual void EisVideo_init();
    vsOutFrame processPreviewEIS(vsInFrame frame_in);
    vsOutFrame processVideoEIS(vsInFrame frame_in);
    void pushEISPreviewQueue(vsGyro *mGyrodata);
    void popEISPreviewQueue(vsGyro *gyro, int gyro_num);
    void pushEISVideoQueue(vsGyro *mGyrodata);
    void popEISVideoQueue(vsGyro *gyro, int gyro_num);
    void EisPreviewFrameStab(struct camera_frame_type *frame);
    vsOutFrame EisVideoFrameStab(struct camera_frame_type *frame,
                                 uint32_t frame_num);
#endif

#ifdef CONFIG_CAMERA_GYRO
    static int gyro_monitor_thread_init(void *p_data);
    static int gyro_monitor_thread_deinit(void *p_data);
    static void *gyro_monitor_thread_proc(void *p_data);
    static void *gyro_ASensorManager_process(void *p_data);
    static void *gyro_SensorManager_process(void *p_data);
    static int gyro_get_data(void *p_data, ASensorEvent *buffer, int n,
                             struct cmr_af_aux_sensor_info *sensor_info);
#endif
    void setCamPreformaceScene(sys_performance_camera_scene camera_scene);

    int mBurstVideoSnapshot;
    int mVideoParameterSetFlag;
    bool mSetCapRatioFlag;
    bool mVideoCopyFromPreviewFlag;
    // sw 3dnr solution used
    bool mUsingSW3DNR;               // only for blacksesame 3dnr (sw solution)
    bool mVideoProcessedWithPreview; // only for blacksesame 3dnr (sw solution)
  private:
    inline void print_time();

    /*
    * fd:       ion fd
    * phys_addr phys_addr is offset from fd
    */
    struct OneFrameMem {
        sp<MemIon> pmem_hp;
        size_t size;
        int fd;
        unsigned long phys_addr;
        unsigned char *virt_addr;
        uint32_t width;
        uint32_t height;
    };

    enum shake_test_state {
        NOT_SHAKE_TEST,
        SHAKE_TEST,
    };

    struct ShakeTest {
        int diff_yuv_color[MAX_LOOP_COLOR_COUNT][MAX_Y_UV_COUNT];
        uint32_t mShakeTestColorCount;
        shake_test_state mShakeTestState;
    };

    enum Sprd_Camera_Temp {
        CAMERA_NORMAL_TEMP,
        CAMERA_HIGH_TEMP,
        CAMERA_DANGER_TEMP,
    };

    void freeCameraMem(sprd_camera_memory_t *camera_memory);
    void freeAllCameraMemIon();
    void canclePreviewMem();
    bool allocatePreviewMemByGraphics();
    bool allocatePreviewMem();
    void freePreviewMem();
    bool allocateCaptureMem(bool initJpegHeap);
    void freeCaptureMem();
    int getRedisplayMem(uint32_t width, uint32_t height);
    void FreeReDisplayMem();
    static void camera_cb(camera_cb_type cb, const void *client_data,
                          camera_func_type func, void *parm4);
    void receiveRawPicture(struct camera_frame_type *frame);
    void receiveJpegPicture(struct camera_frame_type *frame);
    void receivePreviewFrame(struct camera_frame_type *frame);
    void receivePreviewFDFrame(struct camera_frame_type *frame);
    void receiveCameraExitError(void);
    void receiveTakePictureError(void);
    void receiveJpegPictureError(void);
    bool receiveCallbackPicture(uint32_t width, uint32_t height, cmr_s32 fd,
                                cmr_uint phy_addr, char *virtual_addr);
    void HandleStopCamera(enum camera_cb_type cb, void *parm4);
    void HandleStartCamera(enum camera_cb_type cb, void *parm4);
    void HandleStartPreview(enum camera_cb_type cb, void *parm4);
    void HandleStopPreview(enum camera_cb_type cb, void *parm4);
    void HandleTakePicture(enum camera_cb_type cb, void *parm4);
    void HandleEncode(enum camera_cb_type cb, void *parm4);
    void HandleFocus(enum camera_cb_type cb, void *parm4);
    void HandleAutoExposure(enum camera_cb_type cb, void *parm4);
    void HandleCancelPicture(enum camera_cb_type cb, void *parm4);
    void calculateTimestampForSlowmotion(int64_t frm_timestamp);
    void doFaceMakeup(struct camera_frame_type *frame);
    int getCameraTemp();
    void adjustFpsByTemp();

    enum Sprd_camera_state {
        SPRD_INIT,
        SPRD_IDLE,
        SPRD_ERROR,
        SPRD_PREVIEW_IN_PROGRESS,
        SPRD_FOCUS_IN_PROGRESS,
        SPRD_SET_PARAMS_IN_PROGRESS,
        SPRD_WAITING_RAW,
        SPRD_WAITING_JPEG,
        SPRD_FLASH_IN_PROGRESS,

        // internal states
        SPRD_INTERNAL_PREVIEW_STOPPING,
        SPRD_INTERNAL_CAPTURE_STOPPING,
        SPRD_INTERNAL_PREVIEW_REQUESTED,
        SPRD_INTERNAL_RAW_REQUESTED,
        SPRD_INTERNAL_STOPPING,

    };

    enum state_owner {
        STATE_CAMERA,
        STATE_PREVIEW,
        STATE_CAPTURE,
        STATE_FOCUS,
        STATE_SET_PARAMS,
    };

    typedef struct _camera_state {
        Sprd_camera_state camera_state;
        Sprd_camera_state preview_state;
        Sprd_camera_state capture_state;
        Sprd_camera_state focus_state;
        Sprd_camera_state setParam_state;
    } camera_state;

    typedef struct _slow_motion_para {
        int64_t rec_timestamp;
        int64_t last_frm_timestamp;
    } slow_motion_para;

    const char *getCameraStateStr(Sprd_camera_state s);
    Sprd_camera_state transitionState(Sprd_camera_state from,
                                      Sprd_camera_state to, state_owner owner,
                                      bool lock = true);
    void setCameraState(Sprd_camera_state state,
                        state_owner owner = STATE_CAMERA);
    inline Sprd_camera_state getCameraState();
    inline Sprd_camera_state getPreviewState();
    inline Sprd_camera_state getCaptureState();
    inline Sprd_camera_state getFocusState();
    inline Sprd_camera_state getSetParamsState();
    inline bool isCameraError();
    inline bool isCameraIdle();
    inline bool isPreviewing();
    inline bool isPreviewStart();
    inline bool isCapturing();
    bool WaitForPreviewStart();
    bool WaitForPreviewStop();
    bool WaitForCaptureDone();
    bool WaitForCameraStart();
    bool WaitForCameraStop();
    bool WaitForCaptureJpegState();
    bool WaitForFocusCancelDone();
    bool WaitForBurstCaptureDone();
    bool startCameraIfNecessary();
    void getPictureFormat(int *format);
    takepicture_mode getCaptureMode();
    bool getCameraLocation(camera_position_type *pt);
    int startPreviewInternal();
    void stopPreviewInternal();
    int cancelPictureInternal();
    bool initPreview();
    void deinitPreview();
    void deinitCapture(bool isPreAllocCapMem);
    bool setCameraPreviewDimensions();
    bool setCameraCaptureDimensions();
    void setCameraPreviewMode(bool isRecordMode);
    bool setCameraPreviewFormat();
    bool displayOneFrameForCapture(uint32_t width, uint32_t height, int fd,
                                   cmr_uint phy_addr, char *virtual_addr,
                                   struct camera_frame_type *frame);
    bool iSDisplayCaptureFrame();
    bool iSCallbackCaptureFrame();
    bool iSZslMode();
    bool checkPreviewStateForCapture();
    bool getLcdSize(uint32_t *width, uint32_t *height);
    bool switchBufferMode(uint32_t src, uint32_t dst);
    void setCameraPrivateData(void);
    void overwritePreviewFrame(camera_frame_type *frame);
    int overwritePreviewFrameMemInit(
        struct SprdCamera3OEMIf::OneFrameMem *one_frame_mem_ptr);
    void shakeTestInit(ShakeTest *tmpShakeTest);
    void setShakeTestState(shake_test_state state);
    shake_test_state getShakeTestState();

    int
    allocOneFrameMem(struct SprdCamera3OEMIf::OneFrameMem *one_frame_mem_ptr);
    int
    relaseOneFrameMem(struct SprdCamera3OEMIf::OneFrameMem *one_frame_mem_ptr);
    int initDefaultParameters();
    int handleCbData(hal3_trans_info_t &result_info, void *userdata);
    int zslTakePicture();
    int zslTakePictureL();
    int reprocessYuvForJpeg();
    int reprocessYuvForJpeg(frm_info *frm_data);
    int VideoTakePicture();
    int setVideoSnapshotParameter();
    int chooseDefaultThumbnailSize(uint32_t *thumbWidth, uint32_t *thumbHeight);

    int timer_stop();
    int timer_set(void *obj, int32_t delay_ms, timer_handle_func handler);
    static void timer_hand(union sigval arg);
    static void timer_hand_take(union sigval arg);
    void cpu_dvfs_disable(uint8_t is_disable);
    void cpu_hotplug_disable(uint8_t is_disable);
    void prepareForPostProcess(void);
    void exitFromPostProcess(void);

    int Callback_VideoFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_s32 *fd,
                           cmr_u32 sum);
    int Callback_VideoMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                             cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_PreviewFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                             cmr_s32 *fd, cmr_u32 sum);
    int Callback_PreviewMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_ZslFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_s32 *fd,
                         cmr_u32 sum);
    int Callback_ZslMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                           cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_RefocusFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                             cmr_u32 sum);
    int Callback_RefocusMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_PdafRawFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                             cmr_u32 sum);
    int Callback_PdafRawMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_CaptureFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                             cmr_s32 *fd, cmr_u32 sum);
    int Callback_CaptureMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_CapturePathMalloc(cmr_u32 size, cmr_u32 sum,
                                   cmr_uint *phy_addr, cmr_uint *vir_addr,
                                   cmr_s32 *fd);
    int Callback_CapturePathFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                 cmr_s32 *fd, cmr_u32 sum);
    int Callback_Sw3DNRCaptureFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                   cmr_s32 *fd, cmr_u32 sum);
    int Callback_Sw3DNRCaptureMalloc(cmr_u32 size, cmr_u32 sum,
                                     cmr_uint *phy_addr, cmr_uint *vir_addr,
                                     cmr_s32 *fd, void **handle, cmr_uint width,
                                     cmr_uint height);
    int Callback_Sw3DNRCapturePathFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                       cmr_s32 *fd, cmr_u32 sum);
    int Callback_Sw3DNRCapturePathMalloc(cmr_u32 size, cmr_u32 sum,
                                         cmr_uint *phy_addr, cmr_uint *vir_addr,
                                         cmr_s32 *fd, void **handle,
                                         cmr_uint width, cmr_uint height);
    int Callback_OtherFree(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                           cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum);
    int Callback_OtherMalloc(enum camera_mem_cb_type type, cmr_u32 size,
                             cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                             cmr_uint *vir_addr, cmr_s32 *fd);
    static int Callback_Free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                             cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum,
                             void *private_data);
    static int Callback_Malloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr,
                               cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd,
                               void *private_data);
    static int Callback_GPUMalloc(enum camera_mem_cb_type type,
                                  cmr_u32 *size_ptr, cmr_u32 *sum_ptr,
                                  cmr_uint *phy_addr, cmr_uint *vir_addr,
                                  cmr_s32 *fd, void **handle, cmr_uint *width,
                                  cmr_uint *height, void *private_data);

    // zsl start
    int getZSLQueueFrameNum();
    ZslBufferQueue popZSLQueue(uint64_t need_timestamp);
    ZslBufferQueue popZSLQueue();
    void pushZSLQueue(ZslBufferQueue *frame);
    void releaseZSLQueue();
    void setZslBuffers();
    void snapshotZsl(void *p_data);
    void skipZslFrameForFlashCapture();
    uint32_t getZslBufferIDForFd(cmr_s32 fd);
    int pushZslFrame(struct camera_frame_type *frame);
    struct camera_frame_type popZslFrame();

    List<ZslBufferQueue> mZSLQueue;
    bool mSprdZslEnabled;
    uint32_t mZslChannelStatus;
    int32_t mZslShotPushFlag;
    // we want to keep mZslMaxFrameNum buffers in mZSLQueue, for snapshot use
    int32_t mZslMaxFrameNum;
    // total zsl buffer num
    int32_t mZslNum;
    Mutex mZslBufLock;
    Mutex mZslPopLock;
    // zsl end

    bool mSprdPipVivEnabled;
    bool mSprdHighIsoEnabled;

    bool mSprdRefocusEnabled;
    bool mSprd3dCalibrationEnabled; /**add for 3d calibration */
    bool mSprdYuvCallBack;          /**add for 3d capture */
    bool mSprdMultiYuvCallBack;     /**add for 3d capture */
    bool mSprdReprocessing;         /**add for 3d capture */
    uint64_t mNeededTimestamp;      /**add for 3d capture */
    bool mIsUnpopped; /**add for 3dcapture, record unpoped zsl buffer*/
    bool mIsBlur2Zsl; /**add for blur2 capture */

    void yuvNv12ConvertToYv12(struct camera_frame_type *frame, char *tmpbuf);

    /* These constants reflect the number of buffers that libqcamera requires
    for preview and raw, and need to be updated when libqcamera
    changes.
    */
    static const uint32_t kPreviewBufferCount = 24;
    static const uint32_t kPreviewRotBufferCount = 24;
    static const uint32_t kVideoBufferCount = 24;
    static const uint32_t kVideoRotBufferCount = 24;
    static const uint32_t kZslBufferCount = 24;
    static const uint32_t kZslRotBufferCount = 24;
    static const uint32_t kRefocusBufferCount = 24;
    static const uint32_t kPdafRawBufferCount = 4;
    static const uint32_t kRawBufferCount = 1;
    static const uint32_t kJpegBufferCount = 1;
    static const uint32_t kRawFrameHeaderSize = 0x0;
    static const uint32_t kISPB4awbCount = 16;
    static multiCameraMode mMultiCameraMode;
    static multi_camera_zsl_match_frame *mMultiCameraMatchZsl;
    Mutex mLock; // API lock -- all public methods
    Mutex mPreviewCbLock;
    Mutex mCaptureCbLock;
    Mutex mStateLock;
    Condition mStateWait;
    Condition mParamWait;
    Mutex mPrevBufLock;
    Mutex mCapBufLock;
    Mutex mZslLock;
    uint32_t mCapBufIsAvail;

    uint32_t m_zslValidDataWidth;
    uint32_t m_zslValidDataHeight;

    sprd_camera_memory_t *mSubRawHeapArray[MAX_SUB_RAWHEAP_NUM];
    sprd_camera_memory_t *mPathRawHeapArray[MAX_SUB_RAWHEAP_NUM];
    sprd_3dnr_memory_t m3DNRGraphicArray[MAX_SUB_RAWHEAP_NUM];
    sprd_3dnr_memory_t m3DNRGraphicPathArray[MAX_SUB_RAWHEAP_NUM];

    sprd_camera_memory_t *mReDisplayHeap;
    // TODO: put the picture dimensions in the CameraParameters object;
    SprdCameraParameters mParameters;
    uint32_t mPreviewHeight_trimy;
    uint32_t mPreviewWidth_trimx;
    int mPreviewHeight;
    int mPreviewWidth;
    int mRawHeight;
    int mRawWidth;
    int mVideoWidth;
    int mVideoHeight;
    int mCaptureWidth;
    int mCaptureHeight;
    cmr_u16 mLargestPictureWidth;
    cmr_u16 mLargestPictureHeight;
    camera_data_format_type_t mPreviewFormat; // 0:YUV422;1:YUV420;2:RGB
    int mPictureFormat;                       // 0:YUV422;1:YUV420;2:RGB;3:JPEG
    int mPreviewStartFlag;
    uint32_t mIsDvPreview;
    uint32_t mIsStoppingPreview;

    bool mZslPreviewMode;
    bool mRecordingMode;
    bool mIsSetCaptureMode;
    nsecs_t mRecordingFirstFrameTime;
    void *mUser;
    preview_stream_ops *mPreviewWindow;
    static gralloc_module_t const *mGrallocHal;
    oem_module_t *mHalOem;
    bool mIsStoreMetaData;
    bool mIsFreqChanged;
    int32_t mCameraId;
    volatile camera_state mCameraState;
    int miSPreviewFirstFrame;
    takepicture_mode mCaptureMode;
    bool mCaptureRawMode;
    bool mIsRotCapture;
    bool mFlashMask;
    bool mReleaseFLag;
    uint32_t mTimeCoeff;
    uint32_t mPreviewBufferUsage;
    uint32_t mOriginalPreviewBufferUsage;
    uint32_t mCameraDfsPolicyCur;

    /*callback thread*/
    cmr_handle mCameraHandle;
    bool mIsPerformanceTestable;
    bool mIsAndroidZSL;
    ShakeTest mShakeTest;

    channel_callback mChannelCb;
    void *mUserData;
    SprdCamera3Setting *mSetting;
    hal_stream_info_t *mZslStreamInfo;
    List<hal3_trans_info_t> mCbInfoList;
    Condition mBurstCapWait;
    uint8_t BurstCapCnt;
    uint8_t mCapIntent;
    bool jpeg_gps_location;
    uint8_t mGps_processing_method[36];

    void *mRegularChan;
    void *mPictureChan;

    camera_preview_mode_t mParaDCDVMode;
    snapshot_mode_type_t mTakePictureMode;
    int32_t mPicCaptureCnt;

    bool mZSLTakePicture;
    timer_t mPrvTimerID;

    struct cmr_zoom_param mZoomInfo;
    int8_t mFlashMode;

    bool mJpegRotaSet;
    bool mIsAutoFocus;
    bool mIspToolStart;
    uint32_t mPreviewHeapNum;
    uint32_t mVideoHeapNum;
    uint32_t mZslHeapNum;
    uint32_t mRefocusHeapNum;
    uint32_t mPdafRawHeapNum;
    uint32_t mSubRawHeapNum;
    uint32_t m3dnrGraphicHeapNum;
    uint32_t m3dnrGraphicPathHeapNum;
    uint32_t mSubRawHeapSize;
    uint32_t mPathRawHeapNum;
    uint32_t mPathRawHeapSize;

    uint32_t mPreviewDcamAllocBufferCnt;
    sprd_camera_memory_t
        *mPreviewHeapArray[kPreviewBufferCount + kPreviewRotBufferCount + 1];
    sprd_camera_memory_t
        *mVideoHeapArray[kVideoBufferCount + kVideoRotBufferCount + 1];
    sprd_camera_memory_t
        *mZslHeapArray[kZslBufferCount + kZslRotBufferCount + 1];
    sprd_camera_memory_t *mRefocusHeapArray[kRefocusBufferCount + 1];
    sprd_camera_memory_t *mPdafRawHeapArray[kPdafRawBufferCount + 1];
    cmr_uint mRefocusHeapArray_phy[kRefocusBufferCount + 1];
    cmr_uint mRefocusHeapArray_vir[kRefocusBufferCount + 1];
    uint32_t mRefocusHeapArray_size[kRefocusBufferCount + 1];
    uint32_t mRefocusHeapArray_mfd[kRefocusBufferCount + 1];

#ifdef USE_ONE_RESERVED_BUF
    sprd_camera_memory_t *mCommonHeapReserved;
#else
    sprd_camera_memory_t *mPreviewHeapReserved;
    sprd_camera_memory_t *mVideoHeapReserved;
    sprd_camera_memory_t *mZslHeapReserved;
#endif
    frm_info mHDRPlusBackupFrm_info;
    bool mHDRPlusFillState;
    sprd_camera_memory_t *mDepthHeapReserved;
    sprd_camera_memory_t *mPdafRawHeapReserved;
    sprd_camera_memory_t *mIspLscHeapReserved;
    sprd_camera_memory_t *mIspStatisHeapReserved;
    sprd_camera_memory_t *mIspB4awbHeapReserved[kISPB4awbCount];
    sprd_camera_memory_t *mIspPreviewYReserved[2];
    static sprd_camera_memory_t *mIspFirmwareReserved;
    static uint32_t mIspFirmwareReserved_cnt;
    static bool mZslCaptureExitLoop;
    sprd_camera_memory_t *mIspYUVReserved;

    sprd_camera_memory_t *mIspAntiFlickerHeapReserved;
    sprd_camera_memory_t *mIspRawAemHeapReserved[kISPB4awbCount];
    sprd_camera_memory_t *m3DNRPrevHeapReserverd[PRE_3DNR_NUM];
    sprd_camera_memory_t *m3DNRPrevScaleHeapReserverd[PRE_SW_3DNR_RESERVE_NUM];
    sprd_camera_memory_t *m3DNRHeapReserverd;
    sprd_camera_memory_t *m3DNRScaleHeapReserverd[CAP_3DNR_NUM];

    uint32_t mPreviewHeapBakUseFlag;
    uint32_t mPreviewHeapArray_size[kPreviewBufferCount +
                                    kPreviewRotBufferCount + 1];
    buffer_handle_t *mPreviewBufferHandle[kPreviewBufferCount];
    buffer_handle_t *mPreviewCancelBufHandle[kPreviewBufferCount];
    bool mCancelBufferEb[kPreviewBufferCount];
    int32_t mGraphBufferCount[kPreviewBufferCount];

    uint32_t mPreviewFrameNum;
    uint32_t mRecordFrameNum;
    uint32_t mZslFrameNum;
    uint32_t mPictureFrameNum;
    uint32_t mStartFrameNum;
    uint32_t mStopFrameNum;
    uint32_t mDropPreviewFrameNum;
    uint32_t mDropVideoFrameNum;
    uint32_t mDropZslFrameNum;
    slow_motion_para mSlowPara;
    bool mRestartFlag;
    bool mIsRecording;
    int32_t mVideoShotNum;
    int32_t mVideoShotFlag;
    int32_t mVideoShotPushFlag;
    Condition mVideoShotWait;
    Condition mZslShotWait;

    /*pre-alloc capture memory*/
    uint32_t mIsPreAllocCapMem;
    pthread_t mPreAllocCapMemThread;
    uint32_t mPreAllocCapMemInited;
    uint32_t mIsPreAllocCapMemDone;
    sem_t mPreAllocCapMemSemDone;

    /*ZSL Monitor Thread*/
    pthread_t mZSLModeMonitorMsgQueHandle;
    uint32_t mZSLModeMonitorInited;
    bool miSBindcorePreviewFrame;
    int mBindcorePreivewFrameCount;

    /* enable/disable powerhint for hdr */
    uint32_t mHDRPowerHint;
    /* enable/disable powerhint for SW3DNR (only for capture)*/
    uint32_t m3DNRPowerHint;
    SprdCameraSystemPerformance *mSysPerformace;

    /* 1- start acceleration, 0 - finish acceleration*/

    /* for eis*/
    bool mGyroInit;
    bool mGyroExit;
    bool mEisPreviewInit;
    bool mEisVideoInit;
    pthread_t mGyroMsgQueHandle;
    int mGyroNum;
    double mGyromaxtimestamp;
    Mutex mReadGyroPreviewLock;
    Mutex mReadGyroVideoLock;
    Condition mReadGyroPreviewCond;
    Condition mReadGyroVideoCond;
    sem_t mGyro_sem;
    Mutex mEisPreviewLock;
    Mutex mEisVideoLock;
#ifdef CONFIG_CAMERA_EIS
    static const int kGyrocount = 100;
    List<vsGyro *> mGyroPreviewInfo;
    List<vsGyro *> mGyroVideoInfo;
    vsGyro mGyrodata[kGyrocount];
    vsParam mPreviewParam;
    vsParam mVideoParam;
    vsInst mPreviewInst;
    vsInst mVideoInst;
#endif
    bool mSprdEisEnabled;
    bool mIsUpdateRangeFps;
    int64_t mPrvBufferTimestamp;
    int mUpdateRangeFpsCount;
    cmr_uint mPrvMinFps;
    cmr_uint mPrvMaxFps;
    /* 0 - not use, default value is 0; 1 - use video buffer to jpeg enc; */
    int32_t mVideoSnapshotType;

    bool mIommuEnabled;
    /* 0 - snapshot not need flash; 1 - snapshot need flash*/
    uint32_t mFlashCaptureFlag;
    uint32_t mZslPopFlag;
    uint32_t mVideoSnapshotFrameNum;
    uint32_t mFlashCaptureSkipNum;
    bool mFixedFpsEnabled;

    int mTempStates;
    int mIsTempChanged;
    int mSprdCameraLowpower;
    uint32_t mFlagOffLineZslStart;
    int64_t mZslSnapshotTime;
    bool mIsIspToolMode;
#ifdef CONFIG_FACE_BEAUTY
    struct class_fb face_beauty;
#endif
    // grab capability
    struct cmr_path_capability grab_capability;

    int64_t mLastCafDoneTime;
};

}; // namespace sprdcamera

#endif //_SPRDCAMERA3_OEMIF_H
