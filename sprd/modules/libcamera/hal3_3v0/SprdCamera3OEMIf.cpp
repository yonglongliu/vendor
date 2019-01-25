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
#define LOG_TAG "SprdCamera3OEMIf"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include <camera/Camera.h>
#include "SprdCamera3OEMIf.h"
#include <utils/Log.h>
#include <utils/String16.h>
#include <utils/Trace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <sprd_ion.h>
#include <media/hardware/MetadataBufferType.h>
#include "cmr_common.h"
#ifdef SPRD_PERFORMANCE
#include <androidfw/SprdIlog.h>
#endif
#ifdef CONFIG_GPU_PLATFORM_ROGUE
#include <gralloc_public.h>
#else
#include <gralloc_priv.h>
#endif
#include "SprdCamera3HALHeader.h"
#include "SprdCamera3Channel.h"
#include "SprdCamera3Flash.h"
#include <dlfcn.h>

#ifdef CONFIG_CAMERA_ISP
extern "C" {
#include "isp_video.h"
}
#ifdef CONFIG_FACE_BEAUTY
#include "camera_face_beauty.h"
#endif
#endif

using namespace android;

namespace sprdcamera {

/**********************Macro Define**********************/
#define SWITCH_MONITOR_QUEUE_SIZE 50
#define GET_START_TIME                                                         \
    do {                                                                       \
        s_start_timestamp = systemTime();                                      \
    } while (0)
#define GET_END_TIME                                                           \
    do {                                                                       \
        s_end_timestamp = systemTime();                                        \
    } while (0)
#define GET_USE_TIME                                                           \
    do {                                                                       \
        s_use_time = (s_end_timestamp - s_start_timestamp) / 1000000;          \
    } while (0)
#define ZSL_FRAME_TIMEOUT 1000000000 /*1000ms*/
#define SET_PARAM_TIMEOUT 2000000000 /*2000ms*/
#define CAP_TIMEOUT 5000000000       /*5000ms*/
#define PREV_TIMEOUT 5000000000      /*5000ms*/
#define CAP_START_TIMEOUT 5000000000 /* 5000ms*/
#define PREV_STOP_TIMEOUT 3000000000 /* 3000ms*/
#define CANCEL_AF_TIMEOUT 500000000  /*1000ms*/

#define SET_PARAMS_TIMEOUT 250 /*250 means 250*10ms*/
#define ON_OFF_ACT_TIMEOUT 50  /*50 means 50*10ms*/
#define IS_ZOOM_SYNC 0
#define NO_FREQ_REQ 0
#define NO_FREQ_STR "0"
#if defined(CONFIG_CAMERA_SMALL_PREVSIZE)
#define BASE_FREQ_REQ 192
#define BASE_FREQ_STR "0" /*base mode can treated with AUTO*/
#define MEDIUM_FREQ_REQ 200
#define MEDIUM_FREQ_STR "200000"
#define HIGH_FREQ_REQ 300
#define HIGH_FREQ_STR "300000"
#else
#define BASE_FREQ_REQ 200
#define BASE_FREQ_STR "0" /*base mode can treated with AUTO*/
#define MEDIUM_FREQ_REQ 300
#define MEDIUM_FREQ_STR "300000"
#define HIGH_FREQ_REQ 500
#define HIGH_FREQ_STR "500000"
#endif
#define DUALCAM_TIME_DIFF (15e6) /**add for 3d capture*/
#define DUALCAM_ZSL_NUM (7)      /**add for 3d capture*/
#define DUALCAM_MAX_ZSL_NUM (4)

#define UHD_WIDTH 3840
#define UHD_HEIGHT 2160

// dfs policy
enum DFS_POLICY {
    CAM_EXIT,
    CAM_LOW,
    CAM_HIGH,
};

//#define CAM_EXIT_STR          "exit"
#define CAM_LOW_STR "camlow"
#define CAM_HIGH_STR "camhigh"

#define CONFIG_PRE_ALLOC_CAPTURE_MEM /*pre alloc memory for capture*/
#define HAS_CAMERA_HINTS 1
//#define CONFIG_NEED_UNMAP

/*ZSL Thread*/
#define ZSLMode_MONITOR_QUEUE_SIZE 50
#define CMR_EVT_ZSL_MON_BASE 0x800
#define CMR_EVT_ZSL_MON_INIT 0x801
#define CMR_EVT_ZSL_MON_EXIT 0x802
#define CMR_EVT_ZSL_MON_PUSH 0x803
#define CMR_EVT_ZSL_MON_SNP 0x804
#define CMR_EVT_ZSL_STOP_MULTI_LAYER 0x805

#define UPDATE_RANGE_FPS_COUNT 0x04

#define MIN_DIGITAL_ZOOM_RATIO (1.0f)
#define MAX_DIGITAL_ZOOM_RATIO (2.0f)

/**********************Static Members**********************/
static nsecs_t s_start_timestamp = 0;
static nsecs_t s_end_timestamp = 0;
static int s_use_time = 0;
static nsecs_t cam_init_begin_time = 0;
bool gIsApctCamInitTimeShow = false;
bool gIsApctRead = false;

gralloc_module_t const *SprdCamera3OEMIf::mGrallocHal = NULL;
// oem_module_t   * SprdCamera3OEMIf::mHalOem = NULL;
sprd_camera_memory_t *SprdCamera3OEMIf::mIspFirmwareReserved = NULL;
uint32_t SprdCamera3OEMIf::mIspFirmwareReserved_cnt = 0;
bool SprdCamera3OEMIf::mZslCaptureExitLoop = false;
multi_camera_zsl_match_frame *SprdCamera3OEMIf::mMultiCameraMatchZsl = NULL;
multiCameraMode SprdCamera3OEMIf::mMultiCameraMode = MODE_SINGLE_CAMERA;

static void writeCamInitTimeToApct(char *buf) {
    int apct_dir_fd = open("/data/apct", O_CREAT, 0777);

    if (apct_dir_fd >= 0) {
        fchmod(apct_dir_fd, 0777);
        close(apct_dir_fd);
    }

    int apct_fd =
        open("/data/apct/apct_data.log", O_CREAT | O_RDWR | O_TRUNC, 0666);

    if (apct_fd >= 0) {
        char buf[100] = {0};
        sprintf(buf, "\n%s", buf);
        write(apct_fd, buf, strlen(buf));
        fchmod(apct_fd, 0666);
        close(apct_fd);
    }
}

static void writeCamInitTimeToProc(float init_time) {
    char cam_time_buf[256] = {0};
    const char *cam_time_proc = "/proc/benchMark/cam_time";

    sprintf(cam_time_buf, "Camera init time:%.2fs", init_time);

    FILE *f = fopen(cam_time_proc, "r+w");
    if (NULL != f) {
        fseek(f, 0, 0);
        fwrite(cam_time_buf, strlen(cam_time_buf), 1, f);
        fclose(f);
    }
    writeCamInitTimeToApct(cam_time_buf);
}

bool getApctCamInitSupport() {
    if (gIsApctRead) {
        return gIsApctCamInitTimeShow;
    }
    gIsApctRead = true;

    int ret = 0;
    char str[10] = {'\0'};
    const char *FILE_NAME = "/data/data/com.sprd.APCT/apct/apct_support";

    FILE *f = fopen(FILE_NAME, "r");

    if (NULL != f) {
        fseek(f, 0, 0);
        ret = fread(str, 5, 1, f);
        fclose(f);
        if (ret) {
            long apct_config = atol(str);
            gIsApctCamInitTimeShow =
                (apct_config & 0x8010) == 0x8010 ? true : false;
        }
    }
    return gIsApctCamInitTimeShow;
}

void SprdCamera3OEMIf::shakeTestInit(ShakeTest *tmpShakeTest) {
    char is_performance_camera_test[PROPERTY_VALUE_MAX];
    int tmp_diff_yuv_color[MAX_LOOP_COLOR_COUNT][MAX_Y_UV_COUNT] = {
        {0x28, 0xef}, {0x51, 0x5a}, {0x90, 0x36}};
    memcpy(&tmpShakeTest->diff_yuv_color, &tmp_diff_yuv_color,
           sizeof(tmp_diff_yuv_color));
    tmpShakeTest->mShakeTestColorCount = 0;
    property_get("persist.sys.performance_camera", is_performance_camera_test,
                 "0");
    if ((0 == strcmp("1", is_performance_camera_test)) &&
        mIsPerformanceTestable) {
        HAL_LOGD("SHAKE_TEST come in");
        setShakeTestState(SHAKE_TEST);
    } else {
        HAL_LOGV("SHAKE_TEST not come in");
        setShakeTestState(NOT_SHAKE_TEST);
    }
}

SprdCamera3OEMIf::SprdCamera3OEMIf(int cameraId, SprdCamera3Setting *setting)
    : mSetCapRatioFlag(false), mSprdPipVivEnabled(0), mSprdHighIsoEnabled(0),
      mSprdRefocusEnabled(0), mSprd3dCalibrationEnabled(0), mSprdYuvCallBack(0),
      mSprdMultiYuvCallBack(0), mSprdReprocessing(0), mNeededTimestamp(0),
      mRawHeap(NULL), mRawHeapSize(0), mParameters(), mPreviewHeight_trimy(0),
      mPreviewWidth_trimx(0), mPreviewFormat(CAMERA_DATA_FORMAT_YUV422),
      mPictureFormat(1), mPreviewStartFlag(0), mIsCancellingCapture(0),
      mIsDvPreview(0), mIsStoppingPreview(0), mRecordingMode(0),
      mIsSetCaptureMode(false), mRecordingFirstFrameTime(0), mJpegSize(0),
      mUser(0), mPreviewWindow(NULL), mHalOem(NULL), mIsStoreMetaData(false),
      mIsFreqChanged(false), mCameraId(cameraId), miSPreviewFirstFrame(1),
      mCaptureMode(CAMERA_NORMAL_MODE), mCaptureRawMode(0),
#ifdef CONFIG_CAMERA_ROTATION_CAPTURE
      mIsRotCapture(1),
#else
      mIsRotCapture(0),
#endif
      mFlashMask(false), mReleaseFLag(false), mTimeCoeff(1),
      mPreviewBufferUsage(PREVIEW_BUFFER_USAGE_GRAPHICS),
      mCameraDfsPolicyCur(CAM_EXIT), mIsPerformanceTestable(false),
      mIsAndroidZSL(false), mSetting(setting), BurstCapCnt(0), mCapIntent(0),
      mPrvTimerID(NULL), mFlashMode(-1), mIsAutoFocus(false),
      mIspToolStart(false), mSubRawHeapNum(0), mSubRawHeapSize(0),
      mPathRawHeapNum(0), mPreviewDcamAllocBufferCnt(0), mPreviewFrameNum(0),
      mRecordFrameNum(0), mIsRecording(false),
#if defined(CONFIG_PRE_ALLOC_CAPTURE_MEM)
      mIsPreAllocCapMem(1),
#else
      mIsPreAllocCapMem(0),
#endif
      mPreAllocCapMemInited(0), mIsPreAllocCapMemDone(0),
      mZSLModeMonitorMsgQueHandle(0), mZSLModeMonitorInited(0),
      mPowermanageInited(0), mPowerManager(NULL), mPrfmLock(NULL),
      m_pPowerModule(NULL), mHDRPowerHint(0), mHDRPowerHintFlag(0),
      mMultiLayerPowerHint(0), mMultiLayerPowerHintFlag(0), mGyroInit(0),
      mGyroExit(0), mGyroNum(0), mSprdEisEnabled(false),
      mIsUpdateRangeFps(false), mPrvBufferTimestamp(0), mUpdateRangeFpsCount(0),
      mPrvMinFps(0), mPrvMaxFps(0), mVideoSnapshotType(0), mIommuEnabled(false),
      mFlashCaptureFlag(0), mFlashCaptureSkipNum(FLASH_CAPTURE_SKIP_FRAME_NUM),
      mFlagMultiLayerStart(false), mFixedFpsEnabled(0), mTempStates(CAMERA_NORMAL_TEMP),
      mIsTempChanged(0) {
    int i = 0;
    ATRACE_CALL();
#ifdef CONFIG_FACE_BEAUTY
    memset(&face_beauty, 0, sizeof(face_beauty));
#endif
    // mIsPerformanceTestable = sprd_isPerformanceTestable();
    HAL_LOGI("openCameraHardware: E cameraId: %d.", cameraId);


    initPowerHint();
    enablePowerHint();
    changeDfsPolicy(CAM_HIGH);

#if defined(LOWPOWER_DISPLAY_30FPS)
    property_set("lowpower.display.30fps", "true");
#endif

    shakeTestInit(&mShakeTest);
#if defined(CONFIG_BACK_CAMERA_ROTATION)
    if (0 == cameraId) {
        mPreviewBufferUsage = PREVIEW_BUFFER_USAGE_DCAM;
    }
#endif

#if defined(CONFIG_FRONT_CAMERA_ROTATION)
    if (1 == cameraId) {
        mPreviewBufferUsage = PREVIEW_BUFFER_USAGE_DCAM;
    }
#endif

#ifdef MINICAMERA
    mPreviewBufferUsage = PREVIEW_BUFFER_USAGE_DCAM;
#endif
    if (mMultiCameraMatchZsl == NULL) {
        mMultiCameraMatchZsl = (multi_camera_zsl_match_frame *)malloc(
            sizeof(multi_camera_zsl_match_frame));
        memset(mMultiCameraMatchZsl, 0, sizeof(multi_camera_zsl_match_frame));
    }
    mOriginalPreviewBufferUsage = mPreviewBufferUsage;

    memset(mSubRawHeapArray, 0, sizeof(mSubRawHeapArray));
    memset(mZslHeapArray, 0, sizeof(mZslHeapArray));
    memset(mPreviewHeapArray, 0, sizeof(mPreviewHeapArray));
    memset(mVideoHeapArray, 0, sizeof(mVideoHeapArray));
    memset(&mSlowPara, 0, sizeof(slow_motion_para));
    memset(mPdafRawHeapArray, 0, sizeof(mPdafRawHeapArray));
    memset(mPathRawHeapArray, 0, sizeof(mPathRawHeapArray));

    setCameraState(SPRD_INIT, STATE_CAMERA);

#if (MINICAMERA != 1)
    if (!mGrallocHal) {
        int ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
                                (const hw_module_t **)&mGrallocHal);
        if (ret)
            HAL_LOGE("Fail on loading gralloc HAL");
    }
#endif

    if (!mHalOem) {
        void *handle;
        oem_module_t *omi;

        mHalOem = (oem_module_t *)malloc(sizeof(oem_module_t));

        handle = dlopen(OEM_LIBRARY_PATH, RTLD_NOW);

        if (handle == NULL) {
            char const *err_str = dlerror();
            HAL_LOGE("dlopen error%s ", err_str ? err_str : "unknown");
        }

        /* Get the address of the struct hal_module_info. */
        const char *sym = OEM_MODULE_INFO_SYM_AS_STR;
        omi = (oem_module_t *)dlsym(handle, sym);
        if (omi == NULL) {
            HAL_LOGE("load: couldn't find symbol %s", sym);
        }

        mHalOem->dso = handle;
        mHalOem->ops = omi->ops;

        ALOGI("loaded libcamoem.so handle=%p", handle);
    }

    mCameraId = cameraId;
    if (mCameraId == 1) {
        mMultiCameraMatchZsl->cam1_ZSLQueue = &mZSLQueue;
    } else if (mCameraId == 3) {
        mMultiCameraMatchZsl->cam3_ZSLQueue = &mZSLQueue;
    }
    mCbInfoList.clear();
    mSetting->getDefaultParameters(mParameters);

    mPreviewWidth = 0;
    mPreviewHeight = 0;
    mVideoWidth = 0;
    mVideoHeight = 0;
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mLargestSensorWidth = 0;
    mLargestSensorHeight = 0;
    mLargestPictureWidth = 0;
    mLargestPictureHeight = 0;
    mRawWidth = 0;
    mRawHeight = 0;
    mRegularChan = NULL;

    mMetaData = NULL;
    jpeg_gps_location = false;
    mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
    mPreviewHeapBakUseFlag = 0;
    mRawHeapBakUseFlag = 0;
    memset(&mRawHeapInfoBak, 0, sizeof(mRawHeapInfoBak));
    for (int i = 0; i < kPreviewBufferCount; i++) {
        mPreviewBufferHandle[i] = NULL;
        mPreviewCancelBufHandle[i] = NULL;
    }
    mTakePictureMode = SNAPSHOT_DEFAULT_MODE;
    mZSLTakePicture = false;

    mZoomInfo.mode = ZOOM_INFO;
    mZoomInfo.zoom_info.prev_aspect_ratio = 0.0f;
    mZoomInfo.zoom_info.video_aspect_ratio = 0.0f;
    mZoomInfo.zoom_info.capture_aspect_ratio = 0.0f;

    mCameraHandle = NULL;

    memset(mGps_processing_method, 0, sizeof(mGps_processing_method));
    memset(mIspB4awbHeapReserved, 0, sizeof(mIspB4awbHeapReserved));
    memset(mIspRawAemHeapReserved, 0, sizeof(mIspRawAemHeapReserved));
    memset(mIspPreviewYReserved, 0, sizeof(mIspPreviewYReserved));
    memset(mIspRawDataReserved, 0, sizeof(mIspRawDataReserved));

    mJpegRotaSet = false;
    mPicCaptureCnt = 0;

    mRegularChan = NULL;
    mPictureChan = NULL;

    mPreviewHeapNum = 0;
    mVideoHeapNum = 0;
    mZslHeapNum = 0;
    mRefocusHeapNum = 0;
    mPdafRawHeapNum = 0;
    mSubRawHeapSize = 0;

    for (i = 0; i < PREV_RESERVED_FRM_CNT; i++) {
        mPreviewHeapReserved[i] = NULL;
    }

    for (i = 0; i < VIDEO_RESERVED_FRM_CNT; i++) {
        mVideoHeapReserved[i] = NULL;
    }

    for (i = 0; i < CAP_ZSL_RESERVED_FRM_CNT; i++) {
        mZslHeapReserved[i] = NULL;
    }
    mDepthHeapReserved = NULL;
    mIspLscHeapReserved = NULL;
    mHighIsoSnapshotHeapReserved = NULL;
    mIspYUVReserved = NULL;
    mPdafRawHeapReserved = NULL;

    mVideoShotFlag = 0;
    mVideoShotNum = 0;
    mVideoShotPushFlag = 0;

    mRestartFlag = false;
    mReDisplayHeap = NULL;

    mZslPreviewMode = false;

    mSprdZslEnabled = false;
    mZslMaxFrameNum = 1;
    mZslNum = 2;
    mZslShotPushFlag = 0;
    mZslChannelStatus = 1;
    mZSLQueue.clear();

    mBurstVideoSnapshot = 0;
    mSprdBurstModeEnabled = 0;
    mVideoParameterSetFlag = false;
    mZslCaptureExitLoop = false;
    mSprdCameraLowpower = 0;
    HAL_LOGI("openCameraHardware: X cameraId: %d.", cameraId);
}

SprdCamera3OEMIf::~SprdCamera3OEMIf() {
    ATRACE_CALL();

    cmr_int i;
    HAL_LOGI("closeCameraHardware: E cameraId: %d.", mCameraId);

    if (!mReleaseFLag) {
        closeCamera();
    }

    changeDfsPolicy(CAM_EXIT);
    disablePowerHint();
    deinitPowerHint();

#if defined(LOWPOWER_DISPLAY_30FPS)
    char value[PROPERTY_VALUE_MAX];
    property_get("lowpower.display.30fps", value, "false");
    if (!strcmp(value, "true")) {
        property_set("lowpower.display.30fps", "false");
        HAL_LOGI("camera low power mode exit");
    }
#endif

    // clean memory in case memory leak
    if (NULL != mReDisplayHeap) {
        freeCameraMem(mReDisplayHeap);
        mReDisplayHeap = NULL;
    }

    for (i = 0; i < (cmr_int)mZslHeapNum; i++) {
        if (NULL != mZslHeapArray[i]) {
            freeCameraMem(mZslHeapArray[i]);
            mZslHeapArray[i] = NULL;
        }
    }
    mZslHeapNum = 0;

    if (NULL != mIspLscHeapReserved) {
        freeCameraMem(mIspLscHeapReserved);
        mIspLscHeapReserved = NULL;
    }
    /*
            if (NULL != mIspAntiFlickerHeapReserved) {
                    freeCameraMem(mIspAntiFlickerHeapReserved);
                    mIspAntiFlickerHeapReserved = NULL;
            }
    */

    for (i = 0; i < kISPB4awbCount; i++) {
        if (NULL != mIspB4awbHeapReserved[i]) {
            freeCameraMem(mIspB4awbHeapReserved[i]);
            mIspB4awbHeapReserved[i] = NULL;
        }
    }

    for (i = 0; i < 2; i++) {
        if (NULL != mIspPreviewYReserved[i]) {
            freeCameraMem(mIspPreviewYReserved[i]);
            mIspPreviewYReserved[i] = NULL;
        }
    }

    for (i = 0; i < ISP_RAWBUF_NUM; i++) {
        if (NULL != mIspRawDataReserved[i]) {
            freeCameraMem(mIspRawDataReserved[i]);
            mIspRawDataReserved[i] = NULL;
        }
    }

    if (NULL != mIspYUVReserved) {
        freeCameraMem(mIspYUVReserved);
        mIspYUVReserved = NULL;
    }

    for (i = 0; i < kISPB4awbCount; i++) {
        if (NULL != mIspRawAemHeapReserved[i]) {
            mIspRawAemHeapReserved[i]->ion_heap->free_kaddr();
            freeCameraMem(mIspRawAemHeapReserved[i]);
            mIspRawAemHeapReserved[i] = NULL;
        }
    }

    if (mHalOem) {
        if (NULL != mHalOem->dso)
            dlclose(mHalOem->dso);
        free((void *)mHalOem);
        mHalOem = NULL;
    }

    HAL_LOGD("closeCameraHardware: X cameraId: %d.", mCameraId);
    timer_stop();
}

void SprdCamera3OEMIf::closeCamera() {
    ATRACE_CALL();

    HAL_LOGI("E");
    Mutex::Autolock l(&mLock);
    int i = 0;
    mZslCaptureExitLoop = true;
    // Either preview was ongoing, or we are in the middle or taking a
    // picture.  It's the caller's responsibility to make sure the camera
    // is in the idle or init state before destroying this object.
    HAL_LOGD(
        "release:camera state = %s, preview state = %s, capture state = %s",
        getCameraStateStr(getCameraState()),
        getCameraStateStr(getPreviewState()),
        getCameraStateStr(getCaptureState()));

    if (isCapturing()) {
        cancelPictureInternal();
    }

    if (isPreviewing()) {
        stopPreviewInternal();
    }

    for (i = 0; i < PREV_RESERVED_FRM_CNT; i++) {
        if (NULL != mPreviewHeapReserved[i]) {
            freeCameraMem(mPreviewHeapReserved[i]);
            mPreviewHeapReserved[i] = NULL;
        }
    }

    for (i = 0; i < VIDEO_RESERVED_FRM_CNT; i++) {
        if (NULL != mVideoHeapReserved[i]) {
            freeCameraMem(mVideoHeapReserved[i]);
            mVideoHeapReserved[i] = NULL;
        }
    }

    for (i = 0; i < CAP_ZSL_RESERVED_FRM_CNT; i++) {
        if (NULL != mZslHeapReserved[i]) {
            freeCameraMem(mZslHeapReserved[i]);
            mZslHeapReserved[i] = NULL;
        }
    }

    if (NULL != mDepthHeapReserved) {
        freeCameraMem(mDepthHeapReserved);
        mDepthHeapReserved = NULL;
    }

    if (NULL != mHighIsoSnapshotHeapReserved) {
        freeCameraMem(mHighIsoSnapshotHeapReserved);
        mHighIsoSnapshotHeapReserved = NULL;
    }

    for (int i = 0; i < 2; i++) {
        if (NULL != mIspPreviewYReserved[i]) {
            freeCameraMem(mIspPreviewYReserved[i]);
            mIspPreviewYReserved[i] = NULL;
        }
    }

    if (NULL != mIspYUVReserved) {
        freeCameraMem(mIspYUVReserved);
        mIspYUVReserved = NULL;
    }
    if (NULL != mPdafRawHeapReserved) {
        freeCameraMem(mPdafRawHeapReserved);
        mPdafRawHeapReserved = NULL;
    }

    SprdCamera3Flash::releaseFlash(mCameraId);
    // Performance optimization:move Callback_CaptureFree to closeCamera
    // function
    Callback_CaptureFree(0, 0, 0, 0);

#ifdef CONFIG_CAMERA_GYRO
    gyro_monitor_thread_deinit((void *)this);
#endif
    if (isCameraInit()) {
        // When libqcamera detects an error, it calls camera_cb from the
        // call to camera_stop, which would cause a deadlock if we
        // held the mStateLock.  For this reason, we have an intermediate
        // state SPRD_INTERNAL_STOPPING, which we use to check to see if the
        // camera_cb was called inline.
        setCameraState(SPRD_INTERNAL_STOPPING, STATE_CAMERA);

        HAL_LOGD("stopping camera.");
        if (CMR_CAMERA_SUCCESS != mHalOem->ops->camera_deinit(mCameraHandle)) {
            setCameraState(SPRD_ERROR, STATE_CAMERA);
            mReleaseFLag = true;
            HAL_LOGE("release X: fail to camera_stop().");
            return;
        }

        WaitForCameraStop();
    }

    pre_alloc_cap_mem_thread_deinit((void *)this);
    ZSLMode_monitor_thread_deinit((void *)this);

    deinitCapture(0);
    FreeReDisplayMem();

    mReleaseFLag = true;
    HAL_LOGI("X");
}

int SprdCamera3OEMIf::getCameraId() const { return mCameraId; }

void SprdCamera3OEMIf::initialize() {
    mPreviewFrameNum = 0;
    mVideoSnapshotFrameNum = 0;
    mRecordFrameNum = 0;
    mZslFrameNum = 0;
    mPictureFrameNum = 0;
    mStartFrameNum = 0;
    mStopFrameNum = 0;

    mDropPreviewFrameNum = 0;
    mDropVideoFrameNum = 0;
    mDropZslFrameNum = 0;
}

int SprdCamera3OEMIf::checkIfNeedToStopMultiLayer() {
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

    // capture_mode: 1 single capture; >1: n capture
    if (mFlagMultiLayerStart && mSprdBurstModeEnabled == 1 &&
        mSprdZslEnabled == 1 && sprddefInfo.capture_mode == 1) {
        HAL_LOGI("mFlagMultiLayerStart=%d, sprddefInfo.capture_mode=%d",
                 mFlagMultiLayerStart, sprddefInfo.capture_mode);
        CMR_MSG_INIT(message);
        uint32_t ret = 0;

        message.msg_type = CMR_EVT_ZSL_STOP_MULTI_LAYER;
        message.sync_flag = CMR_MSG_SYNC_NONE;
        message.data = NULL;
        ret = cmr_thread_msg_send((cmr_handle)mZSLModeMonitorMsgQueHandle,
                                  &message);
        if (ret) {
            HAL_LOGE("Fail to send one msg!");
            return NO_ERROR;
        }
    }
    return 0;
}

int SprdCamera3OEMIf::start(camera_channel_type_t channel_type,
                            uint32_t frame_number) {
    ATRACE_CALL();

    int ret = NO_ERROR;
    Mutex::Autolock l(&mLock);

    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

    HAL_LOGI("channel_type = %d, frame_number = %d", channel_type,
             frame_number);
    mStartFrameNum = frame_number;

    switch (channel_type) {
    case CAMERA_CHANNEL_TYPE_REGULAR: {
        if (mParaDCDVMode == CAMERA_PREVIEW_FORMAT_DV)
            mRecordingFirstFrameTime = 0;

#ifdef CONFIG_CAMERA_EIS
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        if (sprddefInfo.sprd_eis_enabled == 1) {
            mSprdEisEnabled = true;
        } else {
            mSprdEisEnabled = false;
        }
        if (!mEisPreviewInit && sprddefInfo.sprd_eis_enabled == 1 &&
            mPreviewWidth != 0 && mPreviewHeight != 0) {
            EisPreview_init();
            mEisPreviewInit = true;
        }
        if (!mEisVideoInit && sprddefInfo.sprd_eis_enabled == 1 &&
            mVideoWidth != 0 && mVideoHeight != 0) {
            EisVideo_init();
            mEisVideoInit = true;
        }
#endif
#ifdef CONFIG_CAMERA_UHD_VIDEOSNAPSHOT_SUPPORT
        mUhdVideoSnapshotSupport = true;
#else
        mUhdVideoSnapshotSupport = false;
#endif
        ret = startPreviewInternal();

        if (mUhdVideoSnapshotSupport == false && mUhdRecodingEnabled == true) {
            HAL_LOGI("uhd video snapshot is not support");
            break;
        }
        if ((mVideoSnapshotType == 1) &&
            (mCaptureWidth != 0 && mCaptureHeight != 0) &&
            mVideoParameterSetFlag == 0) {
            mPicCaptureCnt = 1;
            ret = setVideoSnapshotParameter();
            mVideoParameterSetFlag = true;
        }
        break;
    }
    case CAMERA_CHANNEL_TYPE_PICTURE: {
        if (mSprdBurstModeEnabled == 1 && mSprdZslEnabled == 1) {
            ret = zslTakePicture();
        } else {
            if (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE ||
                mTakePictureMode == SNAPSHOT_ONLY_MODE)
                ret = takePicture();
            else if (mTakePictureMode == SNAPSHOT_ZSL_MODE) {
                mVideoSnapshotFrameNum = frame_number;
                ret = zslTakePicture();
            } else if (mTakePictureMode == SNAPSHOT_VIDEO_MODE) {
                if (mVideoParameterSetFlag == false &&
                    mBurstVideoSnapshot == false) {
                    setVideoSnapshotParameter();
                }
                mVideoSnapshotFrameNum = frame_number;
                ret = VideoTakePicture();
            }
        }

        break;
    }
    default:
        break;
    }

    HAL_LOGI("X");
    return ret;
}

int SprdCamera3OEMIf::stop(camera_channel_type_t channel_type,
                           uint32_t frame_number) {
    ATRACE_CALL();

    int ret = NO_ERROR;
    int capture_intent = 0;

    HAL_LOGI("channel_type = %d, frame_number = %d", channel_type,
             frame_number);
    mStopFrameNum = frame_number;

    switch (channel_type) {
    case CAMERA_CHANNEL_TYPE_REGULAR:
        stopPreviewInternal();
        mVideoParameterSetFlag = false;
#ifdef CONFIG_CAMERA_EIS
        if (mEisPreviewInit) {
            video_stab_close(mPreviewInst);
            mEisPreviewInit = false;
            HAL_LOGI("preview stab close");
        }
        if (mEisVideoInit) {
            video_stab_close(mVideoInst);
            mEisVideoInit = false;
            mIsRecording = false;
            HAL_LOGI("video stab close");
        }
#endif
        break;
    case CAMERA_CHANNEL_TYPE_PICTURE:
        cancelPictureInternal();
        break;
    default:
        break;
    }

    return ret;
}

int SprdCamera3OEMIf::takePicture() {
    ATRACE_CALL();

    bool wait_raw = true;

    HAL_LOGI("E");
    GET_START_TIME;
    print_time();

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    if (1 == mHDRPowerHint) {
        enablePowerHint();
        mHDRPowerHintFlag = 1;
    }

    if (SPRD_ERROR == mCameraState.capture_state) {
        HAL_LOGE("take picture in error status, deinit capture at first");
        deinitCapture(mIsPreAllocCapMem);
    } else if (SPRD_IDLE != mCameraState.capture_state) {
        usleep(50 * 1000);
        if (SPRD_IDLE != mCameraState.capture_state) {
            HAL_LOGE("take picture: action alread exist, direct return");
            goto exit;
        }
    }

    if (mIsCancellingCapture) {
        HAL_LOGW("when is cancelling capture, this place can't change capture "
                 "state");
        goto exit;
    } else {
        setCameraState(SPRD_FLASH_IN_PROGRESS, STATE_CAPTURE);
    }

    if (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE ||
        mTakePictureMode == SNAPSHOT_PREVIEW_MODE ||
        mTakePictureMode == SNAPSHOT_ONLY_MODE) {
        if (((mCaptureMode == CAMERA_ISP_TUNING_MODE) ||
             (mCaptureMode == CAMERA_ISP_SIMULATION_MODE)) &&
            !mIspToolStart) {
            mIspToolStart = true;
            timer_set(this, ISP_TUNING_WAIT_MAX_TIME, timer_hand);
        }

        if (isPreviewStart()) {
            HAL_LOGV("Preview not start! wait preview start");
            WaitForPreviewStart();
            usleep(20 * 1000);
        }

        if (isPreviewing()) {
            HAL_LOGD("call stopPreviewInternal in takePicture().");
            if (CAMERA_ZSL_MODE != mCaptureMode) {
                mHalOem->ops->camera_start_preflash(mCameraHandle);
            }
            stopPreviewInternal();
        }
        HAL_LOGI(
            "finish stopPreviewInternal in takePicture. preview state = %d",
            getPreviewState());

        if (isPreviewing()) {
            HAL_LOGE("takePicture: stop preview error!, preview state = %d",
                     getPreviewState());
            goto exit;
        }

        if (!setCameraCaptureDimensions()) {
            deinitCapture(mIsPreAllocCapMem);
            HAL_LOGE("takePicture initCapture failed. Not taking picture.");
            goto exit;
        }
    }

    if (isCapturing()) {
        WaitForCaptureDone();
    }

    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM, mPicCaptureCnt);

    LENS_Tag lensInfo;
    mSetting->getLENSTag(&lensInfo);
    if (lensInfo.focal_length) {
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCAL_LENGTH,
                 (int32_t)(lensInfo.focal_length * 1000));
        HAL_LOGD("lensInfo.focal_length = %f", lensInfo.focal_length);
    }

    JPEG_Tag jpgInfo;
    struct img_size jpeg_thumb_size;
    struct img_size capture_size;
    mSetting->getJPEGTag(&jpgInfo);
    HAL_LOGV("JPEG quality = %d", jpgInfo.quality);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_JPEG_QUALITY,
             jpgInfo.quality);
    HAL_LOGV("JPEG thumbnail quality = %d", jpgInfo.thumbnail_quality);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_QUALITY,
             jpgInfo.thumbnail_quality);
    jpeg_thumb_size.width = jpgInfo.thumbnail_size[0];
    jpeg_thumb_size.height = jpgInfo.thumbnail_size[1];
    if (mCaptureWidth != 0 && mCaptureHeight != 0) {
        if (mVideoWidth != 0 && mVideoHeight != 0 &&
            mCaptureMode != CAMERA_ISP_TUNING_MODE &&
            mCaptureMode != CAMERA_ISP_SIMULATION_MODE) {
            capture_size.width = (cmr_u32)mPreviewWidth;
            capture_size.height = (cmr_u32)mPreviewHeight;
        } else {
            capture_size.width = (cmr_u32)mCaptureWidth;
            capture_size.height = (cmr_u32)mCaptureHeight;
        }
    } else {
        capture_size.width = (cmr_u32)mRawWidth;
        capture_size.height = (cmr_u32)mRawHeight;
    }
    if (jpeg_thumb_size.width > capture_size.width &&
        jpeg_thumb_size.height > capture_size.height) {
        jpeg_thumb_size.width = 0;
        jpeg_thumb_size.height = 0;
    }
    HAL_LOGI("JPEG thumbnail size = %d x %d", jpeg_thumb_size.width,
             jpeg_thumb_size.height);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_SIZE,
             (cmr_uint)&jpeg_thumb_size);

    HAL_LOGD("mSprdZslEnabled=%d", mSprdZslEnabled);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_ZSL_ENABLED,
             (cmr_uint)mSprdZslEnabled);
    HAL_LOGD("mSprdBurstModeEnabled=%d", mSprdBurstModeEnabled);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_BURSTMODE_ENABLED,
             (cmr_uint)mSprdBurstModeEnabled);

    setCameraPreviewFormat();
    if (mIsCancellingCapture) {
        HAL_LOGW("when is cancelling capture, this place can't change capture "
                 "state");
        goto exit;
    } else {
        setCameraState(SPRD_INTERNAL_RAW_REQUESTED, STATE_CAPTURE);
    }
    if (CMR_CAMERA_SUCCESS !=
        mHalOem->ops->camera_take_picture(mCameraHandle, mCaptureMode)) {
        setCameraState(SPRD_ERROR, STATE_CAPTURE);
        HAL_LOGE("fail to camera_take_picture.");
        goto exit;
    }

    if (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE) {
        if ((mCaptureMode == CAMERA_ISP_TUNING_MODE ||
             mCaptureMode == CAMERA_ISP_SIMULATION_MODE) &&
            mIspToolStart) {
            wait_raw = false;
        }
    }

    if (wait_raw) {
        WaitForCaptureStart();
    }

    print_time();
exit:
    HAL_LOGI("X, wait_raw=%d", wait_raw);
    /*must return NO_ERROR, otherwise can't flush camera normal*/
    return NO_ERROR;
}

int SprdCamera3OEMIf::zslTakePicture() {
    ATRACE_CALL();

    uint32_t ret = 0;
    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

    HAL_LOGI("E mCameraId %d", mCameraId);
    GET_START_TIME;
    print_time();

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    if (SPRD_ERROR == mCameraState.capture_state) {
        HAL_LOGE("in error status, deinit capture at first ");
        deinitCapture(mIsPreAllocCapMem);
    }

    if (isCapturing()) {
        WaitForCaptureDone();
    }

    if (isPreviewing()) {
        mHalOem->ops->camera_start_preflash(mCameraHandle);
        mHalOem->ops->camera_snapshot_is_need_flash(mCameraHandle, mCameraId,
                                                    &mFlashCaptureFlag);
        HAL_LOGD("mFlashCaptureFlag=%d", mFlashCaptureFlag);
    }

    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM, mPicCaptureCnt);
    LENS_Tag lensInfo;
    mSetting->getLENSTag(&lensInfo);
    if (lensInfo.focal_length) {
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCAL_LENGTH,
                 (int32_t)(lensInfo.focal_length * 1000));
        HAL_LOGD("lensInfo.focal_length = %f", lensInfo.focal_length);
    }

    JPEG_Tag jpgInfo;
    struct img_size jpeg_thumb_size;
    mSetting->getJPEGTag(&jpgInfo);
    HAL_LOGV("JPEG quality = %d", jpgInfo.quality);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_JPEG_QUALITY,
             jpgInfo.quality);
    HAL_LOGV("JPEG thumbnail quality = %d", jpgInfo.thumbnail_quality);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_QUALITY,
             jpgInfo.thumbnail_quality);
    jpeg_thumb_size.width = jpgInfo.thumbnail_size[0];
    jpeg_thumb_size.height = jpgInfo.thumbnail_size[1];
    HAL_LOGI("JPEG thumbnail size = %d x %d", jpgInfo.thumbnail_size[0],
             jpgInfo.thumbnail_size[1]);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_SIZE,
             (cmr_uint)&jpeg_thumb_size);

    /**add for 3d capture, set raw call back mode & reprocess capture size
     * begin*/
    if (mSprdYuvCallBack && (!mSprdReprocessing)) {
        mSprdYuvCallBack = true;
        if (getMultiCameraMode() == MODE_3D_CAPTURE) {
            mSprd3dCalibrationEnabled = true;
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_3DCAL_ENABLE,
                     mSprd3dCalibrationEnabled);
            HAL_LOGD("yuv call back mode, force enable 3d cal");
        }
        if (getMultiCameraMode() == MODE_BLUR) {
            SET_PARM(mHalOem, mCameraHandle,
                     CAMERA_PARAM_SPRD_YUV_CALLBACK_ENABLE, mSprdYuvCallBack);
            HAL_LOGD("yuv call back mode");
        }
    } else {
        mSprdYuvCallBack = false;
        if (getMultiCameraMode() == MODE_3D_CAPTURE) {
            mSprd3dCalibrationEnabled = sprddefInfo.sprd_3dcalibration_enabled;
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_3DCAL_ENABLE,
                     sprddefInfo.sprd_3dcalibration_enabled);
        }
        if (getMultiCameraMode() == MODE_BLUR) {
            SET_PARM(mHalOem, mCameraHandle,
                     CAMERA_PARAM_SPRD_YUV_CALLBACK_ENABLE, mSprdYuvCallBack);
            HAL_LOGD("reprocess mode, force enable reprocess");
        }
    }
    if (mSprdReprocessing) {
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_EXIF_MIME_TYPE,
                 mMultiCameraMode);
    } else {
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_EXIF_MIME_TYPE,
                 MODE_SINGLE_CAMERA);
    }
    /**add for 3d capture, set raw call back mode & reprocess capture size end*/

    if (mIsCancellingCapture) {
        HAL_LOGW("when is cancelling capture, this place can't change capture "
                 "state");
        return NO_ERROR;
    } else {
        setCameraState(SPRD_INTERNAL_RAW_REQUESTED, STATE_CAPTURE);
    }
    if (CMR_CAMERA_SUCCESS !=
        mHalOem->ops->camera_take_picture(mCameraHandle, mCaptureMode)) {
        setCameraState(SPRD_ERROR, STATE_CAPTURE);
        HAL_LOGE("fail to camera_take_picture.");
        goto exit;
    }

    mFlagMultiLayerStart = true;

    if ((mSprdZslEnabled == true) &&
        (!mSprdReprocessing)) { /**add for 3d capture, enable reprocess
                                   request*/
        CMR_MSG_INIT(message);
        uint32_t ret = 0;

        mZslShotPushFlag = 1;
        message.msg_type = CMR_EVT_ZSL_MON_SNP;
        message.sync_flag = CMR_MSG_SYNC_NONE;
        message.data = NULL;
        ret = cmr_thread_msg_send((cmr_handle)mZSLModeMonitorMsgQueHandle,
                                  &message);
        if (ret) {
            HAL_LOGE("Fail to send one msg!");
            goto exit;
        }
        HAL_LOGD("mZslShotPushFlag %d", mZslShotPushFlag);
    } else {
        PushZslSnapShotbuff();
    }

    print_time();

exit:
    HAL_LOGI("X");
    return NO_ERROR;
}

int SprdCamera3OEMIf::VideoTakePicture() {
    ATRACE_CALL();

    HAL_LOGI("E");
    GET_START_TIME;
    print_time();

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    if (SPRD_ERROR == mCameraState.capture_state) {
        HAL_LOGE("in error status, deinit capture at first ");
        deinitCapture(mIsPreAllocCapMem);
    }

    if (isCapturing()) {
        WaitForCaptureDone();
    }
    mVideoParameterSetFlag = false;
    if (mBurstVideoSnapshot == true) {
        mBurstVideoSnapshot = false;
        setVideoSnapshotParameter();
    }
    if (mIsCancellingCapture) {
        HAL_LOGW("when is cancelling capture, this place can't change capture "
                 "state");
    } else {
        setCameraState(SPRD_INTERNAL_RAW_REQUESTED, STATE_CAPTURE);
    }
    mVideoShotPushFlag = 1;
    mVideoShotWait.signal();
    print_time();
exit:
    HAL_LOGI("X");
    return NO_ERROR;
}

int SprdCamera3OEMIf::setVideoSnapshotParameter() {
    HAL_LOGI("E");
    GET_START_TIME;
    print_time();
    int result = 0;
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }
    if (SPRD_ERROR == mCameraState.capture_state) {
        HAL_LOGE("in error status, deinit capture at first ");
        deinitCapture(mIsPreAllocCapMem);
    }

    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM, mPicCaptureCnt);

    LENS_Tag lensInfo;
    mSetting->getLENSTag(&lensInfo);
    if (lensInfo.focal_length) {
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCAL_LENGTH,
                 (int32_t)(lensInfo.focal_length * 1000));
        HAL_LOGD("lensInfo.focal_length = %f", lensInfo.focal_length);
    }

    JPEG_Tag jpgInfo;
    struct img_size jpeg_thumb_size;
    mSetting->getJPEGTag(&jpgInfo);
    HAL_LOGV("JPEG quality = %d", jpgInfo.quality);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_JPEG_QUALITY,
             jpgInfo.quality);
    HAL_LOGV("JPEG thumbnail quality = %d", jpgInfo.thumbnail_quality);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_QUALITY,
             jpgInfo.thumbnail_quality);
    jpeg_thumb_size.width = jpgInfo.thumbnail_size[0];
    jpeg_thumb_size.height = jpgInfo.thumbnail_size[1];
    HAL_LOGD("JPEG thumbnail size = %d x %d", jpgInfo.thumbnail_size[0],
             jpgInfo.thumbnail_size[1]);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_SIZE,
             (cmr_uint)&jpeg_thumb_size);

    if (CMR_CAMERA_SUCCESS !=
        mHalOem->ops->camera_take_picture(mCameraHandle, mCaptureMode)) {
        setCameraState(SPRD_ERROR, STATE_CAPTURE);
        HAL_LOGE("fail to camera_take_picture.");
        goto exit;
    }

    print_time();
exit:
    HAL_LOGI("X");

    return NO_ERROR;
}

int SprdCamera3OEMIf::cancelPicture() {
    Mutex::Autolock l(&mLock);

    return cancelPictureInternal();
}

int SprdCamera3OEMIf::setTakePictureSize(uint32_t width, uint32_t height) {
    mRawWidth = width;
    mRawHeight = height;

    return NO_ERROR;
}

status_t SprdCamera3OEMIf::faceDectect(bool enable) {
    status_t ret = NO_ERROR;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    if (enable) {
        mHalOem->ops->camera_fd_start(mCameraHandle, 1);
    } else {
        mHalOem->ops->camera_fd_start(mCameraHandle, 0);
    }
    return ret;
}

status_t SprdCamera3OEMIf::faceDectect_enable(bool enable) {
    status_t ret = NO_ERROR;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    if (enable) {
        mHalOem->ops->camera_fd_enable(mCameraHandle, 1);
    } else {
        mHalOem->ops->camera_fd_enable(mCameraHandle, 0);
    }
    return ret;
}

status_t SprdCamera3OEMIf::autoFocus(void *user_data) {
    ATRACE_CALL();

    if (!SprdCamera3Setting::mSensorFocusEnable[mCameraId]) {
        return NO_ERROR;
    }

    HAL_LOGD("E");
    Mutex::Autolock l(&mLock);
    CONTROL_Tag controlInfo;

    mMetaData = user_data;

    if (mCameraId == 1 || mCameraId == 3)
        return NO_ERROR;

    mSetting->getCONTROLTag(&controlInfo);
    if (isPreviewStart()) {
        HAL_LOGV("Preview not start! wait preview start");
        WaitForPreviewStart();
    }

    if (!isPreviewing()) {
        HAL_LOGE("not previewing");
        controlInfo.af_state = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
        mSetting->setAfCONTROLTag(&controlInfo);
        return INVALID_OPERATION;
    }

    if (SPRD_IDLE != getFocusState()) {
        HAL_LOGE("existing, direct return!");
        return NO_ERROR;
    }
    setCameraState(SPRD_FOCUS_IN_PROGRESS, STATE_FOCUS);
    mIsAutoFocus = true;
    /*caf transit to auto focus*/
    if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE ||
        controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_AUTO) {
        if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE)
            mHalOem->ops->camera_transfer_caf_to_af(mCameraHandle);
        else if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_AUTO)
            mHalOem->ops->camera_transfer_af_to_caf(mCameraHandle);

        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_MODE,
                 CAMERA_FOCUS_MODE_AUTO);
    }
    controlInfo.af_state = ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN;
    mSetting->setAfCONTROLTag(&controlInfo);
    if (0 != mHalOem->ops->camera_start_autofocus(mCameraHandle)) {
        HAL_LOGE("auto foucs fail.");
        setCameraState(SPRD_IDLE, STATE_FOCUS);
        controlInfo.af_state = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
        mSetting->setAfCONTROLTag(&controlInfo);
    }

    HAL_LOGD("X");
    return NO_ERROR;
}

status_t SprdCamera3OEMIf::cancelAutoFocus() {
    ATRACE_CALL();

    if (!SprdCamera3Setting::mSensorFocusEnable[mCameraId]) {
        return NO_ERROR;
    }

    bool ret = 0;
    HAL_LOGD("E");

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    Mutex::Autolock l(&mLock);

    if (mCameraId == 1 || mCameraId == 3)
        return NO_ERROR;

    ret = mHalOem->ops->camera_cancel_autofocus(mCameraHandle);

    WaitForFocusCancelDone();
    {
        SprdCamera3MetadataChannel *mChannel =
            (SprdCamera3MetadataChannel *)mMetaData;
        int64_t timeStamp = 0;
        timeStamp = systemTime();

        CONTROL_Tag controlInfo;
        mSetting->getCONTROLTag(&controlInfo);
        if (!(controlInfo.af_state ==
                  ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED &&
              (controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_START ||
               controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_IDLE)))
            controlInfo.af_state = ANDROID_CONTROL_AF_STATE_INACTIVE;
        /*auto focus resume to caf*/
        if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE &&
            mCameraId == 0 && mIsAutoFocus) {
            if (controlInfo.af_mode ==
                ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE)
                mHalOem->ops->camera_transfer_af_to_caf(mCameraHandle);
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_MODE,
                     CAMERA_FOCUS_MODE_CAF);
        }
        mSetting->setAfCONTROLTag(&controlInfo);
    }
    mIsAutoFocus = false;
    HAL_LOGD("X");
    return ret;
}

/**add for 3d capture, set 3d capture special mode begin   */
int SprdCamera3OEMIf::getMultiCameraMode() {
    int mode = (int)mMultiCameraMode;
    return mode;
}

void SprdCamera3OEMIf::setCallBackYuvMode(bool mode) {
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }
    HAL_LOGD("setCallBackYuvMode: %d, %d", mode, mSprdYuvCallBack);
    mSprdYuvCallBack = mode;
}

void SprdCamera3OEMIf::setMultiCallBackYuvMode(bool mode) {
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }
    mSprdMultiYuvCallBack = mode;
    HAL_LOGD("setMultiCallBackYuvMode: %d, %d", mode, mSprdMultiYuvCallBack);
}

void SprdCamera3OEMIf::setCaptureReprocessMode(bool mode, uint32_t width,
                                               uint32_t height) {
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }
    HAL_LOGD("setCaptureReprocessMode: %d, %d, reprocess size: %d, %d", mode,
             mSprdReprocessing, width, height);
    mSprdReprocessing = mode;
    mHalOem->ops->camera_set_reprocess_picture_size(
        mCameraHandle, mSprdReprocessing, mCameraId, width, height);
}

/**add for 3d capture, set 3d capture special mode end   */
status_t SprdCamera3OEMIf::setAePrecaptureSta(uint8_t state) {
    status_t ret = 0;
    Mutex::Autolock l(&mLock);
    CONTROL_Tag controlInfo;

    mSetting->getCONTROLTag(&controlInfo);
    controlInfo.ae_state = state;
    mSetting->setAeCONTROLTag(&controlInfo);
    HAL_LOGD("ae sta %d", state);
    return ret;
}

void SprdCamera3OEMIf::setMultiCameraMode(multiCameraMode mode) {
    mMultiCameraMode = mode;
    if (mHalOem) {
        mHalOem->ops->camera_ioctrl(mCameraHandle,
                                    CAMERA_IOCTRL_SET_MULTI_CAMERAMODE,
                                    &mMultiCameraMode);
    }
    HAL_LOGD("mMultiCameraMode %d", mMultiCameraMode);
}

void SprdCamera3OEMIf::setCaptureRawMode(bool mode) {
    struct img_size req_size;
    struct cmr_zoom_param zoom_param;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    mCaptureRawMode = mode;
    HAL_LOGD("ISP_TOOL: setCaptureRawMode: %d, %d", mode, mCaptureRawMode);
    if (1 == mode) {
        HAL_LOGD("ISP_TOOL: setCaptureRawMode, mRawWidth %d, mRawHeight %d",
                 mRawWidth, mRawHeight);
        req_size.width = (cmr_u32)mRawWidth;
        req_size.height = (cmr_u32)mRawHeight;
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_SIZE,
                 (cmr_uint)&req_size);

        zoom_param.mode = ZOOM_LEVEL;
        zoom_param.zoom_level = 0;
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ZOOM,
                 (cmr_uint)&zoom_param);

        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 0);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ROTATION, 0);
    }
}

void SprdCamera3OEMIf::setIspFlashMode(uint32_t mode) {
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_FLASH, mode);
}

void SprdCamera3OEMIf::antiShakeParamSetup() {
#ifdef CONFIG_CAMERA_ANTI_SHAKE
    mPreviewWidth =
        mPreviewWidth_backup + ((((mPreviewWidth_backup / 10) + 15) >> 4) << 4);
    mPreviewHeight = mPreviewHeight_backup +
                     ((((mPreviewHeight_backup / 10) + 15) >> 4) << 4);
#endif
}

const char *
SprdCamera3OEMIf::getCameraStateStr(SprdCamera3OEMIf::Sprd_camera_state s) {
    static const char *states[] = {
#define STATE_STR(x) #x
        STATE_STR(SPRD_INIT),
        STATE_STR(SPRD_IDLE),
        STATE_STR(SPRD_ERROR),
        STATE_STR(SPRD_PREVIEW_IN_PROGRESS),
        STATE_STR(SPRD_FOCUS_IN_PROGRESS),
        STATE_STR(SPRD_SET_PARAMS_IN_PROGRESS),
        STATE_STR(SPRD_WAITING_RAW),
        STATE_STR(SPRD_WAITING_JPEG),
        STATE_STR(SPRD_FLASH_IN_PROGRESS),
        STATE_STR(SPRD_INTERNAL_PREVIEW_STOPPING),
        STATE_STR(SPRD_INTERNAL_CAPTURE_STOPPING),
        STATE_STR(SPRD_INTERNAL_PREVIEW_REQUESTED),
        STATE_STR(SPRD_INTERNAL_RAW_REQUESTED),
        STATE_STR(SPRD_INTERNAL_STOPPING),
#undef STATE_STR
    };
    return states[s];
}

void SprdCamera3OEMIf::print_time() {
#if PRINT_TIME
    struct timeval time;
    gettimeofday(&time, NULL);
    HAL_LOGD("time: %lld us.", time.tv_sec * 1000000LL + time.tv_usec);
#endif
}

void SprdCamera3OEMIf::initPowerHint() {
#ifdef HAS_CAMERA_HINTS
    Mutex::Autolock l(&mPowermanageLock);
    if (mPowerManager == NULL) {
        // use checkService() to avoid blocking if power service is not up yet
        sp<IBinder> binder =
            defaultServiceManager()->checkService(String16("power"));
        if (binder == NULL) {
            ALOGE("Thread cannot connect to the power manager service");
        } else {
            mPowerManager = interface_cast<IPowerManager>(binder);
        }
    }

    if (!mPowermanageInited)
        mPowermanageInited = 1;
#endif
}

void SprdCamera3OEMIf::deinitPowerHint() {
#ifdef HAS_CAMERA_HINTS
    mPowermanageInited = 0;
    if (mPowerManager != NULL)
        mPowerManager.clear();
    if (mPrfmLock != NULL)
        mPrfmLock.clear();
#endif
}

void SprdCamera3OEMIf::enablePowerHint() {
#ifdef HAS_CAMERA_HINTS
    Mutex::Autolock l(&mPowermanageLock);
    HAL_LOGD("IN ");
    if (mPrfmLock != NULL) {
        if (mPowerManager != 0) {
            ALOGI("releaseWakeLock_l() - Prfmlock ");
            mPowerManager->releasePrfmLock(mPrfmLock);
        }
        mPrfmLock.clear();
    }

    if (mPowerManager != NULL) {
        sp<IBinder> binder = new BBinder();
        mPowerManager->acquirePrfmLock(binder, String16("Camera"),
                                       String16("CameraServer"),
                                       POWER_HINT_VENDOR_CAMERA_PERFORMANCE);
        mPrfmLock = binder;
    }
    HAL_LOGD("OUT");
#endif
}

void SprdCamera3OEMIf::disablePowerHint() {
#ifdef HAS_CAMERA_HINTS
    HAL_LOGD("IN");
    Mutex::Autolock l(&mPowermanageLock);
    if (mPrfmLock != 0) {
        if (mPowerManager != 0) {
            ALOGI("releaseWakeLock_l() - Prfmlock ");
            mPowerManager->releasePrfmLock(mPrfmLock);
        }
        mPrfmLock.clear();
    }

    HAL_LOGD("OUT");
#endif
}
int SprdCamera3OEMIf::changeDfsPolicy(int dfs_policy) {
    switch (dfs_policy) {
    case CAM_EXIT:
        if (CAM_LOW == mCameraDfsPolicyCur) {
            releaseDfsPolicy(CAM_LOW);
        } else if (CAM_HIGH == mCameraDfsPolicyCur) {
            releaseDfsPolicy(CAM_HIGH);
        }
        mCameraDfsPolicyCur = CAM_EXIT;
        break;
    case CAM_LOW:
        if (CAM_EXIT == mCameraDfsPolicyCur) {
            setDfsPolicy(CAM_LOW);
        } else if (CAM_HIGH == mCameraDfsPolicyCur) {
            setDfsPolicy(CAM_LOW);
            releaseDfsPolicy(CAM_HIGH);
        }
        mCameraDfsPolicyCur = CAM_LOW;
        break;
    case CAM_HIGH:
        if (CAM_EXIT == mCameraDfsPolicyCur) {
            setDfsPolicy(CAM_HIGH);
            if (ddrDfsComplete() < 0) {
                HAL_LOGW("dfs failed try again");
                setDfsPolicy(CAM_HIGH);
                ddrDfsComplete();
            }
        } else if (CAM_LOW == mCameraDfsPolicyCur) {
            setDfsPolicy(CAM_HIGH);
            if (ddrDfsComplete() < 0) {
                HAL_LOGW("dfs failed try again");
                setDfsPolicy(CAM_HIGH);
                ddrDfsComplete();
            }
            releaseDfsPolicy(CAM_LOW);
        }
        mCameraDfsPolicyCur = CAM_HIGH;
        break;
    default:
        HAL_LOGW("unrecognize dfs policy");
        break;
    }
    HAL_LOGI("mCameraDfsPolicyCur: %d", mCameraDfsPolicyCur);
    return NO_ERROR;
}

int SprdCamera3OEMIf::setDfsPolicy(int dfs_policy) {
    const char *dfs_scene = NULL;
    const char *const scenario_dfs =
        "/sys/class/devfreq/scene-frequency/sprd_governor/scenario_dfs";
    FILE *fp = fopen(scenario_dfs, "wb");
    if (NULL == fp) {
        HAL_LOGW("failed to open %s X", scenario_dfs);
        return BAD_VALUE;
    }
    switch (dfs_policy) {
    case CAM_LOW:
        dfs_scene = CAM_LOW_STR;
        break;
    case CAM_HIGH:
        dfs_scene = CAM_HIGH_STR;
        break;
    default:
        HAL_LOGW("unrecognize dfs policy");
        break;
    }
    HAL_LOGD("dfs_scene: %s", dfs_scene);
    // echo dfs_scene > scenario_dfs
    fprintf(fp, "%s", dfs_scene);
    fclose(fp);
    fp = NULL;
    return NO_ERROR;
}

int SprdCamera3OEMIf::releaseDfsPolicy(int dfs_policy) {
    const char *dfs_scene = NULL;
    const char *const scenario_dfs =
        "/sys/class/devfreq/scene-frequency/sprd_governor/exit_scene";

    FILE *fp = fopen(scenario_dfs, "wb");
    if (NULL == fp) {
        HAL_LOGW("failed to open %s X", scenario_dfs);
        return BAD_VALUE;
    }
    switch (dfs_policy) {
    case CAM_LOW:
        dfs_scene = CAM_LOW_STR;
        break;
    case CAM_HIGH:
        dfs_scene = CAM_HIGH_STR;
        break;
    default:
        HAL_LOGW("unrecognize dfs policy");
        break;
    }
    HAL_LOGD("release dfs_scene: %s", dfs_scene);
    // echo dfs_scene > scenario_dfs
    fprintf(fp, "%s", dfs_scene);
    fclose(fp);
    fp = NULL;
    return NO_ERROR;
}

int SprdCamera3OEMIf::ddrDfsComplete(void) {
    const char *const ddrinfo_cur_freq =
        "/sys/class/devfreq/scene-frequency/sprd_governor/ddrinfo_cur_freq";
    char str[10] = {'\0'};
    int size = 0;
    int ret = -1;
    int i = 0;

    FILE *fp = fopen(ddrinfo_cur_freq, "rb");
    if (NULL == fp) {
        HAL_LOGW("failed to open %s X", ddrinfo_cur_freq);
        return BAD_VALUE;
    }
    do {
        fseek(fp, 0, 0);
        size = fread(str, 1, 9, fp);
        if (size > 0) {
            long ddr_freq = atol(str);
            if (ddr_freq <= 900) {
                usleep(2 * 1000);
            } else {
                ret = 0;
                break;
            }
        } else {
            break;
        }
        i++;
    } while (i < 5);
    fclose(fp);
    return ret;
}

int SprdCamera3OEMIf::getCameraTemp() {
    const char *const temp = "/sys/devices/virtual/thermal/thermal_zone0/temp";
    int fd = open(temp, O_RDONLY);
    if (fd < 0) {
        ALOGE("error opening %s.", temp);
        return 0;
    }
    char buf[5] = {0};
    int n = read(fd, buf, 5);
    if (n == -1) {
        ALOGE("error reading %s", temp);
        close(fd);
        return 0;
    }
    close(fd);
    return atoi(buf) / 1000;
}

void SprdCamera3OEMIf::adjustFpsByTemp() {
    struct cmr_range_fps_param fps_param;
    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);

    int temp = 0, tempStates = -1;
    temp = getCameraTemp();
    if (temp < 65) {
        if (mTempStates != CAMERA_NORMAL_TEMP) {
            if (temp < 60) {
                tempStates = CAMERA_NORMAL_TEMP;
            }
        } else {
            tempStates = CAMERA_NORMAL_TEMP;
        }
    } else if (temp >= 65 && temp < 75) {
        if (mTempStates == CAMERA_DANGER_TEMP) {
            if (temp < 70) {
                tempStates = CAMERA_HIGH_TEMP;
            }
        } else {
            tempStates = CAMERA_HIGH_TEMP;
        }
    } else {
        tempStates = CAMERA_DANGER_TEMP;
    }

    if (tempStates != mTempStates) {
        mTempStates = tempStates;
        mIsTempChanged = 1;
    }

    if (mIsTempChanged) {
        switch (tempStates) {
        case CAMERA_NORMAL_TEMP:
            setCameraPreviewMode(mRecordingMode);
            break;
        case CAMERA_HIGH_TEMP:
            fps_param.min_fps = fps_param.max_fps =
                (controlInfo.ae_target_fps_range[1] + 10) / 2;
            HAL_LOGD("high temp, set camera min_fps: %lu, max_fps: %lu",
                     fps_param.min_fps, fps_param.max_fps);
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_RANGE_FPS,
                     (cmr_uint)&fps_param);
            break;
        case CAMERA_DANGER_TEMP:
            fps_param.min_fps = fps_param.max_fps = 10;
            HAL_LOGD("danger temp, set camera min_fps: %lu, max_fps: %lu",
                     fps_param.min_fps, fps_param.max_fps);
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_RANGE_FPS,
                     (cmr_uint)&fps_param);
            break;
        default:
            HAL_LOGE("error states");
            break;
        }
        mIsTempChanged = 0;
    }
}

void SprdCamera3OEMIf::setSprdCameraLowpower(int flag) {
    mSprdCameraLowpower = flag;
}

bool SprdCamera3OEMIf::setCameraPreviewDimensions() {
    uint32_t local_width = 0, local_height = 0;
    uint32_t mem_size = 0;
    struct img_size preview_size, video_size = {0, 0}, capture_size = {0, 0};
    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    if (mPreviewWidth != 0 && mPreviewHeight != 0) {
        preview_size.width = (cmr_u32)mPreviewWidth;
        preview_size.height = (cmr_u32)mPreviewHeight;
    } else {
        if ((mVideoWidth != 0 && mVideoHeight != 0) &&
            (mRawWidth != 0 && mRawHeight != 0)) {
            HAL_LOGE("HAL not support video stream and callback stream "
                     "simultaneously");
            return false;
        } else {
            preview_size.width = 720;
            preview_size.height = 576;
        }
    }
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_PREVIEW_SIZE,
             (cmr_uint)&preview_size);

    if (mVideoWidth != 0 && mVideoHeight != 0) {
        video_size.width = (cmr_u32)mVideoWidth;
        video_size.height = (cmr_u32)mVideoHeight;
        bool is_3D_video = false;

        if (mMultiCameraMode == MODE_3D_VIDEO) {
            is_3D_video = true;
        }
        if (sprddefInfo.slowmotion <= 1 && !is_3D_video)
            mCaptureMode = CAMERA_ZSL_MODE;
    }

    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_VIDEO_SIZE,
             (cmr_uint)&video_size);

    if (mZslPreviewMode == false) {
        if (mSprdZslEnabled == false) {
            if (mRawWidth != 0 && mRawHeight != 0) {
                capture_size.width = (cmr_u32)mRawWidth;
                capture_size.height = (cmr_u32)mRawHeight;
                mCaptureMode = CAMERA_ZSL_MODE;
            } else {
                if (mVideoWidth != 0 &&
                    mVideoHeight !=
                        0) { // capture size must equal with video size
                    if (mCaptureWidth != 0 && mCaptureHeight != 0) {
                        capture_size.width = (cmr_u32)mCaptureWidth;
                        capture_size.height = (cmr_u32)mCaptureHeight;
                    } else {
                        capture_size.width = (cmr_u32)mPreviewWidth;
                        capture_size.height = (cmr_u32)mPreviewHeight;
                    }
                } else {
                    if (mCaptureWidth != 0 && mCaptureHeight != 0) {
                        if ((mPreviewWidth >= 1920 && mPreviewHeight >= 1088) &&
                            (mCaptureWidth > 720 && mCaptureHeight > 480)) {
                            capture_size.width = 720;
                            capture_size.height = 480;
                        } else {
                            capture_size.width = (cmr_u32)mPreviewWidth;
                            capture_size.height = (cmr_u32)mPreviewHeight;
                        }
                    }
                }
            }
        } else {
            if (mCaptureWidth != 0 && mCaptureHeight != 0) {
                capture_size.width = mCaptureWidth;
                capture_size.height = mCaptureHeight;
                mCaptureMode = CAMERA_ZSL_MODE;
            }
            if (mRawWidth != 0 && mRawHeight != 0) {
                capture_size.width = (cmr_u32)mRawWidth;
                capture_size.height = (cmr_u32)mRawHeight;
                mCaptureMode = CAMERA_ZSL_MODE;
            }
        }
    } else {
        if (mCaptureWidth != 0 && mCaptureHeight != 0) { // API 1.0 ZSL
            /*if(mCaptureWidth > 2592 || mCaptureHeight >1952) {
                    capture_size.width = 2592;
                    capture_size.height = 1952;
            } else {*/
            capture_size.width = (cmr_u32)mCaptureWidth;
            capture_size.height = (cmr_u32)mCaptureHeight;
            //}
        } else { // API 2.0 ZSL
            capture_size.width = (cmr_u32)mRawWidth;
            capture_size.height = (cmr_u32)mRawHeight;
        }
    }
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_SIZE,
             (cmr_uint)&capture_size);

    HAL_LOGD("preview: width=%d, height=%d, video: width=%d, height=%d, "
             "capture: width=%d, height=%d",
             preview_size.width, preview_size.height, video_size.width,
             video_size.height, capture_size.width, capture_size.height);
    HAL_LOGD("mCaptureMode %d mPreviewWidth %d,mPreviewHeight %d, mRawWidth "
             "=%d, mRawHeight %d,mCaptureWidth =%d,mCaptureHeight  =%d",
             mCaptureMode, mPreviewWidth, mPreviewHeight, mRawWidth, mRawHeight,
             mCaptureWidth, mCaptureHeight);

    return true;
}

bool SprdCamera3OEMIf::setCameraCaptureDimensions() {
    uint32_t local_width = 0, local_height = 0;
    uint32_t mem_size = 0;
    struct img_size capture_size;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    HAL_LOGD("E: mCaptureWidth=%d, mCaptureHeight=%d, mPreviewWidth=%d, "
             "mPreviewHeight=%d, mVideoWidth=%d, mVideoHeight=%d",
             mCaptureWidth, mCaptureHeight, mPreviewWidth, mPreviewHeight,
             mVideoWidth, mVideoHeight);
    //	if(mCaptureMode != CAMERA_ZSL_MODE){
    //		mVideoWidth = 0;
    //		mVideoHeight = 0;
    //	}
    mHalOem->ops->camera_fast_ctrl(mCameraHandle, CAMERA_FAST_MODE_FD, 0);

    if (mCaptureWidth != 0 && mCaptureHeight != 0) {
        if (mVideoWidth != 0 && mVideoHeight != 0 && mRecordingMode == true &&
            ((mCaptureMode != CAMERA_ISP_TUNING_MODE) &&
             (mCaptureMode != CAMERA_ISP_SIMULATION_MODE))) {
            capture_size.width = (cmr_u32)mPreviewWidth;   // mVideoWidth;
            capture_size.height = (cmr_u32)mPreviewHeight; // mVideoHeight;
        } else {
            capture_size.width = (cmr_u32)mCaptureWidth;
            capture_size.height = (cmr_u32)mCaptureHeight;
        }
    } else {
        capture_size.width = (cmr_u32)mRawWidth;
        capture_size.height = (cmr_u32)mRawHeight;
    }
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_SIZE,
             (cmr_uint)&capture_size);

    HAL_LOGD("X");

    return true;
}

void SprdCamera3OEMIf::setCameraPreviewMode(bool isRecordMode) {
    struct cmr_range_fps_param fps_param;
    char value[PROPERTY_VALUE_MAX];
    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    fps_param.is_recording = isRecordMode;
    if (isRecordMode) {
        fps_param.min_fps = controlInfo.ae_target_fps_range[1];
        fps_param.max_fps = controlInfo.ae_target_fps_range[1];
        fps_param.video_mode = 1;

#ifdef CONFIG_CAMRECORDER_DYNAMIC_FPS
        property_get("volte.incall.camera.enable", value, "false");
        if (!strcmp(value, "false") && !mFixedFpsEnabled) {
            fps_param.min_fps = 16;
            fps_param.max_fps = 25;
        }
#endif
        // when 3D video recording with face beautify, fix frame rate at 25fps.
        if (mSprdCameraLowpower) {
            fps_param.min_fps = fps_param.max_fps = 20;
        }

        // to set recording fps by setprop
        char prop[PROPERTY_VALUE_MAX];
        int val_max = 0;
        int val_min = 0;
        property_get("persist.sys.camera.record.fps", prop, "0");
        if (atoi(prop) != 0) {
            val_min = atoi(prop) % 100;
            val_max = atoi(prop) / 100;
            fps_param.min_fps = val_min > 5 ? val_min : 5;
            fps_param.max_fps = val_max;
        }

    } else {
        fps_param.min_fps = controlInfo.ae_target_fps_range[0];
        fps_param.max_fps = controlInfo.ae_target_fps_range[1];
        fps_param.video_mode = 0;

        // to set preview fps by setprop
        char prop[PROPERTY_VALUE_MAX];
        int val_max = 0;
        int val_min = 0;
        property_get("persist.sys.camera.preview.fps", prop, "0");
        if (atoi(prop) != 0) {
            val_min = atoi(prop) % 100;
            val_max = atoi(prop) / 100;
            fps_param.min_fps = val_min > 5 ? val_min : 5;
            fps_param.max_fps = val_max;
        }
    }
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_RANGE_FPS,
             (cmr_uint)&fps_param);

    HAL_LOGD("min_fps=%ld, max_fps=%ld, video_mode=%ld", fps_param.min_fps,
             fps_param.max_fps, fps_param.video_mode);
#if 1 // for cts
    mIsUpdateRangeFps = true;
    mUpdateRangeFpsCount++;
    if (mUpdateRangeFpsCount == UPDATE_RANGE_FPS_COUNT) {
        if ((mPrvMinFps == fps_param.min_fps) &&
            (mPrvMaxFps == fps_param.max_fps))
            mUpdateRangeFpsCount = 0;
    }
    mPrvMinFps = fps_param.min_fps;
    mPrvMaxFps = fps_param.max_fps;
#endif
}

bool SprdCamera3OEMIf::setCameraPreviewFormat() {
    HAL_LOGD("E: mPreviewFormat=%d, mPictureFormat=%d", mPreviewFormat,
             mPictureFormat);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    mPictureFormat = mPreviewFormat;
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_PREVIEW_FORMAT,
             (cmr_uint)mPreviewFormat);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_FORMAT,
             (cmr_uint)mPictureFormat);

    HAL_LOGD("X");

    return true;
}

void SprdCamera3OEMIf::setCameraState(Sprd_camera_state state,
                                      state_owner owner) {
    Sprd_camera_state org_state = SPRD_IDLE;
    volatile Sprd_camera_state *state_owner = NULL;
    Mutex::Autolock stateLock(&mStateLock);
    HAL_LOGD("E:state: %s, owner: %d camera id %d", getCameraStateStr(state),
             owner, mCameraId);

    if (owner == STATE_CAPTURE) {
        if (mCameraState.capture_state == SPRD_INTERNAL_CAPTURE_STOPPING &&
            state != SPRD_INIT && state != SPRD_ERROR && state != SPRD_IDLE)
            return;
    }

    switch (owner) {
    case STATE_CAMERA:
        org_state = mCameraState.camera_state;
        state_owner = &(mCameraState.camera_state);
        break;

    case STATE_PREVIEW:
        org_state = mCameraState.preview_state;
        state_owner = &(mCameraState.preview_state);
        break;

    case STATE_CAPTURE:
        org_state = mCameraState.capture_state;
        state_owner = &(mCameraState.capture_state);
        break;

    case STATE_FOCUS:
        org_state = mCameraState.focus_state;
        state_owner = &(mCameraState.focus_state);
        break;

    case STATE_SET_PARAMS:
        org_state = mCameraState.setParam_state;
        state_owner = &(mCameraState.setParam_state);
        break;
    default:
        HAL_LOGE("owner error!");
        break;
    }

    switch (state) {
    /*camera state*/
    case SPRD_INIT:
        mCameraState.camera_state = SPRD_INIT;
        mCameraState.preview_state = SPRD_IDLE;
        mCameraState.capture_state = SPRD_IDLE;
        mCameraState.focus_state = SPRD_IDLE;
        mCameraState.setParam_state = SPRD_IDLE;
        break;

    case SPRD_IDLE:
        *state_owner = SPRD_IDLE;
        break;

    case SPRD_INTERNAL_STOPPING:
        mCameraState.camera_state = state;
        break;

    case SPRD_ERROR:
        *state_owner = SPRD_ERROR;
        break;

    /*preview state*/
    case SPRD_PREVIEW_IN_PROGRESS:
    case SPRD_INTERNAL_PREVIEW_STOPPING:
    case SPRD_INTERNAL_PREVIEW_REQUESTED:
        mCameraState.preview_state = state;
        break;

    /*capture state*/
    case SPRD_FLASH_IN_PROGRESS:
    case SPRD_INTERNAL_RAW_REQUESTED:
    case SPRD_WAITING_RAW:
    case SPRD_WAITING_JPEG:
    case SPRD_INTERNAL_CAPTURE_STOPPING:
        mCameraState.capture_state = state;
        break;

    /*focus state*/
    case SPRD_FOCUS_IN_PROGRESS:
        mCameraState.focus_state = state;
        break;

    /*set_param state*/
    case SPRD_SET_PARAMS_IN_PROGRESS:
        mCameraState.setParam_state = state;
        break;

    default:
        HAL_LOGE("unknown owner");
        break;
    }

    if (org_state != state)
        mStateWait.broadcast();

    HAL_LOGD("X: camera state = %s, preview state = %s, capture state = %s "
             "focus state = %s set param state = %s",
             getCameraStateStr(mCameraState.camera_state),
             getCameraStateStr(mCameraState.preview_state),
             getCameraStateStr(mCameraState.capture_state),
             getCameraStateStr(mCameraState.focus_state),
             getCameraStateStr(mCameraState.setParam_state));
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getCameraState() {
    HAL_LOGD("%s", getCameraStateStr(mCameraState.camera_state));
    return mCameraState.camera_state;
}

camera_status_t SprdCamera3OEMIf::GetCameraStatus(camera_status_type_t state) {
    camera_status_t ret = CAMERA_PREVIEW_IN_PROC;
    switch (state) {
    case CAMERA_STATUS_PREVIEW:
        switch (mCameraState.preview_state) {
        case SPRD_IDLE:
            ret = CAMERA_PREVIEW_IDLE;
            break;
        default:
            break;
        }
        break;
    case CAMERA_STATUS_SNAPSHOT:
        ret = CAMERA_SNAPSHOT_IDLE;
        switch (mCameraState.capture_state) {
        case SPRD_INTERNAL_RAW_REQUESTED:
        case SPRD_WAITING_JPEG:
        case SPRD_WAITING_RAW:
            ret = CAMERA_SNAPSHOT_IN_PROC;
            break;
        default:
            break;
        }
        break;
    }
    return ret;
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getPreviewState() {
    HAL_LOGV("%s", getCameraStateStr(mCameraState.preview_state));
    return mCameraState.preview_state;
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getCaptureState() {
    HAL_LOGV("%s", getCameraStateStr(mCameraState.capture_state));
    return mCameraState.capture_state;
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getFocusState() {
    HAL_LOGD("%s", getCameraStateStr(mCameraState.focus_state));
    return mCameraState.focus_state;
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getSetParamsState() {
    HAL_LOGD("%s", getCameraStateStr(mCameraState.setParam_state));
    return mCameraState.setParam_state;
}

bool SprdCamera3OEMIf::isCameraInit() {
    HAL_LOGD("%s", getCameraStateStr(mCameraState.camera_state));
    return (SPRD_IDLE == mCameraState.camera_state);
}

bool SprdCamera3OEMIf::isCameraIdle() {
    return (SPRD_IDLE == mCameraState.preview_state &&
            SPRD_IDLE == mCameraState.capture_state);
}

bool SprdCamera3OEMIf::isPreviewing() {
    HAL_LOGD("%s", getCameraStateStr(mCameraState.preview_state));
    return (SPRD_PREVIEW_IN_PROGRESS == mCameraState.preview_state);
}

bool SprdCamera3OEMIf::isPreviewStart() {
    HAL_LOGV("%s", getCameraStateStr(mCameraState.preview_state));
    return (SPRD_INTERNAL_PREVIEW_REQUESTED == mCameraState.preview_state);
}

bool SprdCamera3OEMIf::isCapturing() {
    bool ret = false;
    HAL_LOGV("%s", getCameraStateStr(mCameraState.capture_state));
    if (SPRD_FLASH_IN_PROGRESS == mCameraState.capture_state) {
        return false;
    }

    if ((SPRD_INTERNAL_RAW_REQUESTED == mCameraState.capture_state) ||
        (SPRD_WAITING_RAW == mCameraState.capture_state) ||
        (SPRD_WAITING_JPEG == mCameraState.capture_state)) {
        ret = true;
    } else if ((SPRD_INTERNAL_CAPTURE_STOPPING == mCameraState.capture_state) ||
               (SPRD_ERROR == mCameraState.capture_state)) {
        setCameraState(SPRD_IDLE, STATE_CAPTURE);
        HAL_LOGD("%s", getCameraStateStr(mCameraState.capture_state));
    } else if (SPRD_IDLE != mCameraState.capture_state) {
        HAL_LOGE("error: unknown capture status");
        ret = true;
    }
    return ret;
}

bool SprdCamera3OEMIf::checkPreviewStateForCapture() {
    bool ret = true;
    Sprd_camera_state tmpState = SPRD_IDLE;

    tmpState = getCaptureState();
    if ((SPRD_INTERNAL_CAPTURE_STOPPING == tmpState) ||
        (SPRD_ERROR == tmpState)) {
        HAL_LOGE("incorrect capture status %s", getCameraStateStr(tmpState));
        ret = false;
    } else {
        tmpState = getPreviewState();
        if (iSZslMode() || mSprdZslEnabled || mVideoSnapshotType == 1) {
            if (SPRD_PREVIEW_IN_PROGRESS != tmpState) {
                HAL_LOGE("incorrect preview status %d of ZSL capture mode",
                         (uint32_t)tmpState);
                ret = false;
            }
        } else {
            if (SPRD_IDLE != tmpState) {
                HAL_LOGE("incorrect preview status %d of normal capture mode",
                         (uint32_t)tmpState);
                ret = false;
            }
        }
    }
    return ret;
}

bool SprdCamera3OEMIf::WaitForCameraStart() {
    ATRACE_CALL();

    HAL_LOGD("E");
    Mutex::Autolock stateLock(&mStateLock);

    while (SPRD_IDLE != mCameraState.camera_state &&
           SPRD_ERROR != mCameraState.camera_state) {
        HAL_LOGD("waiting for SPRD_IDLE");
        mStateWait.wait(mStateLock);
        HAL_LOGD("woke up");
    }

    HAL_LOGD("X");
    return SPRD_IDLE == mCameraState.camera_state;
}

bool SprdCamera3OEMIf::WaitForCameraStop() {
    ATRACE_CALL();

    HAL_LOGD("E");
    Mutex::Autolock stateLock(&mStateLock);

    if (SPRD_INTERNAL_STOPPING == mCameraState.camera_state) {
        while (SPRD_INIT != mCameraState.camera_state &&
               SPRD_ERROR != mCameraState.camera_state) {
            HAL_LOGD("waiting for SPRD_IDLE");
            mStateWait.wait(mStateLock);
            HAL_LOGD("woke up");
        }
    }

    HAL_LOGD("X");

    return SPRD_INIT == mCameraState.camera_state;
}

bool SprdCamera3OEMIf::WaitForPreviewStart() {
    ATRACE_CALL();

    HAL_LOGD("E");
    Mutex::Autolock stateLock(&mStateLock);

    while (SPRD_PREVIEW_IN_PROGRESS != mCameraState.preview_state &&
           SPRD_ERROR != mCameraState.preview_state) {
        HAL_LOGD("waiting for SPRD_PREVIEW_IN_PROGRESS");
        if (mStateWait.waitRelative(mStateLock, PREV_TIMEOUT)) {
            HAL_LOGE("timeout");
            break;
        }
        HAL_LOGD("woke up");
    }

    HAL_LOGD("X");

    return SPRD_PREVIEW_IN_PROGRESS == mCameraState.preview_state;
}

bool SprdCamera3OEMIf::WaitForPreviewStop() {
    ATRACE_CALL();

    Mutex::Autolock statelock(&mStateLock);

    while (SPRD_IDLE != mCameraState.preview_state &&
           SPRD_ERROR != mCameraState.preview_state) {
        HAL_LOGD("waiting for SPRD_IDLE");
        if (mStateWait.waitRelative(mStateLock, PREV_STOP_TIMEOUT)) {
            HAL_LOGE("Timeout");
            break;
        }
        HAL_LOGD("woke up");
    }

    return SPRD_IDLE == mCameraState.preview_state;
}

bool SprdCamera3OEMIf::WaitForCaptureStart() {
    ATRACE_CALL();

    HAL_LOGD("E");
    Mutex::Autolock stateLock(&mStateLock);

    /* It's possible for the YUV callback as well as the JPEG callbacks*/
    /*to be invoked before we even make it here, so we check for all*/
    /*possible result states from takePicture.*/
    while (SPRD_WAITING_RAW != mCameraState.capture_state &&
           SPRD_WAITING_JPEG != mCameraState.capture_state &&
           SPRD_IDLE != mCameraState.capture_state &&
           SPRD_ERROR != mCameraState.capture_state &&
           SPRD_ERROR != mCameraState.camera_state) {
        HAL_LOGD("waiting for SPRD_WAITING_RAW or SPRD_WAITING_JPEG");
        if (mStateWait.waitRelative(mStateLock, CAP_START_TIMEOUT)) {
            HAL_LOGE("timeout");
            break;
        }

        HAL_LOGD("woke up, state is %s",
                 getCameraStateStr(mCameraState.capture_state));
    }

    HAL_LOGD("X");
    return (SPRD_WAITING_RAW == mCameraState.capture_state ||
            SPRD_WAITING_JPEG == mCameraState.capture_state ||
            SPRD_IDLE == mCameraState.capture_state);
}

bool SprdCamera3OEMIf::WaitForCaptureDone() {
    ATRACE_CALL();

    Mutex::Autolock stateLock(&mStateLock);
    while (SPRD_IDLE != mCameraState.capture_state &&
           SPRD_ERROR != mCameraState.capture_state) {

        HAL_LOGD("waiting for SPRD_IDLE");

        /*if (camera_capture_is_idle()) {
                HAL_LOGD("WaitForCaptureDone: for OEM cap is IDLE, set capture
        state to %s",
                        getCameraStateStr(mCameraState.capture_state));
                setCameraState(SPRD_IDLE, STATE_CAPTURE);
        } else {*/
        if (mStateWait.waitRelative(mStateLock, CAP_TIMEOUT)) {
            HAL_LOGE("timeout");
            break;
        }
        //}

        HAL_LOGD("woke up");
    }

    return SPRD_IDLE == mCameraState.capture_state;
}

bool SprdCamera3OEMIf::WaitForBurstCaptureDone() {
    ATRACE_CALL();

    Mutex::Autolock stateLock(&mStateLock);

    SprdCamera3PicChannel *pic_channel =
        reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);
    SprdCamera3Stream *callback_stream = NULL;
    SprdCamera3Stream *jpeg_stream = NULL;
    int32_t jpeg_ret = BAD_VALUE;
    int32_t callback_ret = BAD_VALUE;
    cmr_uint jpeg_vir, callback_vir;

    if (pic_channel == NULL)
        return true;
    HAL_LOGD("pic_channel %p", pic_channel);
    pic_channel->getStream(CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT, &jpeg_stream);
    pic_channel->getStream(CAMERA_STREAM_TYPE_PICTURE_CALLBACK,
                           &callback_stream);
    HAL_LOGD("jpeg_stream %p, callback_stream %p", jpeg_stream,
             callback_stream);
    if (jpeg_stream == NULL && callback_stream == NULL)
        return true;

    if (jpeg_stream)
        jpeg_ret = jpeg_stream->getQBuffFirstVir(&jpeg_vir);
    if (callback_stream)
        callback_ret = callback_stream->getQBuffFirstVir(&callback_vir);

    HAL_LOGD("jpeg_ret %d, callback_ret %d, NO_ERROR %d", jpeg_ret,
             callback_ret, NO_ERROR);

    while (NO_ERROR == jpeg_ret || NO_ERROR == callback_ret) {
        // for cts testMandatoryOutputCombinations
        if (mTakePictureMode == SNAPSHOT_PREVIEW_MODE ||
            mTakePictureMode == SNAPSHOT_ONLY_MODE) {
            usleep(5);
            if (mCameraState.capture_state == SPRD_IDLE)
                return true;
        }

        HAL_LOGD("waiting for jpeg_ret = %d, callback_ret = %d", jpeg_ret,
                 callback_ret);

        if (mStateWait.waitRelative(mStateLock, CAP_TIMEOUT)) {
            HAL_LOGE("timeout");
            return false;
        }

        if (jpeg_stream)
            jpeg_ret = jpeg_stream->getQBuffFirstVir(&jpeg_vir);
        if (callback_stream)
            callback_ret = callback_stream->getQBuffFirstVir(&callback_vir);

        HAL_LOGD("woke up:  jpeg_ret = %d, callback_ret = %d", jpeg_ret,
                 callback_ret);
    }

    return true;
}

bool SprdCamera3OEMIf::WaitForCaptureJpegState() {
    ATRACE_CALL();

    Mutex::Autolock stateLock(&mStateLock);

    while (SPRD_WAITING_JPEG != mCameraState.capture_state &&
           SPRD_IDLE != mCameraState.capture_state &&
           SPRD_ERROR != mCameraState.capture_state &&
           SPRD_INTERNAL_CAPTURE_STOPPING != mCameraState.capture_state) {
        HAL_LOGD("waiting for SPRD_WAITING_JPEG");
        mStateWait.wait(mStateLock);
        HAL_LOGD("woke up, state is %s",
                 getCameraStateStr(mCameraState.capture_state));
    }
    return (SPRD_WAITING_JPEG == mCameraState.capture_state);
}

bool SprdCamera3OEMIf::WaitForFocusCancelDone() {
    ATRACE_CALL();

    Mutex::Autolock stateLock(&mStateLock);
    while (SPRD_IDLE != mCameraState.focus_state &&
           SPRD_ERROR != mCameraState.focus_state) {
        HAL_LOGD("waiting for SPRD_IDLE from %s",
                 getCameraStateStr(getFocusState()));
        if (mStateWait.waitRelative(mStateLock, CANCEL_AF_TIMEOUT)) {
            HAL_LOGV("timeout");
        }
        HAL_LOGD("woke up");
    }

    return SPRD_IDLE == mCameraState.focus_state;
}

bool SprdCamera3OEMIf::startCameraIfNecessary() {
    ATRACE_CALL();

    cmr_uint is_support_zsl = 0;
    cmr_uint max_width = 0;
    cmr_uint max_height = 0;
    struct exif_info exif_info = {0, 0};
    LENS_Tag lensInfo;
    /**add for 3d calibration get max sensor size begin*/
    SPRD_DEF_Tag sprddefInfo;
    memset(&sprddefInfo, 0, sizeof(SPRD_DEF_Tag));
    int i = 0;
    struct sensor_mode_info mode_info[SENSOR_MODE_MAX];
    memset(mode_info, 0, sizeof(struct sensor_mode_info) * SENSOR_MODE_MAX);
    /**add for 3d calibration get max sensor size end*/

    if (NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    if (!isCameraInit()) {
        HAL_LOGD("wait for camera_init");
        if (CMR_CAMERA_SUCCESS !=
            mHalOem->ops->camera_init(mCameraId, camera_cb, this, 0,
                                      &mCameraHandle, (void *)Callback_Malloc,
                                      (void *)Callback_Free)) {
            setCameraState(SPRD_INIT);
            HAL_LOGE("CameraIfNecessary: fail to camera_init().");
            return false;
        } else {
            setCameraState(SPRD_IDLE);
        }
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
        cmr_handle isp_handle = 0;
        mHalOem->ops->camera_get_isp_handle(mCameraHandle, &isp_handle);
        setispserver(isp_handle);
#endif
        mHalOem->ops->camera_get_zsl_capability(mCameraHandle, &is_support_zsl,
                                                &max_width, &max_height);

        if (!is_support_zsl) {
            mParameters.setZSLSupport("false");
        }

        /*if (isPreAllocCapMem()) {
                pre_alloc_cap_mem_thread_init((void *)this);
        }*/

        /*get sensor and lens info from oem layer*/
        mHalOem->ops->camera_get_sensor_exif_info(mCameraHandle, &exif_info);
        mSetting->getLENSTag(&lensInfo);
        lensInfo.aperture = exif_info.aperture;
        mSetting->setLENSTag(lensInfo);
        HAL_LOGD("camera_id: %d.apert %f", mCameraId, lensInfo.aperture);
        /*get sensor and lens info from oem layer*/

        /*get sensor otp from oem layer*/

        /*read refoucs mode begin*/
        if (MODE_SINGLE_CAMERA != mMultiCameraMode &&
            MODE_3D_CAPTURE != mMultiCameraMode) {
            mSprdRefocusEnabled = true;
            CMR_LOGI("mSprdRefocusEnabled %d", mSprdRefocusEnabled);
        }
        /*read refoucs mode end*/

        /*read refoucs otp begin*/
        if (mSprdRefocusEnabled == true && mCameraId == 0) {
#ifdef CAMERA_READ_OTP_FROM_FILE
            const char *psPath_OtpData = "data/misc/cameraserver/otp.bin";
            char *dual_otp_data = (char *)malloc(SPRD_DUAL_OTP_SIZE);
            OTP_Tag otpInfo;
            mSetting->getOTPTag(&otpInfo);
            int otp_ret =
                read_file(psPath_OtpData, dual_otp_data, SPRD_DUAL_OTP_SIZE);
            if (otp_ret == 0) {
                struct sensor_otp_cust_info otp_info;
                mHalOem->ops->camera_get_sensor_otp_info(mCameraHandle,
                                                         &otp_info);
                if (otp_info.total_otp.data_ptr != NULL &&
                    otp_info.dual_otp.dual_flag) {
                    save_file(psPath_OtpData, otp_info.total_otp.data_ptr,
                              otp_info.total_otp.size);
                    HAL_LOGD(
                        "camera_id: %d,otp_info %p, data_ptr %p, size 0x%x",
                        mCameraId, &otp_info, otp_info.total_otp.data_ptr,
                        otp_info.total_otp.size);
                    memcpy(otpInfo.otp_data, otp_info.total_otp.data_ptr,
                           otp_info.total_otp.size);
                    otpInfo.dual_otp_flag = 1;
                } else {
                    otpInfo.dual_otp_flag = 0;
                    HAL_LOGD("camera_id: %d, dual_otp_flag %d", mCameraId,
                             otpInfo.dual_otp_flag);
                }
            } else {
                HAL_LOGD("camera_id: %d,dual_otp_data %p", mCameraId,
                         dual_otp_data);
                memcpy(otpInfo.otp_data, dual_otp_data, SPRD_DUAL_OTP_SIZE);
                otpInfo.dual_otp_flag = 1;
                HAL_LOGD("camera_id: %d,dual_otp_data %p dual_otp_flag %d",
                         mCameraId, dual_otp_data, otpInfo.dual_otp_flag);
            }
            mSetting->setOTPTag(&otpInfo);

            if (dual_otp_data != NULL) {
                free(dual_otp_data);
                dual_otp_data = NULL;
            }
#else
            OTP_Tag otpInfo;
            memset(&otpInfo, 0, sizeof(OTP_Tag));
            mSetting->getOTPTag(&otpInfo);

            struct sensor_otp_cust_info otp_info;
            memset(&otp_info, 0, sizeof(struct sensor_otp_cust_info));
            mHalOem->ops->camera_get_sensor_otp_info(mCameraHandle, &otp_info);
            if (otp_info.total_otp.data_ptr != NULL &&
                otp_info.dual_otp.dual_flag) {
                HAL_LOGD(
                    "camera_id: %d,dual_otp_info %p, data_ptr %p, size 0x%x",
                    mCameraId, &otp_info, otp_info.total_otp.data_ptr,
                    otp_info.total_otp.size);
                memcpy(otpInfo.otp_data, otp_info.total_otp.data_ptr,
                       otp_info.total_otp.size);
                otpInfo.dual_otp_flag = 1;
            } else {
                otpInfo.dual_otp_flag = 0;
                HAL_LOGD("camera_id: %d, dual_otp_flag %d", mCameraId,
                         otpInfo.dual_otp_flag);
            }
            mSetting->setOTPTag(&otpInfo);
#endif
        }
        /*read refoucs otp end*/

        /*get sensor otp from oem layer end*/
        /**add for 3d calibration get max sensor size begin*/
        mSetting->getSPRDDEFTag(&sprddefInfo);
        mHalOem->ops->camera_get_sensor_info_for_raw(mCameraHandle, mode_info);
        for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
            HAL_LOGD("trim w=%d, h=%d", mode_info[i].trim_width,
                     mode_info[i].trim_height);
            if (mode_info[i].trim_width * mode_info[i].trim_height >=
                sprddefInfo.sprd_3dcalibration_cap_size[0] *
                    sprddefInfo.sprd_3dcalibration_cap_size[1]) {
                sprddefInfo.sprd_3dcalibration_cap_size[0] =
                    mode_info[i].trim_width;
                sprddefInfo.sprd_3dcalibration_cap_size[1] =
                    mode_info[i].trim_height;
            }
        }
        HAL_LOGI("sprd_3dcalibration_cap_size w=%d, h=%d",
                 sprddefInfo.sprd_3dcalibration_cap_size[0],
                 sprddefInfo.sprd_3dcalibration_cap_size[1]);
        mSetting->setSPRDDEFTag(sprddefInfo);
        /**add for 3d calibration get max sensor size end*/

        HAL_LOGD("initializing param");
    } else {
        HAL_LOGD("camera hardware has been started already");
    }

    mIommuEnabled = IommuIsEnabled();
    HAL_LOGD("mIommuEnabled=%d mSprdRefocusEnabled %d", mIommuEnabled,
             mSprdRefocusEnabled);

    return true;
}

sprd_camera_memory_t *SprdCamera3OEMIf::allocReservedMem(int buf_size,
                                                         int num_bufs,
                                                         uint32_t is_cache) {
    ATRACE_CALL();

    unsigned long paddr = 0;
    size_t psize = 0;
    int result = 0;
    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;

    HAL_LOGD("buf_size %d, num_bufs %d", buf_size, num_bufs);

    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        HAL_LOGE("fatal error! memory pointer is null.");
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        goto getpmem_fail;
    }

    if (is_cache) {
        pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                              (1 << 31) | ION_HEAP_ID_MASK_CAM);
    } else {
        pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                              ION_HEAP_ID_MASK_CAM);
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        HAL_LOGE("pHeapIon is null or getHeapID failed");
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
        HAL_LOGE("error getBase is null.");
        goto getpmem_fail;
    }

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    // memory->phys_addr is offset from memory->fd, always set 0 for yaddr
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    HAL_LOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
             memory->fd, memory->phys_addr, memory->data, memory->phys_size,
             pHeapIon);

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }
    return NULL;
}

sprd_camera_memory_t *SprdCamera3OEMIf::allocCameraMem(int buf_size,
                                                       int num_bufs,
                                                       uint32_t is_cache) {
    ATRACE_CALL();

    unsigned long paddr = 0;
    size_t psize = 0;
    int result = 0;
    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;

    HAL_LOGD("buf_size %d, num_bufs %d", buf_size, num_bufs);
    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        HAL_LOGE("fatal error! memory pointer is null.");
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        goto getpmem_fail;
    }

    if (!mIommuEnabled) {
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_MM);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_MM);
        }
    } else {
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_SYSTEM);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_SYSTEM);
        }
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        HAL_LOGE("pHeapIon is null or getHeapID failed");
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
        HAL_LOGE("error getBase is null.");
        goto getpmem_fail;
    }

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    memory->dev_fd = pHeapIon->getIonDeviceFd();
    // memory->phys_addr is offset from memory->fd, always set 0 for yaddr
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    HAL_LOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
             memory->fd, memory->phys_addr, memory->data, memory->phys_size,
             pHeapIon);

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }
    return NULL;
}

void SprdCamera3OEMIf::freeCameraMem(sprd_camera_memory_t *memory) {
    if (memory) {
        if (memory->ion_heap) {
            HAL_LOGD(
                "fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
                memory->fd, memory->phys_addr, memory->data, memory->phys_size,
                memory->ion_heap);
            delete memory->ion_heap;
            memory->ion_heap = NULL;
        } else {
            HAL_LOGD("memory->ion_heap is null:fd=0x%x, phys_addr=0x%lx, "
                     "virt_addr=%p, size=0x%lx",
                     memory->fd, memory->phys_addr, memory->data,
                     memory->phys_size);
        }
        free(memory);
        memory = NULL;
    }
}

void SprdCamera3OEMIf::canclePreviewMem() {}

int SprdCamera3OEMIf::releasePreviewFrame(int i) {
    int ret = 0;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    ret = mHalOem->ops->camera_release_frame(mCameraHandle, CAMERA_PREVIEW_DATA,
                                             i);
    return ret;
}

bool SprdCamera3OEMIf::initPreview() {
    HAL_LOGD("E");

    setCameraPreviewMode(mRecordingMode);

    if (!setCameraPreviewDimensions()) {
        HAL_LOGE("setCameraDimensions failed");
        return false;
    }

    setCameraPreviewFormat();

    HAL_LOGD("X");
    return true;
}

void SprdCamera3OEMIf::deinitPreview() {
    HAL_LOGV("E");

    // freePreviewMem();
    // camera_set_preview_mem(0, 0, 0, 0);
    // Clean the faceInfo,otherwise it will show face detect rect after
    // startPreview
    FACE_Tag faceInfo;
    mSetting->getFACETag(&faceInfo);
    if (faceInfo.face_num > 0) {
        memset(&faceInfo, 0, sizeof(FACE_Tag));
        mSetting->setFACETag(&faceInfo);
    }

    HAL_LOGV("X");
}

void SprdCamera3OEMIf::deinitCapture(bool isPreAllocCapMem) {
    HAL_LOGD("E %d", isPreAllocCapMem);

    if (0 == isPreAllocCapMem) {
        Callback_CaptureFree(0, 0, 0, 0);
    } else {
        HAL_LOGD("pre_allocate mode.");
    }

    /*
    if(mCaptureMode != CAMERA_ZSL_MODE)
            Callback_CaptureFree(0, 0, 0, 0);
    */
    Callback_CapturePathFree(0, 0, 0, 0);
}

int SprdCamera3OEMIf::startPreviewInternal() {
    ATRACE_CALL();
    int ret = NO_ERROR;
    bool is_push_zsl = false;
    bool is_volte = false;
    char value[PROPERTY_VALUE_MAX];
    char multicameramode[PROPERTY_VALUE_MAX];
    HAL_LOGD("E camera id %d", mCameraId);

    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    if (isCapturing()) {
        // WaitForCaptureDone();
        WaitForBurstCaptureDone();
        cancelPictureInternal();
    }

    if (isPreviewStart()) {
        HAL_LOGV("Preview not start! wait preview start");
        WaitForPreviewStart();
    }

    if (isPreviewing()) {
        HAL_LOGD("X: Preview already in progress, mRestartFlag=%d",
                 mRestartFlag);
        if (mRestartFlag == false) {
            return NO_ERROR;
        } else
            stopPreviewInternal();
    }
    mRestartFlag = false;
    mZslNum = 2;

    if (mRecordingMode == false && sprddefInfo.sprd_zsl_enabled == 1) {
        mSprdZslEnabled = true;
        changeDfsPolicy(CAM_HIGH);
    } else if ((mRecordingMode == true && sprddefInfo.slowmotion > 1) ||
               (mRecordingMode == true && mVideoSnapshotType == 1)) {
        mSprdZslEnabled = false;
        changeDfsPolicy(CAM_HIGH);
    } else if (mRecordingMode == true && mVideoWidth != 0 &&
               mVideoHeight != 0 && mCaptureWidth != 0 && mCaptureHeight != 0) {
        mSprdZslEnabled = true;
        mZslNum = 3;
    } else if (mSprdRefocusEnabled == true && mRawHeight != 0 &&
               mRawWidth != 0) {
        mSprdZslEnabled = true;
        changeDfsPolicy(CAM_HIGH);
        mZslNum = 3;
    } else if (mSprd3dCalibrationEnabled == true && mRawHeight != 0 &&
               mRawWidth != 0) {
        mSprdZslEnabled = true;
    } else {
        mSprdZslEnabled = false;
    }

    property_get("volte.incall.camera.enable", value, "false");
    if (!strcmp(value, "true")) {
        mSprdZslEnabled = false;
        CMR_LOGI("volte incall, don't need to configure zsl ");
    }
    HAL_LOGD("mSprdZslEnabled=%d", mSprdZslEnabled);

    if (!initPreview()) {
        HAL_LOGE("initPreview failed.  Not starting preview.");
        deinitPreview();
        return UNKNOWN_ERROR;
    }
    SprdCamera3Flash::reserveFlash(mCameraId);

    if (mCaptureMode == CAMERA_ZSL_MODE) {
        memset(mZSLCapInfo, 0, sizeof(mZSLCapInfo));
        struct img_size jpeg_thumb_size;
        jpeg_thumb_size.width = 320;
        jpeg_thumb_size.height = 240;
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_SIZE,
                 (cmr_uint)&jpeg_thumb_size);
    }

    setCameraState(SPRD_INTERNAL_PREVIEW_REQUESTED, STATE_PREVIEW);

    property_get("persist.sys.camera.raw.mode", value, "jpeg");
#ifdef CONFIG_CAMERA_ROTATION_CAPTURE
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 1);
    if (!strcmp(value, "raw")) {
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 0);
    }
#else
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 0);
#endif

    HAL_LOGV("mSprdZslEnabled=%d", mSprdZslEnabled);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_ZSL_ENABLED,
             (cmr_uint)mSprdZslEnabled);

    HAL_LOGV("mVideoSnapshotType=%d", mVideoSnapshotType);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_VIDEO_SNAPSHOT_TYPE,
             (cmr_uint)mVideoSnapshotType);
    HAL_LOGD("mCaptureMode=%d", mCaptureMode);

    HAL_LOGD("mSprdBurstModeEnabled=%d", mSprdBurstModeEnabled);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_BURSTMODE_ENABLED,
             (cmr_uint)mSprdBurstModeEnabled);

    if (mVideoWidth * mVideoHeight >= UHD_WIDTH * UHD_HEIGHT) {
        mUhdRecodingEnabled = true;
    } else {
        mUhdRecodingEnabled = false;
    }
    //for cts
    if (mUhdRecodingEnabled && (mCaptureWidth > UHD_WIDTH)) {
        mZslNum = 1;
    }
    HAL_LOGD("mUhdRecodingEnabled=%d", mUhdRecodingEnabled);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_UHD_RECORDING_ENABLED,
             (cmr_uint)mUhdRecodingEnabled);

    /**add for 3dcapture, increase zsl buffer number for frame sync*/
    if (sprddefInfo.sprd_3dcapture_enabled) {
        mZslNum = DUALCAM_ZSL_NUM;
        mZslMaxFrameNum = DUALCAM_MAX_ZSL_NUM;
    }

    mSetting->getLargestPictureSize(mCameraId, &mLargestSensorWidth,
                                    &mLargestSensorHeight);

    mHalOem->ops->camera_pre_capture_set_buffer_size(
        mCameraId, mLargestSensorWidth, mLargestSensorHeight);

    mHalOem->ops->camera_pre_capture_get_buffer_id(
        mCameraId, mLargestSensorWidth, mLargestSensorHeight);

    cmr_int qret =
        mHalOem->ops->camera_start_preview(mCameraHandle, mCaptureMode);

    if (qret != CMR_CAMERA_SUCCESS) {
        HAL_LOGE("failed: sensor error.");
        setCameraState(SPRD_ERROR, STATE_PREVIEW);
        deinitPreview();
        return UNKNOWN_ERROR;
    }
    if (mIspToolStart) {
        mIspToolStart = false;
    }
    if (PushFirstPreviewbuff()) {
        is_push_zsl = true;
        PushFirstZslbuff();
    }

    if (!isFaceBeautyOn(sprddefInfo))
        PushFirstVideobuff();
    if (!is_push_zsl)
        PushFirstZslbuff();

    HAL_LOGD("X camera id %d", mCameraId);

    return NO_ERROR;
}

void SprdCamera3OEMIf::stopPreviewInternal() {
    ATRACE_CALL();

    int ret = NO_ERROR;
    nsecs_t start_timestamp = systemTime();
    nsecs_t end_timestamp;
    mUpdateRangeFpsCount = 0;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    HAL_LOGD("E camera id %d", mCameraId);
    if (isCapturing()) {
        setCameraState(SPRD_INTERNAL_CAPTURE_STOPPING, STATE_CAPTURE);
        if (0 != mHalOem->ops->camera_cancel_takepicture(mCameraHandle)) {
            HAL_LOGE("camera_stop_capture failed!");
            return;
        }
    }

    if (isPreviewStart()) {
        HAL_LOGV("Preview not start! wait preview start");
        WaitForPreviewStart();
    }

    if (!isPreviewing()) {
        HAL_LOGD("Preview not in progress! stopPreviewInternal X");
        return;
    }

    mIsStoppingPreview = 1;
    setCameraState(SPRD_INTERNAL_PREVIEW_STOPPING, STATE_PREVIEW);
    if (CMR_CAMERA_SUCCESS !=
        mHalOem->ops->camera_stop_preview(mCameraHandle)) {
        setCameraState(SPRD_ERROR, STATE_PREVIEW);
        HAL_LOGE("fail to camera_stop_preview()");
    }

    WaitForPreviewStop();
    mIsStoppingPreview = 0;

    deinitPreview();
    end_timestamp = systemTime();
   #ifdef CONFIG_FACE_BEAUTY
     deinit_fb_handle(&face_beauty);
    #endif
    HAL_LOGD("X Time:%" PRId64 "(ms). camera id %d",
             (end_timestamp - start_timestamp) / 1000000, mCameraId);
}

takepicture_mode SprdCamera3OEMIf::getCaptureMode() {
    HAL_LOGD("cap mode %d.\n", mCaptureMode);

    return mCaptureMode;
}

bool SprdCamera3OEMIf::iSDisplayCaptureFrame() {
    if (CAMERA_ZSL_MODE == mCaptureMode)
        return false;

    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    SprdCamera3Stream *stream = NULL;
    int32_t ret = BAD_VALUE;
    cmr_uint addr_vir = 0;

    if (channel) {
        channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &stream);
        if (stream) {
            ret = stream->getQBuffFirstVir(&addr_vir);
        }
    }

    return ret == NO_ERROR;
}

bool SprdCamera3OEMIf::iSZslMode() {
    bool ret = true;

    if (CAMERA_ZSL_MODE != mCaptureMode)
        ret = false;

    if (mSprdZslEnabled == 0)
        ret = false;

    return ret;
}

int SprdCamera3OEMIf::cancelPictureInternal() {
    ATRACE_CALL();

    bool result = true;
    HAL_LOGD("E, state %s", getCameraStateStr(getCaptureState()));

    switch (getCaptureState()) {
    case SPRD_INTERNAL_RAW_REQUESTED:
    case SPRD_WAITING_RAW:
    case SPRD_WAITING_JPEG:
    case SPRD_FLASH_IN_PROGRESS:
        HAL_LOGD("camera state is %s, stopping picture.",
                 getCameraStateStr(getCaptureState()));
        mIsCancellingCapture = 1;

        setCameraState(SPRD_INTERNAL_CAPTURE_STOPPING, STATE_CAPTURE);
        if (0 != mHalOem->ops->camera_cancel_takepicture(mCameraHandle)) {
            HAL_LOGE("camera_stop_capture failed!");
            return UNKNOWN_ERROR;
        }

        result = WaitForCaptureDone();
        mIsCancellingCapture = 0;
        if (!iSZslMode()) {
            deinitCapture(mIsPreAllocCapMem);
            // camera_set_capture_trace(0);
        }
        break;

    default:
        HAL_LOGW("not taking a picture (state %s)",
                 getCameraStateStr(getCaptureState()));
        break;
    }

    HAL_LOGD("X");
    return result ? NO_ERROR : UNKNOWN_ERROR;
}

void SprdCamera3OEMIf::setCameraPrivateData() {
    /*	camera_sensor_exif_info exif_info;
            const char * isFlashSupport =
       mParameters.get("flash-mode-supported");
            memset(&exif_info, 0, sizeof(camera_sensor_exif_info));

            if (isFlashSupport
                    && 0 == strcmp("true", (char*)isFlashSupport)) {
                    exif_info.flash = 1;
            }

            camera_config_exif_info(&exif_info);*/
}

void SprdCamera3OEMIf::getPictureFormat(int *format) {
    *format = mPictureFormat;
}

#define PARSE_LOCATION(what, type, fmt, desc)                                  \
    do {                                                                       \
        pt->what = 0;                                                          \
        const char *what##_str = mParameters.get("gps-" #what);                \
        LOGI("%s: GPS PARM %s --> [%s]", __func__, "gps-" #what, what##_str);  \
        if (what##_str) {                                                      \
            type what = 0;                                                     \
            if (sscanf(what##_str, fmt, &what) == 1) {                         \
                pt->what = what;                                               \
            } else {                                                           \
                LOGE("GPS " #what " %s could not"                              \
                     " be parsed as a " #desc,                                 \
                     what##_str);                                              \
                result = false;                                                \
            }                                                                  \
        } else {                                                               \
            LOGW("%s: GPS " #what " not specified: "                           \
                 "defaulting to zero in EXIF header.",                         \
                 __func__);                                                    \
            result = false;                                                    \
        }                                                                      \
    } while (0)

int SprdCamera3OEMIf::CameraConvertCoordinateToFramework(int32_t *cropRegion) {
    float zoomWidth, zoomHeight;
    uint32_t i = 0;
    int ret = 0;
    uint32_t sensorOrgW = 0, sensorOrgH = 0, fdWid = 0, fdHeight = 0;
    HAL_LOGD("crop %d %d %d %d", cropRegion[0], cropRegion[1], cropRegion[2],
             cropRegion[3]);
#if 0
	if(mCameraId == 0) {
	    sensorOrgW = BACK_SENSOR_ORIG_WIDTH;
	    sensorOrgH = BACK_SENSOR_ORIG_HEIGHT;
	} else if(mCameraId == 1||mCameraId == 3) {
	    sensorOrgW = FRONT_SENSOR_ORIG_WIDTH;
	    sensorOrgH = FRONT_SENSOR_ORIG_HEIGHT;
	} else if(mCameraId == 2) {
	    sensorOrgW = BACK_EXT_SENSOR_ORIG_WIDTH;
	    sensorOrgH = BACK_EXT_SENSOR_ORIG_HEIGHT;
	}
#else
    mSetting->getLargestPictureSize(mCameraId, &mLargestPictureWidth,
                                    &mLargestPictureHeight);
    sensorOrgW = mLargestPictureWidth;
    sensorOrgH = mLargestPictureHeight;
#endif

    fdWid = cropRegion[2] - cropRegion[0];
    fdHeight = cropRegion[3] - cropRegion[1];
    if (fdWid == 0 || fdHeight == 0) {
        HAL_LOGE("parameters error.");
        return 1;
    }
    zoomWidth = (float)sensorOrgW / (float)mPreviewWidth;
    zoomHeight = (float)sensorOrgH / (float)mPreviewHeight;
    cropRegion[0] = (cmr_u32)((float)cropRegion[0] * zoomWidth);
    cropRegion[1] = (cmr_u32)((float)cropRegion[1] * zoomHeight);
    cropRegion[2] = (cmr_u32)((float)cropRegion[2] * zoomWidth);
    cropRegion[3] = (cmr_u32)((float)cropRegion[3] * zoomHeight);
    HAL_LOGD("Crop calculated (xs=%d,ys=%d,xe=%d,ye=%d)", cropRegion[0],
             cropRegion[1], cropRegion[2], cropRegion[3]);
    return ret;
}

int SprdCamera3OEMIf::CameraConvertCoordinateFromFramework(
    int32_t *cropRegion) {
    float zoomWidth, zoomHeight;
    uint32_t i = 0;
    int ret = 0;
    uint32_t sensorOrgW = 0, sensorOrgH = 0, fdWid = 0, fdHeight = 0;
    HAL_LOGD("crop %d %d %d %d", cropRegion[0], cropRegion[1], cropRegion[2],
             cropRegion[3]);
#if 0
	if(mCameraId == 0) {
	    sensorOrgW = BACK_SENSOR_ORIG_WIDTH;
	    sensorOrgH = BACK_SENSOR_ORIG_HEIGHT;
	} else if(mCameraId == 1||mCameraId == 3) {
	    sensorOrgW = FRONT_SENSOR_ORIG_WIDTH;
	    sensorOrgH = FRONT_SENSOR_ORIG_HEIGHT;
	} else if(mCameraId == 2) {
	    sensorOrgW = BACK_EXT_SENSOR_ORIG_WIDTH;
	    sensorOrgH = BACK_EXT_SENSOR_ORIG_HEIGHT;
	}
#else
    mSetting->getLargestPictureSize(mCameraId, &mLargestPictureWidth,
                                    &mLargestPictureHeight);
    sensorOrgW = mLargestPictureWidth;
    sensorOrgH = mLargestPictureHeight;
#endif

    fdWid = cropRegion[2] - cropRegion[0];
    fdHeight = cropRegion[3] - cropRegion[1];
    if (fdWid == 0 || fdHeight == 0) {
        HAL_LOGE("parameters error.");
        return 1;
    }
    zoomWidth = (float)mPreviewWidth / (float)sensorOrgW;
    zoomHeight = (float)mPreviewHeight / (float)sensorOrgH;
    cropRegion[0] = (cmr_u32)((float)cropRegion[0] * zoomWidth);
    cropRegion[1] = (cmr_u32)((float)cropRegion[1] * zoomHeight);
    cropRegion[2] = (cmr_u32)((float)cropRegion[2] * zoomWidth);
    cropRegion[3] = (cmr_u32)((float)cropRegion[3] * zoomHeight);
    HAL_LOGD("Crop calculated (xs=%d,ys=%d,xe=%d,ye=%d)", cropRegion[0],
             cropRegion[1], cropRegion[2], cropRegion[3]);
    return ret;
}

bool SprdCamera3OEMIf::getCameraLocation(camera_position_type *pt) {
    bool result = true;

    PARSE_LOCATION(timestamp, long, "%ld", "long");
    if (0 == pt->timestamp)
        pt->timestamp = time(NULL);

    PARSE_LOCATION(altitude, double, "%lf", "double float");
    PARSE_LOCATION(latitude, double, "%lf", "double float");
    PARSE_LOCATION(longitude, double, "%lf", "double float");

    pt->process_method = mParameters.get("gps-processing-method");
    /*
            HAL_LOGV("%s: setting image location result %d,  ALT %lf LAT %lf LON
       %lf",
                            __func__, result, pt->altitude, pt->latitude,
       pt->longitude);
    */
    return result;
}

int SprdCamera3OEMIf::displayCopy(uintptr_t dst_phy_addr,
                                  uintptr_t dst_virtual_addr,
                                  uintptr_t src_phy_addr,
                                  uintptr_t src_virtual_addr, uint32_t src_w,
                                  uint32_t src_h) {
    int ret = 0;

#ifndef MINICAMERA
    if (!mPreviewWindow || !mGrallocHal)
        return -EOWNERDEAD;
#endif

    if (mIsDvPreview) {
        memcpy((void *)dst_virtual_addr, (void *)src_virtual_addr,
               SIZE_ALIGN(src_w) * SIZE_ALIGN(src_h) * 3 / 2);
    } else {
        memcpy((void *)dst_virtual_addr, (void *)src_virtual_addr,
               src_w * src_h * 3 / 2);
    }

    return ret;
}

bool SprdCamera3OEMIf::isFaceBeautyOn(SPRD_DEF_Tag sprddefInfo) {
    for (int i =0; i < SPRD_FACE_BEAUTY_PARAM_NUM; i++) {
        if (sprddefInfo.perfect_skin_level[i] != 0)
               return true;
    }
    return false;
}

void SprdCamera3OEMIf::receivePreviewFDFrame(struct camera_frame_type *frame) {
    ATRACE_CALL();

    if (NULL == frame) {
        HAL_LOGE("invalid frame pointer");
        return;
    }
    Mutex::Autolock l(&mPreviewCbLock);
    FACE_Tag faceInfo;

    ssize_t offset = frame->buf_id;
    // camera_frame_metadata_t metadata;
    // camera_face_t face_info[FACE_DETECT_NUM];
    int32_t k = 0;
    int32_t sx = 0;
    int32_t sy = 0;
    int32_t ex = 0;
    int32_t ey = 0;
    struct img_rect rect = {0, 0, 0, 0};
    mSetting->getFACETag(&faceInfo);
    memset(&faceInfo, 0, sizeof(FACE_Tag));
    HAL_LOGD("receive face_num %d.mid=%d", frame->face_num, mCameraId);
    int32_t number_of_faces =
        frame->face_num <= FACE_DETECT_NUM ? frame->face_num : FACE_DETECT_NUM;
    faceInfo.face_num = number_of_faces;

    if (0 != number_of_faces) {
        for (k = 0; k < number_of_faces; k++) {
            faceInfo.face[k].id = k;
            sx = MIN(MIN(frame->face_info[k].sx, frame->face_info[k].srx),
                     MIN(frame->face_info[k].ex, frame->face_info[k].elx));
            sy = MIN(MIN(frame->face_info[k].sy, frame->face_info[k].sry),
                     MIN(frame->face_info[k].ey, frame->face_info[k].ely));
            ex = MAX(MAX(frame->face_info[k].sx, frame->face_info[k].srx),
                     MAX(frame->face_info[k].ex, frame->face_info[k].elx));
            ey = MAX(MAX(frame->face_info[k].sy, frame->face_info[k].sry),
                     MAX(frame->face_info[k].ey, frame->face_info[k].ely));
            HAL_LOGD("face rect:s(%d,%d) sr(%d,%d) e(%d,%d) el(%d,%d)",
                     frame->face_info[k].sx, frame->face_info[k].sy,
                     frame->face_info[k].srx, frame->face_info[k].sry,
                     frame->face_info[k].ex, frame->face_info[k].ey,
                     frame->face_info[k].elx, frame->face_info[k].ely);
            faceInfo.face[k].rect[0] = sx;
            faceInfo.face[k].rect[1] = sy;
            faceInfo.face[k].rect[2] = ex;
            faceInfo.face[k].rect[3] = ey;
            faceInfo.angle[k] = frame->face_info[k].angle;
            faceInfo.pose[k] = frame->face_info[k].pose;
            HAL_LOGD("smile level %d. face:%d  %d  %d  %d ,angle %d\n",
                     frame->face_info[k].smile_level, faceInfo.face[k].rect[0],
                     faceInfo.face[k].rect[1], faceInfo.face[k].rect[2],
                     faceInfo.face[k].rect[3],faceInfo.angle[k]);
            CameraConvertCoordinateToFramework(faceInfo.face[k].rect);
            /*When the half of face at the edge of the screen,the smile level
            returned by face detection library  can often more than 30.
            In order to repaier this defetion ,so when the face on the screen is
            too close to the edge of screen, the smile level will be set to 0.
            */
            faceInfo.face[k].score = frame->face_info[k].smile_level;
            if (faceInfo.face[k].score < 0)
                faceInfo.face[k].score = 0;
        }
    }

    mSetting->setFACETag(&faceInfo);
}

SprdCamera3OEMIf::shake_test_state SprdCamera3OEMIf::getShakeTestState() {
    return mShakeTest.mShakeTestState;
}

void SprdCamera3OEMIf::setShakeTestState(shake_test_state state) {
    mShakeTest.mShakeTestState = state;
}

/*
* check if iommu is enabled
* return val:
* 1: iommu is enabled
* 0: iommu is disabled
*/
int SprdCamera3OEMIf::IommuIsEnabled(void) {
    int ret;
    int iommuIsEnabled = 0;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    ret = mHalOem->ops->camera_get_iommu_status(mCameraHandle);
    if (ret) {
        iommuIsEnabled = 0;
        return iommuIsEnabled;
    }

    iommuIsEnabled = 1;
    return iommuIsEnabled;
}

int SprdCamera3OEMIf::allocOneFrameMem(
    struct SprdCamera3OEMIf::OneFrameMem *one_frame_mem_ptr) {
    ATRACE_CALL();

    struct SprdCamera3OEMIf::OneFrameMem *ptr = one_frame_mem_ptr;
    int ret = 0;

    /* alloc input y buffer */
    HAL_LOGD(" %d  %d  %d\n", mIommuEnabled, ptr->width, ptr->height);
    if (!mIommuEnabled) {
        ptr->pmem_hp = new MemIon("/dev/ion", ptr->width * ptr->height,
                                  MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    } else {
        ptr->pmem_hp = new MemIon("/dev/ion", ptr->width * ptr->height,
                                  MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    }
    if (ptr->pmem_hp->getHeapID() < 0) {
        HAL_LOGE("getHeapID failed\n");
        return -1;
    }

    ptr->fd = ptr->pmem_hp->getHeapID();
    // phys_addr is offset from memory->fd, always set 0 for yaddr
    ptr->phys_addr = 0;
    ptr->virt_addr = (unsigned char *)ptr->pmem_hp->getBase();
    if (!ptr->virt_addr) {
        HAL_LOGE("getBase: addr is null\n");
        return -1;
    }

    return 0;
}

int SprdCamera3OEMIf::relaseOneFrameMem(
    struct SprdCamera3OEMIf::OneFrameMem *one_frame_mem_ptr) {
    ATRACE_CALL();

    struct SprdCamera3OEMIf::OneFrameMem *ptr = one_frame_mem_ptr;
    if (!ptr)
        return 0;

    if (ptr->fd && !mIommuEnabled) {
        ptr->pmem_hp.clear();
    }

    return 0;
}

void SprdCamera3OEMIf::cpu_hotplug_disable(uint8_t is_disable) {
#define DISABLE_CPU_HOTPLUG "1"
#define ENABLE_CPU_HOTPLUG "0"
    const char *const hotplug =
        "/sys/devices/system/cpu/cpuhotplug/cpu_hotplug_disable";
    const char *cmd_str = DISABLE_CPU_HOTPLUG;
    uint8_t org_flag = 0;

    FILE *fp = fopen(hotplug, "w");

    HAL_LOGD("cpu hotplug disable %d", is_disable);
    if (!fp) {
        HAL_LOGW("Failed to open: cpu_hotplug_disable, %s", hotplug);
        return;
    }

    if (1 == is_disable) {
        cmd_str = DISABLE_CPU_HOTPLUG;
    } else {
        cmd_str = ENABLE_CPU_HOTPLUG;
    }
    fprintf(fp, "%s", cmd_str);
    fclose(fp);

    return;
}

void SprdCamera3OEMIf::cpu_dvfs_disable(uint8_t is_disable) {
#define DISABLE_CPU_DVFS "10"
#define ENABLE_CPU_DVFS "95"
    const char *const dvfs = "/sys/devices/system/cpu/cpuhotplug/up_threshold";
    const char *cmd_str = DISABLE_CPU_DVFS;
    uint8_t org_flag = 0;

    FILE *fp = fopen(dvfs, "w");

    HAL_LOGD("is_disable %d", is_disable);

    if (!fp) {
        HAL_LOGW("failed to open: cpu_dvfs_disable, %s", dvfs);
        return;
    }

    if (1 == is_disable) {
        cmd_str = DISABLE_CPU_DVFS;
    } else {
        cmd_str = ENABLE_CPU_DVFS;
    }
    fprintf(fp, "%s", cmd_str);
    fclose(fp);

    return;
}

void SprdCamera3OEMIf::prepareForPostProcess(void) { return; }

void SprdCamera3OEMIf::exitFromPostProcess(void) { return; }

int SprdCamera3OEMIf::overwritePreviewFrameMemInit(
    struct SprdCamera3OEMIf::OneFrameMem *one_frame_mem_ptr) {
    struct SprdCamera3OEMIf::OneFrameMem *ptr = one_frame_mem_ptr;
    uint32_t overwrite_offset = ptr->width * ptr->height * 2 / 3;
    uint32_t addr_offset = overwrite_offset;

    HAL_LOGD("%d %d 0x%x", overwrite_offset, addr_offset,
             mShakeTest.diff_yuv_color[mShakeTest.mShakeTestColorCount][0]);
    memset(ptr->virt_addr,
           mShakeTest.diff_yuv_color[mShakeTest.mShakeTestColorCount][0],
           overwrite_offset);
    overwrite_offset = ptr->width * ptr->height / 3;
    HAL_LOGD("%d %d 0x%x", overwrite_offset, addr_offset,
             mShakeTest.diff_yuv_color[mShakeTest.mShakeTestColorCount][1]);
    memset((ptr->virt_addr + addr_offset),
           mShakeTest.diff_yuv_color[mShakeTest.mShakeTestColorCount][1],
           overwrite_offset);
    if ((MAX_LOOP_COLOR_COUNT - 1) == mShakeTest.mShakeTestColorCount) {
        mShakeTest.mShakeTestColorCount = 0;
    } else {
        mShakeTest.mShakeTestColorCount++;
    }

    return 0;
}

void SprdCamera3OEMIf::overwritePreviewFrame(struct camera_frame_type *frame) {
    uint32_t overwrite_offset = 0;
    static struct SprdCamera3OEMIf::OneFrameMem overwrite_preview_frame;
    static struct SprdCamera3OEMIf::OneFrameMem *one_frame_mem_ptr =
        &overwrite_preview_frame;
    struct SprdCamera3OEMIf::OneFrameMem *ptr = one_frame_mem_ptr;
    ptr->width = frame->width;
    ptr->height = frame->height * 3 / 2;

    HAL_LOGD("SHAKE_TEST E");
    allocOneFrameMem(&overwrite_preview_frame);
    overwritePreviewFrameMemInit(&overwrite_preview_frame);
    memcpy((void *)(frame->y_vir_addr + overwrite_offset),
           (void *)ptr->virt_addr, (ptr->width * ptr->height));
    relaseOneFrameMem(&overwrite_preview_frame);
    HAL_LOGD("SHAKE_TEST X");
}

void SprdCamera3OEMIf::calculateTimestampForSlowmotion(int64_t frm_timestamp) {
    int64_t diff_timestamp = 0;
    uint8_t tmp_slow_mot = 1;
    SPRD_DEF_Tag sprddefInfo;

    diff_timestamp = frm_timestamp - mSlowPara.last_frm_timestamp;
    mSetting->getSPRDDEFTag(&sprddefInfo);
    HAL_LOGD("diff time=% " PRId64 " slow=%d", diff_timestamp,
             sprddefInfo.slowmotion);
    tmp_slow_mot = sprddefInfo.slowmotion;
    if (tmp_slow_mot == 0)
        tmp_slow_mot = 1;

    mSlowPara.rec_timestamp += diff_timestamp * tmp_slow_mot;
    mSlowPara.last_frm_timestamp = frm_timestamp;
}

void SprdCamera3OEMIf::receivePreviewFrame(struct camera_frame_type *frame) {
    ATRACE_CALL();

    Mutex::Autolock cbLock(&mPreviewCbLock);
    int ret = NO_ERROR;

    if (NULL == frame) {
        HAL_LOGE("invalid frame pointer");
        return;
    }

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    HAL_LOGV("E");

    if (SHAKE_TEST == getShakeTestState()) {
        overwritePreviewFrame(frame);
    }

    if (miSPreviewFirstFrame) {
        GET_END_TIME;
        GET_USE_TIME;
        HAL_LOGD("Launch Camera Time:%d(ms).", s_use_time);

        float cam_init_time;
        if (getApctCamInitSupport()) {
            cam_init_time =
                ((float)(systemTime() - cam_init_begin_time)) / 1000000000;
            writeCamInitTimeToProc(cam_init_time);
        }
        miSPreviewFirstFrame = 0;

        disablePowerHint();
    }

    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

#ifdef CONFIG_CAMERA_ISP
    send_img_data(ISP_TOOL_YVU420_2FRAME, mPreviewWidth, mPreviewHeight,
                  (char *)frame->y_vir_addr,
                  frame->width * frame->height * 3 / 2);
#endif

#if 1 // for cts
    int64_t buffer_timestamp_fps = 0;

    if ((!mRecordingMode) && (mUpdateRangeFpsCount >= UPDATE_RANGE_FPS_COUNT)) {
        struct cmr_range_fps_param fps_param;
        CONTROL_Tag controlInfo;
        mSetting->getCONTROLTag(&controlInfo);

        fps_param.min_fps = controlInfo.ae_target_fps_range[0];
        fps_param.max_fps = controlInfo.ae_target_fps_range[1];

        int64_t fps_range_up = 0;
        int64_t fps_range_low = 0;
        int64_t fps_range_offset = 0;
        int64_t timestamp = 0;

        fps_range_offset = 1000000000 / fps_param.max_fps;
        fps_range_low = (1000000000 / fps_param.max_fps) * (1 - 0.015);
        fps_range_up = (1000000000 / fps_param.min_fps) * (1 + 0.015);

        if (mPrvBufferTimestamp == 0) {
            buffer_timestamp_fps = frame->timestamp;
        } else {
            if (mIsUpdateRangeFps) {
                buffer_timestamp_fps = frame->timestamp;
                mIsUpdateRangeFps = false;
            } else {
                timestamp = frame->timestamp - mPrvBufferTimestamp;
                HAL_LOGD("timestamp is %" PRId64, timestamp);
                if ((timestamp > fps_range_low) && (timestamp < fps_range_up)) {
                    buffer_timestamp_fps = frame->timestamp;
                } else {
                    buffer_timestamp_fps =
                        mPrvBufferTimestamp + fps_range_offset;
                    HAL_LOGD("fix buffer_timestamp_fps is %" PRId64,
                             buffer_timestamp_fps);
                }
            }
        }
        mPrvBufferTimestamp = buffer_timestamp_fps;
    } else {
        buffer_timestamp_fps = frame->timestamp;
    }
#endif

#ifdef CONFIG_CAMERA_EIS
    vsOutFrame frame_out;
    frame_out.frame_data = NULL;
    int64_t buffer_timestamp;
    if (sprddefInfo.sprd_eis_enabled) {
        buffer_timestamp = buffer_timestamp_fps - frame->ae_time / 2;
    } else {
        buffer_timestamp = buffer_timestamp_fps;
    }
#else
    int64_t buffer_timestamp = buffer_timestamp_fps;
#endif

    VCM_Tag sprdvcmInfo;
    if (mSprdRefocusEnabled == true && mCameraId == 0) {
        mSetting->getVCMTag(&sprdvcmInfo);
        uint32_t vcm_step = 0;
        mHalOem->ops->camera_get_sensor_vcm_step(mCameraHandle, mCameraId,
                                                 &vcm_step);
        HAL_LOGD("vcm step is 0x%x", vcm_step);
        sprdvcmInfo.vcm_step = vcm_step;
        mSetting->setVCMTag(sprdvcmInfo);
    }

    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    cmr_uint pre_addr_vir = 0, rec_addr_vir = 0, callback_addr_vir = 0;
    SprdCamera3Stream *pre_stream = NULL, *rec_stream = NULL,
                      *callback_stream = NULL;
    int32_t pre_dq_num;
    uint32_t frame_num = 0;
    uintptr_t buff_vir = (uintptr_t)(frame->y_vir_addr);
    uint32_t buff_id = frame->buf_id;
    uint16_t img_width = frame->width, img_height = frame->height;
    buffer_handle_t *buff_handle = NULL;
    int32_t buf_deq_num = 0;
    int32_t buf_num = 0;
    cmr_uint videobuf_phy = 0;
    cmr_uint videobuf_vir = 0;
    cmr_uint prebuf_phy = 0;
    cmr_uint prebuf_vir = 0;
    cmr_s32 fd0 = 0;
    cmr_s32 fd1 = 0;
#ifdef CONFIG_FACE_BEAUTY
    int sx,sy,ex,ey,angle,pose;
    struct face_beauty_levels beautyLevels;
    beautyLevels.blemishLevel = (unsigned char)sprddefInfo.perfect_skin_level[0];
    beautyLevels.smoothLevel= (unsigned char)sprddefInfo.perfect_skin_level[1];
    beautyLevels.skinColor = (unsigned char)sprddefInfo.perfect_skin_level[2];
    beautyLevels.skinLevel = (unsigned char)sprddefInfo.perfect_skin_level[3];
    beautyLevels.brightLevel= (unsigned char)sprddefInfo.perfect_skin_level[4];
    beautyLevels.lipColor =(unsigned char)sprddefInfo.perfect_skin_level[5];
    beautyLevels.lipLevel = (unsigned char)sprddefInfo.perfect_skin_level[6];
    beautyLevels.slimLevel = (unsigned char)sprddefInfo.perfect_skin_level[7];
    beautyLevels.largeLevel = (unsigned char)sprddefInfo.perfect_skin_level[8];
#endif
    SENSOR_Tag sensorInfo;

    mSetting->getSENSORTag(&sensorInfo);
    sensorInfo.timestamp = buffer_timestamp;
    mSetting->setSENSORTag(sensorInfo);

    if (channel) {
        channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &pre_stream);
        channel->getStream(CAMERA_STREAM_TYPE_VIDEO, &rec_stream);
        channel->getStream(CAMERA_STREAM_TYPE_CALLBACK, &callback_stream);
        HAL_LOGV("pre_stream %p, rec_stream %p, callback_stream %p", pre_stream,
                 rec_stream, callback_stream);

#ifdef CONFIG_FACE_BEAUTY
    if (PREVIEW_ZSL_FRAME != frame->type && isFaceBeautyOn(sprddefInfo)) {
        faceDectect(1);
        if (isPreviewing() && frame->type == PREVIEW_FRAME) {
            if (MODE_3D_VIDEO != mMultiCameraMode &&
                MODE_3D_PREVIEW != mMultiCameraMode) {
                FACE_Tag faceInfo;
                mSetting->getFACETag(&faceInfo);
                if (faceInfo.face_num>0) {
                    for (int i = 0 ; i < faceInfo.face_num; i++) {
                        CameraConvertCoordinateFromFramework(faceInfo.face[i].rect);
                        sx = faceInfo.face[i].rect[0];
                        sy = faceInfo.face[i].rect[1];
                        ex = faceInfo.face[i].rect[2];
                        ey = faceInfo.face[i].rect[3];
                        angle = faceInfo.angle[i];
                        pose = faceInfo.pose[i];
                        construct_fb_face(&face_beauty, i, sx, sy, ex, ey,angle,pose);
                    }
                }
                init_fb_handle(&face_beauty,1,2);
                construct_fb_image(&face_beauty, frame->width, frame->height, (unsigned char *)(frame->y_vir_addr), (unsigned char *)(frame->y_vir_addr + frame->width * frame->height), 1);
                construct_fb_level(&face_beauty, beautyLevels);
                do_face_beauty(&face_beauty,faceInfo.face_num);
            }
        }
    }else if (PREVIEW_ZSL_FRAME != frame->type){
         deinit_fb_handle(&face_beauty);
    }
#endif

        // recording stream
        if (rec_stream) {
            // reset timestamp and mIsRecording after recording
            rec_stream->getQBufListNum(&buf_deq_num);
            if (buf_deq_num == 0) {
                mSlowPara.last_frm_timestamp = 0;
                mIsRecording = false;
            }
            ret = rec_stream->getQBufNumForVir(buff_vir, &frame_num);
            if (ret == NO_ERROR) {
                ATRACE_BEGIN("video_frame");

                HAL_LOGI("record, fd=0x%x, buff_vir = 0x%lx, frame_num = %d, "
                         "buffer_timestamp = %" PRId64
                         ", frame->type = %ld rec=%" PRId64,
                         frame->fd, (cmr_uint)buff_vir, frame_num,
                         buffer_timestamp, frame->type,
                         mSlowPara.rec_timestamp);
                if (frame->type == PREVIEW_VIDEO_FRAME &&
                    frame_num >= mRecordFrameNum &&
                    (frame_num > mPictureFrameNum || frame_num == 0)) {
                    if (mVideoWidth <= mCaptureWidth &&
                        mVideoHeight <= mCaptureHeight) {
                        if (mVideoShotFlag &&
                            (frame_num >= mVideoSnapshotFrameNum))
                            PushVideoSnapShotbuff(frame_num,
                                                  CAMERA_STREAM_TYPE_VIDEO);
                    }
                    if (mSlowPara.last_frm_timestamp ==
                        0) { /*record first frame*/
                        mSlowPara.last_frm_timestamp = buffer_timestamp;
                        mSlowPara.rec_timestamp = buffer_timestamp;
                        mIsRecording = true;
                    }
                    if (frame_num > mPreviewFrameNum) { /*record first coming*/
                        calculateTimestampForSlowmotion(buffer_timestamp);
                    }
#ifdef CONFIG_CAMERA_EIS
                    HAL_LOGV("eis_enable = %d", sprddefInfo.sprd_eis_enabled);
                    if (sprddefInfo.sprd_eis_enabled) {
                        frame_out = EisVideoFrameStab(frame, frame_num);
                        if (frame_out.frame_data) {
                            channel->channelCbRoutine(frame_out.frame_num,
                                                      frame_out.timestamp *
                                                          1000000000,
                                                      CAMERA_STREAM_TYPE_VIDEO);
                            frame_out.frame_data = NULL;
                        }
                        goto bypass_rec;
                    }
#endif
                    if (mMultiCameraMode == MODE_3D_VIDEO) {
                        mSlowPara.rec_timestamp = buffer_timestamp;
                    }
                    channel->channelCbRoutine(frame_num,
                                              mSlowPara.rec_timestamp,
                                              CAMERA_STREAM_TYPE_VIDEO);
                    if (frame_num == (mDropVideoFrameNum + 1)) // for IOMMU
                                                               // error
                        channel->channelClearInvalidQBuff(
                            mDropVideoFrameNum, buffer_timestamp,
                            CAMERA_STREAM_TYPE_VIDEO);
                } else {
                    channel->channelClearInvalidQBuff(
                        frame_num, buffer_timestamp, CAMERA_STREAM_TYPE_VIDEO);
                }

                if (frame_num > mRecordFrameNum)
                    mRecordFrameNum = frame_num;

                ATRACE_END();
            }
            if ((mVideoSnapshotFrameNum <= mRecordFrameNum) &&
                mZslShotPushFlag == 1) {
                mZslPopFlag = 1;
                mZslShotWait.signal();
            }

        bypass_rec:
            HAL_LOGV("rec_stream X");
        }

        // preview stream
        if (pre_stream) {
            ret = pre_stream->getQBufNumForVir(buff_vir, &frame_num);
            if (ret == NO_ERROR) {
                ATRACE_BEGIN("preview_frame");

                HAL_LOGI("prev buff fd=0x%x, buff_vir=0x%lx, num %d, ret %d, "
                         "time % " PRId64 ", frame type = %ld rec=%" PRId64,
                         frame->fd, (cmr_uint)buff_vir, frame_num, ret,
                         buffer_timestamp, frame->type,
                         mSlowPara.rec_timestamp);
                if (frame->type == PREVIEW_FRAME &&
                    frame_num >= mPreviewFrameNum &&
                    (frame_num > mPictureFrameNum || frame_num == 0)) {
                    if (mVideoWidth > mCaptureWidth &&
                        mVideoHeight > mCaptureHeight) {
                        if (mVideoShotFlag)
                            PushVideoSnapShotbuff(frame_num,
                                                  CAMERA_STREAM_TYPE_PREVIEW);
                    }

#ifdef CONFIG_CAMERA_EIS
                    HAL_LOGV("eis_enable = %d", sprddefInfo.sprd_eis_enabled);
                    if (mRecordingMode && sprddefInfo.sprd_eis_enabled) {
                        EisPreviewFrameStab(frame);
                    }
#endif
                    if (rec_stream && isFaceBeautyOn(sprddefInfo)) {
                        ret = rec_stream->getQBufAddrForNum(
                            frame_num, &videobuf_vir, &videobuf_phy, &fd0);
                        if (ret == NO_ERROR && videobuf_vir != 0) {
                            mIsRecording = true;
                            if (mSlowPara.last_frm_timestamp == 0) {
                                mSlowPara.last_frm_timestamp = buffer_timestamp;
                                mSlowPara.rec_timestamp = buffer_timestamp;
                            }
                        }
                    } // recording cts

                    if (mIsRecording && rec_stream) {
                        if (frame_num > mRecordFrameNum)
                            calculateTimestampForSlowmotion(buffer_timestamp);
                        if (isFaceBeautyOn(sprddefInfo) &&
                            MODE_SINGLE_CAMERA ==
                                mMultiCameraMode) { // for 2D video recoding
                                                    // face_beauty
                            ret = rec_stream->getQBufAddrForNum(
                                frame_num, &videobuf_vir, &videobuf_phy, &fd0);
                            if (ret == NO_ERROR && videobuf_vir != 0) {
                                pre_stream->getQBufAddrForNum(
                                    frame_num, &prebuf_vir, &prebuf_phy, &fd1);
                                HAL_LOGD("frame_num=%d, videobuf_phy=0x%lx, "
                                         "videobuf_vir=0x%lx",
                                         frame_num, videobuf_phy, videobuf_vir);
                                HAL_LOGD("frame_num=%d, prebuf_phy=0x%lx, "
                                         "prebuf_vir=0x%lx",
                                         frame_num, prebuf_phy, prebuf_vir);
                                memcpy((void *)videobuf_vir, (void *)prebuf_vir,
                                       mPreviewWidth * mPreviewHeight * 3 / 2);
                                MemIon::Flush_ion_buffer(
                                    frame->fd, (void *)prebuf_vir, NULL,
                                    mPreviewWidth * mPreviewHeight * 3 / 2);
                                channel->channelCbRoutine(
                                    frame_num, mSlowPara.rec_timestamp,
                                    CAMERA_STREAM_TYPE_VIDEO);
                                if (frame_num ==
                                    (mDropVideoFrameNum + 1)) // for IOMMU error
                                    channel->channelClearInvalidQBuff(
                                        mDropVideoFrameNum, buffer_timestamp,
                                        CAMERA_STREAM_TYPE_VIDEO);
                                if (frame_num > mRecordFrameNum)
                                    mRecordFrameNum = frame_num;
                            }
                        } // copy preview buffer to video

                        channel->channelCbRoutine(frame_num,
                                                  mSlowPara.rec_timestamp,
                                                  CAMERA_STREAM_TYPE_PREVIEW);
                    } else {
                        channel->channelCbRoutine(frame_num, buffer_timestamp,
                                                  CAMERA_STREAM_TYPE_PREVIEW);
                    }
                    if (frame_num ==
                        (mDropPreviewFrameNum + 1)) // for IOMMU error
                        channel->channelClearInvalidQBuff(
                            mDropPreviewFrameNum, buffer_timestamp,
                            CAMERA_STREAM_TYPE_PREVIEW);
                } else {
                    channel->channelClearInvalidQBuff(
                        frame_num, buffer_timestamp,
                        CAMERA_STREAM_TYPE_PREVIEW);
                    if (rec_stream && isFaceBeautyOn(sprddefInfo))
                        channel->channelClearInvalidQBuff(
                            frame_num, buffer_timestamp,
                            CAMERA_STREAM_TYPE_VIDEO);
                }
                if (callback_stream)
                    callback_stream->getQBufListNum(&buf_num);
                if (mTakePictureMode == SNAPSHOT_PREVIEW_MODE && buf_num == 0) {
                    timer_set(this, 1, timer_hand_take);
                }
                if (frame_num > mPreviewFrameNum)
                    mPreviewFrameNum = frame_num;

                if (mSprdCameraLowpower && (0 == frame_num % 100)) {
                    adjustFpsByTemp();
                }

                ATRACE_END();
            } else {
                pre_stream = NULL;
            }
        }

        if (callback_stream) { // callback stream
            ret = callback_stream->getQBufNumForVir(buff_vir, &frame_num);
            if (ret == NO_ERROR) {
                ATRACE_BEGIN("callback_frame");

                HAL_LOGI("callback buff fd=0x%x, vir=0x%lx, num %d, ret %d, "
                         "time %" PRId64 ", frame type = %ld",
                         frame->fd, (cmr_uint)buff_vir, frame_num, ret,
                         buffer_timestamp, frame->type);

                if ((!pre_stream ||
                     (frame->type != PREVIEW_ZSL_CANCELED_FRAME)) &&
                    frame_num >= mZslFrameNum &&
                    (frame_num > mPictureFrameNum || frame_num == 0)) {
                    channel->channelCbRoutine(frame_num, buffer_timestamp,
                                              CAMERA_STREAM_TYPE_CALLBACK);
                    if (frame_num == (mDropZslFrameNum + 1)) // for IOMMU error
                        channel->channelClearInvalidQBuff(
                            mDropZslFrameNum, buffer_timestamp,
                            CAMERA_STREAM_TYPE_CALLBACK);
                } else {
                    channel->channelClearInvalidQBuff(
                        frame_num, buffer_timestamp,
                        CAMERA_STREAM_TYPE_CALLBACK);
                }
                if (mTakePictureMode == SNAPSHOT_PREVIEW_MODE) {
                    timer_set(this, 1, timer_hand_take);
                }
                if (frame_num > mZslFrameNum)
                    mZslFrameNum = frame_num;

                ATRACE_END();
            }
        }

        if (mSprdZslEnabled) {
            cmr_int need_pause;
            CMR_MSG_INIT(message);
            mHalOem->ops->camera_zsl_snapshot_need_pause(mCameraHandle,
                                                         &need_pause);
            if (PREVIEW_ZSL_FRAME == frame->type) {
                ATRACE_BEGIN("zsl_frame");

                HAL_LOGI("zsl buff fd=0x%x, frame type=%ld", frame->fd,
                         frame->type);
                pushZslFrame(frame);
                message.msg_type = CMR_EVT_ZSL_MON_PUSH;
                message.sync_flag = CMR_MSG_SYNC_NONE;
                message.data = NULL;
                ret = cmr_thread_msg_send(
                    (cmr_handle)mZSLModeMonitorMsgQueHandle, &message);
                if (ret) {
                    HAL_LOGE("Fail to send one msg!");
                    return;
                }

                ATRACE_END();
            } else if (PREVIEW_ZSL_CANCELED_FRAME == frame->type) {
                if (!isCapturing() || !need_pause) {
                    mHalOem->ops->camera_set_zsl_buffer(
                        mCameraHandle, frame->y_phy_addr, frame->y_vir_addr,
                        frame->fd);
                }
            }
        }
    }

    HAL_LOGV("X");
}

// display on frame for take picture
int SprdCamera3OEMIf::getRedisplayMem(uint32_t width, uint32_t height) {
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    uint32_t buffer_size = mHalOem->ops->camera_get_size_align_page(
        SIZE_ALIGN(width) * (SIZE_ALIGN(height)) * 3 / 2);

    if (mIsRotCapture) {
        buffer_size <<= 1;
    }

    if (mReDisplayHeap) {
        if (buffer_size > mReDisplayHeap->phys_size) {
            HAL_LOGD("Redisplay need re-allocm. 0x%x 0x%lx", buffer_size,
                     mReDisplayHeap->phys_size);
            freeCameraMem(mReDisplayHeap);
            mReDisplayHeap = allocCameraMem(buffer_size, 1, false);
        }
    } else {
        mReDisplayHeap = allocCameraMem(buffer_size, 1, false);
    }

    if (NULL == mReDisplayHeap)
        return 0;

    HAL_LOGD("addr=0x%p, fd 0x%x", mReDisplayHeap->data, mReDisplayHeap->fd);

    return mReDisplayHeap->fd;
}

void SprdCamera3OEMIf::FreeReDisplayMem() {
    HAL_LOGD("free redisplay mem.");
    if (mReDisplayHeap) {
        freeCameraMem(mReDisplayHeap);
        mReDisplayHeap = NULL;
    }
}

bool SprdCamera3OEMIf::displayOneFrameForCapture(uint32_t width,
                                                 uint32_t height, int fd,
                                                 uintptr_t phy_addr,
                                                 char *virtual_addr) {
    ATRACE_CALL();

    HAL_LOGD("E: size = %dx%d, phy_addr = 0x%lx, virtual_addr = %p", width,
             height, (cmr_uint)phy_addr, virtual_addr);

    Mutex::Autolock cbLock(&mPreviewCbLock);
    int64_t timestamp = systemTime();
    SprdCamera3RegularChannel *regular_channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    SprdCamera3PicChannel *pic_channel =
        reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);
    SprdCamera3Stream *pre_stream = NULL, *pic_stream = NULL;
    int32_t ret = 0;
    cmr_uint addr_vir = 0, addr_phy = 0;
    cmr_s32 ion_fd = 0;
    uint32_t frame_num = 0;

    if (regular_channel) {
        regular_channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &pre_stream);
        pic_channel->getStream(CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT,
                               &pic_stream);
        if (pre_stream) {
            if (pic_stream) {
                ret = pic_stream->getQBuffFirstNum(&frame_num);
                if (ret != NO_ERROR)
                    ret = pre_stream->getQBuffFirstVir(&addr_vir);
                else
                    ret = pre_stream->getQBufAddrForNum(frame_num, &addr_vir,
                                                        &addr_phy, &ion_fd);
            } else {
                ret = pre_stream->getQBuffFirstVir(&addr_vir);
                if (ret == NO_ERROR)
                    ret = pre_stream->getQBuffFirstNum(&frame_num);
            }
            HAL_LOGD("pic_addr_vir = 0x%lx, frame_num = %d, ret = %d", addr_vir,
                     frame_num, ret);
            if (ret == NO_ERROR) {
                if (addr_vir != 0 && virtual_addr != NULL)
                    memcpy((char *)addr_vir, (char *)virtual_addr,
                           (width * height * 3) / 2);
                regular_channel->channelCbRoutine(frame_num, timestamp,
                                                  CAMERA_STREAM_TYPE_PREVIEW);
            }
        }
    }

    HAL_LOGD("X");

    return true;
}

bool SprdCamera3OEMIf::iSCallbackCaptureFrame() {
    SprdCamera3PicChannel *pic_channel =
        reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);
    SprdCamera3Stream *stream = NULL;
    int32_t ret = BAD_VALUE;
    cmr_uint addr_vir = 0;

    if (pic_channel) {
        pic_channel->getStream(CAMERA_STREAM_TYPE_PICTURE_CALLBACK, &stream);
        if (stream) {
            ret = stream->getQBuffFirstVir(&addr_vir);
        }
    }

    return ret == NO_ERROR;
}

bool SprdCamera3OEMIf::receiveCallbackPicture(uint32_t width, uint32_t height,
                                              cmr_s32 fd, uintptr_t phy_addr,
                                              char *virtual_addr) {
    ATRACE_CALL();

    HAL_LOGD("E: size = %dx%d, phy_addr = 0x%lx, virtual_addr = %p", width,
             height, (cmr_uint)phy_addr, virtual_addr);

    Mutex::Autolock cbLock(&mPreviewCbLock);
    int64_t timestamp = systemTime();
    SprdCamera3PicChannel *pic_channel =
        reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);
    SprdCamera3Stream *stream = NULL;
    int32_t ret = 0;
    cmr_uint addr_vir = 0;
    uint32_t frame_num = 0;

    if (pic_channel) {
        pic_channel->getStream(CAMERA_STREAM_TYPE_PICTURE_CALLBACK, &stream);
        if (stream) {
            ret = stream->getQBuffFirstVir(&addr_vir);
            stream->getQBuffFirstNum(&frame_num);
            HAL_LOGD("pic_callback_addr_vir = 0x%lx, frame_num = %d", addr_vir,
                     frame_num);
            if (ret == NO_ERROR) {
                if (addr_vir != 0 && virtual_addr != NULL)
                    memcpy((char *)addr_vir, (char *)virtual_addr,
                           (width * height * 3) / 2);

                pic_channel->channelCbRoutine(
                    frame_num, timestamp, CAMERA_STREAM_TYPE_PICTURE_CALLBACK);
                HAL_LOGD("channelCbRoutine pic_callback_addr_vir = 0x%lx, "
                         "frame_num = %d",
                         addr_vir, frame_num);

                if (frame_num > mPictureFrameNum)
                    mPictureFrameNum = frame_num;
            }
            if ((mTakePictureMode == SNAPSHOT_NO_ZSL_MODE) ||
                (mTakePictureMode == SNAPSHOT_DEFAULT_MODE)) {
                SprdCamera3RegularChannel *regularChannel =
                    reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
                if (regularChannel)
                    regularChannel->channelClearInvalidQBuff(
                        mPictureFrameNum, timestamp,
                        CAMERA_STREAM_TYPE_PREVIEW);
            }
        }
    }

    HAL_LOGD("X");

    return true;
}

void SprdCamera3OEMIf::yuvNv12ConvertToYv12(struct camera_frame_type *frame,
                                            char *tmpbuf) {
    int width, height;

    width = frame->width;
    height = frame->height;
    if (tmpbuf) {
        char *addr0 = (char *)frame->y_vir_addr + width * height;
        char *addr1 = addr0 + SIZE_ALIGN(width / 2) * height / 2;
        char *addr2 = (char *)tmpbuf;

        memcpy((void *)tmpbuf, (void *)addr0, width * height / 2);
        if (width % 32) {
            int gap = SIZE_ALIGN(width / 2) - width / 2;
            for (int i = 0; i < width * height / 4; i++) {
                *addr0++ = *addr2++;
                *addr1++ = *addr2++;
                if (!((i + 1) % (width / 2))) {
                    addr0 = addr0 + gap;
                    addr1 = addr1 + gap;
                }
            }
        } else {
            for (int i = 0; i < width * height / 4; i++) {
                *addr0++ = *addr2++;
                *addr1++ = *addr2++;
            }
        }
    }
}

/**add for 3d calibration save debug img begin*/
cmr_int save_yuv_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width,
                         cmr_u32 height, struct img_addr *addr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    char file_name[40] = {
        0,
    };
    char tmp_str[10] = {
        0,
    };
    FILE *fp = NULL;

    HAL_LOGD("index %d format %d width %d heght %d", index, img_fmt, width,
             height);

    strcpy(file_name, CAMERA_DUMP_PATH);
    sprintf(tmp_str, "%d", width);
    strcat(file_name, tmp_str);
    strcat(file_name, "X");
    sprintf(tmp_str, "%d", height);
    strcat(file_name, tmp_str);

    if (IMG_DATA_TYPE_YUV420 == img_fmt || IMG_DATA_TYPE_YUV422 == img_fmt) {
        strcat(file_name, "_y_");
        sprintf(tmp_str, "%d", index);
        strcat(file_name, tmp_str);
        strcat(file_name, ".raw");
        HAL_LOGD("file name %s", file_name);
        fp = fopen(file_name, "wb");

        if (NULL == fp) {
            HAL_LOGE("can not open file: %s \n", file_name);
            return 0;
        }

        fwrite((void *)addr->addr_y, 1, width * height, fp);
        fclose(fp);

        strcpy(file_name, CAMERA_DUMP_PATH);
        sprintf(tmp_str, "%d", width);
        strcat(file_name, tmp_str);
        strcat(file_name, "X");
        sprintf(tmp_str, "%d", height);
        strcat(file_name, tmp_str);
        strcat(file_name, "_uv_");
        sprintf(tmp_str, "%d", index);
        strcat(file_name, tmp_str);
        strcat(file_name, ".raw");
        HAL_LOGD("file name %s", file_name);
        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            HAL_LOGE("can not open file: %s \n", file_name);
            return 0;
        }

        if (IMG_DATA_TYPE_YUV420 == img_fmt) {
            fwrite((void *)addr->addr_u, 1, width * height / 2, fp);
        } else {
            fwrite((void *)addr->addr_u, 1, width * height, fp);
        }
        fclose(fp);
    } else if (IMG_DATA_TYPE_JPEG == img_fmt) {
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", index);
        strcat(file_name, tmp_str);
        strcat(file_name, ".jpg");
        HAL_LOGD("file name %s", file_name);

        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            HAL_LOGE("can not open file: %s \n", file_name);
            return 0;
        }

        fwrite((void *)addr->addr_y, 1, width * height * 2, fp);
        fclose(fp);
    } else if (IMG_DATA_TYPE_RAW == img_fmt) {
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", index);
        strcat(file_name, tmp_str);
        strcat(file_name, ".mipi_raw");
        HAL_LOGD("file name %s", file_name);

        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            HAL_LOGE("can not open file: %s \n", file_name);
            return 0;
        }

        fwrite((void *)addr->addr_y, 1, (uint32_t)(width * height * 5 / 4), fp);
        fclose(fp);
    }
    return 0;
}

/**add for 3d calibration save debug img end*/
void SprdCamera3OEMIf::receiveRawPicture(struct camera_frame_type *frame) {
    ATRACE_CALL();

    HAL_LOGD("E, mReDisplayHeap = %p,frame->y_vir_addr 0x%lx ", mReDisplayHeap,
             frame->y_vir_addr);
    Mutex::Autolock cbLock(&mCaptureCbLock);
    bool display_flag, callback_flag;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    }; /**add for 3d calibration return yuv buff begin*/

    if (NULL == frame) {
        HAL_LOGE("invalid frame pointer");
        return;
    }

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    if (SPRD_INTERNAL_CAPTURE_STOPPING == getCaptureState()) {
        HAL_LOGW(
            "warning: capture state = SPRD_INTERNAL_CAPTURE_STOPPING, return");
        return;
    }

    display_flag = iSDisplayCaptureFrame();
    callback_flag = iSCallbackCaptureFrame();

    if (display_flag) {
        int dst_fd = 0;
        uintptr_t dst_paddr = 0;
        uint32_t dst_width = mPreviewWidth;
        uint32_t dst_height = mPreviewHeight;
        cmr_uint dst_vaddr = 0;
        dst_fd = getRedisplayMem(dst_width, dst_height);
        if (0 == dst_fd) {
            HAL_LOGE("get review memory failed");
            return;
        }
        if (mReDisplayHeap != NULL)
            dst_vaddr = (cmr_uint)mReDisplayHeap->data;

        if (0 !=
            mHalOem->ops->camera_get_redisplay_data(
                mCameraHandle, dst_fd, dst_paddr, dst_vaddr, dst_width,
                dst_height, frame->fd, frame->y_phy_addr, frame->uv_phy_addr,
                frame->y_vir_addr, frame->width, frame->height)) {
            HAL_LOGE("Fail to camera_get_data_redisplay.");
            FreeReDisplayMem();
            return;
        }

        if (mReDisplayHeap)
            displayOneFrameForCapture(dst_width, dst_height, dst_fd, dst_paddr,
                                      (char *)mReDisplayHeap->data);
        FreeReDisplayMem();
    }
    if (callback_flag) {
        int dst_fd = 0;
        uintptr_t dst_paddr = 0;
        uint32_t dst_width = mRawWidth;
        uint32_t dst_height = mRawHeight;
        cmr_uint dst_vaddr = 0;
        dst_fd = getRedisplayMem(dst_width, dst_height);
        if (0 == dst_fd) {
            HAL_LOGE("get review memory failed");
            return;
        }
        if (mReDisplayHeap != NULL)
            dst_vaddr = (cmr_uint)mReDisplayHeap->data;

        if (0 !=
            mHalOem->ops->camera_get_redisplay_data(
                mCameraHandle, dst_fd, dst_paddr, dst_vaddr, dst_width,
                dst_height, frame->fd, frame->y_phy_addr, frame->uv_phy_addr,
                frame->y_vir_addr, frame->width, frame->height)) {
            HAL_LOGE("Fail to camera_get_data_redisplay.");
            FreeReDisplayMem();
            return;
        }
        /**add for 3d calibration return yuv buff begin*/
        HAL_LOGD("is callback_flag:%d", callback_flag);
        property_get("debug.camera.save.3dcalfile", prop, "0");
        if (atoi(prop) == 1) {
            struct img_addr imgadd;
            bzero(&imgadd, sizeof(struct img_addr));
            HAL_LOGD("3dcalibration yuv picture: width:%d, height:%d, fd:%d,"
                     "yaddp:0x%lx, uaddp:0x%lx, vaddp:0x%lx, yaddv:0x%lx, "
                     "uaddv:0x%lx, vaddv:0x%lx",
                     frame->width, frame->height, frame->fd, frame->y_phy_addr,
                     frame->uv_phy_addr,
                     frame->uv_phy_addr + frame->width * frame->height / 2,
                     frame->y_vir_addr, frame->uv_vir_addr,
                     frame->uv_vir_addr + frame->width * frame->height / 2);
            imgadd.addr_y = frame->y_vir_addr;
            imgadd.addr_u = frame->uv_vir_addr;
            imgadd.addr_v =
                frame->uv_vir_addr + frame->width * frame->height / 2;
            save_yuv_to_file(mCameraId, IMG_DATA_TYPE_YUV420, frame->width,
                             frame->height, &imgadd);
        }
        /**add for 3d calibration return yuv buff end*/

        if (mSprdRefocusEnabled) {
            mReDisplayHeap->data = (void *)frame->y_vir_addr;
        }

        if (mReDisplayHeap) {
            if (mSprd3dCalibrationEnabled) /**modified for 3d calibration return
                                              yuv buff if redisplay exist*/
                mReDisplayHeap->data = (void *)frame->y_vir_addr;
            receiveCallbackPicture(dst_width, dst_height, dst_fd, dst_paddr,
                                   (char *)mReDisplayHeap->data);
        } else {
            /**add for 3d calibration return yuv buff begin*/
            if (mSprd3dCalibrationEnabled) {
                HAL_LOGD("3dcalibration call back picture: width:%d, "
                         "height:%d, fd:%d, phyadd:0x%lx, viradd:0x%lx",
                         frame->width, frame->height, frame->fd,
                         frame->y_phy_addr, frame->y_vir_addr);
                receiveCallbackPicture(frame->width, frame->height, frame->fd,
                                       frame->y_phy_addr,
                                       (char *)&frame->y_vir_addr);
            }
            /**add for 3d calibration return yuv buff end*/
        }
        FreeReDisplayMem();
    }

    HAL_LOGD("X, mReDisplayHeap = %p", mReDisplayHeap);
}

void SprdCamera3OEMIf::receiveJpegPicture(struct camera_frame_type *frame) {
    ATRACE_CALL();

    print_time();
    Mutex::Autolock cbLock(&mCaptureCbLock);
    Mutex::Autolock cbPreviewLock(&mPreviewCbLock);
    struct camera_jpeg_param *encInfo = &frame->jpeg_param;
    int64_t temp = 0, temp1 = 0;
    ;
    buffer_handle_t *jpeg_buff_handle = NULL;
    ssize_t maxJpegSize = -1;

    HAL_LOGD("E encInfo->size = %d, enc->buffer = %p, encInfo->need_free = %d "
             "time=%" PRId64,
             encInfo->size, encInfo->outPtr, encInfo->need_free,
             frame->timestamp);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    SENSOR_Tag sensorInfo;
    mSetting->getSENSORTag(&sensorInfo);
    if (0 != frame->sensor_info.exposure_time_denominator) {
        sensorInfo.exposure_time = 1000000000ll *
                                   frame->sensor_info.exposure_time_numerator /
                                   frame->sensor_info.exposure_time_denominator;
    }
    sensorInfo.timestamp = frame->timestamp;
    mSetting->setSENSORTag(sensorInfo);
    camera3_jpeg_blob *jpegBlob = NULL;
    int64_t timestamp = sensorInfo.timestamp;
    cmr_uint pic_addr_vir = 0;
    SprdCamera3Stream *pic_stream = NULL;
    int ret;
    uint32_t heap_size;
    SprdCamera3PicChannel *picChannel =
        reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);
    uint32_t frame_num = 0;
    char value[PROPERTY_VALUE_MAX];
    char debug_value[PROPERTY_VALUE_MAX];
    property_get("persist.sys.camera.raw.mode", value, "jpeg");
    property_get("persist.sys.camera.debug.mode", debug_value, "non-debug");

    if (picChannel && ((mCaptureMode != CAMERA_ISP_TUNING_MODE) ||
                       ((mCaptureMode == CAMERA_ISP_TUNING_MODE) &&
                        (!strcmp(value, "raw"))) ||
                       ((mCaptureMode == CAMERA_ISP_SIMULATION_MODE) &&
                        (!strcmp(value, "sim"))))) {
        picChannel->getStream(CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT, &pic_stream);
        if (pic_stream) {
            ret = pic_stream->getQBuffFirstVir(&pic_addr_vir);
            pic_stream->getQBuffFirstNum(&frame_num);
            HAL_LOGV("pic_addr_vir = 0x%lx, frame_num = %d", pic_addr_vir,
                     frame_num);
            if (ret == NO_ERROR && pic_addr_vir != 0) {
                void *isp_info_addr;
                int isp_info_size = 0;
                if (encInfo->outPtr != NULL) {
                    memcpy((char *)pic_addr_vir, (char *)(encInfo->outPtr),
                           encInfo->size);
                    if ((mCaptureMode == CAMERA_ISP_TUNING_MODE) ||
                        (!strcmp(debug_value, "debug"))) {
                        if (!mHalOem->ops->camera_get_isp_info(
                                mCameraHandle, &isp_info_addr,
                                &isp_info_size) &&
                            (0 != isp_info_size)) {
                            mJpegSize = encInfo->size;
                            memcpy(((char *)pic_addr_vir + mJpegSize),
                                   (char *)isp_info_addr, isp_info_size);
                        }
                    }
                }

                if ((mCaptureMode == CAMERA_ISP_TUNING_MODE) ||
                    (!strcmp(debug_value, "debug"))) {
                    mHalOem->ops->dump_jpeg_file((void *)pic_addr_vir,
                                                 encInfo->size + isp_info_size,
                                                 mCaptureWidth, mCaptureHeight);
                }

                pic_stream->getHeapSize(&heap_size);
                pic_stream->getQBufHandleForNum(frame_num, &jpeg_buff_handle);
                if (jpeg_buff_handle == NULL) {
                    HAL_LOGE("failed to get jpeg buffer handle");
                    return;
                }

#ifdef CONFIG_GPU_PLATFORM_ROGUE
                maxJpegSize = ADP_WIDTH(*jpeg_buff_handle);
#else
                maxJpegSize = ((private_handle_t *)(*jpeg_buff_handle))->width;
#endif
                if ((uint32_t)maxJpegSize > heap_size) {
                    maxJpegSize = heap_size;
                }

                jpegBlob = (camera3_jpeg_blob *)((char *)pic_addr_vir +
                                                 (maxJpegSize -
                                                  sizeof(camera3_jpeg_blob)));
                jpegBlob->jpeg_size = encInfo->size + isp_info_size;
                jpegBlob->jpeg_blob_id = CAMERA3_JPEG_BLOB_ID;
                picChannel->channelCbRoutine(
                    frame_num, timestamp, CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);

                if (frame_num > mPictureFrameNum)
                    mPictureFrameNum = frame_num;

                /**add for 3d capture, set raw call back mode & reprocess
                 * capture size begin*/
                if (mSprdReprocessing) {
                    HAL_LOGI("jpeg encode doen, reprocessing end");
                    setCaptureReprocessMode(false, mRawWidth, mRawHeight);
                }
            }
        }

        if (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE ||
            mTakePictureMode == SNAPSHOT_DEFAULT_MODE) {
            SprdCamera3RegularChannel *regularChannel =
                reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
            if (regularChannel) {
                regularChannel->channelClearInvalidQBuff(
                    mPictureFrameNum, timestamp, CAMERA_STREAM_TYPE_PREVIEW);
                regularChannel->channelClearInvalidQBuff(
                    mPictureFrameNum, timestamp, CAMERA_STREAM_TYPE_VIDEO);
                regularChannel->channelClearInvalidQBuff(
                    mPictureFrameNum, timestamp, CAMERA_STREAM_TYPE_CALLBACK);
            }
        }

        if (encInfo->need_free) {
            if (!iSZslMode()) {
                deinitCapture(mIsPreAllocCapMem);
            }
        }
    }

    if (mHDRPowerHint == 1 || mMultiLayerPowerHint == 1) {
        disablePowerHint();
        mHDRPowerHintFlag = 0;
        mMultiLayerPowerHintFlag = 0;
    }

    HAL_LOGD("X");
}

void SprdCamera3OEMIf::receiveJpegPictureError(void) {
    print_time();
    Mutex::Autolock cbLock(&mCaptureCbLock);
    if (!checkPreviewStateForCapture()) {
        HAL_LOGE("drop current jpegPictureError msg");
        return;
    }

    int index = 0;

    HAL_LOGD("JPEG callback was cancelled--not delivering image.");

    // NOTE: the JPEG encoder uses the raw image contained in mRawHeap, so we
    // need
    // to keep the heap around until the encoding is complete.

    print_time();
}

void SprdCamera3OEMIf::receiveCameraExitError(void) {
    Mutex::Autolock cbPreviewLock(&mPreviewCbLock);
    Mutex::Autolock cbCaptureLock(&mCaptureCbLock);

    if (!checkPreviewStateForCapture()) {
        HAL_LOGE("drop current cameraExit msg");
        return;
    }

    HAL_LOGE("HandleErrorState:don't enable error msg!");
}

void SprdCamera3OEMIf::receiveTakePictureError(void) {
    Mutex::Autolock cbLock(&mCaptureCbLock);

    if (!checkPreviewStateForCapture()) {
        HAL_LOGE("drop current takePictureError msg");
        return;
    }

    HAL_LOGE("camera cb: invalid state %s for taking a picture!",
             getCameraStateStr(getCaptureState()));
}

/*transite from 'from' state to 'to' state and signal the waitting thread. if
 * the current*/
/*state is not 'from', transite to SPRD_ERROR state should be called from the
 * callback*/
SprdCamera3OEMIf::Sprd_camera_state
SprdCamera3OEMIf::transitionState(SprdCamera3OEMIf::Sprd_camera_state from,
                                  SprdCamera3OEMIf::Sprd_camera_state to,
                                  SprdCamera3OEMIf::state_owner owner,
                                  bool lock) {
    volatile SprdCamera3OEMIf::Sprd_camera_state *which_ptr = NULL;

    if (lock)
        mStateLock.lock();

    HAL_LOGD("owner = %d, lock = %d", owner, lock);
    switch (owner) {
    case STATE_CAMERA:
        which_ptr = &mCameraState.camera_state;
        break;

    case STATE_PREVIEW:
        which_ptr = &mCameraState.preview_state;
        break;

    case STATE_CAPTURE:
        which_ptr = &mCameraState.capture_state;
        break;

    case STATE_FOCUS:
        which_ptr = &mCameraState.focus_state;
        break;

    default:
        HAL_LOGD("changeState: error owner");
        break;
    }

    if (NULL != which_ptr) {
        if (from != *which_ptr) {
            to = SPRD_ERROR;
        }

        HAL_LOGD("changeState: %s --> %s", getCameraStateStr(from),
                 getCameraStateStr(to));

        if (*which_ptr != to) {
            *which_ptr = to;
            // mStateWait.signal();
            mStateWait.broadcast();
        }
    }

    if (lock)
        mStateLock.unlock();

    return to;
}

void SprdCamera3OEMIf::HandleStartPreview(enum camera_cb_type cb, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    HAL_LOGV("in: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getPreviewState()));

    switch (cb) {
    case CAMERA_EXIT_CB_PREPARE:
        if (isPreAllocCapMem() && mTakePictureMode != SNAPSHOT_ONLY_MODE) {
            pre_alloc_cap_mem_thread_init((void *)this);
        }

        if (mCaptureMode == CAMERA_NORMAL_MODE) {
            int fd = 0;
            uint32_t dst_width = mPreviewWidth;
            uint32_t dst_height = mPreviewHeight;
            fd = getRedisplayMem(dst_width, dst_height);
        }
        break;

    case CAMERA_RSP_CB_SUCCESS:
        if (mIsStoppingPreview)
            HAL_LOGW("when is stopping preview, this place will change previw "
                     "status");
        setCameraState(SPRD_PREVIEW_IN_PROGRESS, STATE_PREVIEW);
        break;

    case CAMERA_EVT_CB_FRAME:
        HAL_LOGV("CAMERA_EVT_CB_FRAME");
        switch (getPreviewState()) {
        case SPRD_PREVIEW_IN_PROGRESS:
            receivePreviewFrame((struct camera_frame_type *)parm4);
            break;

        case SPRD_INTERNAL_PREVIEW_STOPPING:
            HAL_LOGD(
                "camera cb: discarding preview frame while stopping preview");
            break;

        default:
            HAL_LOGW("invalid state");
            break;
        }
        break;

    case CAMERA_EVT_CB_FD:
        HAL_LOGV("CAMERA_EVT_CB_FD");
        if (isPreviewing()) {
            receivePreviewFDFrame((struct camera_frame_type *)parm4);
        }
        break;

    case CAMERA_EXIT_CB_FAILED:
        HAL_LOGE("SprdCamera3OEMIf::camera_cb: @CAMERA_EXIT_CB_FAILURE(%p) in "
                 "state %s.",
                 parm4, getCameraStateStr(getPreviewState()));
        transitionState(getPreviewState(), SPRD_ERROR, STATE_PREVIEW);
        receiveCameraExitError();
        break;

    case CAMERA_EVT_SENSOR_DATATYPE:
        HAL_LOGV("CAMERA_EVT_SENSOR_DATATYPE");
        break;

    case CAMERA_EVT_CB_FLUSH:
        HAL_LOGV("CAMERA_EVT_CB_FLUSH");
        {
            struct camera_frame_type *frame = (struct camera_frame_type *)parm4;
            mPrevBufLock.lock();
            if (isPreviewing()) {
                flush_buffer(CAMERA_FLUSH_PREVIEW_HEAP, frame->buf_id,
                             (void *)frame->y_vir_addr,
                             (void *)frame->y_phy_addr, 0);
            }
            mPrevBufLock.unlock();
        }
        break;

    case CAMERA_EVT_CB_RESUME:
        if (isPreviewing() && iSZslMode()) {
            setZslBuffers();
            mZslChannelStatus = 1;
        }
        break;

    default:
        transitionState(getPreviewState(), SPRD_ERROR, STATE_PREVIEW);
        HAL_LOGE("unexpected cb %d for CAMERA_FUNC_START_PREVIEW.", cb);
        break;
    }

    HAL_LOGV("out, state = %s", getCameraStateStr(getPreviewState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleStopPreview(enum camera_cb_type cb, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    Mutex::Autolock cbPreviewLock(&mPreviewCbLock);
    Sprd_camera_state tmpPrevState = SPRD_IDLE;
    tmpPrevState = getPreviewState();
    HAL_LOGD("in: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(tmpPrevState));

    if ((SPRD_IDLE == tmpPrevState) ||
        (SPRD_INTERNAL_PREVIEW_STOPPING == tmpPrevState)) {
        setCameraState(SPRD_IDLE, STATE_PREVIEW);
    } else if (SPRD_PREVIEW_IN_PROGRESS == tmpPrevState && mIsStoppingPreview) {
        HAL_LOGI("when stopping preview, there are some frame frome oem "
                 "callback to hal");
        setCameraState(SPRD_IDLE, STATE_PREVIEW);
    } else {
        HAL_LOGE("HandleEncode: error preview status, %s",
                 getCameraStateStr(tmpPrevState));
        transitionState(tmpPrevState, SPRD_ERROR, STATE_PREVIEW);
    }

    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);
    controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_INACTIVE;
    controlInfo.awb_state = ANDROID_CONTROL_AWB_STATE_INACTIVE;
    mSetting->setAeCONTROLTag(&controlInfo);
    mSetting->setAwbCONTROLTag(&controlInfo);
    HAL_LOGD("state = %s", getCameraStateStr(getPreviewState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleTakePicture(enum camera_cb_type cb, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    HAL_LOGD("E: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCaptureState()));
    bool encode_location = true;
    camera_position_type pt = {0, 0, 0, 0, NULL};
    float aperture = 0;
    SPRD_DEF_Tag sprddefInfo;

    switch (cb) {
    case CAMERA_EXIT_CB_PREPARE:
        prepareForPostProcess();
        break;
    case CAMERA_EVT_CB_FLUSH:
        HAL_LOGV("CAMERA_EVT_CB_FLUSH");
        mCapBufLock.lock();
        if (mCapBufIsAvail == 1) {
            flush_buffer(CAMERA_FLUSH_RAW_HEAP_ALL, 0, (void *)0, (void *)0, 0);
        }
        mCapBufLock.unlock();
        break;
    case CAMERA_RSP_CB_SUCCESS: {
        HAL_LOGV("CAMERA_RSP_CB_SUCCESS");

        Sprd_camera_state tmpCapState = SPRD_INIT;
        tmpCapState = getCaptureState();
        if (SPRD_WAITING_RAW == tmpCapState) {
            HAL_LOGD("CAMERA_RSP_CB_SUCCESS has been called before, skip it");
        } else if (tmpCapState != SPRD_INTERNAL_CAPTURE_STOPPING) {
            transitionState(SPRD_INTERNAL_RAW_REQUESTED, SPRD_WAITING_RAW,
                            STATE_CAPTURE);
        }
        break;
    }
    case CAMERA_EVT_CB_CAPTURE_FRAME_DONE: {
        HAL_LOGV("CAMERA_EVT_CB_CAPTURE_FRAME_DONE");
        JPEG_Tag jpegInfo;
        mSetting->getJPEGTag(&jpegInfo);
        if (jpeg_gps_location) {
            camera_position_type pt = {0, 0, 0, 0, NULL};
            pt.altitude = jpegInfo.gps_coordinates[2];
            pt.latitude = jpegInfo.gps_coordinates[0];
            pt.longitude = jpegInfo.gps_coordinates[1];
            memcpy(mGps_processing_method, jpegInfo.gps_processing_method,
                   sizeof(mGps_processing_method));
            pt.process_method =
                reinterpret_cast<const char *>(&mGps_processing_method[0]);
            pt.timestamp = jpegInfo.gps_timestamp;
            HAL_LOGV("gps pt.latitude = %f, pt.altitude = %f, pt.longitude = "
                     "%f, pt.process_method = %s, jpegInfo.gps_timestamp = "
                     "%" PRId64,
                     pt.latitude, pt.altitude, pt.longitude, pt.process_method,
                     jpegInfo.gps_timestamp);

            /*if (camera_set_position(&pt, NULL, NULL) != CAMERA_SUCCESS) {
                    HAL_LOGE("receiveRawPicture: camera_set_position: error");
            }*/
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_POSITION,
                     (cmr_uint)&pt);

            jpeg_gps_location = false;
        }
        break;
    }
    case CAMERA_EVT_CB_SNAPSHOT_JPEG_DONE:
        exitFromPostProcess();
        HAL_LOGD(
            "CAMERA_EVT_CB_SNAPSHOT_JPEG_DONE, pip enable %d, refocus mode %d",
            mSprdPipVivEnabled, mSprdRefocusEnabled);
        //		mSprdPipVivEnabled = 1;
        if ((mSprdPipVivEnabled || mSprdRefocusEnabled) && (parm4 != NULL)) {
            Sprd_camera_state tmpCapState = getCaptureState();
            HAL_LOGD("PIP HandleTakePicture state = %d, need_free = %d camera "
                     "id = %d",
                     tmpCapState,
                     ((struct camera_frame_type *)parm4)->need_free, mCameraId);
            if ((SPRD_WAITING_JPEG == tmpCapState) ||
                (SPRD_INTERNAL_CAPTURE_STOPPING == tmpCapState)) {
                if (((struct camera_frame_type *)parm4)->need_free) {
                    setCameraState(SPRD_IDLE, STATE_CAPTURE);
                } else {
                    setCameraState(SPRD_INTERNAL_RAW_REQUESTED, STATE_CAPTURE);
                }
            } else if (SPRD_IDLE != tmpCapState) {
                LOGE("HandleEncode: CAMERA_EXIT_CB_DONE error cap status, %s",
                     getCameraStateStr(tmpCapState));
                transitionState(tmpCapState, SPRD_ERROR, STATE_CAPTURE);
            }
        }
        break;
    case CAMERA_EVT_CB_SNAPSHOT_DONE:
        HAL_LOGV("CAMERA_EVT_CB_SNAPSHOT_DONE");
        struct exif_spec_pic_taking_cond_tag exif_pic_info;
        bzero(&exif_pic_info, sizeof(struct exif_spec_pic_taking_cond_tag));
        LENS_Tag lensInfo;
        mHalOem->ops->camera_get_sensor_result_exif_info(mCameraHandle,
                                                         &exif_pic_info);
        if (exif_pic_info.ApertureValue.denominator != 0)
            aperture = (float)exif_pic_info.ApertureValue.numerator /
                       (float)exif_pic_info.ApertureValue.denominator;
        mSetting->getLENSTag(&lensInfo);
        lensInfo.aperture = aperture;
        mSetting->setLENSTag(lensInfo);
        if (checkPreviewStateForCapture() &&
            (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE ||
             mTakePictureMode == SNAPSHOT_DEFAULT_MODE ||
             mTakePictureMode == SNAPSHOT_ZSL_MODE)) {
            receiveRawPicture((struct camera_frame_type *)parm4);
        } else {
            HAL_LOGW("drop current rawPicture");
        }
        break;

    case CAMERA_EXIT_CB_DONE:
        HAL_LOGV("CAMERA_EXIT_CB_DONE");

        if (SPRD_WAITING_RAW == getCaptureState()) {
            /**modified for 3d calibration&3d capture return yuv buffer finished
             * begin */
            HAL_LOGD("mSprdYuvCallBack:%d, mSprd3dCalibrationEnabled:%d, "
                     "mTakePictureMode:%d",
                     mSprdYuvCallBack, mSprd3dCalibrationEnabled,
                     mTakePictureMode);
            if ((mSprdYuvCallBack || mSprd3dCalibrationEnabled) &&
                mTakePictureMode == SNAPSHOT_ZSL_MODE) {
                transitionState(SPRD_WAITING_RAW, SPRD_IDLE, STATE_CAPTURE);
                mSprdYuvCallBack = false;
            } else {
                transitionState(SPRD_WAITING_RAW, SPRD_WAITING_JPEG,
                                STATE_CAPTURE);
            }
            /**modified for 3d calibration return yuv buffer finished end */
        }
        break;

    case CAMERA_EXIT_CB_FAILED:
        HAL_LOGE("SprdCamera3OEMIf::camera_cb: @CAMERA_EXIT_CB_FAILURE(%ld) in "
                 "state %s.",
                 (cmr_uint)parm4, getCameraStateStr(getCaptureState()));
        transitionState(getCaptureState(), SPRD_ERROR, STATE_CAPTURE);
        receiveCameraExitError();
        // camera_set_capture_trace(0);
        break;

    case CAMERA_EVT_CB_ZSL_FRM:
        HAL_LOGV("CAMERA_EVT_CB_HAL2_ZSL_NEW_FRM");
        break;
    case CAMERA_EVT_CB_RETURN_ZSL_BUF:
        if (isPreviewing() && iSZslMode() &&
            (mSprd3dCalibrationEnabled || mSprdYuvCallBack)) {
            cmr_u32 buf_id = 0;
            struct camera_frame_type *zsl_frame = NULL;
            zsl_frame = (struct camera_frame_type *)parm4;
            if (mIsUnpopped) {
                mIsUnpopped = false;
            }
            if (zsl_frame->fd <= 0) {
                HAL_LOGW("zsl lost a buffer, this should not happen");
                break;
            }
            HAL_LOGD("zsl_frame->fd=0x%x", zsl_frame->fd);
            buf_id = getZslBufferIDForFd(zsl_frame->fd);
            if (buf_id != 0xFFFFFFFF) {
                mHalOem->ops->camera_set_zsl_buffer(
                    mCameraHandle, mZslHeapArray[buf_id]->phys_addr,
                    (cmr_uint)mZslHeapArray[buf_id]->data,
                    mZslHeapArray[buf_id]->fd);
            }
        }
        break;
    default:
        HAL_LOGE("unkown cb = %d", cb);
        transitionState(getCaptureState(), SPRD_ERROR, STATE_CAPTURE);
        receiveTakePictureError();
        // camera_set_capture_trace(0);
        break;
    }

    HAL_LOGD("X, state = %s", getCameraStateStr(getCaptureState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleCancelPicture(enum camera_cb_type cb,
                                           void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    HAL_LOGD("E: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCaptureState()));

    if (SPRD_INTERNAL_CAPTURE_STOPPING != getCaptureState()) {
        HAL_LOGD("HandleCancelPicture don't handle");
        return;
    }
    setCameraState(SPRD_IDLE, STATE_CAPTURE);

    HAL_LOGD("X, state = %s", getCameraStateStr(getCaptureState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleEncode(enum camera_cb_type cb, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    HAL_LOGD("E: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCaptureState()));

    switch (cb) {
    case CAMERA_RSP_CB_SUCCESS:
        break;

    case CAMERA_EXIT_CB_DONE:
        HAL_LOGV("CAMERA_EXIT_CB_DONE");
        if (!WaitForCaptureJpegState()) {
            // HAL_LOGE("state error");
            break;
        }

        if ((SPRD_WAITING_JPEG == getCaptureState())) {
            Sprd_camera_state tmpCapState = SPRD_WAITING_JPEG;

            if (checkPreviewStateForCapture()) {
                receiveJpegPicture((struct camera_frame_type *)parm4);
            } else {
                HAL_LOGE("HandleEncode: drop current jpgPicture");
            }

            tmpCapState = getCaptureState();
            if ((SPRD_WAITING_JPEG == tmpCapState) ||
                (SPRD_INTERNAL_CAPTURE_STOPPING == tmpCapState)) {
                if (((struct camera_frame_type *)parm4)->jpeg_param.need_free) {
                    setCameraState(SPRD_IDLE, STATE_CAPTURE);
                } else if (tmpCapState != SPRD_INTERNAL_CAPTURE_STOPPING) {
                    setCameraState(SPRD_INTERNAL_RAW_REQUESTED, STATE_CAPTURE);
                }
            } else if (SPRD_IDLE != tmpCapState) {
                HAL_LOGE(
                    "HandleEncode: CAMERA_EXIT_CB_DONE error cap status, %s",
                    getCameraStateStr(tmpCapState));
                transitionState(tmpCapState, SPRD_ERROR, STATE_CAPTURE);
            }
        } else if (mTakePictureMode != SNAPSHOT_NO_ZSL_MODE) {
            receiveJpegPicture((struct camera_frame_type *)parm4);
            mZSLTakePicture = false;
        }
        break;

    case CAMERA_EXIT_CB_FAILED:
        HAL_LOGD("CAMERA_EXIT_CB_FAILED");
        transitionState(getCaptureState(), SPRD_ERROR, STATE_CAPTURE);
        receiveCameraExitError();
        break;

    case CAMERA_EVT_CB_RETURN_ZSL_BUF:
        if (isPreviewing() && iSZslMode()) {
            cmr_u32 buf_id;
            struct camera_frame_type *zsl_frame;
            zsl_frame = (struct camera_frame_type *)parm4;
            if (zsl_frame->fd <= 0) {
                HAL_LOGW("zsl lost a buffer, this should not happen");
                goto handle_encode_exit;
            }
            HAL_LOGD("zsl_frame->fd=0x%x", zsl_frame->fd);
            buf_id = getZslBufferIDForFd(zsl_frame->fd);
            if (buf_id != 0xFFFFFFFF) {
                mHalOem->ops->camera_set_zsl_buffer(
                    mCameraHandle, mZslHeapArray[buf_id]->phys_addr,
                    (cmr_uint)mZslHeapArray[buf_id]->data,
                    mZslHeapArray[buf_id]->fd);
            }
        }
        break;

    default:
        HAL_LOGD("unkown error = %d", cb);
        transitionState(getCaptureState(), SPRD_ERROR, STATE_CAPTURE);
        receiveJpegPictureError();
        break;
    }

handle_encode_exit:
    HAL_LOGD("X, state = %s", getCameraStateStr(getCaptureState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleFocus(enum camera_cb_type cb, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    SprdCamera3MetadataChannel *channel =
        (SprdCamera3MetadataChannel *)mMetaData;
    int64_t timeStamp = 0;
    timeStamp = systemTime();

    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);
    HAL_LOGD("E: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getPreviewState()));

    setCameraState(SPRD_IDLE, STATE_FOCUS);

    switch (cb) {
    case CAMERA_RSP_CB_SUCCESS:
        HAL_LOGV("camera cb: autofocus has started.");
        break;

    case CAMERA_EXIT_CB_DONE:
        HAL_LOGV("camera cb: autofocus succeeded.");
        {
            controlInfo.af_state = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
            mSetting->setAfCONTROLTag(&controlInfo);
            // channel->channelCbRoutine(0, timeStamp,
            // CAMERA_STREAM_TYPE_DEFAULT);
            if (controlInfo.af_mode ==
                ANDROID_CONTROL_AF_MODE_AUTO) // reset autofocus only in TouchAF
                                              // process
            {
                mIsAutoFocus = false;
            }
        }
        break;

    case CAMERA_EXIT_CB_ABORT:
    case CAMERA_EXIT_CB_FAILED: {
        controlInfo.af_state = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
        mSetting->setAfCONTROLTag(&controlInfo);
        // channel->channelCbRoutine(0, timeStamp, CAMERA_STREAM_TYPE_DEFAULT);
        if (controlInfo.af_mode ==
            ANDROID_CONTROL_AF_MODE_AUTO) // reset autofocus only in TouchAF
                                          // process
        {
            mIsAutoFocus = false;
        }

    } break;

    case CAMERA_EVT_CB_FOCUS_MOVE:
        HAL_LOGV("camera cb: autofocus focus moving  %p autofocus=%d", parm4,
                 mIsAutoFocus);
        if (!mIsAutoFocus) {
            if (parm4) {
                controlInfo.af_state = ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN;
            } else {
                controlInfo.af_state = ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED;
            }
            mSetting->setAfCONTROLTag(&controlInfo);
        }
        break;

    default:
        HAL_LOGE("camera cb: unknown cb %d for CAMERA_FUNC_START_FOCUS!", cb);
        {
            controlInfo.af_state = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
            mSetting->setAfCONTROLTag(&controlInfo);

            // channel->channelCbRoutine(0, timeStamp,
            // CAMERA_STREAM_TYPE_DEFAULT);
        }
        break;
    }

    HAL_LOGD("out, state = %s", getCameraStateStr(getFocusState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleAutoExposure(enum camera_cb_type cb, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);
    HAL_LOGD("E: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getPreviewState()));

    switch (cb) {
    case CAMERA_EVT_CB_AE_STAB_NOTIFY:
        if (controlInfo.ae_state != ANDROID_CONTROL_AE_STATE_LOCKED) {
            controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_CONVERGED;
            controlInfo.awb_state = ANDROID_CONTROL_AWB_STATE_CONVERGED;
            mSetting->setAeCONTROLTag(&controlInfo);
            mSetting->setAwbCONTROLTag(&controlInfo);
        }
        HAL_LOGI("CAMERA_EVT_CB_AE_STAB_NOTIFY, ae_state = %d",
                 controlInfo.ae_state);
        break;
    case CAMERA_EVT_CB_AE_LOCK_NOTIFY:
        // controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_LOCKED;
        // mSetting->setAeCONTROLTag(&controlInfo);
        HAL_LOGI("CAMERA_EVT_CB_AE_LOCK_NOTIFY, ae_state = %d",
                 controlInfo.ae_state);
        break;
    case CAMERA_EVT_CB_AE_UNLOCK_NOTIFY:
        // controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_CONVERGED;
        // mSetting->setAeCONTROLTag(&controlInfo);
        HAL_LOGI("CAMERA_EVT_CB_AE_UNLOCK_NOTIFY, ae_state = %d",
                 controlInfo.ae_state);
        break;
    case CAMERA_EVT_CB_AE_FLASH_FIRED:
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        sprddefInfo.is_takepicture_with_flash = *(uint8_t *)parm4;
        mSetting->setSPRDDEFTag(sprddefInfo);
        HAL_LOGI("is_takepicture_with_flash = %d",
                 sprddefInfo.is_takepicture_with_flash);
        break;
    default:
        break;
    }

    HAL_LOGD("out");
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleStartCamera(enum camera_cb_type cb, void *parm4) {
    HAL_LOGD("in: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCameraState()));

    transitionState(SPRD_INIT, SPRD_IDLE, STATE_CAMERA);

    HAL_LOGD("out, state = %s", getCameraStateStr(getCameraState()));
}

void SprdCamera3OEMIf::HandleStopCamera(enum camera_cb_type cb, void *parm4) {
    HAL_LOGD("in: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCameraState()));

    transitionState(SPRD_INTERNAL_STOPPING, SPRD_INIT, STATE_CAMERA);

    HAL_LOGD("out, state = %s", getCameraStateStr(getCameraState()));
}

void SprdCamera3OEMIf::camera_cb(enum camera_cb_type cb,
                                 const void *client_data,
                                 enum camera_func_type func, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)client_data;
    HAL_LOGV("E");
    HAL_LOGV("cb = %d func = %d parm4 = 0x%lx", cb, func, (cmr_uint)parm4);
    switch (func) {
    case CAMERA_FUNC_START_PREVIEW:
        obj->HandleStartPreview(cb, parm4);
        break;

    case CAMERA_FUNC_STOP_PREVIEW:
        obj->HandleStopPreview(cb, parm4);
        break;

    case CAMERA_FUNC_RELEASE_PICTURE:
        obj->HandleCancelPicture(cb, parm4);
        break;

    case CAMERA_FUNC_TAKE_PICTURE:
        obj->HandleTakePicture(cb, parm4);
        break;

    case CAMERA_FUNC_ENCODE_PICTURE:
        obj->HandleEncode(cb, parm4);
        break;

    case CAMERA_FUNC_START_FOCUS:
        obj->HandleFocus(cb, parm4);
        break;

    case CAMERA_FUNC_AE_STATE_CALLBACK:
        obj->HandleAutoExposure(cb, parm4);
        break;

    case CAMERA_FUNC_START:
        obj->HandleStartCamera(cb, parm4);
        break;

    case CAMERA_FUNC_STOP:
        obj->HandleStopCamera(cb, parm4);
        break;

    default:
        HAL_LOGE("Unknown camera-callback status %d", cb);
        break;
    }

    ATRACE_END();
    HAL_LOGV("X");
}

int SprdCamera3OEMIf::flush_buffer(camera_flush_mem_type_e type, int index,
                                   void *v_addr, void *p_addr, int size) {
    ATRACE_CALL();

    int ret = 0;
    sprd_camera_memory_t *pmem = NULL;
    MemIon *pHeapIon = NULL;
    uint32_t i;

    switch (type) {
    case CAMERA_FLUSH_RAW_HEAP:
        pmem = mRawHeap;
        break;

    case CAMERA_FLUSH_RAW_HEAP_ALL:
        for (i = 0; i < mSubRawHeapNum; i++) {

            pmem = mSubRawHeapArray[i];
            v_addr = (void *)mSubRawHeapArray[i]->data;
            p_addr = (void *)mSubRawHeapArray[i]->phys_addr;
            size = (int)mSubRawHeapArray[i]->phys_size;

            if (pmem) {
                pHeapIon = pmem->ion_heap;
            }
            if (pHeapIon) {
                HAL_LOGD("index=%d,vaddr=%p, paddr=%p,size=0x%x", i, v_addr,
                         p_addr, size);
                ret = pHeapIon->flush_ion_buffer(v_addr, p_addr, size);
                if (ret) {
                    HAL_LOGW("abnormal ret=%d", ret);
                    HAL_LOGW("index=%d,vaddr=%p, paddr=%p", i, v_addr, p_addr);
                }
            }
        }
        break;

    case CAMERA_FLUSH_PREVIEW_HEAP:
        break;

    default:
        break;
    }

    if (pmem) {
        pHeapIon = pmem->ion_heap;
    }

    if (pHeapIon) {
        HAL_LOGV("index=%d,vaddr=%p, paddr=%p,size=0x%x", index, v_addr, p_addr,
                 size);
        ret = pHeapIon->flush_ion_buffer(v_addr, p_addr, size);
        if (ret) {
            HAL_LOGW("abnormal ret=%d", ret);
            HAL_LOGW("index=%d,vaddr=%p, paddr=%p", index, v_addr, p_addr);
        }
    }

    return ret;
}

int SprdCamera3OEMIf::openCamera() {
    ATRACE_CALL();

    int ret = NO_ERROR;

    GET_START_TIME;
    mSetting->getLargestPictureSize(mCameraId, &mLargestSensorWidth,
                                    &mLargestSensorHeight);
    mHalOem->ops->camera_pre_capture_set_buffer_size(
        mCameraId, mLargestSensorWidth, mLargestSensorHeight);

    if (!startCameraIfNecessary()) {
        ret = UNKNOWN_ERROR;
        HAL_LOGE("start failed");
        return ret;
    }

    ZSLMode_monitor_thread_init((void *)this);

#ifdef CONFIG_CAMERA_GYRO
    gyro_monitor_thread_init((void *)this);
#endif
    return ret;
}

int SprdCamera3OEMIf::handleCbData(hal3_trans_info_t &result_info,
                                   void *userdata) {
    cam_stream_type_t type = CAM_STREAM_TYPE_DEFAULT;
    SprdCamera3Channel *channel = (SprdCamera3Channel *)userdata;

    HAL_LOGD("S data=%p", channel);
    for (List<hal3_trans_info_t>::iterator i = mCbInfoList.begin();
         i != mCbInfoList.end(); i++) {
        if (channel == (SprdCamera3Channel *)i->user_data) {
            type = i->stream_type;
            HAL_LOGD("type=%d", type);
            break;
        }
    }
    // channel->channelCbRoutine(result_info.frame.index, result_info.timestamp,
    // NULL, type);
    HAL_LOGD("X");
    return 0;
}

int SprdCamera3OEMIf::setCameraConvertCropRegion(void) {
    float zoomWidth, zoomHeight, zoomRatio = 1.0f;
    float prevAspectRatio, capAspectRatio, videoAspectRatio;
    float sensorAspectRatio, outputAspectRatio;
    uint32_t sensorOrgW = 0, sensorOrgH = 0;
    SCALER_Tag scaleInfo;
    struct img_rect cropRegion;
    int ret = 0;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    mSetting->getSCALERTag(&scaleInfo);
    cropRegion.start_x = scaleInfo.crop_region[0];
    cropRegion.start_y = scaleInfo.crop_region[1];
    cropRegion.width = scaleInfo.crop_region[2];
    cropRegion.height = scaleInfo.crop_region[3];
    HAL_LOGD("crop start_x=%d start_y=%d width=%d height=%d",
             cropRegion.start_x, cropRegion.start_y, cropRegion.width,
             cropRegion.height);

#if 0
	if(mCameraId == 0) {
		sensorOrgW = BACK_SENSOR_ORIG_WIDTH;
		sensorOrgH = BACK_SENSOR_ORIG_HEIGHT;
	} else if(mCameraId == 1 || mCameraId == 3) {
		sensorOrgW = FRONT_SENSOR_ORIG_WIDTH;
		sensorOrgH = FRONT_SENSOR_ORIG_HEIGHT;
	} else if(mCameraId == 2) {
		sensorOrgW = BACK_EXT_SENSOR_ORIG_WIDTH;
		sensorOrgH = BACK_EXT_SENSOR_ORIG_HEIGHT;
	}
#else
    mSetting->getLargestPictureSize(mCameraId, &mLargestPictureWidth,
                                    &mLargestPictureHeight);
    sensorOrgW = mLargestPictureWidth;
    sensorOrgH = mLargestPictureHeight;
#endif

    sensorAspectRatio = static_cast<float>(sensorOrgW) / sensorOrgH;

    if (mPreviewWidth != 0 && mPreviewHeight != 0) {
        prevAspectRatio = static_cast<float>(mPreviewWidth) / mPreviewHeight;
    } else {
        prevAspectRatio = sensorAspectRatio;
    }

    if (mVideoWidth != 0 && mVideoHeight != 0) {
        videoAspectRatio = static_cast<float>(mVideoWidth) / mVideoHeight;
    } else {
        videoAspectRatio = prevAspectRatio;
    }

    if (mSetCapRatioFlag == true && mCaptureWidth != 0 && mCaptureHeight != 0) {
        capAspectRatio = static_cast<float>(mCaptureWidth) / mCaptureHeight;
    } else if (mRawWidth != 0 && mRawHeight != 0) {
        capAspectRatio = static_cast<float>(mRawWidth) / mRawHeight;
    } else if (mCaptureWidth != 0 && mCaptureHeight != 0) {
        capAspectRatio = static_cast<float>(mCaptureWidth) / mCaptureHeight;
    } else {
        capAspectRatio = prevAspectRatio;
    }

    if (cropRegion.width > 0 && cropRegion.height > 0) {
        zoomWidth = static_cast<float>(cropRegion.width);
        zoomHeight = static_cast<float>(cropRegion.height);
    } else {
        zoomWidth = static_cast<float>(sensorOrgW);
        zoomHeight = static_cast<float>(sensorOrgH);
    }

    if (mPreviewWidth != 0 && mPreviewHeight != 0) {
        if (prevAspectRatio >= sensorAspectRatio) {
            zoomRatio = static_cast<float>(sensorOrgW) / zoomWidth;
        } else {
            zoomRatio = static_cast<float>(sensorOrgH) / zoomHeight;
        }
    } else if (mCaptureWidth != 0 && mCaptureHeight != 0) {
        if (capAspectRatio >= sensorAspectRatio) {
            zoomRatio = static_cast<float>(sensorOrgW) / zoomWidth;
        } else {
            zoomRatio = static_cast<float>(sensorOrgH) / zoomHeight;
        }
    } else {
        zoomRatio = static_cast<float>(sensorOrgW) / zoomWidth;
    }

    if (zoomRatio < MIN_DIGITAL_ZOOM_RATIO)
        zoomRatio = MIN_DIGITAL_ZOOM_RATIO;
    if (zoomRatio > MAX_DIGITAL_ZOOM_RATIO)
        zoomRatio = MAX_DIGITAL_ZOOM_RATIO;

    mZoomInfo.mode = ZOOM_INFO;
    mZoomInfo.zoom_info.zoom_ratio = zoomRatio;
    mZoomInfo.zoom_info.prev_aspect_ratio = prevAspectRatio;
    mZoomInfo.zoom_info.video_aspect_ratio = videoAspectRatio;
    mZoomInfo.zoom_info.capture_aspect_ratio = capAspectRatio;
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ZOOM, (cmr_uint)&mZoomInfo);

    HAL_LOGD(
        "camId=%d, zoomRatio=%f, outputAspectRatio: prev=%f, video=%f, cap=%f",
        mCameraId, zoomRatio, prevAspectRatio, videoAspectRatio,
        capAspectRatio);

    return ret;
}

int SprdCamera3OEMIf::CameraConvertCropRegion(uint32_t sensorWidth,
                                              uint32_t sensorHeight,
                                              struct img_rect *cropRegion) {
    float minOutputRatio;
    float zoomWidth, zoomHeight, zoomRatio;
    uint32_t i = 0;
    int ret = 0;
    uint32_t sensorOrgW = 0, sensorOrgH = 0, endX = 0, endY = 0, startXbak = 0,
             startYbak = 0;
    cmr_uint SensorRotate = IMG_ANGLE_0;
#define ALIGN_ZOOM_CROP_BITS (~0x03)

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    HAL_LOGD("crop %d %d %d %d sens w/h %d %d.", cropRegion->start_x,
             cropRegion->start_y, cropRegion->width, cropRegion->height,
             sensorWidth, sensorHeight);
#if 0
	if(mCameraId == 0) {
		sensorOrgW = BACK_SENSOR_ORIG_WIDTH;
		sensorOrgH = BACK_SENSOR_ORIG_HEIGHT;
	} else if(mCameraId == 1||mCameraId == 3) {
		sensorOrgW = FRONT_SENSOR_ORIG_WIDTH;
		sensorOrgH = FRONT_SENSOR_ORIG_HEIGHT;
	} else if(mCameraId == 2) {
		sensorOrgW = BACK_EXT_SENSOR_ORIG_WIDTH;
		sensorOrgH = BACK_EXT_SENSOR_ORIG_HEIGHT;
	}
#else
    mSetting->getLargestPictureSize(mCameraId, &mLargestPictureWidth,
                                    &mLargestPictureHeight);
    sensorOrgW = mLargestPictureWidth;
    sensorOrgH = mLargestPictureHeight;
#endif

    if (sensorWidth == 0 || sensorHeight == 0 || cropRegion->width == 0 ||
        cropRegion->height == 0) {
        HAL_LOGW("cropRegion->height or cropRegion->width is 0");
        return 1;
    }
    SensorRotate = mHalOem->ops->camera_get_preview_rot_angle(mCameraHandle);
    if (sensorWidth == sensorOrgW && sensorHeight == sensorOrgH &&
        SensorRotate == IMG_ANGLE_0) {
        HAL_LOGE("dont' need to convert.");
        return 0;
    }
    /*
    zoomWidth = (float)cropRegion->width;
    zoomHeight = (float)cropRegion->height;
    //get dstRatio and zoomRatio frm framework
    minOutputRatio = zoomWidth / zoomHeight;
    if (minOutputRatio > ((float)sensorOrgW / (float)sensorOrgH)) {
        zoomRatio = (float)sensorOrgW / zoomWidth;
    } else {
        zoomRatio = (float)sensorOrgH / zoomHeight;
    }
    if(IsRotate) {
        minOutputRatio = 1 / minOutputRatio;
    }
    if (minOutputRatio > ((float)sensorWidth / (float)sensorHeight)) {
        zoomWidth = (float)sensorWidth / zoomRatio;
        zoomHeight = zoomWidth / minOutputRatio;
    } else {
        zoomHeight = (float)sensorHeight / zoomRatio;
        zoomWidth = zoomHeight * minOutputRatio;
    }
    cropRegion->start_x = ((uint32_t)(sensorWidth - zoomWidth) >> 1) &
    ALIGN_ZOOM_CROP_BITS;
    cropRegion->start_y = ((uint32_t)(sensorHeight - zoomHeight) >> 1) &
    ALIGN_ZOOM_CROP_BITS;
    cropRegion->width = ((uint32_t)zoomWidth) & ALIGN_ZOOM_CROP_BITS;
    cropRegion->height = ((uint32_t)zoomHeight) & ALIGN_ZOOM_CROP_BITS;*/

    zoomWidth = (float)sensorWidth / (float)sensorOrgW;
    zoomHeight = (float)sensorHeight / (float)sensorOrgH;
    endX = cropRegion->start_x + cropRegion->width;
    endY = cropRegion->start_y + cropRegion->height;
    cropRegion->start_x = (cmr_u32)((float)cropRegion->start_x * zoomWidth);
    cropRegion->start_y = (cmr_u32)((float)cropRegion->start_y * zoomHeight);
    endX = (cmr_u32)((float)endX * zoomWidth);
    endY = (cmr_u32)((float)endY * zoomHeight);
    cropRegion->width = endX - cropRegion->start_x;
    cropRegion->height = endY - cropRegion->start_y;

    switch (SensorRotate) {
    case IMG_ANGLE_90:
        startYbak = cropRegion->start_y;
        cropRegion->start_y = cropRegion->start_x;
        cropRegion->start_x = sensorHeight - endY;
        cropRegion->width = (sensorHeight - startYbak) - cropRegion->start_x;
        cropRegion->height = endX - cropRegion->start_y;
        break;
    case IMG_ANGLE_180:
        startYbak = cropRegion->start_y;
        startXbak = cropRegion->start_x;
        cropRegion->start_x = sensorHeight - endX;
        cropRegion->start_y = sensorWidth - endY;
        cropRegion->width = (sensorHeight - startXbak) - cropRegion->start_x;
        cropRegion->height = (sensorWidth - startYbak) - cropRegion->start_y;
        break;
    case IMG_ANGLE_270:
        startYbak = cropRegion->start_y;
        startXbak = cropRegion->start_x;
        cropRegion->start_x = cropRegion->start_y;
        cropRegion->start_y = sensorHeight - endX;
        cropRegion->width = endY - cropRegion->start_x;
        cropRegion->height = (sensorHeight - startXbak) - cropRegion->start_y;
        break;
    }
    HAL_LOGD("Crop calculated (x=%d,y=%d,w=%d,h=%d rot=%ld)",
             cropRegion->start_x, cropRegion->start_y, cropRegion->width,
             cropRegion->height, SensorRotate);
    return ret;
}

int SprdCamera3OEMIf::SetCameraParaTag(cmr_int cameraParaTag) {
    int ret = 0;
    CONTROL_Tag controlInfo;
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.sys.camera.raw.mode", value, "jpeg");

    HAL_LOGV("set camera para, tag is %ld", cameraParaTag);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    mSetting->getCONTROLTag(&controlInfo);
    switch (cameraParaTag) {
    case ANDROID_CONTROL_SCENE_MODE: {
        int8_t drvSceneMode = 0;
        mSetting->androidSceneModeToDrvMode(controlInfo.scene_mode,
                                            &drvSceneMode);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SCENE_MODE, drvSceneMode);
        if (CAMERA_SCENE_MODE_HDR == drvSceneMode) {
            mHDRPowerHint = 1;
        } else {
            mHDRPowerHint = 0;
        }
    } break;

    case ANDROID_CONTROL_EFFECT_MODE: {
        int8_t effectMode = 0;
        mSetting->androidEffectModeToDrvMode(controlInfo.effect_mode,
                                             &effectMode);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_EFFECT, effectMode);
    } break;

    case ANDROID_CONTROL_CAPTURE_INTENT:
        mCapIntent = controlInfo.capture_intent;
        break;

    case ANDROID_CONTROL_AWB_MODE: {
        int8_t drvAwbMode = 0;
        mSetting->androidAwbModeToDrvAwbMode(controlInfo.awb_mode, &drvAwbMode);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_WB, drvAwbMode);
    } break;

    case ANDROID_SCALER_CROP_REGION:
        // SET_PARM(mHalOem, CAMERA_PARM_ZOOM_RECT, cameraParaValue);
        break;

    case ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION:
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_EXPOSURE_COMPENSATION,
                 controlInfo.ae_exposure_compensation);
        break;
    case ANDROID_CONTROL_AF_REGIONS: {
        struct img_rect zoom1 = {0, 0, 0, 0};
        struct img_rect zoom = {0, 0, 0, 0};
        struct cmr_focus_param focus_para;
        if (mCameraState.preview_state == SPRD_PREVIEW_IN_PROGRESS) {
            zoom.start_x = controlInfo.af_regions[0];
            zoom.start_y = controlInfo.af_regions[1];
            zoom.width = controlInfo.af_regions[2];
            zoom.height = controlInfo.af_regions[3];
            mHalOem->ops->camera_get_sensor_trim(mCameraHandle, &zoom1);
            if ((0 == zoom.start_x && 0 == zoom.start_y && 0 == zoom.width &&
                 0 == zoom.height) ||
                !CameraConvertCropRegion(zoom1.width, zoom1.height, &zoom)) {
                focus_para.zone[0].start_x = zoom.start_x;
                focus_para.zone[0].start_y = zoom.start_y;
                focus_para.zone[0].width = zoom.width;
                focus_para.zone[0].height = zoom.height;
                focus_para.zone_cnt = 1;
                SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCUS_RECT,
                         (cmr_uint)&focus_para);
            }
        }
    } break;
    case ANDROID_SPRD_SENSOR_ORIENTATION: {
        SPRD_DEF_Tag sprddefInfo;

        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("orient=%d", sprddefInfo.sensor_orientation);
        if (1 == sprddefInfo.sensor_orientation) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ORIENTATION,
                     1);
        } else {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ORIENTATION,
                     0);
        }
    } break;
    case ANDROID_SPRD_SENSOR_ROTATION: {
        SPRD_DEF_Tag sprddefInfo;

        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("rot=%d", sprddefInfo.sensor_rotation);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ROTATION,
                 sprddefInfo.sensor_rotation);
    } break;
    case ANDROID_SPRD_UCAM_SKIN_LEVEL: {
        SPRD_DEF_Tag sprddefInfo;
        struct beauty_info fb_param;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        fb_param.blemishLevel = sprddefInfo.perfect_skin_level[0];
        fb_param.smoothLevel = sprddefInfo.perfect_skin_level[1];
        fb_param.skinColor = sprddefInfo.perfect_skin_level[2];
        fb_param.skinLevel = sprddefInfo.perfect_skin_level[3];
        fb_param.brightLevel = sprddefInfo.perfect_skin_level[4];
        fb_param.lipColor = sprddefInfo.perfect_skin_level[5];
        fb_param.lipLevel = sprddefInfo.perfect_skin_level[6];
        fb_param.slimLevel = sprddefInfo.perfect_skin_level[7];
        fb_param.largeLevel = sprddefInfo.perfect_skin_level[8];
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_PERFECT_SKIN_LEVEL, (cmr_uint)&(fb_param));
    } break;
    case ANDROID_SPRD_CONTROL_FRONT_CAMERA_MIRROR: {
        SPRD_DEF_Tag sprddefInfo;

        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("flip_on_level = %d", sprddefInfo.flip_on);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FLIP_ON,
                 sprddefInfo.flip_on);
    } break;

    case ANDROID_SPRD_EIS_ENABLED: {
        SPRD_DEF_Tag sprddefInfo;

        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("sprd_eis_enabled = %d", sprddefInfo.sprd_eis_enabled);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_EIS_ENABLED,
                 sprddefInfo.sprd_eis_enabled);
    } break;

    case ANDROID_CONTROL_AF_MODE: {
        int8_t AfMode = 0;
        mSetting->androidAfModeToDrvAfMode(controlInfo.af_mode, &AfMode);
        if (mCameraId == 0 && !mIsAutoFocus) {
            if (mRecordingMode &&
                CAMERA_FOCUS_MODE_CAF ==
                    AfMode) { /*dv mode but recording not start*/
                AfMode = CAMERA_FOCUS_MODE_CAF_VIDEO;
            }
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_MODE, AfMode);
        }
    } break;
    case ANDROID_CONTROL_AE_ANTIBANDING_MODE: {
        int8_t antibanding_mode = 0;
        mSetting->androidAntibandingModeToDrvAntibandingMode(
            controlInfo.antibanding_mode, &antibanding_mode);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ANTIBANDING,
                 antibanding_mode);
    } break;

    case ANDROID_SPRD_BRIGHTNESS: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_BRIGHTNESS,
                 (uint32_t)sprddefInfo.brightness);
    } break;

    case ANDROID_SPRD_CONTRAST: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CONTRAST,
                 (uint32_t)sprddefInfo.contrast);
    } break;

    case ANDROID_SPRD_SATURATION: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SATURATION,
                 (uint32_t)sprddefInfo.saturation);
    } break;

    case ANDROID_JPEG_ORIENTATION: {
        JPEG_Tag jpegInfo;
        mSetting->getJPEGTag(&jpegInfo);
        mJpegRotaSet = true;
#ifdef CONFIG_CAMERA_ROTATION_CAPTURE
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 1);
        if (!strcmp(value, "raw")) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 0);
        }
#else
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 0);
#endif
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ENCODE_ROTATION,
                 jpegInfo.orientation);
        HAL_LOGD("JPEG orientation = %d", jpegInfo.orientation);
    } break;

    case ANDROID_CONTROL_AE_MODE:
        if (mCameraId == 0 || mCameraId == 1) {
            int8_t drvAeMode;
            mSetting->androidAeModeToDrvAeMode(controlInfo.ae_mode, &drvAeMode);

            if (drvAeMode != CAMERA_FLASH_MODE_TORCH &&
                mFlashMode != CAMERA_FLASH_MODE_TORCH) {
                if (mFlashMode != drvAeMode) {
                    mFlashMode = drvAeMode;
                    HAL_LOGD("set flash mode capture_state:%d mFlashMode:%d",
                             mCameraState.capture_state, mFlashMode);
                    if (mCameraState.capture_state == SPRD_FLASH_IN_PROGRESS) {
                        break;
                    } else {
                        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FLASH,
                                 mFlashMode);
                    }
                }
            }
        }
        break;

    case ANDROID_CONTROL_AE_LOCK: {
        uint8_t ae_lock;
        ae_lock = controlInfo.ae_lock;
        HAL_LOGD("ANDROID_CONTROL_AE_LOCK, ae_lock = %d", ae_lock);
        // SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_AE_LOCK_UNLOCK,
        // ae_lock);
        if (ae_lock == 1) {
            controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_LOCKED;
            mSetting->setAeCONTROLTag(&controlInfo);
        } else {
            controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_CONVERGED;
            mSetting->setAeCONTROLTag(&controlInfo);
        }
    } break;

    case ANDROID_FLASH_MODE:
        if (mCameraId == 0 || mCameraId == 1) {
            int8_t flashMode;
            FLASH_Tag flashInfo;
            mSetting->getFLASHTag(&flashInfo);
            mSetting->androidFlashModeToDrvFlashMode(flashInfo.mode,
                                                     &flashMode);
            if (CAMERA_FLASH_MODE_TORCH == flashMode ||
                CAMERA_FLASH_MODE_TORCH == mFlashMode) {
                mFlashMode = flashMode;
                SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FLASH,
                         mFlashMode);
            }
        }
        break;

    case ANDROID_SPRD_CAPTURE_MODE: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM,
                 sprddefInfo.capture_mode);
        if (sprddefInfo.capture_mode > 1)
            BurstCapCnt = sprddefInfo.capture_mode;
    } break;

    case ANDROID_LENS_FOCAL_LENGTH:
        LENS_Tag lensInfo;
        mSetting->getLENSTag(&lensInfo);
        if (lensInfo.focal_length) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCAL_LENGTH,
                     (cmr_uint)(lensInfo.focal_length * 1000));
        }
        break;

    case ANDROID_JPEG_QUALITY:
        JPEG_Tag jpgInfo;
        mSetting->getJPEGTag(&jpgInfo);
        if (jpgInfo.quality) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_JPEG_QUALITY,
                     jpgInfo.quality);
        }
        break;

    case ANDROID_JPEG_THUMBNAIL_QUALITY:
        break;

    case ANDROID_JPEG_THUMBNAIL_SIZE:
        break;

    case ANDROID_SPRD_METERING_MODE: {
        SPRD_DEF_Tag sprddefInfo;
        struct cmr_ae_param ae_param;
        struct img_rect zoom1 = {0, 0, 0, 0};
        struct img_rect zoom = {0, 0, 0, 0};
        mSetting->getSPRDDEFTag(&sprddefInfo);
        zoom.start_x = sprddefInfo.am_regions[0];
        zoom.start_y = sprddefInfo.am_regions[1];
        zoom.width = sprddefInfo.am_regions[2];
        zoom.height = sprddefInfo.am_regions[3];
        mHalOem->ops->camera_get_sensor_trim(mCameraHandle, &zoom1);
        if (!CameraConvertCropRegion(zoom1.width, zoom1.height, &zoom)) {
            sprddefInfo.am_regions[0] = zoom.start_x;
            sprddefInfo.am_regions[1] = zoom.start_y;
            sprddefInfo.am_regions[2] = zoom.width;
            sprddefInfo.am_regions[3] = zoom.height;
            mSetting->setSPRDDEFTag(sprddefInfo);
        }

        mSetting->androidAmModeToDrvAwbMode(sprddefInfo.am_mode, &ae_param);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AUTO_EXPOSURE_MODE,
                 (cmr_uint)&ae_param);

    } break;

    case ANDROID_SPRD_ISO: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISO,
                 (cmr_uint)sprddefInfo.iso);
    } break;

    case ANDROID_CONTROL_AE_TARGET_FPS_RANGE:
        setCameraPreviewMode(mRecordingMode);
        break;

    case ANDROID_SPRD_ZSL_ENABLED: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("sprd_zsl_enabled=%d", sprddefInfo.sprd_zsl_enabled);
        if (sprddefInfo.sprd_zsl_enabled == 0 && mRecordingMode == false &&
            mSprdRefocusEnabled == false) {
            mSprdZslEnabled = false;
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_ZSL_ENABLED, 0);
        }
    } break;

    case ANDROID_SPRD_SLOW_MOTION: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("slow_motion=%d", sprddefInfo.slowmotion);
        if (sprddefInfo.slowmotion > 1) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SLOW_MOTION_FLAG, 1);
        } else {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SLOW_MOTION_FLAG, 0);
        }
    } break;
#ifdef CONFIG_CAMERA_PIPVIV_SUPPORT
    case ANDROID_SPRD_PIPVIV_ENABLED: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("sprd_pipviv_enabled=%d camera id %d",
                 sprddefInfo.sprd_pipviv_enabled, mCameraId);
        if (sprddefInfo.sprd_pipviv_enabled == 1) {
            mSprdPipVivEnabled = true;
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_PIPVIV_ENABLED,
                     1);
        } else {
            mSprdPipVivEnabled = false;
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_PIPVIV_ENABLED,
                     0);
        }
    } break;
#endif
#ifdef CONFIG_CAMERA_HIGHISO_SUPPORT
    case ANDROID_SPRD_HIGHISO_ENABLED: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        if (sprddefInfo.sprd_highiso_enabled == 1) {
            mSprdHighIsoEnabled = true;
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_HIGHISO_ENABLED,
                     1);
        } else {
            mSprdHighIsoEnabled = false;
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_HIGHISO_ENABLED,
                     0);
        }
    } break;
#endif
    case ANDROID_SPRD_CONTROL_REFOCUS_ENABLE: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        /*if(sprddefInfo.refocus_enable != 0) {
                mSprdRefocusEnabled = true;
        }else {
                mSprdRefocusEnabled = false;
        }*/
        HAL_LOGD("sprddefInfo.refocus_enable=%d mSprdRefocusEnabled %d",
                 sprddefInfo.refocus_enable, mSprdRefocusEnabled);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_REFOCUS_ENABLE,
                 sprddefInfo.refocus_enable);

    } break;
    case ANDROID_SPRD_SET_TOUCH_INFO: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        struct touch_coordinate touch_param;
        touch_param.touchX = sprddefInfo.touchxy[0];
        touch_param.touchY = sprddefInfo.touchxy[1];
        HAL_LOGD("ANDROID_SPRD_SET_TOUCH_INFO, touchX = %d, touchY %d",
                 touch_param.touchX, touch_param.touchY);

        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_TOUCH_XY,
                 (cmr_uint)&touch_param);

    } break;
    case ANDROID_SPRD_3DCALIBRATION_ENABLED: /**add for 3d calibration get max
                                                sensor size begin*/
    {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        mSprd3dCalibrationEnabled = sprddefInfo.sprd_3dcalibration_enabled;
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_3DCAL_ENABLE,
                 sprddefInfo.sprd_3dcalibration_enabled);
        HAL_LOGD("sprddefInfo.sprd_3dcalibration_enabled=%d "
                 "mSprd3dCalibrationEnabled %d",
                 sprddefInfo.sprd_3dcalibration_enabled,
                 mSprd3dCalibrationEnabled);
    } break; /**add for 3d calibration get max sensor size end*/
    case ANDROID_SPRD_BURSTMODE_ENABLED: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);

        char value[PROPERTY_VALUE_MAX];
        property_get("persist.sys.camera.raw.mode", value, "jpeg");

        if (!strcmp(value, "raw")) {
            mSprdBurstModeEnabled = 0;
        } else if (sprddefInfo.sprd_burstmode_enable == 1) {
            mSprdBurstModeEnabled = 1;
        } else {
            mSprdBurstModeEnabled = 0;
        }

        HAL_LOGD("mSprdBurstModeEnabled=%d", mSprdBurstModeEnabled);
    } break;
    case ANDROID_SPRD_FIXED_FPS_ENABLED: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        mFixedFpsEnabled = sprddefInfo.sprd_fixedfps_enabled;
    } break;
    case ANDROID_SPRD_FILTER_TYPE: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("sprddefInfo.sprd_filter_type: %d ", sprddefInfo.sprd_filter_type);
        SET_PARM(mHalOem, mCameraHandle,CAMERA_PARAM_FILTER_TYPE,
                 sprddefInfo.sprd_filter_type);
    } break;
    default:
        ret = BAD_VALUE;
        break;
    }
    return ret;
}

int SprdCamera3OEMIf::SetJpegGpsInfo(bool is_set_gps_location) {
    if (is_set_gps_location)
        jpeg_gps_location = true;

    return 0;
}

snapshot_mode_type_t SprdCamera3OEMIf::GetTakePictureMode() {
    return mTakePictureMode;
}

int SprdCamera3OEMIf::setCapturePara(camera_capture_mode_t cap_mode,
                                     uint32_t frame_number) {
    char value[PROPERTY_VALUE_MAX];
    char value2[PROPERTY_VALUE_MAX];
    property_get("persist.sys.camera.raw.mode", value, "jpeg");
    HAL_LOGD("cap_mode = %d", cap_mode);

    switch (cap_mode) {
    case CAMERA_CAPTURE_MODE_PREVIEW:
        mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
        mCaptureMode = CAMERA_NORMAL_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mTakePictureMode = SNAPSHOT_DEFAULT_MODE;
        mRecordingMode = false;
        mZslPreviewMode = false;
        break;
    case CAMERA_CAPTURE_MODE_VIDEO:
        mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
        // change it to CAMERA_ZSL_MODE when start video record
        mCaptureMode = CAMERA_NORMAL_MODE;
        mTakePictureMode = SNAPSHOT_DEFAULT_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DV;
        mRecordingMode = true;
        mZslPreviewMode = false;
        mVideoShotFlag = 0;
        break;
    case CAMERA_CAPTURE_MODE_ZSL_PREVIEW:
        mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
        mCaptureMode = CAMERA_ZSL_MODE;
        mTakePictureMode = SNAPSHOT_DEFAULT_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mRecordingMode = false;
        mZslPreviewMode = true;
        // setAePrecaptureSta(ANDROID_CONTROL_AE_STATE_CONVERGED);
        break;
    case CAMERA_CAPTURE_MODE_ONLY_SNAPSHOT:
        mTakePictureMode = SNAPSHOT_ONLY_MODE;
        mCaptureMode = CAMERA_NORMAL_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
        mRecordingMode = false;
        mPicCaptureCnt = 1;
        mZslPreviewMode = false;
        break;
    case CAMERA_CAPTURE_MODE_NON_ZSL_SNAPSHOT:
        mTakePictureMode = SNAPSHOT_NO_ZSL_MODE;
        mCaptureMode = CAMERA_NORMAL_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
        mRecordingMode = false;
        mPicCaptureCnt = 1;
        mZslPreviewMode = false;
        if (!strcmp(value, "raw")) {
            HAL_LOGD("enter isp tuning mode ");
            mCaptureMode = CAMERA_ISP_TUNING_MODE;
        } else if (!strcmp(value, "sim")) {
            HAL_LOGD("enter isp simulation mode ");
            mCaptureMode = CAMERA_ISP_SIMULATION_MODE;
        }

        break;
    case CAMERA_CAPTURE_MODE_CONTINUE_NON_ZSL_SNAPSHOT:
        if (mSprdRefocusEnabled == 1 && mSprdZslEnabled == 1) {
            mTakePictureMode = SNAPSHOT_ZSL_MODE;
            mCaptureMode = CAMERA_ZSL_MODE;
            mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DV;
            mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
            mRecordingMode = false;
            mPicCaptureCnt = 1;
            mZslPreviewMode = false;
        } else {
            mTakePictureMode = SNAPSHOT_NO_ZSL_MODE;
            mCaptureMode = CAMERA_NORMAL_MODE;
            mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
            mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
            mRecordingMode = false;
            mPicCaptureCnt = 100;
            mZslPreviewMode = false;
            if (!strcmp(value, "raw")) {
                HAL_LOGE("enter isp tuning mode ");
                mCaptureMode = CAMERA_ISP_TUNING_MODE;
            } else if (!strcmp(value, "sim")) {
                HAL_LOGE("enter isp simulation mode ");
                mCaptureMode = CAMERA_ISP_SIMULATION_MODE;
            }
        }

        break;
    case CAMERA_CAPTURE_MODE_ZSL_SNAPSHOT:
        mTakePictureMode = SNAPSHOT_ZSL_MODE;
        mCaptureMode = CAMERA_ZSL_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
        mRecordingMode = false;
        mPicCaptureCnt = 1;
        mZslPreviewMode = false;
        break;
    case CAMERA_CAPTURE_MODE_VIDEO_SNAPSHOT:
        if (mVideoSnapshotType != 1) {
            mTakePictureMode = SNAPSHOT_ZSL_MODE;
            mCaptureMode = CAMERA_ZSL_MODE;
            mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DV;
            mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
            mRecordingMode = true;
            mPicCaptureCnt = 1;
            mZslPreviewMode = false;
        } else {
            mTakePictureMode = SNAPSHOT_VIDEO_MODE;
            mCaptureMode = CAMERA_ZSL_MODE;
            mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DV;
            mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
            mRecordingMode = true;
            mPicCaptureCnt = 1;
            mZslPreviewMode = false;
            mVideoShotNum = frame_number;
            mVideoShotFlag = 1;
        }
        break;
    case CAMERA_CAPTURE_MODE_ISP_TUNING_TOOL:
        mTakePictureMode = SNAPSHOT_NO_ZSL_MODE;
        mCaptureMode = CAMERA_ISP_TUNING_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
        mRecordingMode = false;
        mPicCaptureCnt = 1;
        mZslPreviewMode = false;
        break;
    case CAMERA_CAPTURE_MODE_ISP_SIMULATION_TOOL:
        mTakePictureMode = SNAPSHOT_NO_ZSL_MODE;
        mCaptureMode = CAMERA_ISP_SIMULATION_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
        mRecordingMode = false;
        mPicCaptureCnt = 1;
        mZslPreviewMode = false;
        break;
    case CAMERA_CAPTURE_MODE_PREVIEW_SNAPSHOT:
        mTakePictureMode = SNAPSHOT_PREVIEW_MODE;
        mCaptureMode = CAMERA_ZSL_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
        mRecordingMode = false;
        mPicCaptureCnt = 1;
        mZslPreviewMode = false;
        break;
    case CAMERA_CAPTURE_MODE_SPRD_ZSL_SNAPSHOT:
        enablePowerHint();
        mMultiLayerPowerHint = 1;
        mMultiLayerPowerHintFlag = 1;

        mTakePictureMode = SNAPSHOT_ZSL_MODE;
        mCaptureMode = CAMERA_ZSL_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DV;
        mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
        mRecordingMode = false;
        mPicCaptureCnt = 1;
        mZslPreviewMode = false;
        break;
    case CAMERA_CAPTURE_MODE_SPRD_ZSL_PREVIEW:
        mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
        mCaptureMode = CAMERA_ZSL_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DV;
        mTakePictureMode = SNAPSHOT_ZSL_MODE;
        mRecordingMode = false;
        mZslPreviewMode = false;
        break;
    default:
        break;
    }

    return NO_ERROR;
}

int SprdCamera3OEMIf::timer_stop() {
    if (mPrvTimerID) {
        timer_delete(mPrvTimerID);
        mPrvTimerID = NULL;
    }

    return NO_ERROR;
}

int SprdCamera3OEMIf::timer_set(void *obj, int32_t delay_ms,
                                timer_handle_func handler) {
    int status;
    struct itimerspec ts;
    struct sigevent se;
    SprdCamera3OEMIf *dev = reinterpret_cast<SprdCamera3OEMIf *>(obj);

    if (!mPrvTimerID) {
        memset(&se, 0, sizeof(struct sigevent));
        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = dev;
        se.sigev_notify_function = handler;
        se.sigev_notify_attributes = NULL;

        status = timer_create(CLOCK_MONOTONIC, &se, &mPrvTimerID);
        if (status != 0) {
            HAL_LOGD("time create fail");
            return status;
        }
        HAL_LOGD("timer id=0x%lx ms=%d", (long)mPrvTimerID, delay_ms);
    }

    ts.it_value.tv_sec = delay_ms / 1000;
    ts.it_value.tv_nsec = (delay_ms - (ts.it_value.tv_sec * 1000)) * 1000000;
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    status = timer_settime(mPrvTimerID, 0, &ts, NULL);

    return status;
}

void SprdCamera3OEMIf::timer_hand(union sigval arg) {
    SprdCamera3Stream *pre_stream = NULL;
    uint32_t frm_num = 0;
    int ret = NO_ERROR;
    bool ispStart = false;
    SENSOR_Tag sensorInfo;
    SprdCamera3OEMIf *dev = reinterpret_cast<SprdCamera3OEMIf *>(arg.sival_ptr);
    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(dev->mRegularChan);
    HAL_LOGD("E proc stat=%d timer id=0x%lx get lock bef", dev->mIspToolStart,
             (long)dev->mPrvTimerID);

    timer_delete(dev->mPrvTimerID);
    dev->mPrvTimerID = NULL;
    {
        Mutex::Autolock l(&dev->mLock);
        ispStart = dev->mIspToolStart;
    }
    if (ispStart) {
        channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &pre_stream);
        if (pre_stream) { // preview stream
            ret = pre_stream->getQBuffFirstNum(&frm_num);
            if (ret == NO_ERROR) {
                dev->mSetting->getSENSORTag(&sensorInfo);
                channel->channelClearInvalidQBuff(frm_num, sensorInfo.timestamp,
                                                  CAMERA_STREAM_TYPE_PREVIEW);
                dev->timer_set(dev, ISP_TUNING_WAIT_MAX_TIME, timer_hand);
            }
        }
    }

    HAL_LOGD("X frm=%d", frm_num);
}

void SprdCamera3OEMIf::timer_hand_take(union sigval arg) {
    int ret = NO_ERROR;
    SprdCamera3OEMIf *dev = reinterpret_cast<SprdCamera3OEMIf *>(arg.sival_ptr);
    HAL_LOGD("E timer id=0x%lx", (cmr_int)dev->mPrvTimerID);

    timer_delete(dev->mPrvTimerID);
    dev->mPrvTimerID = NULL;
    dev->mCaptureMode = CAMERA_NORMAL_MODE;
    dev->takePicture();
}

int SprdCamera3OEMIf::Callback_VideoFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                         cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGD("mVideoHeapNum %d sum %d", mVideoHeapNum, sum);
    for (i = 0; i < mVideoHeapNum; i++) {
        if (NULL != mVideoHeapArray[i]) {
            freeCameraMem(mVideoHeapArray[i]);
        }
        mVideoHeapArray[i] = NULL;
    }

    mVideoHeapNum = 0;

    return 0;
}

int SprdCamera3OEMIf::Callback_VideoMalloc(cmr_u32 size, cmr_u32 sum,
                                           cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i = 0;
    SPRD_DEF_Tag sprddefInfo;
    cmr_u32 BufferCount = (cmr_u32)kVideoBufferCount;

    mSetting->getSPRDDEFTag(&sprddefInfo);
    if (sprddefInfo.slowmotion <= 1)
        BufferCount = 8;

    HAL_LOGD("size %d sum %d mVideoHeapNum %d", size, sum, mVideoHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (mVideoHeapNum >= (kVideoBufferCount + kVideoRotBufferCount + 1)) {
        HAL_LOGE("error mPreviewHeapNum %d", mVideoHeapNum);
        return BAD_VALUE;
    }

    if ((mVideoHeapNum + sum) >=
        (kVideoBufferCount + kVideoRotBufferCount + 1)) {
        HAL_LOGE("malloc is too more %d %d", mVideoHeapNum, sum);
        return BAD_VALUE;
    }

    if (sum > BufferCount) {
        mVideoHeapNum = BufferCount;
        phy_addr += BufferCount;
        vir_addr += BufferCount;
        fd += BufferCount;
        for (i = BufferCount; i < (cmr_u32)sum; i++) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("error memory is null.");
                goto mem_fail;
            }

            mVideoHeapArray[mVideoHeapNum] = memory;
            mVideoHeapNum++;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else {
        HAL_LOGD("Do not need malloc, malloced num %d,request num %d, request "
                 "size 0x%x",
                 mVideoHeapNum, sum, size);
        goto mem_fail;
    }

    return 0;

mem_fail:
    Callback_VideoFree(0, 0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_ZslFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                       cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);
    Mutex::Autolock zsllock(&mZslBufLock);

    HAL_LOGD("mZslHeapNum %d sum %d", mZslHeapNum, sum);
    for (i = 0; i < mZslHeapNum; i++) {
        if (NULL != mZslHeapArray[i]) {
            freeCameraMem(mZslHeapArray[i]);
        }
        mZslHeapArray[i] = NULL;
    }

    mZslHeapNum = 0;
    releaseZSLQueue();

    return 0;
}

int SprdCamera3OEMIf::Callback_ZslMalloc(cmr_u32 size, cmr_u32 sum,
                                         cmr_uint *phy_addr, cmr_uint *vir_addr,
                                         cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i = 0;

    SPRD_DEF_Tag sprddefInfo;
    cmr_u32 BufferCount = (cmr_u32)kZslBufferCount;

    mSetting->getSPRDDEFTag(&sprddefInfo);
    if (sprddefInfo.slowmotion <= 1)
        BufferCount = 8;

    HAL_LOGD("size %d sum %d mZslHeapNum %d, BufferCount %d", size, sum,
             mZslHeapNum, BufferCount);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (mZslHeapNum >= (kZslBufferCount + kZslRotBufferCount + 1)) {
        HAL_LOGE("error mPreviewHeapNum %d", mZslHeapNum);
        return BAD_VALUE;
    }

    if ((mZslHeapNum + sum) >= (kZslBufferCount + kZslRotBufferCount + 1)) {
        HAL_LOGE("malloc is too more %d %d", mZslHeapNum, sum);
        return BAD_VALUE;
    }

    if (mSprdZslEnabled == true) {
        releaseZSLQueue();
        for (i = 0; i < (cmr_u32)mZslNum; i++) {
            if (mZslHeapArray[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("error memory is null.");
                    goto mem_fail;
                }
                mZslHeapArray[i] = memory;
                mZslHeapNum++;
            }
            *phy_addr++ = (cmr_uint)mZslHeapArray[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mZslHeapArray[i]->data;
            *fd++ = mZslHeapArray[i]->fd;
        }
        return 0;
    }

    if (sum > BufferCount) {
        mZslHeapNum = BufferCount;
        phy_addr += BufferCount;
        vir_addr += BufferCount;
        fd += BufferCount;
        for (i = BufferCount; i < (cmr_u32)sum; i++) {
            memory = allocCameraMem(size, 1, true);

            if (NULL == memory) {
                HAL_LOGE("error memory is null.");
                goto mem_fail;
            }

            mZslHeapArray[mZslHeapNum] = memory;
            mZslHeapNum++;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else {
        HAL_LOGD("Do not need malloc, malloced num %d,request num %d, request "
                 "size 0x%x",
                 mZslHeapNum, sum, size);
        goto mem_fail;
    }

    return 0;

mem_fail:
    Callback_ZslFree(0, 0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_RefocusFree(cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGD("mRefocusHeapNum %d sum %d", mRefocusHeapNum, sum);

    for (i = 0; i < mRefocusHeapNum; i++) {
        if (NULL != mRefocusHeapArray[i]) {
            freeCameraMem(mRefocusHeapArray[i]);
        }
        mRefocusHeapArray[i] = NULL;
    }
    mRefocusHeapNum = 0;

    return 0;
}

int SprdCamera3OEMIf::Callback_RefocusMalloc(cmr_u32 size, cmr_u32 sum,
                                             cmr_uint *phy_addr,
                                             cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    HAL_LOGD("size %d sum %d mRefocusHeapNum %d", size, sum, mRefocusHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;

    if (mRefocusHeapNum >= (kRefocusBufferCount + 1)) {
        HAL_LOGE("error mRefocusHeapNum %d", mRefocusHeapNum);
        return BAD_VALUE;
    }

    if ((mRefocusHeapNum + sum) >= (kRefocusBufferCount + 1)) {
        HAL_LOGE("malloc is too more %d %d", mRefocusHeapNum, sum);
        return BAD_VALUE;
    }

    if (sum >= kRefocusBufferCount) {
        // mRefocusHeapNum = kRefocusBufferCount;
        // phy_addr += kRefocusBufferCount;
        // vir_addr += kRefocusBufferCount;
        for (i = mRefocusHeapNum; i < (cmr_int)sum; i++) {
            memory = allocCameraMem(size, 1, true);

            if (NULL == memory) {
                HAL_LOGE("error memory is null.");
                goto mem_fail;
            }

            mRefocusHeapArray[mRefocusHeapNum] = memory;
            mRefocusHeapNum++;
            *phy_addr++ = 0; //(cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            if (NULL != fd)
                *fd++ = (cmr_s32)memory->fd;
        }
    } else {
        HAL_LOGD("Do not need malloc, malloced num %d,request num %d, request "
                 "size 0x%x",
                 mRefocusHeapNum, sum, size);
        goto mem_fail;
    }

    return 0;

mem_fail:
    Callback_RefocusFree(0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_PdafRawFree(cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGD("mPdafRawHeapNum %d sum %d", mPdafRawHeapNum, sum);

    for (i = 0; i < mPdafRawHeapNum; i++) {
        if (NULL != mPdafRawHeapArray[i]) {
            freeCameraMem(mPdafRawHeapArray[i]);
        }
        mPdafRawHeapArray[i] = NULL;
    }
    mPdafRawHeapNum = 0;

    return 0;
}

int SprdCamera3OEMIf::Callback_PdafRawMalloc(cmr_u32 size, cmr_u32 sum,
                                             cmr_uint *phy_addr,
                                             cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    HAL_LOGD("size %d sum %d mPdafRawHeapNum %d", size, sum, mPdafRawHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;

    if (mPdafRawHeapNum >= (kPdafRawBufferCount + 1)) {
        HAL_LOGE("error mPdafRawHeapNum %d", mPdafRawHeapNum);
        return BAD_VALUE;
    }

    if ((mPdafRawHeapNum + sum) >= (kPdafRawBufferCount + 1)) {
        HAL_LOGE("malloc is too more %d %d", mPdafRawHeapNum, sum);
        return BAD_VALUE;
    }

    if (sum >= kPdafRawBufferCount) {
        // mPdafRawHeapNum = kPdafRawBufferCount;
        // phy_addr += kPdafRawBufferCount;
        // vir_addr += kPdafRawBufferCount;
        for (i = mPdafRawHeapNum; i < (cmr_int)sum; i++) {
            memory = allocCameraMem(size, 1, true);

            if (NULL == memory) {
                HAL_LOGE("error memory is null.");
                goto mem_fail;
            }

            mPdafRawHeapArray[mPdafRawHeapNum] = memory;
            mPdafRawHeapNum++;
            *phy_addr++ = 0; //(cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            if (NULL != fd)
                *fd++ = (cmr_s32)memory->fd;
        }
    } else {
        HAL_LOGD("Do not need malloc, malloced num %d,request num %d, request "
                 "size 0x%x",
                 mPdafRawHeapNum, sum, size);
        goto mem_fail;
    }

    return 0;

mem_fail:
    Callback_PdafRawFree(0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_PreviewFree(cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_s32 *fd,
                                           cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGI("mPreviewHeapNum %d sum %d", mPreviewHeapNum, sum);

    for (i = 0; i < mPreviewHeapNum; i++) {
        if (NULL != mPreviewHeapArray[i]) {
            freeCameraMem(mPreviewHeapArray[i]);
        }
        mPreviewHeapArray[i] = NULL;
    }
    mPreviewHeapNum = 0;

    return 0;
}

int SprdCamera3OEMIf::Callback_PreviewMalloc(cmr_u32 size, cmr_u32 sum,
                                             cmr_uint *phy_addr,
                                             cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i = 0;
    SPRD_DEF_Tag sprddefInfo;
    cmr_u32 BufferCount = (cmr_u32)kPreviewBufferCount;

    mSetting->getSPRDDEFTag(&sprddefInfo);
    if (sprddefInfo.slowmotion <= 1)
        BufferCount = 8;

    HAL_LOGD("size %d sum %d mPreviewHeapNum %d, BufferCount %d", size, sum,
             mPreviewHeapNum, BufferCount);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (mPreviewHeapNum >= (kPreviewBufferCount + kPreviewRotBufferCount + 1)) {
        HAL_LOGE("error mPreviewHeapNum %d", mPreviewHeapNum);
        return BAD_VALUE;
    }

    if ((mPreviewHeapNum + sum) >=
        (kPreviewBufferCount + kPreviewRotBufferCount + 1)) {
        HAL_LOGE("malloc is too more %d %d", mPreviewHeapNum, sum);
        return BAD_VALUE;
    }

    if (sum > BufferCount) {
        int start;
        mPreviewHeapNum = BufferCount;
        phy_addr += BufferCount;
        vir_addr += BufferCount;
        fd += BufferCount;

        for (i = BufferCount; i < (cmr_u32)sum; i++) {
            memory = allocCameraMem(size, 1, true);

            if (NULL == memory) {
                HAL_LOGE("error memory is null.");
                goto mem_fail;
            }

            mPreviewHeapArray[mPreviewHeapNum] = memory;
            mPreviewHeapNum++;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else {
        HAL_LOGD("Do not need malloc, malloced num %d,request num %d, request "
                 "size 0x%x",
                 mPreviewHeapNum, sum, size);
        goto mem_fail;
    }

    return 0;

mem_fail:
    Callback_PreviewFree(0, 0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_CaptureFree(cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_s32 *fd,
                                           cmr_u32 sum) {
    cmr_u32 i;
    HAL_LOGD("mSubRawHeapNum %d sum %d", mSubRawHeapNum, sum);
    Mutex::Autolock l(&mCapBufLock);

    for (i = 0; i < mSubRawHeapNum; i++) {
        if (NULL != mSubRawHeapArray[i]) {
            freeCameraMem(mSubRawHeapArray[i]);
        }
        mSubRawHeapArray[i] = NULL;
    }
    mSubRawHeapNum = 0;
    mSubRawHeapSize = 0;
    mCapBufIsAvail = 0;

    return 0;
}

int SprdCamera3OEMIf::Callback_CaptureMalloc(cmr_u32 size, cmr_u32 sum,
                                             cmr_uint *phy_addr,
                                             cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    HAL_LOGD("size %d sum %d mSubRawHeapNum %d", size, sum, mSubRawHeapNum);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

cap_malloc:
    mCapBufLock.lock();

    if (mSubRawHeapNum >= MAX_SUB_RAWHEAP_NUM) {
        HAL_LOGE("error mSubRawHeapNum %d", mSubRawHeapNum);
        return BAD_VALUE;
    }

    if ((mSubRawHeapNum + sum) >= MAX_SUB_RAWHEAP_NUM) {
        HAL_LOGE("malloc is too more %d %d", mSubRawHeapNum, sum);
        return BAD_VALUE;
    }

    if (0 == mSubRawHeapNum) {
        for (i = 0; i < (cmr_int)sum; i++) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("error memory is null.");
                goto mem_fail;
            }

            mSubRawHeapArray[mSubRawHeapNum] = memory;
            mSubRawHeapNum++;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
        mSubRawHeapSize = size;
    } else {
        if ((mSubRawHeapNum >= sum) && (mSubRawHeapSize >= size)) {
            HAL_LOGD("use pre-alloc cap mem");
            for (i = 0; i < (cmr_int)sum; i++) {
                *phy_addr++ = (cmr_uint)mSubRawHeapArray[i]->phys_addr;
                *vir_addr++ = (cmr_uint)mSubRawHeapArray[i]->data;
                *fd++ = mSubRawHeapArray[i]->fd;
            }
        } else {
            mCapBufLock.unlock();
            Callback_CaptureFree(0, 0, 0, 0);
            goto cap_malloc;
        }
    }

    mCapBufIsAvail = 1;
    mCapBufLock.unlock();
    return 0;

mem_fail:
    mCapBufLock.unlock();
    Callback_CaptureFree(0, 0, 0, 0);
    return -1;
}

int SprdCamera3OEMIf::Callback_CapturePathMalloc(cmr_u32 size, cmr_u32 sum,
                                                 cmr_uint *phy_addr,
                                                 cmr_uint *vir_addr,
                                                 cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    LOGI("Callback_CapturePathMalloc: size %d sum %d mPathRawHeapNum %d "
         "mPathRawHeapSize %d",
         size, sum, mPathRawHeapNum, mPathRawHeapSize);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (mPathRawHeapNum >= MAX_SUB_RAWHEAP_NUM) {
        LOGE("Callback_CapturePathMalloc: error mPathRawHeapNum %d",
             mPathRawHeapNum);
        return -1;
    }
    if ((mPathRawHeapNum + sum) >= MAX_SUB_RAWHEAP_NUM) {
        LOGE("Callback_CapturePathMalloc: malloc is too more %d %d",
             mPathRawHeapNum, sum);
        return -1;
    }
    if (0 == mPathRawHeapNum) {
        mPathRawHeapSize = size;
        for (i = 0; i < (cmr_int)sum; i++) {
            memory = allocCameraMem(size, 1, true);

            if (NULL == memory) {
                LOGE("Callback_CapturePathMalloc: error memory is null.");
                goto mem_fail;
            }

            mPathRawHeapArray[mPathRawHeapNum] = memory;
            mPathRawHeapNum++;

            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else {
        if ((mPathRawHeapNum >= sum) && (mPathRawHeapSize >= size)) {
            LOGI("Callback_CapturePathMalloc :test");
            for (i = 0; i < (cmr_int)sum; i++) {
                *phy_addr++ = (cmr_uint)mPathRawHeapArray[i]->phys_addr;
                *vir_addr++ = (cmr_uint)mPathRawHeapArray[i]->data;
                *fd++ = mPathRawHeapArray[i]->fd;
            }
        } else {
            LOGE("failed to malloc memory, malloced num %d,request num %d, "
                 "size 0x%x, request size 0x%x",
                 mPathRawHeapNum, sum, mPathRawHeapSize, size);
            goto mem_fail;
        }
    }
    return 0;

mem_fail:
    Callback_CapturePathFree(0, 0, 0, 0);
    return -1;
}

int SprdCamera3OEMIf::Callback_CapturePathFree(cmr_uint *phy_addr,
                                               cmr_uint *vir_addr, cmr_s32 *fd,
                                               cmr_u32 sum) {
    cmr_u32 i;

    LOGI("Callback_CapturePathFree: mPathRawHeapNum %d sum %d", mPathRawHeapNum,
         sum);

    for (i = 0; i < mPathRawHeapNum; i++) {
        if (NULL != mPathRawHeapArray[i]) {
            freeCameraMem(mPathRawHeapArray[i]);
        }
        mPathRawHeapArray[i] = NULL;
    }
    mPathRawHeapNum = 0;
    mPathRawHeapSize = 0;

    return 0;
}

int SprdCamera3OEMIf::Callback_OtherFree(enum camera_mem_cb_type type,
                                         cmr_uint *phy_addr, cmr_uint *vir_addr,
                                         cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGD("sum %d", sum);

    if (type == CAMERA_PREVIEW_RESERVED) {
        for (i = 0; i < sum; i++) {
            if (NULL != mPreviewHeapReserved[i]) {
                freeCameraMem(mPreviewHeapReserved[i]);
                mPreviewHeapReserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_VIDEO_RESERVED) {
        for (i = 0; i < VIDEO_RESERVED_FRM_CNT; i++) {
            if (NULL != mVideoHeapReserved[i]) {
                freeCameraMem(mVideoHeapReserved[i]);
                mVideoHeapReserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_SNAPSHOT_ZSL_RESERVED) {
        for (i = 0; i < sum; i++) {
            if (NULL != mZslHeapReserved[i]) {
                freeCameraMem(mZslHeapReserved[i]);
                mZslHeapReserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_ISP_LSC) {
        if (NULL != mIspLscHeapReserved) {
            freeCameraMem(mIspLscHeapReserved);
        }
        mIspLscHeapReserved = NULL;
    }

    if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (NULL != mIspAntiFlickerHeapReserved) {
            freeCameraMem(mIspAntiFlickerHeapReserved);
        }
        mIspAntiFlickerHeapReserved = NULL;
    }

    if (type == CAMERA_PDAF_RAW_RESERVED) {
        if (NULL != mPdafRawHeapReserved) {
            freeCameraMem(mPdafRawHeapReserved);
        }
        mPdafRawHeapReserved = NULL;
    }

    if (type == CAMERA_ISP_BINGING4AWB) {
        for (i = 0; i < kISPB4awbCount; i++) {
            if (NULL != mIspB4awbHeapReserved[i]) {
                freeCameraMem(mIspB4awbHeapReserved[i]);
            }
            mIspB4awbHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_FIRMWARE) {
        if (NULL != mIspFirmwareReserved && !(--mIspFirmwareReserved_cnt)) {
            freeCameraMem(mIspFirmwareReserved);
            mIspFirmwareReserved = NULL;
        }
    }

    if (type == CAMERA_SNAPSHOT_HIGHISO) {
        if (NULL != mHighIsoSnapshotHeapReserved) {
            freeCameraMem(mHighIsoSnapshotHeapReserved);
        }
        mHighIsoSnapshotHeapReserved = NULL;
    }

    if (type == CAMERA_ISP_PREVIEW_Y) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspPreviewYReserved[i]) {
                freeCameraMem(mIspPreviewYReserved[i]);
            }
            mIspPreviewYReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_PREVIEW_YUV) {
        if (NULL != mIspYUVReserved) {
            freeCameraMem(mIspYUVReserved);
        }
        mIspYUVReserved = NULL;
    }

    if (type == CAMERA_ISP_RAW_DATA) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspRawDataReserved[i]) {
                freeCameraMem(mIspRawDataReserved[i]);
            }
            mIspRawDataReserved[i] = NULL;
        }
    }

    return 0;
}

int SprdCamera3OEMIf::Callback_OtherMalloc(enum camera_mem_cb_type type,
                                           cmr_u32 size, cmr_u32 *sum_ptr,
                                           cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i;
    cmr_u32 mem_size;
    cmr_u32 mem_sum;
    int buffer_id;

    cmr_u32 sum = *sum_ptr;

    HAL_LOGD("size=%d, sum=%d, mem_type=%d", size, sum, type);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;
    if (type == CAMERA_PREVIEW_RESERVED) {
        for (i = 0; i < sum; i++) {
            if (mPreviewHeapReserved[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("memory is null.");
                    goto mem_fail;
                }
                mPreviewHeapReserved[i] = memory;
            }
            *phy_addr++ = (cmr_uint)mPreviewHeapReserved[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mPreviewHeapReserved[i]->data;
            *fd++ = mPreviewHeapReserved[i]->fd;
        }
    } else if (type == CAMERA_VIDEO_RESERVED) {
        for (i = 0; i < VIDEO_RESERVED_FRM_CNT; i++) {
            if (mVideoHeapReserved[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("memory is null.");
                    goto mem_fail;
                }
                mVideoHeapReserved[i] = memory;
            }
            *phy_addr++ = (cmr_uint)mVideoHeapReserved[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mVideoHeapReserved[i]->data;
            *fd++ = mVideoHeapReserved[i]->fd;
        }
    } else if (type == CAMERA_SNAPSHOT_ZSL_RESERVED) {
        for (i = 0; i < sum; i++) {
            if (mZslHeapReserved[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("memory is null.");
                    goto mem_fail;
                }
                mZslHeapReserved[i] = memory;
            }
            *phy_addr++ = (cmr_uint)mZslHeapReserved[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mZslHeapReserved[i]->data;
            *fd++ = mZslHeapReserved[i]->fd;
        }
    } else if (type == CAMERA_SENSOR_DATATYPE_MAP_RESERVED) {
        if (mDepthHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mDepthHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mDepthHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mDepthHeapReserved->data;
        *fd++ = mDepthHeapReserved->fd;
    } else if (type == CAMERA_PDAF_RAW_RESERVED) {
        if (mPdafRawHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mPdafRawHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mPdafRawHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mPdafRawHeapReserved->data;
        *fd++ = mPdafRawHeapReserved->fd;
    } else if (type == CAMERA_ISP_LSC) {
        if (mIspLscHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("memory is null");
                goto mem_fail;
            }
            mIspLscHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mIspLscHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mIspLscHeapReserved->data;
        *fd++ = mIspLscHeapReserved->fd;
    } else if (type == CAMERA_ISP_BINGING4AWB) {
        cmr_u64 *phy_addr_64 = (cmr_u64 *)phy_addr;
        cmr_u64 *vir_addr_64 = (cmr_u64 *)vir_addr;
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        for (i = 0; i < sum; i++) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspB4awbHeapReserved[i] = memory;
            *phy_addr_64++ = (cmr_u64)memory->phys_addr;
            *vir_addr_64++ = (cmr_u64)memory->data;
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
            *phy_addr++ = kaddr;
            *phy_addr = kaddr >> 32;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (mIspAntiFlickerHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspAntiFlickerHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mIspAntiFlickerHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mIspAntiFlickerHeapReserved->data;
        *fd++ = mIspAntiFlickerHeapReserved->fd;
    } else if (type == CAMERA_ISP_FIRMWARE) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        if (++mIspFirmwareReserved_cnt == 1) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspFirmwareReserved = memory;
        } else {
            memory = mIspFirmwareReserved;
        }
        if (memory->ion_heap)
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
        *phy_addr++ = kaddr;
        *phy_addr++ = kaddr >> 32;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
        *fd++ = memory->dev_fd;
    } else if (type == CAMERA_SNAPSHOT_HIGHISO) {
        if (mHighIsoSnapshotHeapReserved == NULL) {
            memory = allocReservedMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mHighIsoSnapshotHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mHighIsoSnapshotHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mHighIsoSnapshotHeapReserved->data;
        *fd++ = mHighIsoSnapshotHeapReserved->fd;
    } else if (type == CAMERA_ISP_PREVIEW_Y) {
        for (i = 0; i < sum; i++) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspPreviewYReserved[i] = memory;
            *phy_addr++ = 0;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_PREVIEW_YUV) {
        if (mIspYUVReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mIspYUVReserved = memory;
        }
        *phy_addr++ = 0;
        *vir_addr++ = (cmr_uint)mIspYUVReserved->data;
        *fd++ = mIspYUVReserved->fd;
    } else if (type == CAMERA_ISP_RAW_DATA) {
        cmr_u64 *kaddr = (cmr_u64 *)vir_addr;
        size_t ksize = 0;

        for (i = 0; i < sum; i++) {
            if (mIspRawDataReserved[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("memory is null.");
                    break;
                }
                mIspRawDataReserved[i] = memory;
            }
            //			*phy_addr++ = 0;
            *phy_addr++ = (cmr_uint)mIspRawDataReserved[i]->data;
            mIspRawDataReserved[i]->ion_heap->get_kaddr(kaddr, &ksize);
            kaddr++;
            *fd++ = mIspRawDataReserved[i]->fd;
            HAL_LOGD("isp raw data fd=0x%0x, vir_addr=%p",
                     mIspRawDataReserved[i]->fd, mIspRawDataReserved[i]->data);
        }
        *sum_ptr = i;
    }

    return 0;

mem_fail:
    Callback_OtherFree(type, 0, 0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_Free(enum camera_mem_cb_type type,
                                    cmr_uint *phy_addr, cmr_uint *vir_addr,
                                    cmr_s32 *fd, cmr_u32 sum,
                                    void *private_data) {
    SprdCamera3OEMIf *camera = (SprdCamera3OEMIf *)private_data;
    int ret = 0;
    HAL_LOGV("E");

    if (!private_data || !vir_addr || !fd) {
        HAL_LOGE("error param 0x%x 0x%lx 0x%lx", *fd, (cmr_uint)vir_addr,
                 (cmr_uint)private_data);
        return BAD_VALUE;
    }

    if (type >= CAMERA_MEM_CB_TYPE_MAX) {
        HAL_LOGE("mem type error %ld", (cmr_uint)type);
        return BAD_VALUE;
    }

    if (CAMERA_PREVIEW == type) {
        ret = camera->Callback_PreviewFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_SNAPSHOT == type) {
        // Performance optimization:move Callback_CaptureFree to closeCamera
        // function
        // ret = camera->Callback_CaptureFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_VIDEO == type) {
        ret = camera->Callback_VideoFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_SNAPSHOT_ZSL == type) {
        ret = camera->Callback_ZslFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_SENSOR_DATATYPE_MAP == type) {
        ret = camera->Callback_RefocusFree(phy_addr, vir_addr, sum);
    } else if (CAMERA_PDAF_RAW == type) {
        ret = camera->Callback_PdafRawFree(phy_addr, vir_addr, sum);
    } else if (CAMERA_SNAPSHOT_PATH == type) {
        ret = camera->Callback_CapturePathFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_PREVIEW_RESERVED == type ||
               CAMERA_VIDEO_RESERVED == type || CAMERA_ISP_FIRMWARE == type ||
               CAMERA_SNAPSHOT_ZSL_RESERVED == type ||
               CAMERA_SENSOR_DATATYPE_MAP_RESERVED == type ||
               CAMERA_PDAF_RAW_RESERVED == type || CAMERA_ISP_LSC == type ||
               CAMERA_ISP_BINGING4AWB == type ||
               CAMERA_SNAPSHOT_HIGHISO == type || CAMERA_ISP_RAW_DATA == type ||
               CAMERA_ISP_PREVIEW_Y == type || CAMERA_ISP_PREVIEW_YUV == type) {
        ret = camera->Callback_OtherFree(type, phy_addr, vir_addr, fd, sum);
    }

    HAL_LOGV("X");
    return ret;
}

int SprdCamera3OEMIf::Callback_Malloc(enum camera_mem_cb_type type,
                                      cmr_u32 *size_ptr, cmr_u32 *sum_ptr,
                                      cmr_uint *phy_addr, cmr_uint *vir_addr,
                                      cmr_s32 *fd, void *private_data) {
    SprdCamera3OEMIf *camera = (SprdCamera3OEMIf *)private_data;
    int ret = 0, i = 0;
    uint32_t size, sum;
    SprdCamera3RegularChannel *channel = NULL;
    HAL_LOGV("E");

    if (!private_data || !vir_addr || !fd || !size_ptr || !sum_ptr ||
        (0 == *size_ptr) || (0 == *sum_ptr)) {
        HAL_LOGE("param error 0x%x 0x%lx 0x%lx 0x%lx 0x%lx", *fd,
                 (cmr_uint)vir_addr, (cmr_uint)private_data,
                 (cmr_uint)*size_ptr, (cmr_uint)*sum_ptr);
        return BAD_VALUE;
    }

    size = *size_ptr;
    sum = *sum_ptr;

    if (type >= CAMERA_MEM_CB_TYPE_MAX) {
        HAL_LOGE("mem type error %ld", (cmr_uint)type);
        return BAD_VALUE;
    }

    if (CAMERA_PREVIEW == type) {
        ret = camera->Callback_PreviewMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SNAPSHOT == type) {
        ret = camera->Callback_CaptureMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_VIDEO == type) {
        ret = camera->Callback_VideoMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SNAPSHOT_ZSL == type) {
        ret = camera->Callback_ZslMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SENSOR_DATATYPE_MAP == type) {
        ret = camera->Callback_RefocusMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_PDAF_RAW == type) {
        ret = camera->Callback_PdafRawMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SNAPSHOT_PATH == type) {
        ret = camera->Callback_CapturePathMalloc(size, sum, phy_addr, vir_addr,
                                                 fd);
    } else if (CAMERA_PREVIEW_RESERVED == type ||
               CAMERA_VIDEO_RESERVED == type || CAMERA_ISP_FIRMWARE == type ||
               CAMERA_SNAPSHOT_ZSL_RESERVED == type ||
               CAMERA_SENSOR_DATATYPE_MAP_RESERVED == type ||
               CAMERA_PDAF_RAW_RESERVED == type || CAMERA_ISP_LSC == type ||
               CAMERA_ISP_BINGING4AWB == type ||
               CAMERA_SNAPSHOT_HIGHISO == type || CAMERA_ISP_RAW_DATA == type ||
               CAMERA_ISP_PREVIEW_Y == type || CAMERA_ISP_PREVIEW_YUV == type) {
        ret = camera->Callback_OtherMalloc(type, size, sum_ptr, phy_addr,
                                           vir_addr, fd);
    }

    HAL_LOGV("X");
    return ret;
}

int SprdCamera3OEMIf::SetChannelHandle(void *regular_chan, void *picture_chan) {
    mRegularChan = regular_chan;
    mPictureChan = picture_chan;
    return NO_ERROR;
}

int SprdCamera3OEMIf::SetDimensionPreview(cam_dimension_t preview_size) {
    if ((mPreviewWidth != preview_size.width) ||
        (mPreviewHeight != preview_size.height)) {
        mPreviewWidth = preview_size.width;
        mPreviewHeight = preview_size.height;
        mRestartFlag = true;
    }

    return NO_ERROR;
}

int SprdCamera3OEMIf::SetDimensionVideo(cam_dimension_t video_size) {
    if ((mVideoWidth != video_size.width) ||
        (mVideoHeight != video_size.height)) {
        mVideoWidth = video_size.width;
        mVideoHeight = video_size.height;
        mRestartFlag = true;
    }

    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

#ifdef CONFIG_CAMERA_VIDEOSNAPSHOT_USE_VIDEOBUFFER
    if (mVideoWidth > 0 && mVideoHeight > 0) {
        mVideoSnapshotType = 1;
    } else {
        mVideoSnapshotType = 0;
    }
#else
    mVideoSnapshotType = 0;
#endif
    return NO_ERROR;
}

int SprdCamera3OEMIf::SetDimensionRaw(cam_dimension_t raw_size) {
    if ((mRawWidth != raw_size.width) || (mRawHeight != raw_size.height)) {
        mRawWidth = raw_size.width;
        mRawHeight = raw_size.height;
        mRestartFlag = true;
    }

    return NO_ERROR;
}

int SprdCamera3OEMIf::SetDimensionCapture(cam_dimension_t capture_size) {
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.sys.camera.raw.mode", value, "jpeg");
    struct img_rect trim = {0, 0, 0, 0};
    struct sensor_mode_info mode_info[SENSOR_MODE_MAX];
    int i;
    int raw_width = 0;
    int raw_height = 0;

    HAL_LOGD("capture_size.width = %d, capture_size.height = %d",
             capture_size.width, capture_size.height);
    if ((mCaptureWidth != capture_size.width) ||
        (mCaptureHeight != capture_size.height)) {
        mCaptureWidth = capture_size.width;
        mCaptureHeight = capture_size.height;
        mRestartFlag = true;
    }

    if (!strcmp(value, "raw")) {
        HAL_LOGD("raw capture mode");

        if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
            HAL_LOGE("oem is null or oem ops is null");
            return UNKNOWN_ERROR;
        }
        mHalOem->ops->camera_get_sensor_info_for_raw(mCameraHandle, mode_info);
        if (mCameraId == 2) {
            for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
                HAL_LOGD("trim w=%d, h=%d", mode_info[i].trim_width,
                         mode_info[i].trim_height);
                if (mode_info[i].trim_width * mode_info[i].trim_height != 0 &&
                    mode_info[i].trim_width * mode_info[i].trim_height <=
                        mCaptureWidth * mCaptureHeight) {
                    if (raw_width * raw_height <=
                        mode_info[i].trim_width * mode_info[i].trim_height) {
                        raw_width = mode_info[i].trim_width;
                        raw_height = mode_info[i].trim_height;
                    }
                }
            }
            if (raw_width * raw_height != 0) {
                mCaptureWidth = raw_width;
                mCaptureHeight = raw_height;
            } else
                HAL_LOGW("There no raw setting for capture");
        } else {
            for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
                HAL_LOGD("trim w=%d, h=%d", mode_info[i].trim_width,
                         mode_info[i].trim_height);
                if (mode_info[i].trim_width * mode_info[i].trim_height >=
                    mCaptureWidth * mCaptureHeight) {
                    mCaptureWidth = mode_info[i].trim_width;
                    mCaptureHeight = mode_info[i].trim_height;
                    break;
                }
            }
        }
        HAL_LOGD("raw capture mode: mCaptureWidth=%d, mCaptureHeight=%d",
                 mCaptureWidth, mCaptureHeight);
    }

    return NO_ERROR;
}

int SprdCamera3OEMIf::PushPreviewbuff(buffer_handle_t *buff_handle) {
    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    cmr_uint addr_vir = 0, addr_phy = 0;
    cmr_s32 fd = 0;
    SprdCamera3Stream *stream = NULL;
    int ret = NO_ERROR;

    HAL_LOGV("E");

    if (channel && (getPreviewState() == SPRD_INTERNAL_PREVIEW_REQUESTED ||
                    getPreviewState() == SPRD_PREVIEW_IN_PROGRESS)) {
        channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &stream);

        if (stream) {
            ret = stream->getQBufForHandle(buff_handle, &addr_vir, &addr_phy,
                                           &fd);
            HAL_LOGV("addr_phy = 0x%lx, addr_vir = 0x%lx, fd = 0x%x, ret = %d",
                     addr_phy, addr_vir, fd, ret);
            if (ret == NO_ERROR) {
                if (addr_vir != 0) {
                    if (NULL == mCameraHandle || NULL == mHalOem ||
                        NULL == mHalOem->ops) {
                        HAL_LOGE("oem is null or oem ops is null");
                        return UNKNOWN_ERROR;
                    }
                    mHalOem->ops->camera_set_preview_buffer(
                        mCameraHandle, addr_phy, addr_vir, fd);
                } else
                    stream->getQBufNumForVir(addr_vir, &mDropPreviewFrameNum);
            }
        }
    }

    HAL_LOGV("X");

    return NO_ERROR;
}

int SprdCamera3OEMIf::PushVideobuff(buffer_handle_t *buff_handle) {
    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    cmr_uint addr_vir = 0, addr_phy = 0;
    cmr_s32 fd = 0;
    SprdCamera3Stream *stream = NULL;
    int ret = NO_ERROR;

    HAL_LOGV("E, previewState = %d", getPreviewState());

    if (channel && (getPreviewState() == SPRD_INTERNAL_PREVIEW_REQUESTED ||
                    getPreviewState() == SPRD_PREVIEW_IN_PROGRESS)) {
        channel->getStream(CAMERA_STREAM_TYPE_VIDEO, &stream);

        if (stream) {
            ret = stream->getQBufForHandle(buff_handle, &addr_vir, &addr_phy,
                                           &fd);
            HAL_LOGV("addr_phy = 0x%lx, addr_vir = 0x%lx, fd = 0x%x, ret = %d",
                     addr_phy, addr_vir, fd, ret);
            if (ret == NO_ERROR) {
                if (addr_vir != 0) {
                    if (NULL == mCameraHandle || NULL == mHalOem ||
                        NULL == mHalOem->ops) {
                        HAL_LOGE("oem is null or oem ops is null");
                        return UNKNOWN_ERROR;
                    }
                    mHalOem->ops->camera_set_video_buffer(
                        mCameraHandle, addr_phy, addr_vir, fd);
                } else
                    stream->getQBufNumForVir(addr_vir, &mDropVideoFrameNum);
            }
        }
    }

    HAL_LOGV("X");

    return NO_ERROR;
}

int SprdCamera3OEMIf::PushZslbuff(buffer_handle_t *buff_handle) {
    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    cmr_uint addr_vir = 0, addr_phy = 0;
    cmr_s32 fd = 0;
    SprdCamera3Stream *stream = NULL;
    int ret = NO_ERROR;

    HAL_LOGV("E, previewState = %d", getPreviewState());

    if (channel && (getPreviewState() == SPRD_INTERNAL_PREVIEW_REQUESTED ||
                    getPreviewState() == SPRD_PREVIEW_IN_PROGRESS)) {
        channel->getStream(CAMERA_STREAM_TYPE_CALLBACK, &stream);

        if (stream) {
            ret = stream->getQBufForHandle(buff_handle, &addr_vir, &addr_phy,
                                           &fd);
            HAL_LOGV("addr_phy = 0x%lx, addr_vir = 0x%lx, fd = 0x%x, ret = %d",
                     addr_phy, addr_vir, fd, ret);
            if (ret == NO_ERROR) {
                if (addr_vir != 0) {
                    if (NULL == mCameraHandle || NULL == mHalOem ||
                        NULL == mHalOem->ops) {
                        HAL_LOGE("oem is null or oem ops is null");
                        return UNKNOWN_ERROR;
                    }
                    mHalOem->ops->camera_set_zsl_buffer(mCameraHandle, addr_phy,
                                                        addr_vir, fd);
                } else
                    stream->getQBufNumForVir(addr_vir, &mDropZslFrameNum);
            }
        }
    }

    HAL_LOGV("X");

    return NO_ERROR;
}

int SprdCamera3OEMIf::PushFirstPreviewbuff() {
    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    cmr_uint addr_vir = 0, addr_phy = 0;
    cmr_s32 fd = 0;
    SprdCamera3Stream *stream = NULL;
    int ret = NO_ERROR;

    if (channel) {
        ret = channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &stream);

        if (stream) {
            ret = stream->getQBuffFirstVir(&addr_vir);
            stream->getQBuffFirstPhy(&addr_phy);
            stream->getQBuffFirstFd(&fd);
            HAL_LOGD("addr_phy = 0x%lx, addr_vir = 0x%lx, fd = 0x%x, ret = %d",
                     addr_phy, addr_vir, fd, ret);
            if (ret == NO_ERROR) {
                if (addr_vir != 0) {
                    if (NULL == mCameraHandle || NULL == mHalOem ||
                        NULL == mHalOem->ops) {
                        HAL_LOGE("oem is null or oem ops is null");
                        return UNKNOWN_ERROR;
                    }
                    mHalOem->ops->camera_set_preview_buffer(
                        mCameraHandle, addr_phy, addr_vir, fd);
                } else
                    stream->getQBuffFirstNum(&mDropPreviewFrameNum);
            }
        }
    }

    return ret;
}

int SprdCamera3OEMIf::PushFirstVideobuff() {
    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    cmr_uint addr_vir = 0, addr_phy = 0;
    cmr_s32 fd = 0;
    SprdCamera3Stream *stream = NULL;
    uint32_t frame_number = 0;
    int ret = NO_ERROR;

    if (channel) {
        channel->getStream(CAMERA_STREAM_TYPE_VIDEO, &stream);

        if (stream) {
            ret = stream->getQBuffFirstVir(&addr_vir);
            stream->getQBuffFirstPhy(&addr_phy);
            stream->getQBuffFirstFd(&fd);
            HAL_LOGD("addr_phy = 0x%lx, addr_vir = 0x%lx, fd = 0x%x, ret = %d",
                     addr_phy, addr_vir, fd, ret);
            if (ret == NO_ERROR) {
                if (addr_vir != 0) {
                    if (NULL == mCameraHandle || NULL == mHalOem ||
                        NULL == mHalOem->ops) {
                        HAL_LOGE("oem is null or oem ops is null");
                        return UNKNOWN_ERROR;
                    }
                    mHalOem->ops->camera_set_video_buffer(
                        mCameraHandle, addr_phy, addr_vir, fd);
                } else
                    stream->getQBuffFirstNum(&mDropVideoFrameNum);
            }
        }
    }

    return NO_ERROR;
}

int SprdCamera3OEMIf::PushFirstZslbuff() {
    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    cmr_uint addr_vir = 0, addr_phy = 0;
    cmr_s32 fd = 0;
    SprdCamera3Stream *stream = NULL;
    uint32_t frame_number = 0;
    int ret = NO_ERROR;

    if (channel) {
        channel->getStream(CAMERA_STREAM_TYPE_CALLBACK, &stream);

        if (stream) {
            ret = stream->getQBuffFirstVir(&addr_vir);
            stream->getQBuffFirstPhy(&addr_phy);
            stream->getQBuffFirstFd(&fd);
            HAL_LOGD("addr_phy = 0x%lx, addr_vir = 0x%lx, fd = 0x%x, ret = %d",
                     addr_phy, addr_vir, fd, ret);
            if (ret == NO_ERROR) {
                if (addr_vir != 0) {
                    if (NULL == mCameraHandle || NULL == mHalOem ||
                        NULL == mHalOem->ops) {
                        HAL_LOGE("oem is null or oem ops is null");
                        return UNKNOWN_ERROR;
                    }
                    mHalOem->ops->camera_set_zsl_buffer(mCameraHandle, addr_phy,
                                                        addr_vir, fd);
                } else
                    stream->getQBuffFirstNum(&mDropZslFrameNum);
            }
        }
    }

    return NO_ERROR;
}

int SprdCamera3OEMIf::PushVideoSnapShotbuff(int32_t frame_number,
                                            camera_stream_type_t type) {
    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    cmr_uint addr_vir = 0, addr_phy = 0;
    cmr_s32 fd = 0;
    SprdCamera3Stream *stream = NULL;
    int ret = NO_ERROR;

    HAL_LOGD("E");

    if (mVideoShotPushFlag == 0 && frame_number == mVideoShotNum)
        mVideoShotWait.wait(mPreviewCbLock);

    if (mVideoShotPushFlag) {
        channel->getStream(type, &stream);

        if (stream) {
            ret = stream->getQBufAddrForNum(frame_number, &addr_vir, &addr_phy,
                                            &fd);
            HAL_LOGD("addr_phy = 0x%lx, addr_vir = 0x%lx, fd = 0x%x, "
                     "frame_number = %d",
                     addr_phy, addr_vir, fd, frame_number);
            if (mCameraHandle != 0 && mHalOem != 0 && mHalOem->ops != 0 &&
                ret == NO_ERROR && addr_vir != 0)
                mHalOem->ops->camera_set_video_snapshot_buffer(
                    mCameraHandle, addr_phy, addr_vir, fd);

            mVideoShotPushFlag = 0;
            mVideoShotFlag = 0;
        }
    }

    HAL_LOGD("X");

    return NO_ERROR;
}

int SprdCamera3OEMIf::PushZslSnapShotbuff() {
    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    cmr_uint addr_vir = 0, addr_phy = 0, zsl_private = 0;
    int ret = NO_ERROR;

    HAL_LOGD("E");

    if (channel) {
        ret = channel->getZSLInputBuff(&addr_vir, &addr_phy, &zsl_private);
        if ((mCameraHandle != NULL) && (mHalOem != NULL) &&
            (mHalOem->ops != NULL) && (ret == NO_ERROR) && (addr_vir != 0))
            mHalOem->ops->camera_set_zsl_snapshot_buffer(
                mCameraHandle, addr_phy, addr_vir, zsl_private);
        channel->releaseZSLInputBuff();
    }

    HAL_LOGD("X");

    return NO_ERROR;
}

ZslBufferQueue SprdCamera3OEMIf::popZSLQueue() {
    Mutex::Autolock l(&mZslLock);

    List<ZslBufferQueue>::iterator frame;
    ZslBufferQueue ret;

    bzero(&ret, sizeof(ZslBufferQueue));
    if (mZSLQueue.size() == 0) {
        return ret;
    }
    frame = mZSLQueue.begin()++;
    ret = static_cast<ZslBufferQueue>(*frame);
    mZSLQueue.erase(frame);

    return ret;
}

ZslBufferQueue SprdCamera3OEMIf::popZSLQueue(uint64_t need_timestamp) {
    Mutex::Autolock l(&mZslLock);
    List<ZslBufferQueue>::iterator frame;
    ZslBufferQueue zsl_frame;
    bzero(&zsl_frame, sizeof(ZslBufferQueue));
    if (mZSLQueue.size() == 0) {
        HAL_LOGV("cameraid:%d,find invalid zsl Frame.mCameraId", mCameraId);
        return zsl_frame;
    }
    if (need_timestamp == 0) {
        for (frame = mZSLQueue.begin(); frame != mZSLQueue.end(); frame++) {
            if (frame->frame.isMatchFlag != 1) {
                zsl_frame = static_cast<ZslBufferQueue>(*frame);
                mZSLQueue.erase(frame);
                HAL_LOGV("pop zsl frame vddr=0x%lx", frame->frame.y_vir_addr);
                return zsl_frame;
            }
        }
        frame = mZSLQueue.begin();
        zsl_frame = static_cast<ZslBufferQueue>(*frame);
        mZSLQueue.erase(frame);
        HAL_LOGV("pop zsl end frame vddr=0x%lx", frame->frame.y_vir_addr);
        return zsl_frame;
    }

    // process match frame need
    if (mMultiCameraMatchZsl->match_frame1.heap_array == NULL &&
        mMultiCameraMatchZsl->match_frame3.heap_array == NULL) {
        frame = mZSLQueue.begin();
        zsl_frame = static_cast<ZslBufferQueue>(*frame);
        mZSLQueue.erase(frame);
        mIsUnpopped = true;
        HAL_LOGD("match_Frame NULL");
        return zsl_frame;
    }

    if (mCameraId == 1) {
        if (ns2ms(abs((int)((int64_t)mMultiCameraMatchZsl->match_frame1.frame
                                .timestamp -
                            (int64_t)need_timestamp))) > 1500) {
            HAL_LOGD("match timestmp is too long,no use it");
            for (frame = mZSLQueue.begin(); frame != mZSLQueue.end(); frame++) {
                if (frame->frame.isMatchFlag != 1) {
                    zsl_frame = static_cast<ZslBufferQueue>(*frame);
                    mZSLQueue.erase(frame);
                    bzero(&mMultiCameraMatchZsl->match_frame1,
                          sizeof(ZslBufferQueue));
                    HAL_LOGV("pop zsl frame vddr=0x%lx",
                             frame->frame.y_vir_addr);
                    mIsUnpopped = true;
                    return zsl_frame;
                }
            }
            frame = mZSLQueue.begin();
            zsl_frame = static_cast<ZslBufferQueue>(*frame);
            mZSLQueue.erase(frame);
            bzero(&mMultiCameraMatchZsl->match_frame1, sizeof(ZslBufferQueue));
            mIsUnpopped = true;
            HAL_LOGV("pop zsl end frame vddr=0x%lx", frame->frame.y_vir_addr);
            return zsl_frame;
        }
        mIsUnpopped = true;
        zsl_frame = mMultiCameraMatchZsl->match_frame1;
        bzero(&mMultiCameraMatchZsl->match_frame1, sizeof(ZslBufferQueue));
    }
    if (mCameraId == 3) {
        if (ns2ms(abs((int)((int64_t)mMultiCameraMatchZsl->match_frame3.frame
                                .timestamp -
                            (int64_t)need_timestamp))) > 1500) {
            HAL_LOGD("match timestmp is too long,no use it");
            for (frame = mZSLQueue.begin(); frame != mZSLQueue.end(); frame++) {
                if (frame->frame.isMatchFlag != 1) {
                    zsl_frame = static_cast<ZslBufferQueue>(*frame);
                    mZSLQueue.erase(frame);
                    mIsUnpopped = true;
                    HAL_LOGV("pop zsl frame vddr=0x%lx",
                             frame->frame.y_vir_addr);
                    bzero(&mMultiCameraMatchZsl->match_frame3,
                          sizeof(ZslBufferQueue));
                    return zsl_frame;
                }
            }
            frame = mZSLQueue.begin();
            zsl_frame = static_cast<ZslBufferQueue>(*frame);
            mZSLQueue.erase(frame);
            bzero(&mMultiCameraMatchZsl->match_frame3, sizeof(ZslBufferQueue));
            mIsUnpopped = true;
            HAL_LOGV("pop zsl end frame vddr=0x%lx", frame->frame.y_vir_addr);
            return zsl_frame;
        }
        mIsUnpopped = true;
        zsl_frame = mMultiCameraMatchZsl->match_frame3;
        bzero(&mMultiCameraMatchZsl->match_frame3, sizeof(ZslBufferQueue));
    }
    frame = mZSLQueue.begin();
    while (frame != mZSLQueue.end()) {
        if (frame->frame.timestamp == zsl_frame.frame.timestamp) {
            mZSLQueue.erase(frame);
            HAL_LOGD("erase match frame,mid=%d", mCameraId);
            break;
        }
        frame++;
    }
    if (frame == mZSLQueue.end()) {
        frame = mZSLQueue.begin();
        zsl_frame = static_cast<ZslBufferQueue>(*frame);
        mZSLQueue.erase(frame);
    }
    mIsUnpopped = true;
    return zsl_frame;
}

int SprdCamera3OEMIf::getZSLQueueFrameNum() {
    Mutex::Autolock l(&mZslLock);

    int ret = 0;
    ret = mZSLQueue.size();
    HAL_LOGV("%d", ret);
    return ret;
}

void SprdCamera3OEMIf::matchZSLQueue(ZslBufferQueue *frame) {
    List<ZslBufferQueue>::iterator itor1, itor2;
    List<ZslBufferQueue> *match_ZSLQueue;
    ZslBufferQueue *frame1 = NULL;
    ZslBufferQueue frame_queue;
    HAL_LOGV("E");
    if (mCameraId == 1) {
        match_ZSLQueue = mMultiCameraMatchZsl->cam3_ZSLQueue;
    }
    if (mCameraId == 3) {
        match_ZSLQueue = mMultiCameraMatchZsl->cam1_ZSLQueue;
    }
    if (match_ZSLQueue->empty()) {
        HAL_LOGD("camera %d,match_queue.cam3_ZSLQueue empty", mCameraId);
        return;
    } else {
        itor1 = match_ZSLQueue->begin();
        while (itor1 != match_ZSLQueue->end()) {
            int diff = (int64_t)frame->frame.timestamp -
                       (int64_t)itor1->frame.timestamp;
            if (abs(diff) < DUALCAM_TIME_DIFF) {
                itor2 = mMultiCameraMatchZsl->cam1_ZSLQueue->begin();
                while (itor2 != mMultiCameraMatchZsl->cam1_ZSLQueue->end()) {
                    if (itor2->frame.timestamp ==
                        mMultiCameraMatchZsl->match_frame1.frame.timestamp) {
                        itor2->frame.isMatchFlag = 0;
                        break;
                    }
                    itor2++;
                }
                itor2 = mMultiCameraMatchZsl->cam3_ZSLQueue->begin();
                while (itor2 != mMultiCameraMatchZsl->cam3_ZSLQueue->end()) {
                    if (itor2->frame.timestamp ==
                        mMultiCameraMatchZsl->match_frame3.frame.timestamp) {
                        itor2->frame.isMatchFlag = 0;
                        break;
                    }
                    itor2++;
                }
                frame->frame.isMatchFlag = 1;
                itor1->frame.isMatchFlag = 1;
                if (mCameraId == 1) {
                    mMultiCameraMatchZsl->match_frame1 = *frame;
                    mMultiCameraMatchZsl->match_frame3 =
                        static_cast<ZslBufferQueue>(*itor1);
                }
                if (mCameraId == 3) {
                    mMultiCameraMatchZsl->match_frame1 =
                        static_cast<ZslBufferQueue>(*itor1);
                    mMultiCameraMatchZsl->match_frame3 = *frame;
                }
                break;
            }
            itor1++;
        }
    }
}

void SprdCamera3OEMIf::pushZSLQueue(ZslBufferQueue *frame) {
    Mutex::Autolock l(&mZslLock);
    if (getMultiCameraMode() == MODE_3D_CAPTURE) {
        frame->frame.isMatchFlag = 0;
        if (!mSprdMultiYuvCallBack) {
            matchZSLQueue(frame);
        }
    }
    mZSLQueue.push_back(*frame);
}

void SprdCamera3OEMIf::releaseZSLQueue() {
    Mutex::Autolock l(&mZslLock);

    List<ZslBufferQueue>::iterator round;
    HAL_LOGD("para changed.size : %zu", mZSLQueue.size());
    while (mZSLQueue.size() > 0) {
        round = mZSLQueue.begin()++;
        mZSLQueue.erase(round);
    }
}

void SprdCamera3OEMIf::setZslBuffers() {
    cmr_uint i = 0;

    HAL_LOGD("mZslHeapNum %d", mZslHeapNum);

    releaseZSLQueue();
    for (i = 0; i < mZslHeapNum; i++) {
        mHalOem->ops->camera_set_zsl_buffer(
            mCameraHandle, mZslHeapArray[i]->phys_addr,
            (cmr_uint)mZslHeapArray[i]->data, mZslHeapArray[i]->fd);
    }
}

uint32_t SprdCamera3OEMIf::getZslBufferIDForFd(cmr_s32 fd) {
    uint32_t id = 0xFFFFFFFF;
    cmr_u32 i;
    for (i = 0; i < mZslHeapNum; i++) {
        if (0 != mZslHeapArray[i]->fd && mZslHeapArray[i]->fd == fd) {
            id = i;
            break;
        }
    }

    return id;
}

int SprdCamera3OEMIf::pushZslFrame(struct camera_frame_type *frame) {
    int ret = 0;
    ZslBufferQueue zsl_buffer_q;

    frame->buf_id = getZslBufferIDForFd(frame->fd);
    memset(&zsl_buffer_q, 0, sizeof(zsl_buffer_q));
    zsl_buffer_q.frame = *frame;
    zsl_buffer_q.heap_array = mZslHeapArray[frame->buf_id];
    pushZSLQueue(&zsl_buffer_q);
    return ret;
}

struct camera_frame_type SprdCamera3OEMIf::popZslFrame() {
    ZslBufferQueue zslFrame;
    struct camera_frame_type zsl_frame;

    bzero(&zslFrame, sizeof(zslFrame));
    bzero(&zsl_frame, sizeof(struct camera_frame_type));
    if (getMultiCameraMode() == MODE_REFOCUS ||
        getMultiCameraMode() == MODE_3D_CAPTURE) {
        zslFrame = popZSLQueue(mNeededTimestamp);
    } else {
        zslFrame = popZSLQueue();
    }
    zsl_frame = zslFrame.frame;

    return zsl_frame;
}

/**add for 3dcapture, record received zsl buffer begin*/
uint64_t SprdCamera3OEMIf::getZslBufferTimestamp() {
    Mutex::Autolock l(&mZslLock);
    uint64_t timestamp = 0;
    List<ZslBufferQueue>::iterator frame;

    if (mZSLQueue.size() == 0)
        return 0;

    frame = mZSLQueue.begin();
    for (frame = mZSLQueue.begin(); frame != mZSLQueue.end(); frame++) {
        timestamp = frame->frame.timestamp;
    }

    HAL_LOGD("current timestamp:%" PRId64, timestamp);
    return timestamp;
}

void SprdCamera3OEMIf::setZslBufferTimestamp(uint64_t timestamp) {
    mNeededTimestamp = timestamp;
}

void SprdCamera3OEMIf::receiveZslFrame(struct camera_frame_type *frame) {
    ZslBufferQueue zsl_frame;
    ZslBufferQueue zsl_buffer_q;
    hal_mem_info_t mem_info;
    cmr_int need_pause;
    int i;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }
    mHalOem->ops->camera_zsl_snapshot_need_pause(mCameraHandle, &need_pause);
    if (PREVIEW_ZSL_FRAME == frame->type) {
        if (1 == mZslShotPushFlag && 1 == mZslChannelStatus) {
            HAL_LOGV("getZSLQueueFrameNum %d", getZSLQueueFrameNum());
            for (i = getZSLQueueFrameNum(); i > mZslMaxFrameNum; i--) {
                HAL_LOGV("getZSLQueueFrameNum %d", getZSLQueueFrameNum());
                zsl_frame = popZSLQueue();
                if (!need_pause) {
                    mHalOem->ops->camera_set_zsl_buffer(
                        mCameraHandle, zsl_frame.frame.y_phy_addr,
                        zsl_frame.frame.y_vir_addr, zsl_frame.frame.fd);
                }
            }
            if (0 == getZSLQueueFrameNum()) {
                Mutex::Autolock zsllock(&mZslBufLock);
                mHalOem->ops->camera_set_zsl_snapshot_buffer(
                    mCameraHandle, frame->y_phy_addr, frame->y_vir_addr,
                    frame->fd);
            } else {
                Mutex::Autolock zsllock(&mZslBufLock);
                zsl_frame = popZSLQueue();
                mHalOem->ops->camera_set_zsl_snapshot_buffer(
                    mCameraHandle, zsl_frame.frame.y_phy_addr,
                    zsl_frame.frame.y_vir_addr, zsl_frame.frame.fd);
                if (!need_pause) {
                    mHalOem->ops->camera_set_zsl_buffer(
                        mCameraHandle, zsl_frame.frame.y_phy_addr,
                        zsl_frame.frame.y_vir_addr, zsl_frame.frame.fd);
                }
            }
            mZslShotPushFlag = 0;
            mZslChannelStatus = 0;
        } else {
            if (SPRD_INTERNAL_RAW_REQUESTED == getCaptureState()) {
                zsl_buffer_q.frame = *frame;
                pushZSLQueue(&zsl_buffer_q);
                if (mZslMaxFrameNum < getZSLQueueFrameNum() &&
                    1 == mZslChannelStatus) {
                    zsl_frame = popZSLQueue();
                    mHalOem->ops->camera_set_zsl_buffer(
                        mCameraHandle, zsl_frame.frame.y_phy_addr,
                        zsl_frame.frame.y_vir_addr, zsl_frame.frame.fd);
                }
            }
        }
        if (!isCapturing() || !need_pause) {
            zsl_buffer_q.frame = *frame;
            pushZSLQueue(&zsl_buffer_q);
            if (mZslMaxFrameNum < getZSLQueueFrameNum() &&
                1 == mZslChannelStatus) {
                zsl_frame = popZSLQueue();
                HAL_LOGV("zsl_frame.frame.fd=0x%x", zsl_frame.frame.fd);
                mHalOem->ops->camera_set_zsl_buffer(
                    mCameraHandle, zsl_frame.frame.y_phy_addr,
                    zsl_frame.frame.y_vir_addr, zsl_frame.frame.fd);
            }
        }
    } else if (PREVIEW_ZSL_CANCELED_FRAME == frame->type) {
        if (!isCapturing() || !need_pause) {
            mHalOem->ops->camera_set_zsl_buffer(
                mCameraHandle, frame->y_phy_addr, frame->y_vir_addr, frame->fd);
        }
    }
}

void SprdCamera3OEMIf::snapshotZsl(void *p_data) {
    ATRACE_CALL();

    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    cmr_int need_pause;
    struct camera_frame_type zsl_frame;
    bzero(&zsl_frame, sizeof(struct camera_frame_type));
    uint32_t cnt = 0;
    SPRD_DEF_Tag sprddefInfo;
    obj->mSetting->getSPRDDEFTag(&sprddefInfo);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    if (mZslPopFlag == 0 && (mVideoWidth != 0 && mVideoHeight != 0)) {
        mZslShotWait.waitRelative(mZslPopLock, ZSL_FRAME_TIMEOUT);
        mZslPopLock.unlock();
    }
    mZslPopFlag = 0;
    if (1 == obj->mZslShotPushFlag) {
        zsl_frame = obj->popZslFrame();
        // for burst mode
        if (obj->mSprdBurstModeEnabled == 1 && obj->mSprdZslEnabled == 1 &&
            (sprddefInfo.capture_mode == 1 || sprddefInfo.capture_mode == 30)) {
            if (zsl_frame.y_vir_addr != 0) {
                // set last time buffer to kernel for right scene
                mHalOem->ops->camera_set_zsl_buffer(
                    obj->mCameraHandle, zsl_frame.y_phy_addr,
                    zsl_frame.y_vir_addr, zsl_frame.fd);
                zsl_frame.y_vir_addr = 0;
            }
        }

        if (/*obj->mFlashCaptureFlag == 1*/ 0) {
            while (1) {
                if ((zsl_frame.y_vir_addr != 0) &&
                    (obj->mFlashCaptureFlag == 1) &&
                    (cnt <= obj->mFlashCaptureSkipNum)) {
                    HAL_LOGV("skip one frame");
                    mHalOem->ops->camera_set_zsl_buffer(
                        obj->mCameraHandle, zsl_frame.y_phy_addr,
                        zsl_frame.y_vir_addr, zsl_frame.fd);
                    zsl_frame.y_vir_addr = 0;
                    cnt++;
                }

                HAL_LOGD("wait for zsl frame");
                usleep(20 * 1000);
                zsl_frame = obj->popZslFrame();
                if ((zsl_frame.y_vir_addr != 0) &&
                    (cnt > obj->mFlashCaptureSkipNum)) {
                    HAL_LOGD("cnt=%d", cnt);
                    break;
                }
                if (true == mZslCaptureExitLoop) {
                    HAL_LOGD("zsl loop exit done.");
                    break;
                }
            }
        } else {
            while (zsl_frame.y_vir_addr == 0) {
                HAL_LOGD("wait for zsl frame");
                usleep(20 * 1000);
                zsl_frame = obj->popZslFrame();
                if (true == mZslCaptureExitLoop) {
                    HAL_LOGD("zsl loop exit done.");
                    break;
                }
            }
        }

        if (zsl_frame.y_vir_addr != 0) {
            HAL_LOGD("fd=0x%x", zsl_frame.fd);
            mNeededTimestamp = 0; /**add for 3dcapture, clean needed timestamp*/
            mHalOem->ops->camera_set_zsl_snapshot_buffer(
                obj->mCameraHandle, zsl_frame.y_phy_addr, zsl_frame.y_vir_addr,
                zsl_frame.fd);
            obj->mZslShotPushFlag = 0;
        }
        obj->mFlashCaptureFlag = 0;
    }
}

void SprdCamera3OEMIf::processZslFrame(void *p_data) {
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    struct camera_frame_type zsl_frame;
    cmr_int need_pause;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    mHalOem->ops->camera_zsl_snapshot_need_pause(obj->mCameraHandle,
                                                 &need_pause);
    if (SPRD_INTERNAL_RAW_REQUESTED == obj->getCaptureState() ||
        !obj->isCapturing() || !need_pause) {
        if (obj->mZslMaxFrameNum < obj->getZSLQueueFrameNum()) {
            zsl_frame = obj->popZslFrame();
            if (zsl_frame.y_vir_addr != 0) {
                mHalOem->ops->camera_set_zsl_buffer(
                    obj->mCameraHandle, zsl_frame.y_phy_addr,
                    zsl_frame.y_vir_addr, zsl_frame.fd);
            }
        }
    }
}

void SprdCamera3OEMIf::processStopMultiLayer(void *p_data) {
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    int ret = 0;
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    ret = mHalOem->ops->camera_stop_multi_layer(mCameraHandle);
    if (ret) {
        HAL_LOGE("camera_stop_multi_layer failed");
    }
    obj->mFlagMultiLayerStart = false;
}

int SprdCamera3OEMIf::ZSLMode_monitor_thread_init(void *p_data) {
    CMR_MSG_INIT(message);
    int ret = NO_ERROR;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

    if (!obj->mZSLModeMonitorInited) {
        ret = cmr_thread_create((cmr_handle *)&obj->mZSLModeMonitorMsgQueHandle,
                                ZSLMode_MONITOR_QUEUE_SIZE,
                                ZSLMode_monitor_thread_proc, (void *)obj);
        if (ret) {
            HAL_LOGE("create send msg failed!");
            return ret;
        }
        obj->mZSLModeMonitorInited = 1;

        message.msg_type = CMR_EVT_ZSL_MON_INIT;
        message.sync_flag = CMR_MSG_SYNC_RECEIVED;
        ret = cmr_thread_msg_send((cmr_handle)obj->mZSLModeMonitorMsgQueHandle,
                                  &message);
        if (ret) {
            HAL_LOGE("send msg failed!");
            return ret;
        }
    }
    return ret;
}

int SprdCamera3OEMIf::ZSLMode_monitor_thread_deinit(void *p_data) {
    CMR_MSG_INIT(message);
    int ret = NO_ERROR;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

    if (obj->mZSLModeMonitorInited) {
        message.msg_type = CMR_EVT_ZSL_MON_EXIT;
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
        ret = cmr_thread_msg_send((cmr_handle)obj->mZSLModeMonitorMsgQueHandle,
                                  &message);
        if (ret) {
            HAL_LOGE("send msg failed!");
        }

        ret = cmr_thread_destroy((cmr_handle)obj->mZSLModeMonitorMsgQueHandle);
        obj->mZSLModeMonitorMsgQueHandle = 0;
        obj->mZSLModeMonitorInited = 0;
    }
    return ret;
}

cmr_int SprdCamera3OEMIf::ZSLMode_monitor_thread_proc(struct cmr_msg *message,
                                                      void *p_data) {
    int exit_flag = 0;
    int ret = NO_ERROR;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    cmr_int need_pause;

    HAL_LOGD("zsl thread message.msg_type 0x%x, sub-type 0x%x, ret %d",
             message->msg_type, message->sub_msg_type, ret);

    switch (message->msg_type) {
    case CMR_EVT_ZSL_MON_INIT:
        HAL_LOGD("zsl thread msg init");
        break;

    case CMR_EVT_ZSL_MON_PUSH:
        HAL_LOGV("obj->getPreviewState() %d", obj->getPreviewState());
        if (obj->getPreviewState() != SPRD_INTERNAL_PREVIEW_STOPPING) {
            obj->processZslFrame(p_data);
        }
        break;
    case CMR_EVT_ZSL_MON_SNP:
        obj->snapshotZsl(p_data);
        break;
    case CMR_EVT_ZSL_MON_EXIT:
        HAL_LOGD("zsl thread msg exit");
        exit_flag = 1;
        break;

    case CMR_EVT_ZSL_STOP_MULTI_LAYER:
        obj->processStopMultiLayer(p_data);
        break;
    default:
        HAL_LOGE("unsupported zsl message");
        break;
    }

    if (exit_flag) {
        HAL_LOGD("zsl monitor thread exit ");
    }

    return 0;
}

uint32_t SprdCamera3OEMIf::isPreAllocCapMem() {
    if (0 == mIsPreAllocCapMem) {
        return 0;
    } else {
        return 1;
    }
}

int SprdCamera3OEMIf::pre_alloc_cap_mem_thread_init(void *p_data) {
    int ret = NO_ERROR;
    pthread_attr_t attr;

    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

    if (!obj) {
        HAL_LOGE("obj null  error");
        return -1;
    }

    HAL_LOGD("inited=%d", obj->mPreAllocCapMemInited);

    if (!obj->mPreAllocCapMemInited) {
        obj->mPreAllocCapMemInited = 1;
        obj->mIsPreAllocCapMemDone = 0;
        sem_init(&obj->mPreAllocCapMemSemDone, 0, 0);
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        ret = pthread_create(&obj->mPreAllocCapMemThread, &attr,
                             pre_alloc_cap_mem_thread_proc, (void *)obj);
        if (ret) {
            obj->mPreAllocCapMemInited = 0;
            sem_destroy(&obj->mPreAllocCapMemSemDone);
            HAL_LOGE("fail to send init msg");
        }
    }

    return ret;
}

int SprdCamera3OEMIf::pre_alloc_cap_mem_thread_deinit(void *p_data) {
    int ret = NO_ERROR;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

    if (!obj) {
        HAL_LOGE("obj null  error");
        return -1;
    }

    HAL_LOGD("inited=%d", obj->mPreAllocCapMemInited);

    if (obj->mPreAllocCapMemInited) {
        sem_wait(&obj->mPreAllocCapMemSemDone);
        sem_destroy(&obj->mPreAllocCapMemSemDone);
        obj->mPreAllocCapMemInited = 0;
        obj->mIsPreAllocCapMemDone = 0;
    }
    return ret;
}

void *SprdCamera3OEMIf::pre_alloc_cap_mem_thread_proc(void *p_data) {

    uint32_t mem_size = 0;
    int32_t buffer_id = 0;
    cmr_u32 sum = 0;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    HAL_LOGD("E");

    if (!obj) {
        HAL_LOGE("obj null  error");
        return NULL;
    }

    if (NULL == obj->mHalOem || NULL == obj->mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return NULL;
    }

    buffer_id = obj->mHalOem->ops->camera_pre_capture_get_buffer_id(
        obj->mCameraId, obj->mLargestSensorWidth, obj->mLargestSensorHeight);

    if (obj->mHalOem->ops->camera_pre_capture_get_buffer_size(
            obj->mCameraId, buffer_id, &mem_size, &sum)) {
        obj->mIsPreAllocCapMem = 0;
        HAL_LOGE("buffer size error, using normal alloc cap buffer mode");
    } else {
        cmr_uint phy_addr[MAX_SUB_RAWHEAP_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        cmr_uint virt_addr[MAX_SUB_RAWHEAP_NUM] = {0, 0, 0, 0, 0,
                                                   0, 0, 0, 0, 0};
        cmr_s32 fd[MAX_SUB_RAWHEAP_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        // obj->mSubRawHeapSize = mem_size;
        if (!obj->Callback_CaptureMalloc(mem_size, sum, &phy_addr[0],
                                         &virt_addr[0], &fd[0])) {
            obj->mIsPreAllocCapMemDone = 1;
            HAL_LOGD("pre alloc capture mem sum %d, fd:0x%x 0x%x 0x%x 0x%x",
                     sum, fd[0], fd[1], fd[2], fd[3]);
        } else {
            obj->mIsPreAllocCapMem = 0;
            HAL_LOGE("buffer alloc error, using normal alloc cap buffer mode");
        }
    }

    sem_post(&obj->mPreAllocCapMemSemDone);

    HAL_LOGD("X");

    return NULL;
}

void SprdCamera3OEMIf::setSensorCloseFlag() {
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    mHalOem->ops->camera_set_sensor_close_flag(mCameraHandle);
}

#ifdef CONFIG_CAMERA_EIS
void SprdCamera3OEMIf::EisPreview_init() {
    int i = 0;
    int num = sizeof(eis_init_info_tab) / sizeof(sprd_eis_init_info_t);
    video_stab_param_default(&mPreviewParam);
    mPreviewParam.src_w = (uint16_t)mPreviewWidth;
    mPreviewParam.src_h = (uint16_t)mPreviewHeight;
    mPreviewParam.dst_w = (uint16_t)mPreviewWidth * 5 / 6;
    mPreviewParam.dst_h = (uint16_t)mPreviewHeight * 5 / 6;
    mPreviewParam.method = 0;
    mPreviewParam.wdx = 0;
    mPreviewParam.wdy = 0;
    mPreviewParam.wdz = 0;
    // f,td,ts is different in each project.
    mPreviewParam.f = 1230.0f;
    mPreviewParam.td = 0.0001f;
    mPreviewParam.ts = 0.0001f;
    // EIS parameter depend on board version
    for (i = 0; i < num; i++) {
        if (strcmp(eis_init_info_tab[i].board_name, CAMERA_EIS_BOARD_PARAM) ==
            0) {
            mPreviewParam.f = eis_init_info_tab[i].f;   // 1230;
            mPreviewParam.td = eis_init_info_tab[i].td; // 0.004;
            mPreviewParam.ts = eis_init_info_tab[i].ts; // 0.021;
        }
    }
    // clear preview  gyro
    mGyroPreviewInfo.clear();
    HAL_LOGI("mParam f: %lf, td:%lf, ts:%lf", mPreviewParam.f, mPreviewParam.td,
             mPreviewParam.ts);
    video_stab_open(&mPreviewInst, &mPreviewParam);
    HAL_LOGI("mParam src_w: %d, src_h:%d, dst_w:%d, dst_h:%d",
             mPreviewParam.src_w, mPreviewParam.src_h, mPreviewParam.dst_w,
             mPreviewParam.dst_h);
}

void SprdCamera3OEMIf::EisVideo_init() {
    // mParam = {0};
    int i = 0;
    int num = sizeof(eis_init_info_tab) / sizeof(sprd_eis_init_info_t);
    video_stab_param_default(&mVideoParam);
    mVideoParam.src_w = (uint16_t)mVideoWidth;
    mVideoParam.src_h = (uint16_t)mVideoHeight;
    mVideoParam.dst_w = (uint16_t)mVideoWidth;
    mVideoParam.dst_h = (uint16_t)mVideoHeight;
    mVideoParam.method = 1;
    mVideoParam.wdx = 0;
    mVideoParam.wdy = 0;
    mVideoParam.wdz = 0;
    // f,td,ts is different in each project.
    mVideoParam.f = 1230.0f;
    mVideoParam.td = 0.0001f;
    mVideoParam.ts = 0.0001f;
    // EIS parameter depend on board version
    for (i = 0; i < num; i++) {
        if (strcmp(eis_init_info_tab[i].board_name, CAMERA_EIS_BOARD_PARAM) ==
            0) {
            mVideoParam.f = eis_init_info_tab[i].f;   // 1230;
            mVideoParam.td = eis_init_info_tab[i].td; // 0.004;
            mVideoParam.ts = eis_init_info_tab[i].ts; // 0.021;
        }
    }
    // clear video  gyro
    mGyroVideoInfo.clear();
    mIsRecording = true;
    HAL_LOGI("mParam f: %lf, td:%lf, ts:%lf", mVideoParam.f, mVideoParam.td,
             mVideoParam.ts);
    video_stab_open(&mVideoInst, &mVideoParam);
    HAL_LOGI("mParam src_w: %d, src_h:%d, dst_w:%d, dst_h:%d",
             mVideoParam.src_w, mVideoParam.src_h, mVideoParam.dst_w,
             mVideoParam.dst_h);
}

vsOutFrame SprdCamera3OEMIf::processPreviewEIS(vsInFrame frame_in) {
    ATRACE_CALL();

    int gyro_num = 0;
    int ret_eis = 1;
    uint count = 0;
    vsGyro *gyro = NULL;
    vsOutFrame frame_out_preview;
    memset(&frame_out_preview, 0x00, sizeof(vsOutFrame));

    if (mGyromaxtimestamp) {
        HAL_LOGD("in frame timestamp: %lf, mGyromaxtimestamp %lf",
                 frame_in.timestamp, mGyromaxtimestamp);
        video_stab_write_frame(mPreviewInst, &frame_in);
        do {
            gyro_num = mGyroPreviewInfo.size();
            if (gyro_num) {
                gyro = (vsGyro *)malloc(gyro_num * sizeof(vsGyro));
                if (NULL == gyro) {
                    HAL_LOGE(" malloc gyro buffer is fail");
                    goto eis_process_fail;
                }
                memset(gyro, 0, gyro_num * sizeof(vsGyro));
                popEISPreviewQueue(gyro, gyro_num);
                video_stab_write_gyro(mPreviewInst, gyro, gyro_num);
                if (gyro) {
                    free(gyro);
                    gyro = NULL;
                }
            }
            ret_eis = video_stab_read(mPreviewInst, &frame_out_preview);
            if (ret_eis == 0) {
                HAL_LOGD("frame_in %p, frame_out %p", frame_in.frame_data,
                         frame_out_preview.frame_data);
                if (frame_in.frame_data == frame_out_preview.frame_data)
                    return frame_out_preview;
                else
                    ret_eis = 1;
            } else if (ret_eis != 1) {
                HAL_LOGE("fail to read matrix");
                goto eis_process_fail;
            }
            if (++count >= 4 || (NO_ERROR !=
                                 mReadGyroPreviewCond.waitRelative(
                                     mReadGyroPreviewLock, 30000000))) {
                HAL_LOGD("gyro data is too slow for eis process");
                goto eis_process_fail;
            }
        } while (ret_eis == 1);
    } else
        HAL_LOGW("no gyro data to process EIS");

eis_process_fail:
    frame_out_preview.warp.dat[0][0] = frame_out_preview.warp.dat[1][1] =
        frame_out_preview.warp.dat[2][2] = 1.0;
    frame_out_preview.warp.dat[0][1] = frame_out_preview.warp.dat[0][2] =
        frame_out_preview.warp.dat[1][0] = 0.0;
    frame_out_preview.warp.dat[1][2] = frame_out_preview.warp.dat[2][0] =
        frame_out_preview.warp.dat[2][1] = 0.0;
    return frame_out_preview;
}

vsOutFrame SprdCamera3OEMIf::processVideoEIS(vsInFrame frame_in) {
    ATRACE_CALL();

    int gyro_num = 0;
    int ret_eis = 1;
    uint count = 0;
    vsGyro *gyro = NULL;
    vsOutFrame frame_out_video;
    memset(&frame_out_video, 0x00, sizeof(vsOutFrame));

    if (mGyromaxtimestamp) {
        HAL_LOGD("in frame num %d timestamp: %lf, mGyromaxtimestamp %lf",
                 frame_in.frame_num, frame_in.timestamp, mGyromaxtimestamp);
        video_stab_write_frame(mVideoInst, &frame_in);
        do {
            gyro_num = mGyroVideoInfo.size();
            HAL_LOGV("gyro_num = %d", gyro_num);
            if (gyro_num) {
                gyro = (vsGyro *)malloc(gyro_num * sizeof(vsGyro));
                if (NULL == gyro) {
                    HAL_LOGE(" malloc gyro buffer is fail");
                    goto eis_process_fail;
                }
                memset(gyro, 0, gyro_num * sizeof(vsGyro));
                popEISVideoQueue(gyro, gyro_num);
                video_stab_write_gyro(mVideoInst, gyro, gyro_num);
                if (gyro) {
                    free(gyro);
                    gyro = NULL;
                }
            }
            ret_eis = video_stab_read(mVideoInst, &frame_out_video);
            if (ret_eis == 0) {
                HAL_LOGD("out frame_num =%d,frame timestamp %lf, frame_out %p",
                         frame_out_video.frame_num, frame_out_video.timestamp,
                         frame_out_video.frame_data);
                return frame_out_video;
            } else {
                ret_eis = -1;
                HAL_LOGE("no frame out,exit directly");
                goto eis_process_fail;
            }
            if (++count >= 4 || (NO_ERROR !=
                                 mReadGyroVideoCond.waitRelative(
                                     mReadGyroVideoLock, 30000000))) {
                HAL_LOGW("gyro data is too slow for eis process");
                goto eis_process_fail;
            }
        } while (ret_eis == 1);
    } else
        HAL_LOGW("no gyro data to process EIS");

eis_process_fail:
    frame_out_video.warp.dat[0][0] = frame_out_video.warp.dat[1][1] =
        frame_out_video.warp.dat[2][2] = 1.0;
    frame_out_video.warp.dat[0][1] = frame_out_video.warp.dat[0][2] =
        frame_out_video.warp.dat[1][0] = 0.0;
    frame_out_video.warp.dat[1][2] = frame_out_video.warp.dat[2][0] =
        frame_out_video.warp.dat[2][1] = 0.0;
    return frame_out_video;
}

void SprdCamera3OEMIf::pushEISPreviewQueue(vsGyro *mGyrodata) {
    Mutex::Autolock l(&mEisPreviewLock);

    mGyroPreviewInfo.push_back(mGyrodata);
}

void SprdCamera3OEMIf::popEISPreviewQueue(vsGyro *gyro, int gyro_num) {
    Mutex::Autolock l(&mEisPreviewLock);

    int i;
    vsGyro *GyroInfo = NULL;
    for (i = 0; i < gyro_num; i++) {
        if (*mGyroPreviewInfo.begin() != NULL) {
            GyroInfo = *mGyroPreviewInfo.begin();
            gyro[i].t = GyroInfo->t / 1000000000;
            gyro[i].w[0] = GyroInfo->w[0];
            gyro[i].w[1] = GyroInfo->w[1];
            gyro[i].w[2] = GyroInfo->w[2];
            HAL_LOGV("gyro i %d,timestamp %lf, x: %lf, y: %lf, z: %lf", i,
                     gyro[i].t, gyro[i].w[0], gyro[i].w[1], gyro[i].w[2]);
        } else
            HAL_LOGW("gyro data is null in %d", i);
        mGyroPreviewInfo.erase(mGyroPreviewInfo.begin());
    }
}

void SprdCamera3OEMIf::pushEISVideoQueue(vsGyro *mGyrodata) {
    Mutex::Autolock l(&mEisVideoLock);

    mGyroVideoInfo.push_back(mGyrodata);
}

void SprdCamera3OEMIf::popEISVideoQueue(vsGyro *gyro, int gyro_num) {
    Mutex::Autolock l(&mEisVideoLock);

    int i;
    vsGyro *GyroInfo = NULL;
    for (i = 0; i < gyro_num; i++) {
        if (*mGyroVideoInfo.begin() != NULL) {
            GyroInfo = *mGyroVideoInfo.begin();
            gyro[i].t = GyroInfo->t / 1000000000;
            gyro[i].w[0] = GyroInfo->w[0];
            gyro[i].w[1] = GyroInfo->w[1];
            gyro[i].w[2] = GyroInfo->w[2];
            HAL_LOGV("gyro i %d,timestamp %lf, x: %lf, y: %lf, z: %lf", i,
                     gyro[i].t, gyro[i].w[0], gyro[i].w[1], gyro[i].w[2]);
        } else
            HAL_LOGW("gyro data is null in %d", i);
        mGyroVideoInfo.erase(mGyroVideoInfo.begin());
    }
}

void SprdCamera3OEMIf::EisPreviewFrameStab(struct camera_frame_type *frame) {
    vsInFrame frame_in;
    vsOutFrame frame_out;
    frame_in.frame_data = NULL;
    frame_out.frame_data = NULL;
    EIS_CROP_Tag eiscrop_Info;
    int64_t boot_time = frame->monoboottime;
    int64_t ae_time = frame->ae_time;
    uintptr_t buff_vir = (uintptr_t)(frame->y_vir_addr);
    // int64_t sleep_time = boot_time - buffer_timestamp;
    memset(&eiscrop_Info, 0x00, sizeof(EIS_CROP_Tag));

    if (mGyroInit && !mGyroExit) {
        if (frame->zoom_ratio == 0)
            frame->zoom_ratio = 1.0f;
        float zoom_ratio = frame->zoom_ratio;
        HAL_LOGV("boot_time = %" PRId64, boot_time);
        HAL_LOGV("ae_time = %" PRId64 ", zoom_ratio = %f", ae_time, zoom_ratio);
        frame_in.frame_data = (uint8_t *)buff_vir;
        frame_in.timestamp = (double)(boot_time - ae_time / 2);
        frame_in.timestamp = frame_in.timestamp / 1000000000;
        frame_in.zoom = (double)zoom_ratio;
        frame_in.frame_num = 0;
        frame_out = processPreviewEIS(frame_in);
        HAL_LOGD(
            "transfer_matrix wrap %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf",
            frame_out.warp.dat[0][0], frame_out.warp.dat[0][1],
            frame_out.warp.dat[0][2], frame_out.warp.dat[1][0],
            frame_out.warp.dat[1][1], frame_out.warp.dat[1][2],
            frame_out.warp.dat[2][0], frame_out.warp.dat[2][1],
            frame_out.warp.dat[2][2]);

        double crop_start_w =
            frame_out.warp.dat[0][2] + mPreviewParam.src_w / 12;
        double crop_start_h =
            frame_out.warp.dat[1][2] + mPreviewParam.src_h / 12;
        eiscrop_Info.crop[0] = (int)(crop_start_w + 0.5);
        eiscrop_Info.crop[1] = (int)(crop_start_h + 0.5);
        eiscrop_Info.crop[2] = (int)(crop_start_w + 0.5) + mPreviewParam.dst_w;
        eiscrop_Info.crop[3] = (int)(crop_start_h + 0.5) + mPreviewParam.dst_h;
        mSetting->setEISCROPTag(eiscrop_Info);
    } else {
        HAL_LOGW("gyro is not enable, eis process is not work");
        eiscrop_Info.crop[0] = 0;
        eiscrop_Info.crop[1] = 0;
        eiscrop_Info.crop[2] = mPreviewWidth;
        eiscrop_Info.crop[3] = mPreviewHeight;
        mSetting->setEISCROPTag(eiscrop_Info);
    }
}

vsOutFrame SprdCamera3OEMIf::EisVideoFrameStab(struct camera_frame_type *frame,
                                               uint32_t frame_num) {
    vsInFrame frame_in;
    vsOutFrame frame_out;
    frame_in.frame_data = NULL;
    EIS_CROP_Tag eiscrop_Info;
    int64_t boot_time = frame->monoboottime;
    int64_t ae_time = frame->ae_time;
    uintptr_t buff_vir = (uintptr_t)(frame->y_vir_addr);

    memset(&frame_out, 0x00, sizeof(vsOutFrame));
    memset(&eiscrop_Info, 0x00, sizeof(EIS_CROP_Tag));

    // gyro data is should be used for video frame stab
    if (mGyroInit && !mGyroExit) {
        if (frame->zoom_ratio == 0)
            frame->zoom_ratio = 1.0f;
        float zoom_ratio = frame->zoom_ratio;
        HAL_LOGV("boot_time = %" PRId64 ",ae_time =%" PRId64 ",zoom_ratio = %f",
                 boot_time, ae_time, zoom_ratio);
        frame_in.frame_data = (uint8_t *)buff_vir;
        frame_in.timestamp = (double)(boot_time - ae_time / 2);
        frame_in.timestamp = frame_in.timestamp / 1000000000;
        frame_in.zoom = (double)zoom_ratio;
        frame_in.frame_num = frame_num;
        frame_out = processVideoEIS(frame_in);
        if (frame_out.frame_data)
            HAL_LOGV("transfer_matrix wrap %lf, %lf, %lf, %lf, %lf, %lf, %lf, "
                     "%lf, %lf",
                     frame_out.warp.dat[0][0], frame_out.warp.dat[0][1],
                     frame_out.warp.dat[0][2], frame_out.warp.dat[1][0],
                     frame_out.warp.dat[1][1], frame_out.warp.dat[1][2],
                     frame_out.warp.dat[2][0], frame_out.warp.dat[2][1],
                     frame_out.warp.dat[2][2]);
    } else {
        HAL_LOGD("gyro is not enable, eis process is not work");
    }

    if (frame_out.frame_data) {
        char *eis_buff = (char *)(frame_out.frame_data) +
                         frame->width * frame->height * 3 / 2;
        double *warp_buff = (double *)eis_buff;
        HAL_LOGV("video vir address 0x%lx,warp address %p",
                 frame_out.frame_data, warp_buff);
        if (warp_buff) {
            *warp_buff++ = 16171225;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    if (warp_buff)
                        *warp_buff++ = frame_out.warp.dat[i][j];
                }
            }
        }
    }

    return frame_out;
}
#endif

#ifdef CONFIG_CAMERA_GYRO
int SprdCamera3OEMIf::gyro_monitor_thread_init(void *p_data) {
    int ret = NO_ERROR;
    pthread_attr_t attr;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

    if (!obj) {
        HAL_LOGE("obj null  error");
        return -1;
    }

    HAL_LOGD("E inited=%d", obj->mGyroInit);

    if (!obj->mGyroInit) {
        obj->mGyroInit = 1;
        sem_init(&obj->mGyro_sem, 0, 0);
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        ret = pthread_create(&obj->mGyroMsgQueHandle, &attr,
                             gyro_monitor_thread_proc, (void *)obj);
        pthread_attr_destroy(&attr);
        if (ret) {
            obj->mGyroInit = 0;
            sem_destroy(&obj->mGyro_sem);
            HAL_LOGE("fail to init gyro thread");
        }
    }

    return ret;
}

void *SprdCamera3OEMIf::gyro_monitor_thread_proc(void *p_data) {
    int ret = 0;
    ssize_t n;
    ASensorEvent buffer[8];
    uint32_t GyroRate = 10 * 1000; //  us
    uint32_t default_max_fps = 30;
    uint32_t max_fps = default_max_fps;
    uint32_t eventRate = 1000 * 1000 / default_max_fps; // us

    int events;
    int32_t result = 0;
    uint32_t Timeout = 60; // ms
    uint32_t Gyro_flag = 0;
    uint32_t Gsensor_flag = 0;

    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    HAL_LOGD("E");

    if (!obj) {
        HAL_LOGE("obj null  error");
        sem_post(&obj->mGyro_sem);
        return NULL;
    }

    if (NULL == obj->mCameraHandle || NULL == obj->mHalOem ||
        NULL == obj->mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        sem_post(&obj->mGyro_sem);
        return NULL;
    }

    obj->mGyroNum = 0;
    obj->mGyromaxtimestamp = 0;
#ifdef CONFIG_CAMERA_EIS
    memset(obj->mGyrodata, 0, sizeof(obj->mGyrodata));
#endif

    SensorManager &mgr(
        SensorManager::getInstanceForPackage(String16("EIS intergrate")));

    Sensor const *const *list;
    ssize_t count = mgr.getSensorList(&list);
    sp<SensorEventQueue> q = mgr.createEventQueue();
    Sensor const *gyroscope = mgr.getDefaultSensor(Sensor::TYPE_GYROSCOPE);
    Sensor const *gsensor = mgr.getDefaultSensor(Sensor::TYPE_ACCELEROMETER);
    if (q == NULL) {
        HAL_LOGE("createEventQueue error");
        sem_post(&obj->mGyro_sem);
        return NULL;
    }
    const int fd = q->getFd();
    HAL_LOGI("EventQueue fd %d", fd);
    sp<Looper> loop = NULL;

    if (gyroscope != NULL) {
        ret = q->enableSensor(gyroscope, GyroRate);
        if (ret)
            HAL_LOGE("enable gyroscope fail");
        else
            Gyro_flag = 1;
    } else
        HAL_LOGW("this device not support gyro");

    if (NULL != obj->mCameraHandle) {
        obj->mHalOem->ops->camera_get_sensor_max_fps(obj->mCameraHandle,
                                                     obj->mCameraId, &max_fps);
        if (0 == max_fps) {
            max_fps = default_max_fps;
        }
        eventRate = 1000 * 1000 / max_fps; // fps->rate:ms
    }
    if (gsensor != NULL) {
        ret = q->enableSensor(gsensor, eventRate);
        if (ret) {
            HAL_LOGE("enable gsensor fail");
            if (Gyro_flag == 0)
                goto exit;
        } else
            Gsensor_flag = 1;
    } else {
        HAL_LOGW("this device not support Gsensor");
        if (Gyro_flag == 0)
            goto exit;
    }

    loop = new Looper(true);
    if (loop == NULL) {
        HAL_LOGE("creat loop fail");
        if (Gsensor_flag) {
            q->disableSensor(gsensor);
            Gsensor_flag = 0;
        }
        if (Gyro_flag) {
            q->disableSensor(gyroscope);
            Gyro_flag = 0;
        }
        goto exit;
    }
    loop->addFd(fd, fd, ALOOPER_EVENT_INPUT, NULL, NULL);

    struct cmr_af_aux_sensor_info sensor_info;
    HAL_LOGD("loop for pull gyro and gsensor data");
    while (true == obj->mGyroInit) {
        result = loop->pollOnce(Timeout, NULL, &events, NULL);
        if ((result == ALOOPER_POLL_ERROR) || (events & ALOOPER_EVENT_HANGUP)) {
            HAL_LOGE("SensorEventQueue::waitForEvent error");
            if (Gsensor_flag) {
                q->disableSensor(gsensor);
                Gsensor_flag = 0;
            }
            if (Gyro_flag) {
                q->disableSensor(gyroscope);
                Gyro_flag = 0;
            }
            goto exit;
        } else if (result == ALOOPER_POLL_TIMEOUT) {
            HAL_LOGW("SensorEventQueue::waitForEvent timeout");
        }
        if (!obj) {
            HAL_LOGE("obj is null,exit thread loop");
            if (Gsensor_flag) {
                q->disableSensor(gsensor);
                Gsensor_flag = 0;
            }
            if (Gyro_flag) {
                q->disableSensor(gyroscope);
                Gyro_flag = 0;
            }
            goto exit;
        }
        if ((result == fd) && (n = q->read(buffer, 8)) > 0) {
            for (int i = 0; i < n; i++) {
                memset((void *)&sensor_info, 0, sizeof(sensor_info));
                switch (buffer[i].type) {
                case Sensor::TYPE_GYROSCOPE: {
#ifdef CONFIG_CAMERA_EIS
                    // push gyro data for eis preview & video queue
                    SPRD_DEF_Tag sprddefInfo;
                    obj->mSetting->getSPRDDEFTag(&sprddefInfo);
                    if (obj->mRecordingMode && sprddefInfo.sprd_eis_enabled) {
                        struct timespec t1;
                        clock_gettime(CLOCK_BOOTTIME, &t1);
                        nsecs_t time1 = (t1.tv_sec) * 1000000000LL + t1.tv_nsec;
                        HAL_LOGV("mGyroCamera CLOCK_BOOTTIME =%" PRId64, time1);
                        obj->mGyrodata[obj->mGyroNum].t = buffer[i].timestamp;
                        obj->mGyrodata[obj->mGyroNum].w[0] = buffer[i].data[0];
                        obj->mGyrodata[obj->mGyroNum].w[1] = buffer[i].data[1];
                        obj->mGyrodata[obj->mGyroNum].w[2] = buffer[i].data[2];
                        obj->mGyromaxtimestamp =
                            obj->mGyrodata[obj->mGyroNum].t / 1000000000;
                        obj->pushEISPreviewQueue(
                            &obj->mGyrodata[obj->mGyroNum]);
                        obj->mReadGyroPreviewCond.signal();
                        if (obj->mIsRecording) {
                            obj->pushEISVideoQueue(
                                &obj->mGyrodata[obj->mGyroNum]);
                            obj->mReadGyroVideoCond.signal();
                            if (++obj->mGyroNum >= obj->kGyrocount)
                                obj->mGyroNum = 0;
                            HAL_LOGV("gyro timestamp %" PRId64
                                     ", x: %f, y: %f, z: %f",
                                     buffer[i].timestamp, buffer[i].data[0],
                                     buffer[i].data[1], buffer[i].data[2]);
                        }
#endif

                        sensor_info.type = CAMERA_AF_GYROSCOPE;
                        sensor_info.gyro_info.timestamp = buffer[i].timestamp;
                        sensor_info.gyro_info.x = buffer[i].data[0];
                        sensor_info.gyro_info.y = buffer[i].data[1];
                        sensor_info.gyro_info.z = buffer[i].data[2];
                        if (NULL != obj->mCameraHandle &&
                            SPRD_IDLE == obj->mCameraState.capture_state &&
                            NULL != obj->mHalOem) {
                            obj->mHalOem->ops->camera_set_sensor_info_to_af(
                                obj->mCameraHandle, &sensor_info);
                        }
                        break;
                    }

                case Sensor::TYPE_ACCELEROMETER: {
                    sensor_info.type = CAMERA_AF_ACCELEROMETER;
                    sensor_info.gsensor_info.timestamp = buffer[i].timestamp;
                    sensor_info.gsensor_info.vertical_up = buffer[i].data[0];
                    sensor_info.gsensor_info.vertical_down = buffer[i].data[1];
                    sensor_info.gsensor_info.horizontal = buffer[i].data[2];
                    HAL_LOGV("gsensor timestamp %" PRId64
                             ", x: %f, y: %f, z: %f",
                             buffer[i].timestamp, buffer[i].data[0],
                             buffer[i].data[1], buffer[i].data[2]);
                    if (NULL != obj->mCameraHandle && NULL != obj->mHalOem) {
                        obj->mHalOem->ops->camera_set_sensor_info_to_af(
                            obj->mCameraHandle, &sensor_info);
                    }
                    break;
                }

                default:
                    break;
                }
                }
            }
        }

        if (Gsensor_flag) {
            q->disableSensor(gsensor);
            Gsensor_flag = 0;
        }
        if (Gyro_flag) {
            q->disableSensor(gyroscope);
            Gyro_flag = 0;
        }
    exit:
        sem_post(&obj->mGyro_sem);
        obj->mGyroExit = 1;
        // mgr.sensorManagerDied();
        HAL_LOGD("X");

        return NULL;
    }

    int SprdCamera3OEMIf::gyro_monitor_thread_deinit(void *p_data) {
        int ret = NO_ERROR;
        void *dummy;
        struct timespec ts;
        SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

        if (!obj) {
            HAL_LOGE("obj null error");
            return UNKNOWN_ERROR;
        }

        HAL_LOGD("E inited=%d, Deinit = %d", obj->mGyroInit, obj->mGyroExit);

        if (obj->mGyroInit) {
            obj->mGyroInit = 0;

            if (!obj->mGyroExit) {
                if (clock_gettime(CLOCK_REALTIME, &ts)) {
                    HAL_LOGE("get time failed");
                    return UNKNOWN_ERROR;
                }
                /*when gyro thread proc time is long when camera close, we
                 * should
                 * wait for thread end at last 1000ms*/
                ts.tv_nsec += ms2ns(1000);
                if (ts.tv_nsec > 1000000000) {
                    ts.tv_sec += 1;
                    ts.tv_nsec -= 1000000000;
                }
                ret = sem_timedwait(&obj->mGyro_sem, &ts);
                if (ret)
                    HAL_LOGW("wait for gyro timeout");
            } else
                HAL_LOGW("gyro thread already end");

            sem_destroy(&obj->mGyro_sem);
            // ret = pthread_join(obj->mGyroMsgQueHandle, &dummy);
            obj->mGyroMsgQueHandle = 0;

#ifdef CONFIG_CAMERA_EIS
            while (!obj->mGyroPreviewInfo.empty())
                obj->mGyroPreviewInfo.erase(obj->mGyroPreviewInfo.begin());
            while (!obj->mGyroVideoInfo.empty())
                obj->mGyroVideoInfo.erase(obj->mGyroVideoInfo.begin());
#endif
        }
        HAL_LOGD("X inited=%d, Deinit = %d", obj->mGyroInit, obj->mGyroExit);

        return ret;
    }

#endif
}

