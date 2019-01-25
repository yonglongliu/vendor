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
#define LOG_TAG "Cam3RealBokeh"
//#define LOG_NDEBUG 0
#include "SprdCamera3RealBokeh.h"
#include "../../sensor/otp_drv/otp_info.h"

using namespace android;
namespace sprdcamera {

#define BOKEH_CIRCLE_SIZE_SCALE (3)
#define BOKEH_SMOOTH_SIZE_SCALE (8)
#define BOKEH_CIRCLE_VALUE_MIN (20)
#define BOKEH_THREAD_TIMEOUT 50e6
#define LIB_BOKEH_PATH "libsprdbokeh.so"
#define LIB_DEPTH_PATH "libsprddepth.so"
#define LIB_BOKEH_PREVIEW_PATH "libbokeh_depth.so"
#define LIB_ARCSOFT_BOKEH_PATH "libarcsoft_dualcam_refocus.so"

#ifdef YUV_CONVERT_TO_JPEG
#define BOKEH_REFOCUS_COMMON_PARAM_NUM (13)
#define ARCSOFT_BOKEH_REFOCUS_COMMON_PARAM_NUM (10)
#else
#define BOKEH_REFOCUS_COMMON_PARAM_NUM (11)
#define ARCSOFT_BOKEH_REFOCUS_COMMON_PARAM_NUM (8)
#endif

#define DEPTH_OUTPUT_WIDTH (160)
#define DEPTH_OUTPUT_HEIGHT (120)
#define DEPTH_SNAP_OUTPUT_WIDTH (324)
#define DEPTH_SNAP_OUTPUT_HEIGHT (243)

#define DEPTH_DATA_SIZE (68)
#define ARCSOFT_DEPTH_DATA_SIZE (561616)

/* refocus api error code */
#define ALRNB_ERR_SUCCESS 0x00

#define BOKEH_TIME_DIFF (100e6)
#define PENDINGTIME (1000000)
#define PENDINGTIMEOUT (5000000000)

SprdCamera3RealBokeh *mRealBokeh = NULL;

// Error Check Macros
#define CHECK_CAPTURE()                                                        \
    if (!mRealBokeh) {                                                         \
        HAL_LOGE("Error getting capture ");                                    \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_CAPTURE_ERROR()                                                  \
    if (!mRealBokeh) {                                                         \
        HAL_LOGE("Error getting capture ");                                    \
        return -ENODEV;                                                        \
    }

#define CHECK_HWI_ERROR(hwi)                                                   \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return -ENODEV;                                                        \
    }

camera3_device_ops_t SprdCamera3RealBokeh::mCameraCaptureOps = {
    .initialize = SprdCamera3RealBokeh::initialize,
    .configure_streams = SprdCamera3RealBokeh::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3RealBokeh::construct_default_request_settings,
    .process_capture_request = SprdCamera3RealBokeh::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3RealBokeh::dump,
    .flush = SprdCamera3RealBokeh::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3RealBokeh::callback_ops_main = {
    .process_capture_result = SprdCamera3RealBokeh::process_capture_result_main,
    .notify = SprdCamera3RealBokeh::notifyMain};

camera3_callback_ops SprdCamera3RealBokeh::callback_ops_aux = {
    .process_capture_result = SprdCamera3RealBokeh::process_capture_result_aux,
    .notify = SprdCamera3RealBokeh::notifyAux};

/*===========================================================================
 * FUNCTION         : SprdCamera3RealBokeh
 *
 * DESCRIPTION     : SprdCamera3RealBokeh Constructor
 *
 * PARAMETERS:
 *   @num_of_cameras  : Number of Physical Cameras on device
 *
 *==========================================================================*/
SprdCamera3RealBokeh::SprdCamera3RealBokeh() {
    HAL_LOGI(" E");
    m_nPhyCameras = 2; // m_nPhyCameras should always be 2 with dual camera mode
    bzero(&m_VirtualCamera, sizeof(sprd_virtual_camera_t));
    m_VirtualCamera.id = (uint8_t)CAM_BOKEH_MAIN_ID;
    mStaticMetadata = NULL;
    m_pPhyCamera = NULL;
    mCaptureWidth = 0;
    mCaptureHeight = 0;
#ifdef BOKEH_YUV_DATA_TRANSFORM
    mTransformWidth = 0;
    mTransformHeight = 0;
#endif
    mPreviewWidth = 0;
    mPreviewHeight = 0;
    mCallbackWidth = 0;
    mCallbackHeight = 0;
    mDepthOutWidth = 0;
    mDepthOutHeight = 0;
    mDepthSnapOutWidth = 0;
    mDepthSnapOutHeight = 0;
    mDepthPrevImageWidth = 0;
    mDepthPrevImageHeight = 0;
    mDepthSnapImageWidth = 0;
    mDepthSnapImageHeight = 0;
    mCaptureStreamsNum = 0;
    mCallbackStreamsNum = 0;
    mPreviewStreamsNum = 0;
    mPendingRequest = 0;
    mSavedCapStreams = NULL;
    mBokehCapApi = NULL;
    mDepthApi = NULL;
    mBokehPrevApi = NULL;
    mArcSoftBokehApi = NULL;
    mCapFrameNumber = 0;
    mPrevFrameNumber = 0;
    mIsCapturing = false;
    mjpegSize = 0;
    mCameraId = 0;
    mFlushing = false;
    mFirstArcBokeh = false;
    mFirstSprdBokeh = false;
    mVcmSteps = 0;
    mOtpSize = 0;
    mOtpType = 0;
    mOtpExist = false;
    mApiVersion = SPRD_API_MODE;
    mLocalBufferNumber = 0;
    mIsSupportPBokeh = false;
    mDepthPrevFormat = MODE_WEIGHTMAP;
    mhasCallbackStream = false;
    mJpegOrientation = 0;
    mMaxPendingCount = 0;
    mUpdateDepthFlag = false;
    mCallbackOps = NULL;
    m_pMainSnapBuffer = NULL;
    m_pSprdDepthBuffer = NULL;
#ifdef YUV_CONVERT_TO_JPEG
    mOrigJpegSize = 0;
    m_pDstJpegBuffer = NULL;
#endif
    mSavedRequestList.clear();
    setupPhysicalCameras();
    mCaptureThread = new BokehCaptureThread();
    mPreviewMuxerThread = new PreviewMuxerThread();
    memset(mLocalBuffer, 0, sizeof(new_mem_t) * LOCAL_BUFFER_NUM);
    memset(mAuxStreams, 0,
           sizeof(camera3_stream_t) * REAL_BOKEH_MAX_NUM_STREAMS);
    memset(mMainStreams, 0,
           sizeof(camera3_stream_t) * REAL_BOKEH_MAX_NUM_STREAMS);
    memset(mFaceInfo, 0, sizeof(int32_t) * 4);
    memset(mOtpData, 0, sizeof(uint8_t) * SPRD_DUAL_OTP_SIZE);
    memset(&mArcSoftInfo, 0, sizeof(ARC_REFOCUSCAMERAIMAGE_PARAM));
    memset(&mCaliData, 0, sizeof(ARC_DC_CALDATA));
    memset(mArcSoftCalibData, 0, sizeof(char) * THIRD_OTP_SIZE);
    memset(&mPerfectskinlevel, 0, sizeof(face_beauty_levels));
    memset(&mArcParam, 0, sizeof(ArcParam));
    memset(&mThumbReq, 0, sizeof(multi_request_saved_t));
    mLocalBufferList.clear();
    mMetadataList.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mNotifyListMain.clear();
    mNotifyListAux.clear();
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : ~SprdCamera3RealBokeh
 *
 * DESCRIPTION     : SprdCamera3RealBokeh Desctructor
 *
 *==========================================================================*/
SprdCamera3RealBokeh::~SprdCamera3RealBokeh() {
    HAL_LOGI("E");
    mCaptureThread = NULL;
    if (m_pPhyCamera) {
        delete[] m_pPhyCamera;
        m_pPhyCamera = NULL;
    }
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : getCameraCapture
 *
 * DESCRIPTION     : Creates Camera Capture if not created
 *
 * PARAMETERS:
 *   @pCapture               : Pointer to retrieve Camera Capture
 *
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera3RealBokeh::getCameraBokeh(SprdCamera3RealBokeh **pCapture) {
    *pCapture = NULL;
    if (!mRealBokeh) {
        mRealBokeh = new SprdCamera3RealBokeh();
    }
    CHECK_CAPTURE();
    *pCapture = mRealBokeh;
    HAL_LOGV("mRealBokeh: %p ", mRealBokeh);

    return;
}

/*===========================================================================
 * FUNCTION         : get_camera_info
 *
 * DESCRIPTION     : get logical camera info
 *
 * PARAMETERS:
 *   @camera_id     : Logical Camera ID
 *   @info              : Logical main Camera Info
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              ENODEV : Camera not found
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3RealBokeh::get_camera_info(__unused int camera_id,
                                          struct camera_info *info) {

    int rc = NO_ERROR;

    HAL_LOGI("E");
    if (info) {
        rc = mRealBokeh->getCameraInfo(info);
    }
    HAL_LOGI("X, rc: %d", rc);

    return rc;
}

/*===========================================================================
 * FUNCTION   : camera_device_open
 *
 * DESCRIPTION: static function to open a camera device by its ID
 *
 * PARAMETERS :
 *   @modue: hw module
 *   @id : camera ID
 *   @hw_device : ptr to struct storing camera hardware device info
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              BAD_VALUE : Invalid Camera ID
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3RealBokeh::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    int rc = NO_ERROR;

    HAL_LOGD("id= %d", atoi(id));
    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    rc = mRealBokeh->cameraDeviceOpen(atoi(id), hw_device);
    HAL_LOGD("id= %d, rc: %d", atoi(id), rc);

    return rc;
}

/*===========================================================================
 * FUNCTION   : close_camera_device
 *
 * DESCRIPTION: Close the camera
 *
 * PARAMETERS :
 *   @hw_dev : camera hardware device info
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3RealBokeh::close_camera_device(__unused hw_device_t *hw_dev) {
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }
    return mRealBokeh->closeCameraDevice();
}

/*===========================================================================
 * FUNCTION   : close  amera device
 *
 * DESCRIPTION: Close the camera
 *
 * PARAMETERS :
 *   @hw_dev : camera hardware device info
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3RealBokeh::closeCameraDevice() {

    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;

    HAL_LOGI("E");
    // Attempt to close all cameras regardless of unbundle results
    for (uint32_t i = m_nPhyCameras; i > 0; i--) {
        sprdCam = &m_pPhyCamera[i - 1];
        hw_device_t *dev = (hw_device_t *)(sprdCam->dev);
        if (dev == NULL)
            continue;

        HAL_LOGW("camera id:%d", sprdCam->id);
        rc = SprdCamera3HWI::close_camera_device(dev);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error, camera id:%d", sprdCam->id);
        }
        sprdCam->hwi = NULL;
        sprdCam->dev = NULL;
    }
    if (!mFlushing) {
        bokehThreadExit();
    }

    freeLocalBuffer();
    mSavedRequestList.clear();
    mLocalBufferList.clear();
    mMetadataList.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mNotifyListMain.clear();
    mNotifyListAux.clear();
    if (mCaptureThread->mArcSoftDepthMap != NULL) {
        free(mCaptureThread->mArcSoftDepthMap);
        mCaptureThread->mArcSoftDepthMap = NULL;
    }
    if (mBokehCapApi != NULL) {
        unLoadBokehApi();
        free(mBokehCapApi);
        mBokehCapApi = NULL;
    }
    if (mDepthApi != NULL) {
        unLoadDepthApi();
        free(mDepthApi);
        mDepthApi = NULL;
    }
    if (mBokehPrevApi != NULL) {
        unLoadBokehPreviewApi();
        free(mBokehPrevApi);
        mBokehPrevApi = NULL;
    }
    if (mArcSoftBokehApi != NULL) {
        unLoadBokehApi();
        free(mArcSoftBokehApi);
        mArcSoftBokehApi = NULL;
    }
    mFirstArcBokeh = false;
    mFirstSprdBokeh = false;
    HAL_LOGI("X, rc: %d", rc);

    return rc;
}

/*===========================================================================
 * FUNCTION   :initialize
 *
 * DESCRIPTION: initialize camera device
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3RealBokeh::initialize(
    __unused const struct camera3_device *device,
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;

    HAL_LOGI("E");
    CHECK_CAPTURE_ERROR();

    rc = mRealBokeh->initialize(callback_ops);

    HAL_LOGI("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :construct_default_request_settings
 *
 * DESCRIPTION: construc default request settings
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
const camera_metadata_t *
SprdCamera3RealBokeh::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *rc;

    HAL_LOGI("E");
    if (!mRealBokeh) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }
    rc = mRealBokeh->constructDefaultRequestSettings(device, type);

    HAL_LOGI("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :configure_streams
 *
 * DESCRIPTION: configure streams
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3RealBokeh::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;

    HAL_LOGI(" E");
    CHECK_CAPTURE_ERROR();

    rc = mRealBokeh->configureStreams(device, stream_list);

    HAL_LOGI(" X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_request
 *
 * DESCRIPTION: process capture request
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3RealBokeh::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;

    HAL_LOGV("idx:%d", request->frame_number);
    CHECK_CAPTURE_ERROR();
    rc = mRealBokeh->processCaptureRequest(device, request);

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3RealBokeh
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3RealBokeh::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_CAPTURE();
    mRealBokeh->processCaptureResultMain(result);
}

/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3RealBokeh
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3RealBokeh::process_capture_result_aux(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_CAPTURE();
    mRealBokeh->processCaptureResultAux(result);
}

/*===========================================================================
 * FUNCTION   :notifyMain
 *
 * DESCRIPTION:  main sernsor notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3RealBokeh::notifyMain(const struct camera3_callback_ops *ops,
                                      const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%d", msg->message.shutter.frame_number);
    mRealBokeh->notifyMain(msg);
}

/*===========================================================================
 * FUNCTION   :notifyAux
 *
 * DESCRIPTION: aux sensor  notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3RealBokeh::notifyAux(const struct camera3_callback_ops *ops,
                                     const camera3_notify_msg_t *msg) {
    CHECK_CAPTURE();
    mRealBokeh->notifyAux(msg);
}

/*===========================================================================
 * FUNCTION   :allocatebuf
 *
 * DESCRIPTION: deconstructor of SprdCamera3RealBokeh
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3RealBokeh::allocateBuff() {
    int rc = 0;
    size_t count = 0;
    size_t j = 0;
    size_t capture_num = 0;
    size_t preview_num = LOCAL_PREVIEW_NUM / 2;
    int w = 0, h = 0;
    HAL_LOGI(":E");
    mLocalBufferList.clear();
    freeLocalBuffer();

    for (size_t j = 0; j < preview_num; j++) {
        if (0 > allocateOne(mPreviewWidth, mPreviewHeight, &(mLocalBuffer[j]),
                            YUV420)) {
            HAL_LOGE("request one buf failed.");
            goto mem_fail;
        }
        mLocalBuffer[j].type = PREVIEW_MAIN_BUFFER;
        mLocalBufferList.push_back(&(mLocalBuffer[j]));

        if (0 > allocateOne(mDepthPrevImageWidth, mDepthPrevImageHeight,
                            &(mLocalBuffer[preview_num + j]), YUV420)) {
            HAL_LOGE("request one buf failed.");
            goto mem_fail;
        }
        mLocalBuffer[preview_num + j].type = PREVIEW_DEPTH_BUFFER;
        mLocalBufferList.push_back(&(mLocalBuffer[preview_num + j]));
    }
    count += LOCAL_PREVIEW_NUM;

    if (mCaptureThread->mAbokehGallery) {
        capture_num = LOCAL_CAPBUFF_NUM - 1;
    } else {
        capture_num = LOCAL_CAPBUFF_NUM;
    }

    for (j = 0; j < capture_num; j++) {
        if (0 > allocateOne(mCallbackWidth, mCallbackHeight,
                            &(mLocalBuffer[count + j]), YUV420)) {
            HAL_LOGE("request one buf failed.");
            goto mem_fail;
        }
        mLocalBuffer[count + j].type = SNAPSHOT_MAIN_BUFFER;
        mLocalBufferList.push_back(&(mLocalBuffer[count + j]));
    }
    count += capture_num;

    for (j = 0; j < SNAP_DEPTH_NUM; j++) {
        if (0 > allocateOne(mDepthSnapImageWidth, mDepthSnapImageHeight,
                            &(mLocalBuffer[count + j]), YUV420)) {
            HAL_LOGE("request one buf failed.");
            goto mem_fail;
        }
        mLocalBuffer[count + j].type = SNAPSHOT_DEPTH_BUFFER;
        mLocalBufferList.push_back(&(mLocalBuffer[count + j]));
    }

    count += SNAP_DEPTH_NUM;
#ifdef BOKEH_YUV_DATA_TRANSFORM
    if (mTransformWidth != 0) {
        for (j = 0; j < SNAP_TRANSF_NUM; j++) {
            if (0 > allocateOne(mTransformWidth, mTransformHeight,
                                &(mLocalBuffer[count + j]), YUV420)) {
                HAL_LOGE("request one buf failed.");
                goto mem_fail;
            }
            mLocalBuffer[count + j].type = SNAPSHOT_TRANSFORM_BUFFER;
            mLocalBufferList.push_back(&(mLocalBuffer[count + j]));
        }
    }
    count += SNAP_TRANSF_NUM;
#endif

    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {

        if (mDepthPrevFormat == MODE_WEIGHTMAP) {
            w = mPreviewWidth;
            h = mPreviewHeight;
        } else {
            w = mDepthOutWidth;
            h = mDepthOutHeight;
        }

        mSprdDepthBuffer[0] = malloc(w * h * 2);
        if (mSprdDepthBuffer[0] == NULL) {
            HAL_LOGE("mSprdDepthBuffer[0]  malloc failed.");
            goto mem_fail;
        } else {
            memset(mSprdDepthBuffer[0], 0, sizeof(w * h * 2));
        }

        mSprdDepthBuffer[1] =
            malloc(mDepthSnapOutWidth * mDepthSnapOutHeight + 68);
        if (mSprdDepthBuffer[1] == NULL) {
            HAL_LOGE("mSprdDepthBuffer[1] malloc failed.");
            goto mem_fail;
        } else {
            memset(mSprdDepthBuffer[1], 0,
                   sizeof(mDepthSnapOutWidth * mDepthSnapOutHeight + 68));
        }
    }

    HAL_LOGI(":X");

    return rc;

mem_fail:

    freeLocalBuffer();
    mLocalBufferList.clear();
    return -1;
}

/*===========================================================================
 * FUNCTION         : freeLocalBuffer
 *
 * DESCRIPTION     : free native_handle_t buffer
 *
 * PARAMETERS:
 *
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera3RealBokeh::freeLocalBuffer() {
    for (int i = 0; i < mLocalBufferNumber; i++) {
        freeOneBuffer(&mLocalBuffer[i]);
    }

    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
        if (mSprdDepthBuffer[0] != NULL) {
            free(mSprdDepthBuffer[0]);
            mSprdDepthBuffer[0] = NULL;
        }
        if (mSprdDepthBuffer[1] != NULL) {
            free(mSprdDepthBuffer[1]);
            mSprdDepthBuffer[1] = NULL;
        }
    }
}

/*===========================================================================
 * FUNCTION   : cameraDeviceOpen
 *
 * DESCRIPTION: open a camera device with its ID
 *
 * PARAMETERS :
 *   @camera_id : camera ID
 *   @hw_device : ptr to struct storing camera hardware device info
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3RealBokeh::cameraDeviceOpen(__unused int camera_id,
                                           struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    uint8_t phyId = 0;

    HAL_LOGI(" E");
    hw_device_t *hw_dev[m_nPhyCameras];
    mCameraId = CAM_BOKEH_MAIN_ID;
    m_VirtualCamera.id = (uint8_t)CAM_BOKEH_MAIN_ID;
    // Open all physical cameras
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        phyId = m_pPhyCamera[i].id;
        SprdCamera3HWI *hw = new SprdCamera3HWI((int)phyId);
        if (!hw) {
            HAL_LOGE("Allocation of hardware interface failed");
            return NO_MEMORY;
        }
        hw_dev[i] = NULL;

        hw->setMultiCameraMode(MODE_BOKEH);
        rc = hw->openCamera(&hw_dev[i]);
        if (rc != NO_ERROR) {
            HAL_LOGE("failed, camera id:%d", phyId);
            delete hw;
            closeCameraDevice();
            return rc;
        }

        HAL_LOGD("open id=%d success", phyId);
        m_pPhyCamera[i].dev = reinterpret_cast<camera3_device_t *>(hw_dev[i]);
        m_pPhyCamera[i].hwi = hw;
    }

    m_VirtualCamera.dev.common.tag = HARDWARE_DEVICE_TAG;
    m_VirtualCamera.dev.common.version = CAMERA_DEVICE_API_VERSION_3_2;
    m_VirtualCamera.dev.common.close = close_camera_device;
    m_VirtualCamera.dev.ops = &mCameraCaptureOps;
    m_VirtualCamera.dev.priv = (void *)&m_VirtualCamera;
    *hw_device = &m_VirtualCamera.dev.common;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.sys.cam.api.version", prop, "0");
    mApiVersion = atoi(prop);
    HAL_LOGD("api version %d", mApiVersion);
    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
        mBokehCapApi = (BokehAPI_t *)malloc(sizeof(BokehAPI_t));
        if (mBokehCapApi == NULL) {
            HAL_LOGE("mBokehCapApi malloc failed.");
        } else {
            memset(mBokehCapApi, 0, sizeof(BokehAPI_t));
        }
        if (loadBokehApi() < 0) {
            HAL_LOGE("load bokeh api failed.");
        }
        mDepthApi = (depth_api_t *)malloc(sizeof(depth_api_t));
        if (mDepthApi == NULL) {
            HAL_LOGE("mDepthApi malloc failed.");
        } else {
            memset(mDepthApi, 0, sizeof(depth_api_t));
        }
        if (loadDepthApi() < 0) {
            HAL_LOGE("load depth api failed.");
        }
        mBokehPrevApi = (BokehPreviewAPI_t *)malloc(sizeof(BokehPreviewAPI_t));
        if (mBokehPrevApi == NULL) {
            HAL_LOGE("mBokehPrevApi malloc failed.");
        } else {
            memset(mBokehPrevApi, 0, sizeof(BokehPreviewAPI_t));
        }
        if (loadBokehPreviewApi() < 0) {
            HAL_LOGE("load bokeh preview api failed.");
        }
    } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        mArcSoftBokehApi =
            (ArcSoftBokehAPI_t *)malloc(sizeof(ArcSoftBokehAPI_t));
        if (mArcSoftBokehApi == NULL) {
            HAL_LOGE("mArcSoftBokehApi malloc failed.");
        } else {
            memset(mArcSoftBokehApi, 0, sizeof(ArcSoftBokehAPI_t));
        }
        if (loadBokehApi() < 0) {
            HAL_LOGE("load arcsoft bokeh api failed.");
        }
    }
    mLocalBufferNumber = LOCAL_BUFFER_NUM;
    HAL_LOGI("X");
    return rc;
}

/*===========================================================================
 * FUNCTION   : getCameraInfo
 *
 * DESCRIPTION: query camera information with its ID
 *
 * PARAMETERS :
 *   @camera_id : camera ID
 *   @info      : ptr to camera info struct
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3RealBokeh::getCameraInfo(struct camera_info *info) {
    int rc = NO_ERROR;
    int camera_id = 0;
    int32_t img_size = 0;
    int version = SPRD_API_MODE;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };

    HAL_LOGI("E, camera_id = %d", camera_id);
    m_VirtualCamera.id = CAM_BOKEH_MAIN_ID;
    camera_id = (int)m_VirtualCamera.id;
    SprdCamera3Setting::initDefaultParameters(camera_id);
    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }

    CameraMetadata metadata = mStaticMetadata;
    property_get("persist.sys.cam.api.version", prop, "0");
    version = atoi(prop);
    if (version == SPRD_API_MODE) {
        img_size =
            SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size * 2 +
            (DEPTH_SNAP_OUTPUT_WIDTH * DEPTH_SNAP_OUTPUT_HEIGHT +
             DEPTH_DATA_SIZE) +
            (BOKEH_REFOCUS_COMMON_PARAM_NUM * 4) + sizeof(camera3_jpeg_blob_t) +
            1024;
    } else if (version == ARCSOFT_API_MODE) {
        img_size =
            SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size * 2 +
            (ARCSOFT_DEPTH_DATA_SIZE) +
            (ARCSOFT_BOKEH_REFOCUS_COMMON_PARAM_NUM * 4) +
            sizeof(camera3_jpeg_blob_t) + 1024;
    }
    SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size = img_size;
    metadata.update(
        ANDROID_JPEG_MAX_SIZE,
        &(SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size), 1);
    mStaticMetadata = metadata.release();

    SprdCamera3Setting::getCameraInfo(camera_id, info);

    info->device_version =
        CAMERA_DEVICE_API_VERSION_3_2; // CAMERA_DEVICE_API_VERSION_3_0;
    info->static_camera_characteristics = mStaticMetadata;
    info->conflicting_devices_length = 0;
    HAL_LOGI("X rc=%d", rc);

    return rc;
}

/*===========================================================================
 * FUNCTION         : getDepthImageSize
 *
 * DESCRIPTION     : getDepthImageSize
 *
 *==========================================================================*/
void SprdCamera3RealBokeh::getDepthImageSize(int inputWidth, int inputHeight,
                                             int *outWidth, int *outHeight,
                                             int type) {
    if (mApiVersion == SPRD_API_MODE) {
        int aspect_ratio = (inputWidth * SFACTOR) / inputHeight;
        if (abs(aspect_ratio - AR4_3) < 1) {
            *outWidth = 240;
            *outHeight = 180;
        } else if (abs(aspect_ratio - AR16_9) < 1) {
            *outWidth = 320;
            *outHeight = 180;
        } else {
            *outWidth = 240;
            *outHeight = 180;
        }
        // TODO:Remove this after depth engine support 16:9
        if (type == PREVIEW_STREAM) {
            *outWidth = 320;  // 800
            *outHeight = 240; // 600
        } else if (type == SNAPSHOT_STREAM) {
            *outWidth = 800;
            *outHeight = 600;
        }
    } else if (mApiVersion == ARCSOFT_API_MODE) {
        if (type == PREVIEW_STREAM) {
            char prop[PROPERTY_VALUE_MAX] = {
                0,
            };
            int value = 0;
            property_get("persist.sys.camera.aux.res", prop, "1");
            value = atoi(prop);
            if (1 == value) {
                *outWidth = inputWidth;
                *outHeight = inputHeight;
            } else if (2 == value) {
                *outWidth = 800;
                *outHeight = 600;
            } else if (3 == value) {
                *outWidth = 1280;
                *outHeight = 720;
            } else if (4 == value) {
                *outWidth = 1440;
                *outHeight = 1080;
            } else if (5 == value) {
                *outWidth = 1920;
                *outHeight = 1080;
            }
        } else if (type == SNAPSHOT_STREAM) {
            *outWidth = 2592;
            *outHeight = 1944;
        }
    }

    HAL_LOGD("iw:%d,ih:%d,ow:%d,oh:%d", inputWidth, inputHeight, *outWidth,
             *outHeight);
}

/*===========================================================================
 * FUNCTION         : setupPhysicalCameras
 *
 * DESCRIPTION     : Creates Camera Capture if not created
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3RealBokeh::setupPhysicalCameras() {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[m_nPhyCameras];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }
    memset(m_pPhyCamera, 0x00,
           (m_nPhyCameras * sizeof(sprdcamera_physical_descriptor_t)));
    m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].id = (uint8_t)CAM_BOKEH_MAIN_ID;
    m_pPhyCamera[CAM_TYPE_DEPTH].id = (uint8_t)CAM_DEPTH_ID;

    return NO_ERROR;
}
/*===========================================================================
 * FUNCTION   :PreviewMuxerThread
 *
 * DESCRIPTION: constructor of PreviewMuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3RealBokeh::PreviewMuxerThread::PreviewMuxerThread() {
    HAL_LOGI("E");
    mPreviewMuxerMsgList.clear();
    mPrevDepthhandle = NULL;
    mArcSoftPrevHandle = NULL;
    memset(&mPreviewbokehParam, 0, sizeof(bokeh_prev_params_t));
    memset(&mArcSoftPrevParam, 0, sizeof(ARC_DCVR_PARAM));
    mPrevAfState = -1;
    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :~PreviewMuxerThread
 *
 * DESCRIPTION: deconstructor of PreviewMuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3RealBokeh::PreviewMuxerThread::~PreviewMuxerThread() {
    HAL_LOGI(" E");

    mPreviewMuxerMsgList.clear();

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :threadLoop
 *
 * DESCRIPTION: threadLoop
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/

bool SprdCamera3RealBokeh::PreviewMuxerThread::threadLoop() {
    muxer_queue_msg_t muxer_msg;
    buffer_handle_t *output_buffer = NULL;
    void *depth_output_buffer = NULL;
    uint32_t frame_number = 0;
    int rc = 0;

    while (!mPreviewMuxerMsgList.empty()) {
        List<muxer_queue_msg_t>::iterator it;
        {
            Mutex::Autolock l(mMergequeueMutex);
            it = mPreviewMuxerMsgList.begin();
            muxer_msg = *it;
            mPreviewMuxerMsgList.erase(it);
        }
        switch (muxer_msg.msg_type) {
        case MUXER_MSG_EXIT: {
            List<multi_request_saved_t>::iterator itor =
                mRealBokeh->mSavedRequestList.begin();
            HAL_LOGD("exit frame_number %u", itor->frame_number);
            while (itor != mRealBokeh->mSavedRequestList.end()) {
                frame_number = itor->frame_number;
                itor++;
                mRealBokeh->CallBackResult(frame_number,
                                           CAMERA3_BUFFER_STATUS_ERROR);
            }
        }
            return false;
        case MUXER_MSG_DATA_PROC: {
            {
                Mutex::Autolock l(mRealBokeh->mRequestLock);
                List<multi_request_saved_t>::iterator itor =
                    mRealBokeh->mSavedRequestList.begin();
                HAL_LOGV("muxer_msg.combo_frame.frame_number %d",
                         muxer_msg.combo_frame.frame_number);
                while (itor != mRealBokeh->mSavedRequestList.end()) {
                    if (itor->frame_number ==
                        muxer_msg.combo_frame.frame_number) {
                        output_buffer = itor->buffer;
                        mRealBokeh->mPrevFrameNumber =
                            muxer_msg.combo_frame.frame_number;
                        frame_number = muxer_msg.combo_frame.frame_number;
                        break;
                    }
                    itor++;
                }
            }
            if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
                depth_output_buffer = mRealBokeh->mSprdDepthBuffer[0];
            }

            if (output_buffer != NULL) {
                void *input_buf1_addr = NULL;
                rc = mRealBokeh->map(muxer_msg.combo_frame.buffer1,
                                     &input_buf1_addr);
                if (rc != NO_ERROR) {
                    HAL_LOGE("fail to map input buffer1");
                    return false;
                }

                if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
                    CONTROL_Tag controlInfo;
                    mRealBokeh->m_pPhyCamera[CAM_TYPE_MAIN]
                        .hwi->mSetting->getCONTROLTag(&controlInfo);
                    int currAfState = controlInfo.af_state;
                    if (mPrevAfState != currAfState) {
                        mRealBokeh->mUpdateDepthFlag = true;
                        mPrevAfState = currAfState;
                    }
                    if (mRealBokeh->mIsSupportPBokeh &&
                        mRealBokeh->mUpdateDepthFlag) {
                        rc = depthPreviewHandle(depth_output_buffer, NULL,
                                                muxer_msg.combo_frame.buffer1,
                                                input_buf1_addr,
                                                muxer_msg.combo_frame.buffer2);
                        if (rc != NO_ERROR) {
                            HAL_LOGE("depthPreviewHandle failed");
                            mRealBokeh->unmap(muxer_msg.combo_frame.buffer1);
                            return false;
                        }
                    }
                    rc = bokehPreviewHandle(
                        output_buffer, muxer_msg.combo_frame.buffer1,
                        input_buf1_addr, depth_output_buffer);
                    if (rc != NO_ERROR) {
                        HAL_LOGE("bokehPreviewHandle failed");
                        mRealBokeh->unmap(muxer_msg.combo_frame.buffer1);
                        return false;
                    }
                } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
                    rc = depthPreviewHandle(
                        NULL, output_buffer, muxer_msg.combo_frame.buffer1,
                        input_buf1_addr, muxer_msg.combo_frame.buffer2);
                    if (rc != NO_ERROR) {
                        HAL_LOGE("depthPreviewHandle failed");
                        mRealBokeh->unmap(muxer_msg.combo_frame.buffer1);
                        return false;
                    }
                }
                mRealBokeh->unmap(muxer_msg.combo_frame.buffer1);
                mRealBokeh->pushBufferList(mRealBokeh->mLocalBuffer,
                                           muxer_msg.combo_frame.buffer1,
                                           mRealBokeh->mLocalBufferNumber,
                                           mRealBokeh->mLocalBufferList);
                mRealBokeh->pushBufferList(mRealBokeh->mLocalBuffer,
                                           muxer_msg.combo_frame.buffer2,
                                           mRealBokeh->mLocalBufferNumber,
                                           mRealBokeh->mLocalBufferList);
                mRealBokeh->CallBackResult(frame_number,
                                           CAMERA3_BUFFER_STATUS_OK);
            }
        } break;
        default:
            break;
        }
    };

    waitMsgAvailable();

    return true;
}

/*===========================================================================
 * FUNCTION   :depthPreviewHandle
 *
 * DESCRIPTION: depthPreviewHandle
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3RealBokeh::PreviewMuxerThread::depthPreviewHandle(
    void *depth_output_buffer, buffer_handle_t *output_buffer,
    buffer_handle_t *input_buf1, void *input_buf1_addr,
    buffer_handle_t *input_buf2) {
    int rc = NO_ERROR;
    void *output_buffer_addr = NULL;
    void *input_buf2_addr = NULL;

    char acVersion[256] = {
        0,
    };

    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
        if (input_buf1 == NULL || input_buf2 == NULL ||
            depth_output_buffer == NULL) {
            HAL_LOGE("buffer is NULL!");
            return BAD_VALUE;
        }
    } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        if (input_buf1 == NULL || input_buf2 == NULL || output_buffer == NULL) {
            HAL_LOGE("buffer is NULL!");
            return BAD_VALUE;
        }
    }

    if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        rc = mRealBokeh->map(output_buffer, &output_buffer_addr);
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to map output buffer");
            goto fail_map_output;
        }
    }
    rc = mRealBokeh->map(input_buf2, &input_buf2_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map input buffer2");
        goto fail_map_input2;
    }

    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
        int64_t depthRun = systemTime();
        distanceRet distance;
        rc = mRealBokeh->mDepthApi->sprd_depth_Run_distance(
            mPrevDepthhandle, depth_output_buffer, NULL, input_buf2_addr,
            input_buf1_addr, &(mPreviewbokehParam.depth_param), &distance);
        if (rc != ALRNB_ERR_SUCCESS) {
            HAL_LOGE("sprd_depth_Run failed! %d", rc);
            goto fail_depth_run;
        }

        HAL_LOGD("depth run cost %lld ms", ns2ms(systemTime() - depthRun));
        {
            char prop9[PROPERTY_VALUE_MAX] = {
                0,
            };
            property_get("persist.sys.camera.savyuv", prop9, "0");
            if (1 == atoi(prop9)) {
                mRealBokeh->dumpData(
                    (unsigned char *)depth_output_buffer, 1,
                    mRealBokeh->mPreviewWidth * mRealBokeh->mPreviewHeight * 2,
                    mRealBokeh->mPreviewWidth, mRealBokeh->mPreviewHeight,
                    mRealBokeh->mPrevFrameNumber, "output");
                mRealBokeh->dumpData((unsigned char *)(input_buf2_addr), 1,
                                     ADP_BUFSIZE(*input_buf2),
                                     mRealBokeh->mDepthPrevImageWidth,
                                     mRealBokeh->mDepthPrevImageHeight,
                                     mRealBokeh->mPrevFrameNumber, "prevSub");
                mRealBokeh->dumpData((unsigned char *)(input_buf1_addr), 1,
                                     ADP_BUFSIZE(*input_buf1),
                                     mRealBokeh->mPreviewWidth,
                                     mRealBokeh->mPreviewHeight,
                                     mRealBokeh->mPrevFrameNumber, "prevMain");
            }
        }
        mRealBokeh->mUpdateDepthFlag = false;
    } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        HAL_LOGV("arcsoft preview bokeh run began");
        MRESULT rc = MOK;
        ASVLOFFSCREEN leftImg, rightImg, dstImg;

        MLong lWidth = 0, lHeight = 0;
        MByte *pLeftImgDataY = NULL, *pLeftImgDataUV = NULL,
              *pRightImgDataY = NULL, *pRightImgDataUV = NULL,
              *pDstDataY = NULL, *pDstDataUV = NULL;

        lWidth = (MLong)(mRealBokeh->mPreviewWidth);
        lHeight = (MLong)(mRealBokeh->mPreviewHeight);
        pLeftImgDataY = (MByte *)(input_buf1_addr);
        pLeftImgDataUV = pLeftImgDataY + lWidth * lHeight;

        pRightImgDataY = (MByte *)(input_buf2_addr);
        pRightImgDataUV = pRightImgDataY + lWidth * lHeight;

        pDstDataY = (MByte *)(output_buffer_addr);
        pDstDataUV = pDstDataY + lWidth * lHeight;

        memset(&leftImg, 0, sizeof(ASVLOFFSCREEN));
        leftImg.i32Width = lWidth;
        leftImg.i32Height = lHeight;
        leftImg.u32PixelArrayFormat = ASVL_PAF_NV21;
        leftImg.pi32Pitch[0] = lWidth;
        leftImg.ppu8Plane[0] = pLeftImgDataY;
        leftImg.pi32Pitch[1] = lWidth / 2 * 2;
        leftImg.ppu8Plane[1] = pLeftImgDataUV;

        memset(&rightImg, 0, sizeof(ASVLOFFSCREEN));
        rightImg.i32Width = lWidth;
        rightImg.i32Height = lHeight;
        rightImg.u32PixelArrayFormat = ASVL_PAF_NV21;
        rightImg.pi32Pitch[0] = lWidth;
        rightImg.ppu8Plane[0] = pRightImgDataY;
        rightImg.pi32Pitch[1] = lWidth / 2 * 2;
        rightImg.ppu8Plane[1] = pRightImgDataUV;

        memset(&dstImg, 0, sizeof(ASVLOFFSCREEN));
        dstImg.i32Width = lWidth;
        dstImg.i32Height = lHeight;
        dstImg.u32PixelArrayFormat = ASVL_PAF_NV21;
        dstImg.pi32Pitch[0] = lWidth;
        dstImg.ppu8Plane[0] = pDstDataY;
        dstImg.pi32Pitch[1] = lWidth / 2 * 2;
        dstImg.ppu8Plane[1] = pDstDataUV;

        int64_t arcsoftprevRun = systemTime();
        rc = mRealBokeh->mArcSoftBokehApi->ARC_DCVR_PrevProcess(
            mArcSoftPrevHandle, &leftImg, &rightImg, &dstImg,
            &mArcSoftPrevParam);
        if ((rc != MOK) && (rc == MERR_BAD_STATE)) {
            HAL_LOGW("there is no calibration file");
            // if no calibration file just return left image.
            memcpy(pDstDataY, pLeftImgDataY, ADP_BUFSIZE(*input_buf1));
        } else {
            HAL_LOGV("arcsoft ARC_DCVR_PrevProcess ,rc = %ld", rc);
        }
        mRealBokeh->flushIonBuffer(ADP_BUFFD(*output_buffer),
                                   output_buffer_addr,
                                   ADP_BUFSIZE(*output_buffer));
        HAL_LOGD("arcsoftprevRun cost %lld ms",
                 ns2ms(systemTime() - arcsoftprevRun));
        {
            char prop9[PROPERTY_VALUE_MAX] = {
                0,
            };
            property_get("persist.sys.camera.savyuv", prop9, "0");
            if (1 == atoi(prop9)) {
                mRealBokeh->dumpData(
                    (unsigned char *)output_buffer_addr, 1,
                    ADP_BUFSIZE(*output_buffer), mRealBokeh->mPreviewWidth,
                    mRealBokeh->mPreviewHeight, arcsoftprevRun, "output");
                mRealBokeh->dumpData((unsigned char *)(input_buf2_addr), 1,
                                     ADP_BUFSIZE(*input_buf2),
                                     mRealBokeh->mDepthPrevImageWidth,
                                     mRealBokeh->mDepthPrevImageHeight,
                                     arcsoftprevRun, "PrevSub");
                mRealBokeh->dumpData(
                    (unsigned char *)(input_buf1_addr), 1,
                    ADP_BUFSIZE(*input_buf1), mRealBokeh->mPreviewWidth,
                    mRealBokeh->mPreviewHeight, arcsoftprevRun, "PrevMain");
            }
        }
        HAL_LOGV("arcsoft preview bokeh run end");
    }

fail_depth_run:
    mRealBokeh->unmap(input_buf2);
fail_map_input2:
    if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        mRealBokeh->unmap(output_buffer);
    }
fail_map_output:

    return rc;
}

/*===========================================================================
 * FUNCTION   :bokehPreviewHandle
 *
 * DESCRIPTION: bokehPreviewHandle
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3RealBokeh::PreviewMuxerThread::bokehPreviewHandle(
    buffer_handle_t *output_buf, buffer_handle_t *input_buf1,
    void *input_buf1_addr, void *depth_buffer) {
    int rc = NO_ERROR;
    void *output_buf_addr = NULL;
    Mutex::Autolock l(mLock);
    HAL_LOGI(":E");

    if (output_buf == NULL || input_buf1 == NULL || depth_buffer == NULL) {
        HAL_LOGE("buffer is NULL!");
        return BAD_VALUE;
    }
    rc = mRealBokeh->map(output_buf, &output_buf_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map output buffer");
        goto fail_map_output;
    }
    if (mRealBokeh->mIsSupportPBokeh) {
        /*Bokeh GPU interface start*/
        uint32_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                                GraphicBuffer::USAGE_SW_READ_OFTEN |
                                GraphicBuffer::USAGE_SW_WRITE_OFTEN;
        int32_t yuvTextFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        uint32_t inWidth = 0, inHeight = 0, inStride = 0;

#if defined(CONFIG_SPRD_ANDROID_8)
        uint32_t inLayCount = 1;
#endif
        inWidth = mRealBokeh->mPreviewWidth;
        inHeight = mRealBokeh->mPreviewHeight;
        inStride = mRealBokeh->mPreviewWidth;
#if defined(CONFIG_SPRD_ANDROID_8)
        sp<GraphicBuffer> srcBuffer = new GraphicBuffer(
            (native_handle_t *)(*input_buf1),
            GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, inWidth, inHeight,
            yuvTextFormat, inLayCount, yuvTextUsage, inStride);
        sp<GraphicBuffer> dstBuffer = new GraphicBuffer(
            (native_handle_t *)(*output_buf),
            GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, inWidth, inHeight,
            yuvTextFormat, inLayCount, yuvTextUsage, inStride);
#else
        sp<GraphicBuffer> srcBuffer =
            new GraphicBuffer(inWidth, inHeight, yuvTextFormat, yuvTextUsage,
                              inStride, (native_handle_t *)(*input_buf1), 0);
        sp<GraphicBuffer> dstBuffer =
            new GraphicBuffer(inWidth, inHeight, yuvTextFormat, yuvTextUsage,
                              inStride, (native_handle_t *)(*output_buf), 0);
#endif
        mPreviewbokehParam.weight_params.DisparityImage =
            (unsigned char *)depth_buffer;

        int64_t bokehCreateWeightMap = systemTime();
        rc = mRealBokeh->mBokehPrevApi->iBokehCreateWeightMap(
            mRealBokeh->mBokehPrevApi->mHandle,
            &(mPreviewbokehParam.weight_params));
        if (rc != ALRNB_ERR_SUCCESS) {
            HAL_LOGE("iBokehCreateWeightMap failed!");
            goto fail_create_weightmap;
        }
        HAL_LOGD("iBokehCreateWeightMap cost %lld ms",
                 ns2ms(systemTime() - bokehCreateWeightMap));
        int64_t bokehBlurImage = systemTime();
        rc = mRealBokeh->mBokehPrevApi->iBokehBlurImage(
            mRealBokeh->mBokehPrevApi->mHandle, &(*srcBuffer), &(*dstBuffer));

        if (rc != ALRNB_ERR_SUCCESS) {
            HAL_LOGE("iBokehBlurImage failed!");
            goto fail_blur_image;
        }
        HAL_LOGD("iBokehBlurImage cost %lld ms",
                 ns2ms(systemTime() - bokehBlurImage));
        {
            char prop8[PROPERTY_VALUE_MAX] = {
                0,
            };
            property_get("persist.sys.camera.bokeh8", prop8, "0");
            if (1 == atoi(prop8)) {
                mRealBokeh->dumpData((unsigned char *)(output_buf_addr), 1,
                                     ADP_BUFSIZE(*output_buf),
                                     mRealBokeh->mPreviewWidth,
                                     mRealBokeh->mPreviewHeight, 0, "bokeh");
            }
        }

    } else {
        memcpy(output_buf_addr, input_buf1_addr, ADP_BUFSIZE(*input_buf1));
    }
    HAL_LOGI(":X");

fail_blur_image:
fail_create_weightmap:
    mRealBokeh->unmap(output_buf);
fail_map_output:

    return rc;
}

/*===========================================================================
 * FUNCTION   :requestExit
 *
 * DESCRIPTION: request thread exit
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::PreviewMuxerThread::requestExit() {
    HAL_LOGI("E");
    Mutex::Autolock l(mMergequeueMutex);
    muxer_queue_msg_t muxer_msg;
    muxer_msg.msg_type = MUXER_MSG_EXIT;
    mPreviewMuxerMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
}

/*===========================================================================
 * FUNCTION   :waitMsgAvailable
 *
 * DESCRIPTION: wait util list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3RealBokeh::PreviewMuxerThread::waitMsgAvailable() {
    while (mPreviewMuxerMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, BOKEH_THREAD_TIMEOUT);
    }
}

/*===========================================================================
 * FUNCTION   :CaptureThread
 *
 * DESCRIPTION: constructor of CaptureThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3RealBokeh::BokehCaptureThread::BokehCaptureThread() {
    HAL_LOGI(" E");
    mSavedResultBuff = NULL;
    mSavedCapReqsettings = NULL;
    mReprocessing = false;
    mBokehConfigParamState = false;
    mCapDepthhandle = NULL;
    mArcSoftCapHandle = NULL;
    mArcSoftDepthMap = NULL;
    mArcSoftDepthSize = 0;
    mSavedOneResultBuff = NULL;
    mDevmain = NULL;
    mCallbackOps = NULL;
    char prop1[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.sys.gallery.abokeh", prop1, "0");
    if (1 == atoi(prop1)) {
        mAbokehGallery = true;
    } else {
        mAbokehGallery = false;
    }
    mBokehResult = true;
    memset(&mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
    memset(&mSavedCapReqStreamBuff, 0, sizeof(camera3_stream_buffer_t));
    memset(&mCapbokehParam, 0, sizeof(bokeh_cap_params_t));
    memset(&mArcSoftDcrParam, 0, sizeof(ARC_DCIR_PARAM));
    memset(&mArcSoftCapFace, 0, sizeof(ARC_DCIR_FACE_PARAM));
    memset(&mArcSoftCapParam, 0, sizeof(ARC_DCIR_REFOCUS_PARAM));
    mCaptureMsgList.clear();
    HAL_LOGI(" X");
}

/*===========================================================================
 * FUNCTION   :~~CaptureThread
 *
 * DESCRIPTION: deconstructor of CaptureThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3RealBokeh::BokehCaptureThread::~BokehCaptureThread() {
    HAL_LOGI(" E");
    mCaptureMsgList.clear();
    HAL_LOGI(" X");
}

/*===========================================================================
 * FUNCTION   :saveCaptureBokehParams
 *
 * DESCRIPTION: save capture bokeh params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3RealBokeh::BokehCaptureThread::saveCaptureBokehParams(
    unsigned char *result_buffer_addr, uint32_t result_buffer_size,
    size_t jpeg_size) {
    int i = 0;
    int ret = NO_ERROR;
    uint32_t depth_size = 0;
    int para_size = 0;
    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
        if (result_buffer_addr == NULL) {
            HAL_LOGE("result_buff is NULL!");
            return BAD_VALUE;
        }
        para_size = BOKEH_REFOCUS_COMMON_PARAM_NUM * 4;
        depth_size =
            mRealBokeh->mDepthSnapOutWidth * mRealBokeh->mDepthSnapOutHeight +
            DEPTH_DATA_SIZE;
    } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        if (result_buffer_addr == NULL) {
            HAL_LOGE("result_buff is NULL!");
            return BAD_VALUE;
        }
        para_size = ARCSOFT_BOKEH_REFOCUS_COMMON_PARAM_NUM * 4;
        depth_size = mArcSoftDepthSize;
    }

    unsigned char *buffer_base = result_buffer_addr;
    ;
    unsigned char *depth_yuv = NULL;
#ifdef YUV_CONVERT_TO_JPEG
    unsigned char *orig_jpeg_data = NULL;
#else
    unsigned char *orig_yuv_data = NULL;
#endif
    uint32_t orig_yuv_size =
        mRealBokeh->mCaptureWidth * mRealBokeh->mCaptureHeight * 3 / 2;
#ifdef YUV_CONVERT_TO_JPEG
    uint32_t use_size =
        para_size + depth_size + mRealBokeh->mOrigJpegSize + jpeg_size;
    uint32_t orig_jpeg_size = mRealBokeh->mOrigJpegSize;
    uint32_t process_jpeg = jpeg_size;
#else
    uint32_t use_size = para_size + depth_size + orig_yuv_size + jpeg_size;
#endif
    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
        depth_yuv = (unsigned char *)mRealBokeh->m_pSprdDepthBuffer;

        // refocus commom param
        uint32_t main_width = mRealBokeh->mCaptureWidth;
        uint32_t main_height = mRealBokeh->mCaptureHeight;
        uint32_t depth_width = mRealBokeh->mDepthSnapOutWidth;
        uint32_t depth_height = mRealBokeh->mDepthSnapOutHeight;
        uint32_t bokeh_level = mCapbokehParam.bokeh_level;
        uint32_t position_x = mCapbokehParam.sel_x;
        uint32_t position_y = mCapbokehParam.sel_y;
        uint32_t param_state = mBokehConfigParamState;
        uint32_t rotation = SprdCamera3Setting::s_setting[mRealBokeh->mCameraId]
                                .jpgInfo.orientation;
        unsigned char bokeh_flag[] = {'B', 'O', 'K', 'E'};
        depth_size = depth_width * depth_height + DEPTH_DATA_SIZE;
        unsigned char *p[] = {
            (unsigned char *)&main_width,   (unsigned char *)&main_height,
            (unsigned char *)&depth_width,  (unsigned char *)&depth_height,
            (unsigned char *)&depth_size,   (unsigned char *)&bokeh_level,
            (unsigned char *)&position_x,   (unsigned char *)&position_y,
            (unsigned char *)&param_state,  (unsigned char *)&rotation,
#ifdef YUV_CONVERT_TO_JPEG
            (unsigned char *)&process_jpeg, (unsigned char *)&orig_jpeg_size,
#endif
            (unsigned char *)&bokeh_flag};
        HAL_LOGD("sprd param: %d ,%d , %d, %d, %d, %d, %d, %d, %d, %d, %s",
                 main_width, main_height, depth_width, depth_height, depth_size,
                 bokeh_level, position_x, position_y, param_state, rotation,
                 bokeh_flag);
#ifdef YUV_CONVERT_TO_JPEG
        HAL_LOGD("process_jpeg %d,orig_jpeg_size %d", process_jpeg,
                 orig_jpeg_size);
#endif

        // cpoy common param to tail
        buffer_base += (use_size - BOKEH_REFOCUS_COMMON_PARAM_NUM * 4);
        for (i = 0; i < BOKEH_REFOCUS_COMMON_PARAM_NUM; i++) {
            memcpy(buffer_base + i * 4, p[i], 4);
        }

    } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        // refocus commom param
        uint32_t main_width = mRealBokeh->mCaptureWidth;
        uint32_t main_height = mRealBokeh->mCaptureHeight;
        uint32_t i32BlurIntensity = mArcSoftCapParam.i32BlurIntensity;
        uint32_t position_x = mArcSoftCapParam.ptFocus.x;
        uint32_t position_y = mArcSoftCapParam.ptFocus.y;
        uint32_t rotation = SprdCamera3Setting::s_setting[mRealBokeh->mCameraId]
                                .jpgInfo.orientation;
        unsigned char bokeh_flag[] = {'B', 'O', 'K', 'E'};
        depth_size = mArcSoftDepthSize;
        unsigned char *p[] = {
            (unsigned char *)&main_width,   (unsigned char *)&main_height,
            (unsigned char *)&depth_size,   (unsigned char *)&i32BlurIntensity,
            (unsigned char *)&position_x,   (unsigned char *)&position_y,
            (unsigned char *)&rotation,
#ifdef YUV_CONVERT_TO_JPEG
            (unsigned char *)&process_jpeg, (unsigned char *)&orig_jpeg_size,
#endif
            (unsigned char *)&bokeh_flag};
        HAL_LOGD("arcsoft param: %d ,%d , %d, %d, %d, %d, %d, %s", main_width,
                 main_height, depth_size, i32BlurIntensity, position_x,
                 position_y, rotation, bokeh_flag);
#ifdef YUV_CONVERT_TO_JPEG
        HAL_LOGD("process_jpeg %d,orig_jpeg_size %d", process_jpeg,
                 orig_jpeg_size);
#endif
        // cpoy common param to tail
        buffer_base += (use_size - ARCSOFT_BOKEH_REFOCUS_COMMON_PARAM_NUM * 4);
        for (i = 0; i < ARCSOFT_BOKEH_REFOCUS_COMMON_PARAM_NUM; i++) {
            memcpy(buffer_base + i * 4, p[i], 4);
        }

        depth_yuv = (unsigned char *)mArcSoftDepthMap;
    }
    // cpoy depth yuv
    buffer_base -= depth_size;
    memcpy(buffer_base, depth_yuv, depth_size);

#ifdef YUV_CONVERT_TO_JPEG
    // cpoy original jpeg
    ret =
        mRealBokeh->map(mRealBokeh->m_pDstJpegBuffer, (void **)&orig_jpeg_data);
    if (ret != NO_ERROR) {
        HAL_LOGE("fail to map dst jpeg buffer");
        goto fail_map_dst;
    }
    buffer_base -= orig_jpeg_size;
    memcpy(buffer_base, orig_jpeg_data, orig_jpeg_size);
#else
    // cpoy original yuv
    ret =
        mRealBokeh->map(mRealBokeh->m_pMainSnapBuffer, (void **)&orig_yuv_data);
    if (ret != NO_ERROR) {
        HAL_LOGE("fail to map snap buffer");
        goto fail_map_dst;
    }
    buffer_base -= orig_yuv_size;
    memcpy(buffer_base, orig_yuv_data, orig_yuv_size);
#endif

    // blob to indicate all image size(use_size)
    mRealBokeh->setJpegSize((char *)result_buffer_addr, result_buffer_size,
                            use_size);

#ifdef YUV_CONVERT_TO_JPEG
    mRealBokeh->unmap(mRealBokeh->m_pDstJpegBuffer);
#else
    mRealBokeh->unmap(mRealBokeh->m_pMainSnapBuffer);
#endif
fail_map_dst:

    return ret;
}

/*===========================================================================
 * FUNCTION   :reprocessReq
 *
 * DESCRIPTION: reprocessReq
 *
 * PARAMETERS : buffer_handle_t *output_buffer, capture_queue_msg_t_bokeh
 * capture_msg
 *               buffer_handle_t *depth_output_buffer, buffer_handle_t
 * scaled_buffer
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::BokehCaptureThread::reprocessReq(
    buffer_handle_t *output_buffer, capture_queue_msg_t_bokeh capture_msg,
    void *depth_output_buffer, buffer_handle_t *scaled_buffer) {
    camera3_capture_request_t request = mSavedCapRequest;
    camera3_stream_buffer_t output_buffers;
    camera3_stream_buffer_t input_buffer;

    memset(&output_buffers, 0x00, sizeof(camera3_stream_buffer_t));
    memset(&input_buffer, 0x00, sizeof(camera3_stream_buffer_t));

    memcpy(&input_buffer, &mSavedCapReqStreamBuff,
           sizeof(camera3_stream_buffer_t));
    if (mSavedCapReqsettings) {
        request.settings = mSavedCapReqsettings;
    } else {
        HAL_LOGE("metadata is null,failed.");
    }
    input_buffer.stream =
        &(mRealBokeh->mMainStreams[mRealBokeh->mCaptureStreamsNum]);

    memcpy((void *)&output_buffers, &mSavedCapReqStreamBuff,
           sizeof(camera3_stream_buffer_t));
    output_buffers.stream =
        &(mRealBokeh->mMainStreams[mRealBokeh->mCaptureStreamsNum]);
#ifdef BOKEH_YUV_DATA_TRANSFORM
    buffer_handle_t *transform_buffer = NULL;
    if (mRealBokeh->mTransformWidth != 0) {
        transform_buffer = (mRealBokeh->popBufferList(
            mRealBokeh->mLocalBufferList, SNAPSHOT_TRANSFORM_BUFFER));
        buffer_handle_t *output_handle = NULL;
        void *output_handle_addr = NULL;
        void *transform_buffer_addr = NULL;
        if (!mAbokehGallery && mBokehResult) {
            output_handle = output_buffer;
        } else {
            output_handle = capture_msg.combo_buff.buffer1;
        }
        mRealBokeh->map(output_handle, &output_handle_addr);
        mRealBokeh->map(transform_buffer, &transform_buffer_addr);
        int64_t alignTransform = systemTime();
        mRealBokeh->alignTransform(
            output_handle_addr, mRealBokeh->mCaptureWidth,
            mRealBokeh->mCaptureHeight, mRealBokeh->mTransformWidth,
            mRealBokeh->mTransformHeight, transform_buffer_addr);
        HAL_LOGD("alignTransform run cost %lld ms",
                 ns2ms(systemTime() - alignTransform));

        input_buffer.stream->width = mRealBokeh->mTransformWidth;
        input_buffer.stream->height = mRealBokeh->mTransformHeight;
        input_buffer.buffer = transform_buffer;
        output_buffers.stream->width = mRealBokeh->mTransformWidth;
        output_buffers.stream->height = mRealBokeh->mTransformHeight;

        mRealBokeh->unmap(output_handle);
        mRealBokeh->unmap(transform_buffer);
    } else {
        input_buffer.stream->width = mRealBokeh->mCaptureWidth;
        input_buffer.stream->height = mRealBokeh->mCaptureHeight;
        output_buffers.stream->width = mRealBokeh->mCaptureWidth;
        output_buffers.stream->height = mRealBokeh->mCaptureHeight;
        if (!mAbokehGallery && mBokehResult) {
            input_buffer.buffer = output_buffer;
        } else {
            input_buffer.buffer = capture_msg.combo_buff.buffer1;
        }
    }
#else
    input_buffer.stream->width = mRealBokeh->mCaptureWidth;
    input_buffer.stream->height = mRealBokeh->mCaptureHeight;
    output_buffers.stream->width = mRealBokeh->mCaptureWidth;
    output_buffers.stream->height = mRealBokeh->mCaptureHeight;
    if (!mAbokehGallery && mBokehResult) {
        input_buffer.buffer = output_buffer;
    } else {
        input_buffer.buffer = capture_msg.combo_buff.buffer1;
    }

#endif

    request.num_output_buffers = 1;
    request.frame_number = mRealBokeh->mCapFrameNumber;
    request.output_buffers = &output_buffers;
    request.input_buffer = &input_buffer;
    mReprocessing = true;

    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
        ADP_FAKESETBUFATTR_CAMERAONLY(
            *request.output_buffers[0].buffer,
            ADP_BUFSIZE(*mSavedResultBuff) -
                (mRealBokeh->mCaptureWidth * mRealBokeh->mCaptureHeight * 3 /
                     2 +
                 (mRealBokeh->mDepthSnapOutWidth *
                      mRealBokeh->mDepthSnapOutHeight +
                  DEPTH_DATA_SIZE) +
                 BOKEH_REFOCUS_COMMON_PARAM_NUM * 4),
            ADP_WIDTH(*request.output_buffers[0].buffer),
            ADP_HEIGHT(*request.output_buffers[0].buffer));
        HAL_LOGD("capture combined success: framenumber %d",
                 request.frame_number);
        mRealBokeh->pushBufferList(mRealBokeh->mLocalBuffer, scaled_buffer,
                                   mRealBokeh->mLocalBufferNumber,
                                   mRealBokeh->mLocalBufferList);
    } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        ADP_FAKESETBUFATTR_CAMERAONLY(
            *request.output_buffers[0].buffer,
            ADP_BUFSIZE(*mSavedResultBuff) -
                (mRealBokeh->mCaptureWidth * mRealBokeh->mCaptureHeight * 3 /
                     2 +
                 (ARCSOFT_DEPTH_DATA_SIZE) +
                 ARCSOFT_BOKEH_REFOCUS_COMMON_PARAM_NUM * 4),
            ADP_WIDTH(*request.output_buffers[0].buffer),
            ADP_HEIGHT(*request.output_buffers[0].buffer));
        HAL_LOGD("capture combined success: framenumber %d",
                 request.frame_number);
    }
    if (!mAbokehGallery) {
        mRealBokeh->pushBufferList(mRealBokeh->mLocalBuffer, output_buffer,
                                   mRealBokeh->mLocalBufferNumber,
                                   mRealBokeh->mLocalBufferList);
    }
#ifdef BOKEH_YUV_DATA_TRANSFORM
    if (mRealBokeh->mTransformWidth != 0) {
        mRealBokeh->pushBufferList(mRealBokeh->mLocalBuffer, transform_buffer,
                                   mRealBokeh->mLocalBufferNumber,
                                   mRealBokeh->mLocalBufferList);
    }
#endif
    mRealBokeh->pushBufferList(
        mRealBokeh->mLocalBuffer, capture_msg.combo_buff.buffer1,
        mRealBokeh->mLocalBufferNumber, mRealBokeh->mLocalBufferList);
    mRealBokeh->pushBufferList(
        mRealBokeh->mLocalBuffer, capture_msg.combo_buff.buffer2,
        mRealBokeh->mLocalBufferNumber, mRealBokeh->mLocalBufferList);

    if (mRealBokeh->mFlushing) {
        mRealBokeh->CallBackResult(mRealBokeh->mCapFrameNumber,
                                   CAMERA3_BUFFER_STATUS_ERROR);
        goto exit;
    }
    if (0 > mDevmain->hwi->process_capture_request(mDevmain->dev, &request)) {
        HAL_LOGE("failed. process capture request!");
    }

exit:
    if (NULL != mSavedCapReqsettings) {
        free_camera_metadata(mSavedCapReqsettings);
        mSavedCapReqsettings = NULL;
    }
}

/*===========================================================================
 * FUNCTION   :threadLoop
 *
 * DESCRIPTION: threadLoop
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
bool SprdCamera3RealBokeh::BokehCaptureThread::threadLoop() {
    buffer_handle_t *output_buffer = NULL;
    buffer_handle_t *main_buffer = NULL;
    void *depth_output_buffer = NULL;
    buffer_handle_t *scaled_buffer = NULL;
    capture_queue_msg_t_bokeh capture_msg;
    void *src_vir_addr = NULL;
    void *pic_vir_addr = NULL;
    int rc = 0;
    int mime_type = 0;

    while (!mCaptureMsgList.empty()) {
        List<capture_queue_msg_t_bokeh>::iterator itor1 =
            mCaptureMsgList.begin();
        capture_msg = *itor1;
        mCaptureMsgList.erase(itor1);
        switch (capture_msg.msg_type) {
        case BOKEH_MSG_EXIT: {
            // flush queue
            HAL_LOGI("E.BOKEH_MSG_EXIT");
            memset(&mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
            memset(&mSavedCapReqStreamBuff, 0, sizeof(camera3_stream_buffer_t));
            if (NULL != mSavedCapReqsettings) {
                free_camera_metadata(mSavedCapReqsettings);
                mSavedCapReqsettings = NULL;
            }
            HAL_LOGI("X.BOKEH_MSG_EXIT");
            return false;
        }
        case BOKEH_MSG_DATA_PROC: {
            void *input_buf1_addr = NULL;
            rc = mRealBokeh->map(capture_msg.combo_buff.buffer1,
                                 &input_buf1_addr);
            if (rc != NO_ERROR) {
                HAL_LOGE("fail to map input buffer1");
                return false;
            }
#ifdef CONFIG_FACE_BEAUTY
            if (mRealBokeh->mPerfectskinlevel.smoothLevel > 0 &&
                mRealBokeh->mFaceInfo[2] - mRealBokeh->mFaceInfo[0] > 0 &&
                mRealBokeh->mFaceInfo[3] - mRealBokeh->mFaceInfo[1] > 0) {
                mRealBokeh->bokehFaceMakeup(capture_msg.combo_buff.buffer1,
                                            input_buf1_addr);
            }
#endif
            mBokehResult = true;
            mRealBokeh->m_pDstJpegBuffer = (mRealBokeh->popBufferList(
                mRealBokeh->mLocalBufferList, SNAPSHOT_MAIN_BUFFER));

            if (!mAbokehGallery) {
                output_buffer = (mRealBokeh->popBufferList(
                    mRealBokeh->mLocalBufferList, SNAPSHOT_MAIN_BUFFER));
            }

            if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
                depth_output_buffer = mRealBokeh->mSprdDepthBuffer[1];
                scaled_buffer = (mRealBokeh->popBufferList(
                    mRealBokeh->mLocalBufferList, SNAPSHOT_DEPTH_BUFFER));

                rc = depthCaptureHandle(
                    depth_output_buffer, NULL, scaled_buffer,
                    capture_msg.combo_buff.buffer1, input_buf1_addr,
                    capture_msg.combo_buff.buffer2);
                if (rc != NO_ERROR) {
                    HAL_LOGE("depthCaptureHandle failed");
                    mBokehResult = false;
                    mRealBokeh->unmap(capture_msg.combo_buff.buffer1);
                    return false;
                }

                if (!mAbokehGallery) {
                    rc = bokehCaptureHandle(
                        output_buffer, capture_msg.combo_buff.buffer1,
                        input_buf1_addr, depth_output_buffer);
                    if (rc != NO_ERROR) {
                        mRealBokeh->mOrigJpegSize = 0;
                        mBokehResult = false;
                        mime_type = 0;
                        HAL_LOGE("bokehCaptureHandle failed");
                    }
                    mime_type = (int)MODE_BOKEH;
                } else {
                    mime_type = (1 << 8) | (int)MODE_BOKEH;
                }
#ifdef YUV_CONVERT_TO_JPEG
                mRealBokeh->map(mRealBokeh->m_pDstJpegBuffer, &pic_vir_addr);

                mRealBokeh->mOrigJpegSize =
                    mRealBokeh->jpeg_encode_exif_simplify(
                        capture_msg.combo_buff.buffer1, input_buf1_addr,
                        mRealBokeh->m_pDstJpegBuffer, pic_vir_addr, NULL, NULL,
                        mRealBokeh->m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].hwi);
                mRealBokeh->unmap(mRealBokeh->m_pDstJpegBuffer);
#else
                mRealBokeh->m_pMainSnapBuffer = capture_msg.combo_buff.buffer1;
#endif
                mRealBokeh->m_pSprdDepthBuffer = depth_output_buffer;
            } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
                rc = depthCaptureHandle(
                    NULL, output_buffer, NULL, capture_msg.combo_buff.buffer1,
                    input_buf1_addr, capture_msg.combo_buff.buffer2);
                if (rc != NO_ERROR) {
                    HAL_LOGW("depthCaptureHandle error");
                    mRealBokeh->mOrigJpegSize = 0;
                    mBokehResult = false;
                    mime_type = 0;
                } else {
                    if (mAbokehGallery) {
                        mime_type = (1 << 8) | (int)MODE_BOKEH;
                    } else {
                        mime_type = (int)MODE_BOKEH;
                    }
#ifdef YUV_CONVERT_TO_JPEG
                    mRealBokeh->map(mRealBokeh->m_pDstJpegBuffer,
                                    &pic_vir_addr);
                    mRealBokeh->mOrigJpegSize =
                        mRealBokeh->jpeg_encode_exif_simplify(
                            capture_msg.combo_buff.buffer1, input_buf1_addr,
                            mRealBokeh->m_pDstJpegBuffer, pic_vir_addr, NULL,
                            NULL,
                            mRealBokeh->m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].hwi);
                    mRealBokeh->unmap(mRealBokeh->m_pDstJpegBuffer);
#else
                    mRealBokeh->m_pMainSnapBuffer =
                        capture_msg.combo_buff.buffer1;
#endif
                }
            }

            mRealBokeh->unmap(capture_msg.combo_buff.buffer1);
            mDevmain->hwi->camera_ioctrl(CAMERA_IOCTRL_SET_MIME_TYPE,
                                         &mime_type, NULL);
            reprocessReq(output_buffer, capture_msg, depth_output_buffer,
                         scaled_buffer);
        } break;
        default:
            break;
        }
    };
    waitMsgAvailable();

    return true;
}

#ifdef YUV_CONVERT_TO_JPEG
/*===========================================================================
 * FUNCTION   :yuvToJpeg
 *
 * DESCRIPTION: get the size of yuv convert to jpeg
 *
 * PARAMETERS : the buffer_handle_t of yuv
 *
 * RETURN     : jpeg size
 *==========================================================================*/
cmr_uint SprdCamera3RealBokeh::yuvToJpeg(buffer_handle_t *input_handle) {
    img_frm src_img, pic_enc_img;
    cmr_uint size = 0;
    memset(&src_img, 0, sizeof(struct img_frm));
    memset(&pic_enc_img, 0, sizeof(struct img_frm));
    void *temp;
#if 0
    cmr_u8 *addr =
        (cmr_u8 *)(mRealBokeh->map(mCaptureThread->mSavedResultBuff));
    cmr_uint offset = mCallbackWidth * mCallbackHeight * 3 / 2;
    temp = (void *)(addr + mjpegSize - offset);
    cmr_uint addr_phy = (cmr_uint)(mjpegSize - offset);

    convertToImg_frm((void *)addr_phy, temp, mCallbackWidth, mCallbackHeight,
                     ADP_BUFFD(*mCaptureThread->mSavedResultBuff),
                     IMG_DATA_TYPE_JPEG, &pic_enc_img);
    convertToImg_frm(input_handle, &src_img, IMG_DATA_TYPE_YUV420);
    size = jpeg_encode_exif_simplify(&src_img, &pic_enc_img, NULL,
                                     m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].hwi);
#endif
    return size;
}
#endif

/*===========================================================================
 * FUNCTION   :requestExit
 *
 * DESCRIPTION: request thread exit
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::BokehCaptureThread::requestExit() {
    HAL_LOGI("E");
    Mutex::Autolock l(mMergequeueMutex);
    capture_queue_msg_t_bokeh capture_msg;
    capture_msg.msg_type = BOKEH_MSG_EXIT;
    mCaptureMsgList.push_back(capture_msg);
    mMergequeueSignal.signal();
}

/*===========================================================================
 * FUNCTION   :waitMsgAvailable
 *
 * DESCRIPTION: wait util two list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3RealBokeh::BokehCaptureThread::waitMsgAvailable() {
    // TODO:what to do for timeout
    while (mCaptureMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, BOKEH_THREAD_TIMEOUT);
    }
}

/*===========================================================================
 * FUNCTION   :depthCaptureHandle
 *
 * DESCRIPTION: depthCaptureHandle
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3RealBokeh::BokehCaptureThread::depthCaptureHandle(
    void *depth_output_buffer, buffer_handle_t *output_buffer,
    buffer_handle_t *scaled_buffer, buffer_handle_t *input_buf1,
    void *input_buf1_addr, buffer_handle_t *input_buf2) {
    int rc = NO_ERROR;
    void *output_buffer_addr = NULL;
    void *input_buf2_addr = NULL;
    void *scaled_buffer_addr = NULL;
    char acVersion[256] = {
        0,
    };
    HAL_LOGI(":E");

    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
        if (input_buf1 == NULL || input_buf2 == NULL || scaled_buffer == NULL) {
            HAL_LOGE("buffer is NULL!");
            return BAD_VALUE;
        }
    } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        if (input_buf1 == NULL || input_buf2 == NULL) {
            HAL_LOGE("buffer is NULL!");
            return BAD_VALUE;
        }
        if ((!mAbokehGallery) && (output_buffer == NULL)) {
            HAL_LOGE("output_buffer is NULL!");
            return BAD_VALUE;
        }
    }

    if (!mAbokehGallery) {
        if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
            rc = mRealBokeh->map(output_buffer, &output_buffer_addr);
            if (rc != NO_ERROR) {
                HAL_LOGE("fail to map output buffer");
                goto fail_map_output;
            }
        }
    }

    rc = mRealBokeh->map(input_buf2, &input_buf2_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map output buffer");
        goto fail_map_input2;
    }

    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
        rc = mRealBokeh->map(scaled_buffer, &scaled_buffer_addr);
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to map output buffer");
            goto fail_map_scale;
        }
        mRealBokeh->ScaleNV21(
            (uint8_t *)scaled_buffer_addr, mRealBokeh->mDepthSnapImageWidth,
            mRealBokeh->mDepthSnapImageHeight, (uint8_t *)(input_buf1_addr),
            mRealBokeh->mCaptureWidth, mRealBokeh->mCaptureHeight,
            ADP_BUFSIZE(*input_buf1));
        int64_t depthRun = systemTime();
        weightmap_param weightParams;
        weightParams.F_number = mCapbokehParam.bokeh_level;
        weightParams.sel_x = mCapbokehParam.sel_x;
        weightParams.sel_y = mCapbokehParam.sel_y;
        weightParams.DisparityImage = NULL;
        rc = mRealBokeh->mDepthApi->sprd_depth_Run(
            mCapDepthhandle, depth_output_buffer, NULL, input_buf2_addr,
            scaled_buffer_addr, &weightParams);
        if (rc != ALRNB_ERR_SUCCESS) {
            HAL_LOGE("sprd_depth_Run failed! %d", rc);
            goto exit;
        }
        HAL_LOGD("depth run cost %lld ms", ns2ms(systemTime() - depthRun));
    } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        ASVLOFFSCREEN leftImg;
        ASVLOFFSCREEN dstImg;
        ASVLOFFSCREEN rightImg;
        MInt32 lDMSize = 0;
        MLong LWidth = 0, LHeight = 0;
        MLong RWidth = 0, RHeight = 0;
        MByte *pLeftImgDataY = NULL, *pLeftImgDataUV = NULL,
              *pRightImgDataY = NULL, *pRightImgDataUV = NULL;

        LWidth = (MLong)(mRealBokeh->mCaptureWidth);
        LHeight = (MLong)(mRealBokeh->mCaptureHeight);
        RWidth = (MLong)(mRealBokeh->mDepthSnapImageWidth);
        RHeight = (MLong)(mRealBokeh->mDepthSnapImageHeight);
        pLeftImgDataY = (MByte *)input_buf1_addr;
        pLeftImgDataUV = pLeftImgDataY + LWidth * LHeight;

        pRightImgDataY = (MByte *)input_buf2_addr;
        pRightImgDataUV = pRightImgDataY + RWidth * RHeight;

        memset(&leftImg, 0, sizeof(ASVLOFFSCREEN));
        leftImg.i32Width = LWidth;
        leftImg.i32Height = LHeight;
        leftImg.u32PixelArrayFormat = ASVL_PAF_NV21;
        leftImg.pi32Pitch[0] = LWidth;
        leftImg.ppu8Plane[0] = pLeftImgDataY;
        leftImg.pi32Pitch[1] = LWidth / 2 * 2;
        leftImg.ppu8Plane[1] = pLeftImgDataUV;

        memset(&rightImg, 0, sizeof(ASVLOFFSCREEN));
        rightImg.i32Width = RWidth;
        rightImg.i32Height = RHeight;
        rightImg.u32PixelArrayFormat = ASVL_PAF_NV21;
        rightImg.pi32Pitch[0] = RWidth;
        rightImg.ppu8Plane[0] = pRightImgDataY;
        rightImg.pi32Pitch[1] = RWidth / 2 * 2;
        rightImg.ppu8Plane[1] = pRightImgDataUV;

        if (!mAbokehGallery) {
            MByte *pDstDataY = NULL, *pDstDataUV = NULL;
            memset(&dstImg, 0, sizeof(ASVLOFFSCREEN));
            pDstDataY = (MByte *)output_buffer_addr;
            pDstDataUV = pDstDataY + LWidth * LHeight;

            dstImg.i32Width = LWidth;
            dstImg.i32Height = LHeight;
            dstImg.u32PixelArrayFormat = ASVL_PAF_NV21;
            dstImg.pi32Pitch[0] = LWidth;
            dstImg.ppu8Plane[0] = pDstDataY;
            dstImg.pi32Pitch[1] = LWidth / 2 * 2;
            dstImg.ppu8Plane[1] = pDstDataUV;
        }

        rc = mRealBokeh->mArcSoftBokehApi->ARC_DCIR_CapInit(
            &mArcSoftCapHandle, arcsoft_config_para.lFocusMode);
        if (rc != MOK) {
            HAL_LOGE("ARC_DCIR_Init failed!");
            goto exit;
        } else {
            HAL_LOGD("ARC_DCIR_Init succeed");
        }
        // read cal data
        HAL_LOGI("begin ARC_DCIR_SetCaliData");

#ifdef CONFIG_ALTEK_ZTE_CALI
        if (mRealBokeh->mOtpType == OTP_CALI_ALTEK) {
            rc = mRealBokeh->createArcSoftCalibrationData(
                (unsigned char *)mRealBokeh->mArcSoftCalibData,
                mRealBokeh->mOtpSize);
            if (rc != MOK) {
                HAL_LOGE("create arcsoft calibration failed");
            }
        }
#endif

        rc = mRealBokeh->mArcSoftBokehApi->ARC_DCIR_SetCaliData(
            mArcSoftCapHandle, &(mRealBokeh->mCaliData));
        if (rc != MOK) {
            HAL_LOGE("arcsoft ARC_DCIR_SetCaliData failed");
        }

#ifndef CONFIG_ALTEK_ZTE_CALI
        rc = mRealBokeh->mArcSoftBokehApi->ARC_DCIR_SetDistortionCoef(
            mArcSoftCapHandle, arcsoft_config_para.leftDis,
            arcsoft_config_para.rightDis);
        if (rc != MOK) {
            HAL_LOGE("arcsoft ARC_DCIR_SetDistortionCoef failed");
        }
#endif
        rc = mRealBokeh->mArcSoftBokehApi->ARC_DCIR_SetCameraImageInfo(
            mArcSoftCapHandle, &(mRealBokeh->mArcSoftInfo));
        if (rc != MOK) {
            HAL_LOGE("arcsoft ARC_DCIR_SetCameraImageInfo failed");
        }
        HAL_LOGI("ARC_DCIR_SetCameraImageInfo i32LeftFullWidth:%d "
                 "i32LeftFullHeight:%d i32RightFullWidth:%d "
                 "i32RightFullHeight:%d",
                 mRealBokeh->mArcSoftInfo.i32LeftFullWidth,
                 mRealBokeh->mArcSoftInfo.i32LeftFullHeight,
                 mRealBokeh->mArcSoftInfo.i32RightFullWidth,
                 mRealBokeh->mArcSoftInfo.i32RightFullHeight);

        int64_t depthRun = systemTime();
        rc = mRealBokeh->mArcSoftBokehApi->ARC_DCIR_CalcDisparityData(
            mArcSoftCapHandle, &leftImg, &rightImg, &mArcSoftDcrParam);
        if (rc != MOK) {
            HAL_LOGE("arcsoft ARC_DCIR_CalcDisparityData failed,res=%d", rc);
            goto exit_arc;
        }
        HAL_LOGD("ARC_DCIR_CalcDisparityData cost %lld ms",
                 ns2ms(systemTime() - depthRun));
        HAL_LOGD("ARC_DCIR_CalcDisparityData leftImg.i32Width:%d, "
                 "leftImg.i32Height:%d, rightImg.i32Width:%d "
                 "rightImg.i32Height:%d",
                 leftImg.i32Width, leftImg.i32Height, rightImg.i32Width,
                 rightImg.i32Height);
        HAL_LOGD("ARC_DCIR_CalcDisparityData fMaxFOV:%f, i32ImgDegree:%d "
                 "i32FacesNum:%d",
                 mArcSoftDcrParam.fMaxFOV, mArcSoftDcrParam.i32ImgDegree,
                 mArcSoftDcrParam.faceParam.i32FacesNum);

        rc = mRealBokeh->mArcSoftBokehApi->ARC_DCIR_GetDisparityDataSize(
            mArcSoftCapHandle, &lDMSize);
        if (rc != MOK) {
            HAL_LOGE("arcsoft ARC_DCIR_GetDisparityDataSize failed");
            goto exit_arc;
        }
        if (mArcSoftDepthSize != lDMSize) {
            mArcSoftDepthSize = lDMSize;
            if (mArcSoftDepthMap != NULL) {
                free(mArcSoftDepthMap);
            }
            mArcSoftDepthMap = malloc(mArcSoftDepthSize);
            if (!mArcSoftDepthMap) {
                HAL_LOGE("arcsoft pDispMap error!");
                goto exit_arc;
            } else {
                HAL_LOGD("arcsoft pDispMap apply memory succeed, lDMSize %ld",
                         lDMSize);
            }
        }
        rc = mRealBokeh->mArcSoftBokehApi->ARC_DCIR_GetDisparityData(
            mArcSoftCapHandle, mArcSoftDepthMap);
        if (rc != MOK) {
            HAL_LOGE("arcsoft AARC_DCIR_GetDisparityData failed");
        }
        if (!mAbokehGallery) {
            int64_t bokehRun = systemTime();
            rc = mRealBokeh->mArcSoftBokehApi->ARC_DCIR_CapProcess(
                mArcSoftCapHandle, mArcSoftDepthMap, lDMSize, &leftImg,
                &mArcSoftCapParam, &dstImg);
            if (rc != MOK) {
                HAL_LOGE("arcsoft ARC_DCIR_CapProcess failed");
            }
            mRealBokeh->flushIonBuffer(ADP_BUFFD(*output_buffer),
                                       output_buffer_addr,
                                       ADP_BUFSIZE(*output_buffer));
            HAL_LOGD("ARC_DCIR_CapProcess cost %lld ms",
                     ns2ms(systemTime() - bokehRun));
        }
    exit_arc:
        if (mRealBokeh->mArcSoftBokehApi->ARC_DCIR_Uninit(&mArcSoftCapHandle)) {
            HAL_LOGE("arcsoft ARC_DCIR_Uninit failed");
            rc = -1;
        }
    }
exit : { // dump yuv data
    char prop6[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.sys.camera.bokeh.data", prop6, "0");
    if (1 == atoi(prop6)) {
        // input_handle1 or left image
        mRealBokeh->dumpData(
            (unsigned char *)input_buf1_addr, 1, ADP_BUFSIZE(*input_buf1),
            mRealBokeh->mCaptureWidth, mRealBokeh->mCaptureHeight,
            mRealBokeh->mCapFrameNumber, "Main");

        // input_handle2 or right image
        mRealBokeh->dumpData(
            (unsigned char *)input_buf2_addr, 1, ADP_BUFSIZE(*input_buf2),
            mRealBokeh->mDepthSnapImageWidth, mRealBokeh->mDepthSnapImageHeight,
            mRealBokeh->mCapFrameNumber, "Sub");

        if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
            mRealBokeh->dumpData((unsigned char *)depth_output_buffer, 1,
                                 mRealBokeh->mDepthSnapOutWidth *
                                         mRealBokeh->mDepthSnapOutHeight +
                                     68,
                                 mRealBokeh->mDepthSnapOutWidth,
                                 mRealBokeh->mDepthSnapOutHeight,
                                 mRealBokeh->mCapFrameNumber, "output");

            mRealBokeh->dumpData((unsigned char *)scaled_buffer_addr, 1,
                                 ADP_BUFSIZE(*scaled_buffer),
                                 mRealBokeh->mDepthSnapImageWidth,
                                 mRealBokeh->mDepthSnapImageHeight,
                                 mRealBokeh->mCapFrameNumber, "MainScale");

        } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
            if (!mAbokehGallery) {
                mRealBokeh->dumpData((unsigned char *)output_buffer_addr, 1,
                                     ADP_BUFSIZE(*output_buffer),
                                     mRealBokeh->mCaptureWidth,
                                     mRealBokeh->mCaptureHeight,
                                     mRealBokeh->mCapFrameNumber, "output");
            }
            mRealBokeh->dumpData((unsigned char *)(mArcSoftDepthMap), 1,
                                 mArcSoftDepthSize, mRealBokeh->mCaptureWidth,
                                 mRealBokeh->mCaptureHeight,
                                 mRealBokeh->mCapFrameNumber, "depth");
        }
    }
}
    HAL_LOGI(":X");
    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
        mRealBokeh->unmap(scaled_buffer);
    }
fail_map_scale:
    mRealBokeh->unmap(input_buf2);
fail_map_input2:
    if (!mAbokehGallery) {
        if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
            mRealBokeh->unmap(output_buffer);
        }
    }
fail_map_output:

    return rc;
}

/*===========================================================================
 * FUNCTION   :bokehCaptureHandle
 *
 * DESCRIPTION: bokehCaptureHandle
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3RealBokeh::BokehCaptureThread::bokehCaptureHandle(
    buffer_handle_t *output_buf, buffer_handle_t *input_buf1,
    void *input_buf1_addr, void *depth_buffer) {
    int rc = NO_ERROR;
    void *output_buf_addr = NULL;
    int64_t bokehReFocusGen;
    int64_t bokehInit;
    int64_t bokehReFocusProcess;
    char acVersion[256] = {
        0,
    };
    HAL_LOGI("E");
    if (output_buf == NULL || input_buf1 == NULL || depth_buffer == NULL) {
        HAL_LOGE("buffer is NULL!");
        return BAD_VALUE;
    }
    rc = mRealBokeh->map(output_buf, &output_buf_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map output buffer");
        goto fail_map_output;
    }

    mRealBokeh->mBokehCapApi->sprd_bokeh_VersionInfo_Get(acVersion, 256);
    HAL_LOGD("Bokeh Api Version [%s]", acVersion);
    bokehInit = systemTime();

    bokehReFocusProcess = systemTime();
    rc = mRealBokeh->mBokehCapApi->sprd_bokeh_ReFocusPreProcess(
        (void *)mRealBokeh->mBokehCapApi->muti_handle, input_buf1_addr,
        depth_buffer);
    if (rc != ALRNB_ERR_SUCCESS) {
        HAL_LOGE("sprd_bokeh_ReFocusPreProcess failed!");
        goto exit;
    }
    HAL_LOGD("bokeh ReFocusProcess cost %lld ms",
             ns2ms(systemTime() - bokehReFocusProcess));
    bokehReFocusGen = systemTime();
    rc = mRealBokeh->mBokehCapApi->sprd_bokeh_ReFocusGen(
        (void *)mRealBokeh->mBokehCapApi->muti_handle, output_buf_addr,
        mCapbokehParam.bokeh_level, mCapbokehParam.sel_x, mCapbokehParam.sel_y);
    if (rc != ALRNB_ERR_SUCCESS) {
        HAL_LOGE("sprd_bokeh_ReFocusGen failed!,rc=%d", rc);
        rc = ALRNB_ERR_SUCCESS;
    }
    HAL_LOGD("bokeh ReFocusGen cost %lld ms",
             ns2ms(systemTime() - bokehReFocusGen));
    {
        char prop5[PROPERTY_VALUE_MAX] = {
            0,
        };
        property_get("persist.sys.camera.bokeh.data", prop5, "0");
        if (1 == atoi(prop5)) {
            mRealBokeh->dumpData((unsigned char *)output_buf_addr, 1,
                                 ADP_BUFSIZE(*output_buf),
                                 mRealBokeh->mCaptureWidth,
                                 mRealBokeh->mCaptureHeight, 20, "bokeh");
        }
    }

    HAL_LOGI(":X");
exit:
    mRealBokeh->unmap(output_buf);
fail_map_output:

    return rc;
}

/*===========================================================================
 * FUNCTION   :loadBokehApi
 *
 * DESCRIPTION: loadBokehApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3RealBokeh::loadBokehApi() {
    const char *error = NULL;
    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
        mBokehCapApi->handle = dlopen(LIB_BOKEH_PATH, RTLD_LAZY);
        if (mBokehCapApi->handle == NULL) {
            error = dlerror();
            HAL_LOGE("open bokeh API failed.error = %s", error);
            return -1;
        }

        mBokehCapApi->sprd_bokeh_VersionInfo_Get =
            (int (*)(void *a_pOutBuf, int a_dInBufMaxSize))dlsym(
                mBokehCapApi->handle, "sprd_bokeh_VersionInfo_Get");
        if (mBokehCapApi->sprd_bokeh_VersionInfo_Get == NULL) {
            error = dlerror();
            HAL_LOGE("sym sprd_bokeh_VersionInfo_Get failed.error = %s", error);
            return -1;
        }

        mBokehCapApi->sprd_bokeh_Init =
            (int (*)(void **muti_handle, int a_dInImgW, int a_dInImgH,
                     char *param))dlsym(mBokehCapApi->handle,
                                        "sprd_bokeh_Init");
        if (mBokehCapApi->sprd_bokeh_Init == NULL) {
            error = dlerror();
            HAL_LOGE("sym sprd_bokeh_Init failed.error = %s", error);
            return -1;
        }

        mBokehCapApi->sprd_bokeh_ReFocusPreProcess =
            (int (*)(void *muti_handle, void *a_pInBokehBufYCC420NV21,
                     void *a_pInDisparityBuf16))
                dlsym(mBokehCapApi->handle, "sprd_bokeh_ReFocusPreProcess");
        if (mBokehCapApi->sprd_bokeh_ReFocusPreProcess == NULL) {
            error = dlerror();
            HAL_LOGE("sym sprd_bokeh_ReFocusPreProcess failed.error = %s",
                     error);
            return -1;
        }

        mBokehCapApi->sprd_bokeh_ReFocusGen =
            (int (*)(void *muti_handle, void *a_pOutBlurYCC420NV21,
                     int a_dInBlurStrength, int a_dInPositionX,
                     int a_dInPositionY))dlsym(mBokehCapApi->handle,
                                               "sprd_bokeh_ReFocusGen");
        if (mBokehCapApi->sprd_bokeh_ReFocusGen == NULL) {
            error = dlerror();
            HAL_LOGE("sym sprd_bokeh_ReFocusGen failed.error = %s", error);
            return -1;
        }
        mBokehCapApi->sprd_bokeh_Close = (int (*)(void *muti_handle))dlsym(
            mBokehCapApi->handle, "sprd_bokeh_Close");
        if (mBokehCapApi->sprd_bokeh_Close == NULL) {
            error = dlerror();
            HAL_LOGE("sym sprd_bokeh_Close failed.error = %s", error);
            return -1;
        }

        HAL_LOGD("load Bokeh Api succuss.");
    } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        HAL_LOGD("load arcsoft Api");
        mArcSoftBokehApi->handle = dlopen(LIB_ARCSOFT_BOKEH_PATH, RTLD_LAZY);
        if (mArcSoftBokehApi->handle == NULL) {
            error = dlerror();
            HAL_LOGE("open arcsoft bokeh API failed.error = %s", error);
            return -1;
        }

        mArcSoftBokehApi->ARC_DCVR_GetVersion =
            (const MPBASE_Version *(*)())dlsym(mArcSoftBokehApi->handle,
                                               "ARC_DCVR_GetVersion");
        if (mArcSoftBokehApi->ARC_DCVR_GetVersion == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCVR_GetVersion failed.error = %s", error);
            return -1;
        }

        mArcSoftBokehApi->ARC_DCVR_GetDefaultPrevParam =
            (MRESULT (*)(LPARC_DCVR_PARAM pParam))dlsym(
                mArcSoftBokehApi->handle, "ARC_DCVR_GetDefaultParam");
        if (mArcSoftBokehApi->ARC_DCVR_GetDefaultPrevParam == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCVR_GetDefaultPrevParam failed.error = %s",
                     error);
            return -1;
        }

        mArcSoftBokehApi->ARC_DCIR_GetDefaultCapParam =
            (MRESULT (*)(LPARC_DCIR_PARAM pParam))dlsym(
                mArcSoftBokehApi->handle, "ARC_DCIR_GetDefaultParam");
        if (mArcSoftBokehApi->ARC_DCIR_GetDefaultCapParam == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCIR_GetDefaultCapParam failed.error = %s",
                     error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCVR_PrevInit =
            (MRESULT (*)(MHandle *phHandle))dlsym(mArcSoftBokehApi->handle,
                                                  "ARC_DCVR_Init");
        if (mArcSoftBokehApi->ARC_DCVR_PrevInit == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCVR_PrevInit failed.error = %s", error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCIR_CapInit =
            (MRESULT (*)(MHandle *phHandle, MInt32 i32Mode))dlsym(
                mArcSoftBokehApi->handle, "ARC_DCIR_Init");
        if (mArcSoftBokehApi->ARC_DCIR_CapInit == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCIR_CapInit failed.error = %s", error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCVR_Uninit =
            (MRESULT (*)(MHandle *phHandle))dlsym(mArcSoftBokehApi->handle,
                                                  "ARC_DCVR_Uninit");
        if (mArcSoftBokehApi->ARC_DCVR_Uninit == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCVR_Uninit failed.error = %s", error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCIR_Uninit =
            (MRESULT (*)(MHandle *phHandle))dlsym(mArcSoftBokehApi->handle,
                                                  "ARC_DCIR_Uninit");
        if (mArcSoftBokehApi->ARC_DCIR_Uninit == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCIR_Uninit failed.error = %s", error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCIR_SetCameraImageInfo =
            (MRESULT (*)(MHandle hHandle,
                         LPARC_REFOCUSCAMERAIMAGE_PARAM pParam))
                dlsym(mArcSoftBokehApi->handle, "ARC_DCIR_SetCameraImageInfo");
        if (mArcSoftBokehApi->ARC_DCIR_SetCameraImageInfo == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCIR_SetCameraImageInfo failed.error = %s",
                     error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCVR_SetCameraImageInfo =
            (MRESULT (*)(MHandle hHandle,
                         LPARC_REFOCUSCAMERAIMAGE_PARAM pParam))
                dlsym(mArcSoftBokehApi->handle, "ARC_DCVR_SetCameraImageInfo");
        if (mArcSoftBokehApi->ARC_DCVR_SetCameraImageInfo == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCVR_SetCameraImageInfo failed.error = %s",
                     error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCVR_SetImageDegree =
            (MRESULT (*)(MHandle hHandle, MInt32 i32ImgDegree))dlsym(
                mArcSoftBokehApi->handle, "ARC_DCVR_SetImageDegree");
        if (mArcSoftBokehApi->ARC_DCVR_SetImageDegree == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCVR_SetImageDegree failed.error = %s", error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCVR_SetCaliData =
            (MRESULT (*)(MHandle hHandle, LPARC_DC_CALDATA pCaliData))dlsym(
                mArcSoftBokehApi->handle, "ARC_DCVR_SetCaliData");
        if (mArcSoftBokehApi->ARC_DCVR_SetCaliData == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCVR_SetCaliData failed.error = %s", error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCIR_SetCaliData =
            (MRESULT (*)(MHandle hHandle, LPARC_DC_CALDATA pCaliData))dlsym(
                mArcSoftBokehApi->handle, "ARC_DCIR_SetCaliData");
        if (mArcSoftBokehApi->ARC_DCIR_SetCaliData == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCIR_SetCaliData failed.error = %s", error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCIR_SetDistortionCoef =
            (MRESULT (*)(MHandle hHandle, MFloat leftDis[], MFloat rightDis[]))
                dlsym(mArcSoftBokehApi->handle, "ARC_DCIR_SetDistortionCoef");
        if (mArcSoftBokehApi->ARC_DCIR_SetDistortionCoef == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCIR_SetDistortionCoef failed.error = %s", error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCIR_CalcDisparityData =
            (MRESULT (*)(MHandle hHandle, LPASVLOFFSCREEN pMainImg,
                         LPASVLOFFSCREEN pAuxImg, LPARC_DCIR_PARAM pDCIRParam))
                dlsym(mArcSoftBokehApi->handle, "ARC_DCIR_CalcDisparityData");
        if (mArcSoftBokehApi->ARC_DCIR_CalcDisparityData == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCIR_CalcDisparityData failed.error = %s", error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCIR_GetDisparityDataSize =
            (MRESULT (*)(MHandle hHandle, MInt32 *pi32Size))dlsym(
                mArcSoftBokehApi->handle, "ARC_DCIR_GetDisparityDataSize");
        if (mArcSoftBokehApi->ARC_DCIR_GetDisparityDataSize == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCIR_GetDisparityDataSize failed.error = %s",
                     error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCIR_GetDisparityData =
            (MRESULT (*)(MHandle hHandle, MVoid *pDisparityData))dlsym(
                mArcSoftBokehApi->handle, "ARC_DCIR_GetDisparityData");
        if (mArcSoftBokehApi->ARC_DCIR_GetDisparityData == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCIR_GetDisparityData failed.error = %s", error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCIR_CapProcess =
            (MRESULT (*)(MHandle hHandle, MVoid *pDisparityData,
                         MInt32 i32DisparityDataSize, LPASVLOFFSCREEN pMainImg,
                         LPARC_DCIR_REFOCUS_PARAM pRFParam,
                         LPASVLOFFSCREEN pDstImg))
                dlsym(mArcSoftBokehApi->handle, "ARC_DCIR_Process");
        if (mArcSoftBokehApi->ARC_DCIR_CapProcess == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCIR_CapProcess failed.error = %s", error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCVR_PrevProcess =
            (MRESULT (*)(MHandle hHandle, LPASVLOFFSCREEN pMainImg,
                         LPASVLOFFSCREEN pAuxImg, LPASVLOFFSCREEN pDstImg,
                         LPARC_DCVR_PARAM pParam))
                dlsym(mArcSoftBokehApi->handle, "ARC_DCVR_Process");
        if (mArcSoftBokehApi->ARC_DCVR_PrevProcess == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCVR_PrevProcess failed.error = %s", error);
            return -1;
        }
        mArcSoftBokehApi->ARC_DCIR_GetVersion =
            (const MPBASE_Version *(*)())dlsym(mArcSoftBokehApi->handle,
                                               "ARC_DCIR_GetVersion");
        if (mArcSoftBokehApi->ARC_DCIR_GetVersion == NULL) {
            error = dlerror();
            HAL_LOGE("sym ARC_DCIR_GetVersion failed.error = %s", error);
            return -1;
        }
        HAL_LOGD("load arcsoft Api succuss.");
    }

    return 0;
}

/*===========================================================================
 * FUNCTION   :unLoadReFocusApi
 *
 * DESCRIPTION: unLoadReFocusApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::unLoadBokehApi() {
    HAL_LOGI("E");
    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
        if (mFirstSprdBokeh)
            mRealBokeh->mBokehCapApi->sprd_bokeh_Close(
                mBokehCapApi->muti_handle);
        if (mBokehCapApi->handle != NULL) {
            dlclose(mBokehCapApi->handle);
        }
    } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        if (mFirstArcBokeh) {
            MRESULT rc = MOK;
            if (mPreviewMuxerThread->mArcSoftPrevHandle != NULL) {
                rc = mArcSoftBokehApi->ARC_DCVR_Uninit(
                    &(mPreviewMuxerThread->mArcSoftPrevHandle));
                if (rc != MOK) {
                    HAL_LOGE("preview ARC_DCVR_Uninit failed!");
                    return;
                }
            }
        }

        if (mArcSoftBokehApi->handle != NULL) {
            dlclose(mArcSoftBokehApi->handle);
        }
    }
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :loadBokehPreviewApi
 *
 * DESCRIPTION: loadBokehPreviewApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3RealBokeh::loadBokehPreviewApi() {
    const char *error = NULL;
    mBokehPrevApi->handle = dlopen(LIB_BOKEH_PREVIEW_PATH, RTLD_LAZY);
    if (mBokehPrevApi->handle == NULL) {
        error = dlerror();
        HAL_LOGE("open bokeh preview API failed.error = %s", error);
        return -1;
    }

    mBokehPrevApi->iBokehInit =
        (int (*)(void **handle, InitParams *params))dlsym(mBokehPrevApi->handle,
                                                          "iBokehInit");
    if (mBokehPrevApi->iBokehInit == NULL) {
        error = dlerror();
        HAL_LOGE("sym iBokehInit failed.error = %s", error);
        return -1;
    }

    mBokehPrevApi->iBokehDeinit =
        (int (*)(void *handle))dlsym(mBokehPrevApi->handle, "iBokehDeinit");
    if (mBokehPrevApi->iBokehDeinit == NULL) {
        error = dlerror();
        HAL_LOGE("sym iBokehDeinit failed.error = %s", error);
        return -1;
    }

    mBokehPrevApi->iBokehCreateWeightMap =
        (int (*)(void *handle, WeightParams *params))dlsym(
            mBokehPrevApi->handle, "iBokehCreateWeightMap");
    if (mBokehPrevApi->iBokehCreateWeightMap == NULL) {
        error = dlerror();
        HAL_LOGE("sym iBokehCreateWeightMap failed.error = %s", error);
        return -1;
    }

    mBokehPrevApi->iBokehBlurImage =
        (int (*)(void *handle, GraphicBuffer *Src_YUV,
                 GraphicBuffer *Output_YUV))dlsym(mBokehPrevApi->handle,
                                                  "iBokehBlurImage");
    if (mBokehPrevApi->iBokehBlurImage == NULL) {
        error = dlerror();
        HAL_LOGE("sym iBokehBlurImage failed.error = %s", error);
        return -1;
    }
    mBokehPrevApi->mHandle = NULL;
    HAL_LOGD("load BokehPreview Api succuss.");
    return 0;
}

/*===========================================================================
 * FUNCTION   :unLoadBokehPreviewApi
 *
 * DESCRIPTION: unLoadBokehPreviewApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::unLoadBokehPreviewApi() {
    HAL_LOGI("E");
    int rc = NO_ERROR;
    if (mFirstSprdBokeh) {
        if (mRealBokeh->mBokehPrevApi->mHandle != NULL) {
            int64_t deinitStart = systemTime();
            rc = mRealBokeh->mBokehPrevApi->iBokehDeinit(
                mRealBokeh->mBokehPrevApi->mHandle);
            if (rc != ALRNB_ERR_SUCCESS) {
                HAL_LOGE("Deinit Err:%d", rc);
            }
            HAL_LOGD("iBokehDeinit cost %lld ms",
                     ns2ms(systemTime() - deinitStart));
        }
    }
    mRealBokeh->mBokehPrevApi->mHandle = NULL;

    if (mBokehPrevApi->handle != NULL) {
        dlclose(mBokehPrevApi->handle);
        mBokehPrevApi->handle = NULL;
    }
    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :loadDepthApi
 *
 * DESCRIPTION: loadDepthApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3RealBokeh::loadDepthApi() {
    const char *error = NULL;

    mDepthApi->handle = dlopen(LIB_DEPTH_PATH, RTLD_LAZY);
    if (mDepthApi->handle == NULL) {
        error = dlerror();
        HAL_LOGE("open depth API failed.error = %s", error);
        return -1;
    }

    mDepthApi->sprd_depth_VersionInfo_Get =
        (int (*)(char a_acOutRetbuf[256], unsigned int a_udInSize))dlsym(
            mDepthApi->handle, "sprd_depth_VersionInfo_Get");
    if (mDepthApi->sprd_depth_VersionInfo_Get == NULL) {
        error = dlerror();
        HAL_LOGE("sym sprd_depth_VersionInfo_Get failed.error = %s", error);
        return -1;
    }

    mDepthApi->sprd_depth_Set_Stopflag =
        (void (*)(void *handle, depth_stop_flag stop_flag))dlsym(
            mDepthApi->handle, "sprd_depth_Set_Stopflag");
    if (mDepthApi->sprd_depth_Set_Stopflag == NULL) {
        error = dlerror();
        HAL_LOGE("sym sprd_depth_Set_Stopflag failed.error = %s", error);
        return -1;
    }

    HAL_LOGD("load mDepth Api succuss.");
    mDepthApi->sprd_depth_Init =
        (void *(*)(depth_init_inputparam *inparam,
                   depth_init_outputparam *outputinfo, depth_mode mode,
                   outFormat format))dlsym(mDepthApi->handle,
                                           "sprd_depth_Init");
    if (mDepthApi->sprd_depth_Init == NULL) {
        error = dlerror();
        HAL_LOGE("sym sprd_depth_Init failed.error = %s", error);
        return -1;
    }

    mDepthApi->sprd_depth_Run_distance =
        (int (*)(void *handle, void *a_pOutDisparity, void *a_pOutMaptable,
                 void *a_pInSub_YCC420NV21, void *a_pInMain_YCC420NV21,
                 weightmap_param *wParams,
                 distanceRet *distance))dlsym(mDepthApi->handle,
                                              "sprd_depth_Run_distance");
    if (mDepthApi->sprd_depth_Run_distance == NULL) {
        error = dlerror();
        HAL_LOGE("sym sprd_depth_Run_distance failed.error = %s", error);
        return -1;
    }

    HAL_LOGD("load mDepth Api succuss.");
    mDepthApi->sprd_depth_Run =
        (int (*)(void *handle, void *a_pOutDisparity, void *a_pOutMaptable,
                 void *a_pInSub_YCC420NV21, void *a_pInMain_YCC420NV21,
                 weightmap_param *wParams))dlsym(mDepthApi->handle,
                                                 "sprd_depth_Run");
    if (mDepthApi->sprd_depth_Run == NULL) {
        error = dlerror();
        HAL_LOGE("sym sprd_depth_Run failed.error = %s", error);
        return -1;
    }

    mDepthApi->sprd_depth_Close =
        (int (*)(void *handle))dlsym(mDepthApi->handle, "sprd_depth_Close");
    if (mDepthApi->sprd_depth_Close == NULL) {
        error = dlerror();
        HAL_LOGE("sym sprd_depth_Close failed.error = %s", error);
        return -1;
    }
    HAL_LOGD("load mDepth Api succuss.");

    return 0;
}

/*===========================================================================
 * FUNCTION   :unLoadDepthApi
 *
 * DESCRIPTION: unLoadDepthApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::unLoadDepthApi() {
    HAL_LOGI("E");
    int rc = 0;
    if (mFirstSprdBokeh) {
        if (mCaptureThread->mCapDepthhandle != NULL) {
            int64_t depthClose = systemTime();
            rc = mDepthApi->sprd_depth_Close(mCaptureThread->mCapDepthhandle);
            if (rc != ALRNB_ERR_SUCCESS) {
                HAL_LOGE("cap sprd_depth_Close failed! %d", rc);
                return;
            }
            HAL_LOGD("cap depth close cost %lld ms",
                     ns2ms(systemTime() - depthClose));
        }
        if (mPreviewMuxerThread->mPrevDepthhandle != NULL) {
            int64_t depthClose = systemTime();
            rc = mDepthApi->sprd_depth_Close(
                mPreviewMuxerThread->mPrevDepthhandle);
            if (rc != ALRNB_ERR_SUCCESS) {
                HAL_LOGE("prev sprd_depth_Close failed! %d", rc);
                return;
            }
            HAL_LOGD("prev depth close cost %lld ms",
                     ns2ms(systemTime() - depthClose));
        }
    }
    mCaptureThread->mCapDepthhandle = NULL;
    mPreviewMuxerThread->mPrevDepthhandle = NULL;
    if (mDepthApi->handle != NULL) {
        dlclose(mDepthApi->handle);
        mDepthApi->handle = NULL;
    }

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :initBokehApiParams
 *
 * DESCRIPTION: init Bokeh Api Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
void SprdCamera3RealBokeh::initBokehApiParams() {
    int rc = NO_ERROR;
    char acVersion[256] = {
        0,
    };

    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
        if (mFirstSprdBokeh) {
            mRealBokeh->mBokehCapApi->sprd_bokeh_Close(
                mBokehCapApi->muti_handle);
        }
        // preview bokeh params
        mPreviewMuxerThread->mPreviewbokehParam.init_params.width =
            mPreviewWidth;
        mPreviewMuxerThread->mPreviewbokehParam.init_params.height =
            mPreviewHeight;
        mPreviewMuxerThread->mPreviewbokehParam.init_params.depth_width =
            mDepthOutWidth;
        mPreviewMuxerThread->mPreviewbokehParam.init_params.depth_height =
            mDepthOutHeight;
        mPreviewMuxerThread->mPreviewbokehParam.init_params.SmoothWinSize = 11;
        mPreviewMuxerThread->mPreviewbokehParam.init_params.ClipRatio = 50;
        mPreviewMuxerThread->mPreviewbokehParam.init_params.Scalingratio = 2;
        mPreviewMuxerThread->mPreviewbokehParam.init_params
            .DisparitySmoothWinSize = 11;
        mPreviewMuxerThread->mPreviewbokehParam.weight_params.sel_x =
            mPreviewWidth / 2;
        mPreviewMuxerThread->mPreviewbokehParam.weight_params.sel_y =
            mPreviewHeight / 2;
        mPreviewMuxerThread->mPreviewbokehParam.weight_params.F_number = 20;
        mPreviewMuxerThread->mPreviewbokehParam.weight_params.DisparityImage =
            NULL;

        mPreviewMuxerThread->mPreviewbokehParam.depth_param.sel_x =
            mPreviewMuxerThread->mPreviewbokehParam.weight_params.sel_x;
        mPreviewMuxerThread->mPreviewbokehParam.depth_param.sel_y =
            mPreviewMuxerThread->mPreviewbokehParam.weight_params.sel_y;
        mPreviewMuxerThread->mPreviewbokehParam.depth_param.F_number =
            mPreviewMuxerThread->mPreviewbokehParam.weight_params.F_number;
        mPreviewMuxerThread->mPreviewbokehParam.depth_param.DisparityImage =
            NULL;
        // capture bokeh params
        mCaptureThread->mCapbokehParam.sel_x = mCaptureWidth / 2;
        mCaptureThread->mCapbokehParam.sel_y = mCaptureHeight / 2;
        mCaptureThread->mCapbokehParam.bokeh_level = 255;
        mCaptureThread->mCapbokehParam.config_param = NULL;
        mCaptureThread->mBokehConfigParamState = false;

        rc = mRealBokeh->mBokehCapApi->sprd_bokeh_Init(
            &(mBokehCapApi->muti_handle), mRealBokeh->mCaptureWidth,
            mRealBokeh->mCaptureHeight,
            mCaptureThread->mCapbokehParam.config_param);
        if (rc != ALRNB_ERR_SUCCESS) {
            HAL_LOGE("sprd_bokeh_Init failed!");
            return;
        }

    } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        HAL_LOGD("arcsoft init Bokeh Api Params");
        MRESULT rc = MOK;
        const MPBASE_Version *version = NULL;

#ifndef CONFIG_ALTEK_ZTE_CALI
        mArcSoftInfo.i32LeftFullWidth =
            (MInt32)SprdCamera3Setting::s_setting[CAM_BOKEH_MAIN_ID]
                .sensor_InfoInfo.pixer_array_size[0];
        mArcSoftInfo.i32LeftFullHeight =
            (MInt32)SprdCamera3Setting::s_setting[CAM_BOKEH_MAIN_ID]
                .sensor_InfoInfo.pixer_array_size[1];
        mArcSoftInfo.i32RightFullWidth =
            (MInt32)SprdCamera3Setting::s_setting[CAM_DEPTH_ID]
                .sensor_InfoInfo.pixer_array_size[0];
        mArcSoftInfo.i32RightFullHeight =
            (MInt32)SprdCamera3Setting::s_setting[CAM_DEPTH_ID]
                .sensor_InfoInfo.pixer_array_size[1];
#else
        mArcSoftInfo.i32LeftFullWidth = mArcParam.left_image_height;
        mArcSoftInfo.i32LeftFullHeight = mArcParam.left_image_width;
        mArcSoftInfo.i32RightFullWidth = mArcParam.left_image_height;
        mArcSoftInfo.i32RightFullHeight = mArcParam.left_image_width;
#endif
        HAL_LOGD("i32LeftFullWidth %d , i32LeftFullHeight %d,i32RightFullWidth "
                 "%d,i32RightFullHeight %d",
                 mArcSoftInfo.i32LeftFullWidth, mArcSoftInfo.i32LeftFullHeight,
                 mArcSoftInfo.i32RightFullWidth,
                 mArcSoftInfo.i32RightFullHeight);

        // arcsoft preview params
        mPreviewMuxerThread->mArcSoftPrevParam.ptFocus.x =
            (MInt32)(mPreviewWidth / 2);
        mPreviewMuxerThread->mArcSoftPrevParam.ptFocus.y =
            (MInt32)(mPreviewHeight / 2);
        mPreviewMuxerThread->mArcSoftPrevParam.i32BlurLevel = 100;
        mPreviewMuxerThread->mArcSoftPrevParam.bRefocusOn = 1;

        // arcsoft capture params
        mCaptureThread->mArcSoftCapParam.ptFocus.x =
            (MInt32)(mCaptureWidth / 2);
        mCaptureThread->mArcSoftCapParam.ptFocus.y =
            (MInt32)(mCaptureHeight / 2);
        mCaptureThread->mArcSoftCapParam.i32BlurIntensity = 100;

        mCaptureThread->mArcSoftDcrParam.fMaxFOV = arcsoft_config_para.fMaxFOV;
        mCaptureThread->mArcSoftDcrParam.i32ImgDegree = 270;

        version = mArcSoftBokehApi->ARC_DCVR_GetVersion();
        if (version == NULL) {
            HAL_LOGE("ARC_DCVR_GetVersion failed!");
        } else {
            HAL_LOGD("ARC_DCVR_GetVersion version [%s]", version->Version);
        }

        version = mArcSoftBokehApi->ARC_DCIR_GetVersion();
        if (version == NULL) {
            HAL_LOGE("ARC_DCIR_GetVersion failed!");
        } else {
            HAL_LOGD("ARC_DCIR_GetVersion version [%s]", version->Version);
        }

        if (mFirstArcBokeh) {
            if (mPreviewMuxerThread->mArcSoftPrevHandle != NULL) {
                rc = mArcSoftBokehApi->ARC_DCVR_Uninit(
                    &(mPreviewMuxerThread->mArcSoftPrevHandle));
                if (rc != MOK) {
                    HAL_LOGE("preview ARC_DCVR_Uninit failed!");
                    return;
                }
            }
        }
#ifndef CONFIG_ALTEK_ZTE_CALI
        rc = mArcSoftBokehApi->ARC_DCVR_PrevInit(
            &(mPreviewMuxerThread->mArcSoftPrevHandle));
        if (rc != MOK) {
            HAL_LOGE("ARC_DCVR_PrevInit failed ,rc =%ld", rc);
            return;
        }
        // preview parameter settings
        rc = mArcSoftBokehApi->ARC_DCVR_SetCameraImageInfo(
            mPreviewMuxerThread->mArcSoftPrevHandle, &mArcSoftInfo);
        if (rc != MOK) {
            HAL_LOGE("arcsoft ARC_DCVR_SetCameraImageInfo failed");
        } else {
            HAL_LOGD("arcsoft ARC_DCVR_SetCameraImageInfo succeed");
        }
        MInt32 i32ImgDegree = 270;
        rc = mArcSoftBokehApi->ARC_DCVR_SetImageDegree(
            mPreviewMuxerThread->mArcSoftPrevHandle, i32ImgDegree);
        if (rc != MOK) {
            HAL_LOGE("arcsoft ARC_DCVR_SetImageDegree failed");
        } else {
            HAL_LOGD("arcsoft ARC_DCVR_SetImageDegree succeed");
        }
        // read cal data
        rc = mArcSoftBokehApi->ARC_DCVR_SetCaliData(
            mPreviewMuxerThread->mArcSoftPrevHandle, &mCaliData);
        if (rc != MOK) {
            HAL_LOGE("arcsoft ARC_DCVR_SetCaliData failed");
        } else {
            HAL_LOGD("arcsoft ARC_DCVR_SetCaliData succeed");
        }
#endif
        if (mCaptureThread->mArcSoftDepthMap != NULL) {
            free(mCaptureThread->mArcSoftDepthMap);
        }
        mCaptureThread->mArcSoftDepthMap = malloc(ARCSOFT_DEPTH_DATA_SIZE);
        if (mCaptureThread->mArcSoftDepthMap == NULL) {
            HAL_LOGE("mArcSoftDepthMap malloc failed.");
        }

        HAL_LOGD("arcsoft OK mFirstArcBokeh %d to true", mFirstArcBokeh);
        mFirstArcBokeh = true;
    }
}

/*===========================================================================
 * FUNCTION   :checkDepthPara
 *
 * DESCRIPTION: check depth config parameters
 *
 * PARAMETERS : struct sprd_depth_configurable_para *depth_config_param
 *
 * RETURN     :
 *                  0  : success
 *                  other: non-zero failure code
 *==========================================================================*/

int SprdCamera3RealBokeh::checkDepthPara(
    struct sprd_depth_configurable_para *depth_config_param) {
    int rc = NO_ERROR;
    char para[50] = {0};
    FILE *fid =
        fopen("/data/misc/cameraserver/depth_config_parameter.bin", "rb");
    if (fid != NULL) {
        HAL_LOGD("open depth_config_parameter.bin file success");
        rc = fread(para, sizeof(char),
                   sizeof(struct sprd_depth_configurable_para), fid);
        HAL_LOGD("read depth_config_parameter.bin size %d bytes", rc);
        depth_config_param = (struct sprd_depth_configurable_para *)para;
        HAL_LOGD(
            "read sprd_depth_configurable_para: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
            depth_config_param->SensorDirection,
            depth_config_param->DepthScaleMin,
            depth_config_param->DepthScaleMax,
            depth_config_param->CalibInfiniteZeroPt,
            depth_config_param->SearhRange, depth_config_param->MinDExtendRatio,
            depth_config_param->inDistance, depth_config_param->inRatio,
            depth_config_param->outDistance, depth_config_param->outRatio);
        fclose(fid);
    } else {
        HAL_LOGW("open depth_config_parameter.bin file error");
        rc = UNKNOWN_ERROR;
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   :initDepthApiParams
 *
 * DESCRIPTION: init Depth Api Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
void SprdCamera3RealBokeh::initDepthApiParams() {
    if (mFirstSprdBokeh) {
        mDepthApi->sprd_depth_Close(mCaptureThread->mCapDepthhandle);
        mDepthApi->sprd_depth_Close(mPreviewMuxerThread->mPrevDepthhandle);
    }

    int rc = NO_ERROR;
    struct sprd_depth_configurable_para *depth_config_param = NULL;
    char acVersion[256] = {
        0,
    };

    // preview depth params
    depth_init_inputparam prev_input_param;
    depth_init_outputparam prev_output_info;
    depth_mode prev_mode;
    outFormat prev_outformat;
    prev_input_param.input_width_main = mPreviewWidth;
    prev_input_param.input_height_main = mPreviewHeight;
    prev_input_param.input_width_sub = mDepthPrevImageWidth;
    prev_input_param.input_height_sub = mDepthPrevImageHeight;
    prev_input_param.output_depthwidth = mDepthOutWidth;
    prev_input_param.output_depthheight = mDepthOutHeight;
    prev_input_param.online_depthwidth = 0;
    prev_input_param.online_depthheight = 0;
    prev_input_param.depth_threadNum = 1;
    prev_input_param.online_threadNum = 0;
    prev_input_param.imageFormat_main = YUV420_NV12;
    prev_input_param.imageFormat_sub = YUV420_NV12;
    prev_input_param.potpbuf = mOtpData;
    prev_input_param.otpsize = mOtpSize;
    prev_input_param.config_param = NULL;
    prev_mode = MODE_PREVIEW;
    prev_outformat = mDepthPrevFormat;
    mPreviewMuxerThread->mPrevDepthhandle = NULL;

    // capture depth params
    depth_init_inputparam cap_input_param;
    depth_init_outputparam cap_output_info;
    depth_mode cap_mode;
    outFormat cap_outformat;
    cap_input_param.input_width_main = mDepthSnapImageWidth;
    cap_input_param.input_height_main = mDepthSnapImageHeight;
    cap_input_param.input_width_sub = mDepthSnapImageWidth;
    cap_input_param.input_height_sub = mDepthSnapImageHeight;
    cap_input_param.output_depthwidth = mDepthSnapOutWidth;
    cap_input_param.output_depthheight = mDepthSnapOutHeight;
    cap_input_param.online_depthwidth = 0;
    cap_input_param.online_depthheight = 0;
    cap_input_param.depth_threadNum = 2;
    cap_input_param.online_threadNum = 0;
    cap_input_param.imageFormat_main = YUV420_NV12;
    cap_input_param.imageFormat_sub = YUV420_NV12;
    cap_input_param.potpbuf = mOtpData;
    cap_input_param.otpsize = SPRD_DUAL_OTP_SIZE;
    cap_input_param.config_param = NULL;
    cap_mode = MODE_CAPTURE;
    cap_outformat = MODE_DISPARITY;
    mCaptureThread->mCapDepthhandle = NULL;

    rc = checkDepthPara(depth_config_param);
    if (rc) {
        prev_input_param.config_param = (char *)(&sprd_depth_config_para);
        cap_input_param.config_param = (char *)(&sprd_depth_config_para);
    } else {
        prev_input_param.config_param = (char *)depth_config_param;
        cap_input_param.config_param = (char *)depth_config_param;
    }
    rc = mDepthApi->sprd_depth_VersionInfo_Get(acVersion, 256);
    HAL_LOGD("depth api version [%s]", acVersion);
    if (mIsSupportPBokeh) {
        if (mPreviewMuxerThread->mPrevDepthhandle == NULL) {
            int64_t depthInit = systemTime();
            mPreviewMuxerThread->mPrevDepthhandle = mDepthApi->sprd_depth_Init(
                &(prev_input_param), &(prev_output_info), prev_mode,
                prev_outformat);
            if (mPreviewMuxerThread->mPrevDepthhandle == NULL) {
                HAL_LOGE("sprd_depth_Init failed!");
                return;
            }
            HAL_LOGD("depth init cost %lld ms",
                     ns2ms(systemTime() - depthInit));
        }
    }
    if (mCaptureThread->mCapDepthhandle == NULL) {
        int64_t depthInit = systemTime();
        mCaptureThread->mCapDepthhandle =
            mRealBokeh->mDepthApi->sprd_depth_Init(&(cap_input_param),
                                                   &(cap_output_info), cap_mode,
                                                   cap_outformat);
        if (mCaptureThread->mCapDepthhandle == NULL) {
            HAL_LOGE("sprd_depth_Init failed!");
            return;
        }
        HAL_LOGD("depth init cost %lld ms", ns2ms(systemTime() - depthInit));
    }
}

void SprdCamera3RealBokeh::initBokehPrevApiParams() {
    int rc = NO_ERROR;
    if (mFirstSprdBokeh) {
        rc = mRealBokeh->mBokehPrevApi->iBokehDeinit(
            mRealBokeh->mBokehPrevApi->mHandle);
        if (rc != ALRNB_ERR_SUCCESS) {
            HAL_LOGE("Deinit Err:%d", rc);
        }
    }
    rc = mRealBokeh->mBokehPrevApi->iBokehInit(
        &(mRealBokeh->mBokehPrevApi->mHandle),
        &(mPreviewMuxerThread->mPreviewbokehParam.init_params));
    if (rc != ALRNB_ERR_SUCCESS) {
        HAL_LOGE("iBokehInit failed!");
    } else {
        HAL_LOGD("iBokehInit success");
    }
}

/*===========================================================================
 * FUNCTION   :updateApiParams
 *
 * DESCRIPTION: update bokeh and depth api Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
void SprdCamera3RealBokeh::updateApiParams(CameraMetadata metaSettings,
                                           int type) {
    // always get f_num in request
    int32_t origW =
        SprdCamera3Setting::s_setting[0].sensor_InfoInfo.pixer_array_size[0];
    int32_t origH =
        SprdCamera3Setting::s_setting[0].sensor_InfoInfo.pixer_array_size[1];

    if (metaSettings.exists(ANDROID_SPRD_BLUR_F_NUMBER)) {
        int fnum = metaSettings.find(ANDROID_SPRD_BLUR_F_NUMBER).data.i32[0];
        if (fnum < MIN_F_FUMBER) {
            fnum = MIN_F_FUMBER;
        } else if (fnum > MAX_F_FUMBER) {
            fnum = MAX_F_FUMBER;
        }

        if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
            mUpdateDepthFlag = true;
            mPreviewMuxerThread->mPreviewbokehParam.weight_params.F_number =
                fnum * MAX_BLUR_F_FUMBER / MAX_F_FUMBER;
            fnum = (MAX_F_FUMBER + 1 - fnum) * 255 / MAX_F_FUMBER;
            mCaptureThread->mCapbokehParam.bokeh_level = fnum;
            mPreviewMuxerThread->mPreviewbokehParam.depth_param.F_number =
                mPreviewMuxerThread->mPreviewbokehParam.weight_params.F_number;
        } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
            fnum = MAX_F_FUMBER + 1 - fnum;
            mPreviewMuxerThread->mArcSoftPrevParam.i32BlurLevel =
                (MInt32)(fnum * 100 / MAX_F_FUMBER);
            mCaptureThread->mArcSoftCapParam.i32BlurIntensity =
                mPreviewMuxerThread->mArcSoftPrevParam.i32BlurLevel *
                arcsoft_config_para.blurCapPrevRatio;
        }
        HAL_LOGD("f_number=%d sprd:%d,%d,arcsoft:%d,%d,",
                 metaSettings.find(ANDROID_SPRD_BLUR_F_NUMBER).data.i32[0],
                 mPreviewMuxerThread->mPreviewbokehParam.weight_params.F_number,
                 mCaptureThread->mCapbokehParam.bokeh_level,
                 mPreviewMuxerThread->mArcSoftPrevParam.i32BlurLevel,
                 mCaptureThread->mArcSoftCapParam.i32BlurIntensity);
    }

    if (metaSettings.exists(ANDROID_CONTROL_AF_REGIONS)) {
        int left = metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[0];
        int top = metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[1];
        int right = metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[2];
        int bottom = metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[3];
        int x = left, y = top;
        if (left != 0 && top != 0 && right != 0 && bottom != 0) {
            x = left + (right - left) / 2;
            y = top + (bottom - top) / 2;
            x = x * mPreviewWidth / origW;
            y = y * mPreviewHeight / origH;

            if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
                if (x !=
                        mPreviewMuxerThread->mPreviewbokehParam.weight_params
                            .sel_x ||
                    y !=
                        mPreviewMuxerThread->mPreviewbokehParam.weight_params
                            .sel_y) {
                    mPreviewMuxerThread->mPreviewbokehParam.weight_params
                        .sel_x = x;
                    mPreviewMuxerThread->mPreviewbokehParam.weight_params
                        .sel_y = y;
                    mCaptureThread->mCapbokehParam.sel_x =
                        x * mCaptureWidth / mPreviewWidth;
                    mCaptureThread->mCapbokehParam.sel_y =
                        y * mCaptureHeight / mPreviewHeight;
                    mPreviewMuxerThread->mPreviewbokehParam.weight_params
                        .sel_x = mPreviewMuxerThread->mPreviewbokehParam
                                     .depth_param.sel_x;
                    mPreviewMuxerThread->mPreviewbokehParam.weight_params
                        .sel_y = mPreviewMuxerThread->mPreviewbokehParam
                                     .depth_param.sel_y;
                }
            } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
                mPreviewMuxerThread->mArcSoftPrevParam.ptFocus.x = (MInt32)x;
                mPreviewMuxerThread->mArcSoftPrevParam.ptFocus.y = (MInt32)y;
                mCaptureThread->mArcSoftCapParam.ptFocus.x =
                    (MInt32)x * mCaptureWidth / mPreviewWidth;
                mCaptureThread->mArcSoftCapParam.ptFocus.y =
                    (MInt32)y * mCaptureHeight / mPreviewHeight;
            }
            HAL_LOGD("sel_x %d ,sel_y %d", x, y);
        }
    } else {
        if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
            if (mPreviewMuxerThread->mPreviewbokehParam.weight_params.sel_x !=
                    mPreviewWidth / 2 &&
                mPreviewMuxerThread->mPreviewbokehParam.weight_params.sel_y !=
                    mPreviewHeight / 2) {
                CONTROL_Tag controlInfo;
                mRealBokeh->m_pPhyCamera[CAM_TYPE_MAIN]
                    .hwi->mSetting->getCONTROLTag(&controlInfo);
                if (controlInfo.af_state ==
                        ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN &&
                    controlInfo.af_mode ==
                        ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE) {
                    mPreviewMuxerThread->mPreviewbokehParam.weight_params
                        .sel_x = mPreviewWidth / 2;
                    mPreviewMuxerThread->mPreviewbokehParam.weight_params
                        .sel_y = mPreviewHeight / 2;
                    mCaptureThread->mCapbokehParam.sel_x = mCaptureWidth / 2;
                    mCaptureThread->mCapbokehParam.sel_y = mCaptureHeight / 2;

                    mPreviewMuxerThread->mPreviewbokehParam.weight_params
                        .sel_x = mPreviewMuxerThread->mPreviewbokehParam
                                     .depth_param.sel_x;
                    mPreviewMuxerThread->mPreviewbokehParam.weight_params
                        .sel_y = mPreviewMuxerThread->mPreviewbokehParam
                                     .depth_param.sel_y;
                    HAL_LOGD("autofocus and bokeh center");
                }
            }
        } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
            if (mPreviewMuxerThread->mArcSoftPrevParam.ptFocus.x !=
                    mPreviewWidth / 2 &&
                mPreviewMuxerThread->mArcSoftPrevParam.ptFocus.y !=
                    mPreviewHeight / 2) {
                CONTROL_Tag controlInfo;
                mRealBokeh->m_pPhyCamera[CAM_TYPE_MAIN]
                    .hwi->mSetting->getCONTROLTag(&controlInfo);
                if (controlInfo.af_state ==
                        ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN &&
                    controlInfo.af_mode ==
                        ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE) {
                    mPreviewMuxerThread->mArcSoftPrevParam.ptFocus.x =
                        mPreviewWidth / 2;
                    mPreviewMuxerThread->mArcSoftPrevParam.ptFocus.y =
                        mPreviewHeight / 2;
                    mCaptureThread->mArcSoftCapParam.ptFocus.x =
                        mCaptureWidth / 2;
                    mCaptureThread->mArcSoftCapParam.ptFocus.y =
                        mCaptureHeight / 2;

                    HAL_LOGD("autofocus and bokeh center");
                }
            }
        }
    }
#ifdef CONFIG_FACE_BEAUTY
    if (metaSettings.exists(ANDROID_STATISTICS_FACE_RECTANGLES)) {
        mFaceInfo[0] =
            metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[0];
        mFaceInfo[1] =
            metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[1];
        mFaceInfo[2] =
            metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[2];
        mFaceInfo[3] =
            metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[3];
    }
#endif
    if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        // get face info
        uint8_t phyId = 0;
        phyId = m_pPhyCamera[CAM_TYPE_MAIN].id;

        SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].hwi;

        FACE_Tag *faceDetectionInfo =
            (FACE_Tag *)&(hwiMain->mSetting->s_setting[phyId].faceInfo);
        uint8_t numFaces = faceDetectionInfo->face_num;
        uint8_t faceScores[CAMERA3MAXFACE];
        int32_t faceRectangles[CAMERA3MAXFACE * 4];
        MRECT face_rect[CAMERA3MAXFACE];
        int j = 0;

        for (int i = 0; i < numFaces; i++) {
            faceScores[i] = faceDetectionInfo->face[i].score;
            if (faceScores[i] == 0) {

                faceScores[i] = 1;
            }
            convertToRegions(faceDetectionInfo->face[i].rect,
                             faceRectangles + j, -1);
            face_rect[i].left = faceRectangles[0 + j];
            face_rect[i].top = faceRectangles[1 + j];
            face_rect[i].right = faceRectangles[2 + j];
            face_rect[i].bottom = faceRectangles[3 + j];
            j += 4;
        }
        mCaptureThread->mArcSoftDcrParam.faceParam.i32FacesNum =
            faceDetectionInfo->face_num;
        mCaptureThread->mArcSoftDcrParam.faceParam.prtFaces = face_rect;

        // get af state
        uint8_t af_state =
            hwiMain->mSetting->s_setting[mCameraId].controlInfo.af_state;
        HAL_LOGD("af_state %d", af_state);
        if (af_state == ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN ||
            af_state == ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN || mIsCapturing) {
            mPreviewMuxerThread->mArcSoftPrevParam.bRefocusOn = false;
        } else {
            mPreviewMuxerThread->mArcSoftPrevParam.bRefocusOn = true;
        }
        HAL_LOGD("bRefocusOn %d",
                 mPreviewMuxerThread->mArcSoftPrevParam.bRefocusOn);
    }
    {
        char prop2[PROPERTY_VALUE_MAX] = {
            0,
        };
        property_get("persist.sys.bokeh.param", prop2, "0");
        if (1 == atoi(prop2)) {
            char OTPFileName[256] = {0};
            char param_tune[320] = {0};
            uint32_t size = 320;

            snprintf(OTPFileName, sizeof(OTPFileName),
                     "/data/misc/cameraserver/bokeh_param.bin");
            uint32_t read_bytes =
                read_file(OTPFileName, (void *)(&param_tune), size);
            HAL_LOGV("read_file %u", read_bytes);
            mCaptureThread->mCapbokehParam.config_param = param_tune;
            mCaptureThread->mBokehConfigParamState = true;
        }
    }
}

#ifdef CONFIG_FACE_BEAUTY
/*===========================================================================
 * FUNCTION   :cap_3d_doFaceMakeup
 *
 * DESCRIPTION:
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::bokehFaceMakeup(buffer_handle_t *buffer_handle,
                                           void *input_buf1_addr) {

    struct camera_frame_type cap_3d_frame;
    struct camera_frame_type *frame = NULL;
    int faceInfo[4];
    FACE_Tag newFace;
    bzero(&cap_3d_frame, sizeof(struct camera_frame_type));
    frame = &cap_3d_frame;
    frame->y_vir_addr = (cmr_uint)input_buf1_addr;
    frame->width = ADP_WIDTH(*buffer_handle);
    frame->height = ADP_HEIGHT(*buffer_handle);

    faceInfo[0] = mFaceInfo[0] * mCaptureWidth / mPreviewWidth;
    faceInfo[1] = mFaceInfo[1] * mCaptureWidth / mPreviewWidth;
    faceInfo[2] = mFaceInfo[2] * mCaptureWidth / mPreviewWidth;
    faceInfo[3] = mFaceInfo[3] * mCaptureWidth / mPreviewWidth;

    newFace.face_num =
        SprdCamera3Setting::s_setting[mRealBokeh->mCameraId].faceInfo.face_num;
    newFace.face[0].rect[0] = faceInfo[0];
    newFace.face[0].rect[1] = faceInfo[1];
    newFace.face[0].rect[2] = faceInfo[2];
    newFace.face[0].rect[3] = faceInfo[3];
    mRealBokeh->doFaceMakeup2(frame, mRealBokeh->mPerfectskinlevel, &newFace,
                              0); // work mode 1 for preview, 0 for picture
}
#endif

/*======================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: dump fd
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3RealBokeh::dump(const struct camera3_device *device, int fd) {
    HAL_LOGI("E");

    CHECK_CAPTURE();

    mRealBokeh->_dump(device, fd);

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :flush
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3RealBokeh::flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGI(" E");
    CHECK_CAPTURE_ERROR();

    rc = mRealBokeh->_flush(device);

    HAL_LOGI(" X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :initialize
 *
 * DESCRIPTION: initialize device
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3RealBokeh::initialize(
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;
    HAL_LOGI("E");
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mPreviewStreamsNum = 0;
    mCaptureStreamsNum = 1;
    mCaptureThread->mReprocessing = false;
    mIsCapturing = false;
    mCapFrameNumber = 0;
    mPrevFrameNumber = 0;
    mjpegSize = 0;
    mFlushing = false;
    mCallbackOps = callback_ops;
    mLocalBufferList.clear();
    mMetadataList.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mNotifyListMain.clear();
    mNotifyListAux.clear();
    mFirstArcBokeh = false;
    mFirstSprdBokeh = false;
    mOtpExist = false;
    mVcmSteps = 0;
    mOtpSize = 0;
    mOtpType = 0;
    mJpegOrientation = 0;
    m_pMainSnapBuffer = NULL;
    m_pSprdDepthBuffer = NULL;
#ifdef YUV_CONVERT_TO_JPEG
    mOrigJpegSize = 0;
    m_pDstJpegBuffer = NULL;
#endif

    sprdcamera_physical_descriptor_t sprdCam =
        m_pPhyCamera[CAM_TYPE_BOKEH_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiMain);

    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }

    sprdCam = m_pPhyCamera[CAM_TYPE_DEPTH];
    SprdCamera3HWI *hwiAux = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiAux);

    rc = hwiAux->initialize(sprdCam.dev, &callback_ops_aux);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error aux camera while initialize !! ");
        return rc;
    }
    memset(mAuxStreams, 0,
           sizeof(camera3_stream_t) * REAL_BOKEH_MAX_NUM_STREAMS);
    memset(mMainStreams, 0,
           sizeof(camera3_stream_t) * REAL_BOKEH_MAX_NUM_STREAMS);
    memset(&mCaptureThread->mSavedCapRequest, 0,
           sizeof(camera3_capture_request_t));
    memset(&mCaptureThread->mSavedCapReqStreamBuff, 0,
           sizeof(camera3_stream_buffer_t));
    mCaptureThread->mSavedCapReqsettings = NULL;
    mCaptureThread->mSavedResultBuff = NULL;
    mCaptureThread->mSavedOneResultBuff = NULL;

    mCaptureThread->mCallbackOps = callback_ops;
    mCaptureThread->mDevmain = &m_pPhyCamera[CAM_TYPE_BOKEH_MAIN];

    HAL_LOGI("X");
    return rc;
}

/*===========================================================================
 * FUNCTION   :configureStreams
 *
 * DESCRIPTION: configureStreams
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3RealBokeh::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGI("E");
    int rc = 0;
    size_t i = 0;
    size_t j = 0;
    camera3_stream_t *pmainStreams[REAL_BOKEH_MAX_NUM_STREAMS];
    camera3_stream_t *pauxStreams[REAL_BOKEH_MAX_NUM_STREAMS];
    camera3_stream_t *newStream = NULL;
    camera3_stream_t *previewStream = NULL;
    camera3_stream_t *snapStream = NULL;
    camera3_stream_t *thumbStream = NULL;
    mDepthOutWidth = DEPTH_OUTPUT_WIDTH;
    mDepthOutHeight = DEPTH_OUTPUT_HEIGHT;
    mDepthSnapOutWidth = DEPTH_SNAP_OUTPUT_WIDTH;
    mDepthSnapOutHeight = DEPTH_SNAP_OUTPUT_HEIGHT;
    mFlushing = false;
    mhasCallbackStream = false;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    if (mOtpExist) {
        property_get("persist.sys.cam.pbokeh.enable", prop, "1");
        mIsSupportPBokeh = atoi(prop);
        HAL_LOGD("mIsSupportPBokeh prop %d", mIsSupportPBokeh);
    } else {
        mIsSupportPBokeh = false;
        HAL_LOGD("mIsSupportPBokeh %d", mIsSupportPBokeh);
    }
    memset(pmainStreams, 0,
           sizeof(camera3_stream_t *) * REAL_BOKEH_MAX_NUM_STREAMS);
    memset(pauxStreams, 0,
           sizeof(camera3_stream_t *) * REAL_BOKEH_MAX_NUM_STREAMS);

    Mutex::Autolock l(mLock);

    for (size_t i = 0; i < stream_list->num_streams; i++) {
        int requestStreamType = getStreamType(stream_list->streams[i]);
        HAL_LOGD("configurestreams, org streamtype:%d",
                 stream_list->streams[i]->stream_type);
        if (requestStreamType == PREVIEW_STREAM) {
            previewStream = stream_list->streams[i];

            mPreviewWidth = stream_list->streams[i]->width;
            mPreviewHeight = stream_list->streams[i]->height;
            getDepthImageSize(mPreviewWidth, mPreviewHeight,
                              &mDepthPrevImageWidth, &mDepthPrevImageHeight,
                              requestStreamType);

            mMainStreams[mPreviewStreamsNum] = *stream_list->streams[i];
            mAuxStreams[mPreviewStreamsNum] = *stream_list->streams[i];
            (mAuxStreams[mPreviewStreamsNum]).width = mDepthPrevImageWidth;
            (mAuxStreams[mPreviewStreamsNum]).height = mDepthPrevImageHeight;
            pmainStreams[i] = &mMainStreams[mPreviewStreamsNum];
            pauxStreams[i] = &mAuxStreams[mPreviewStreamsNum];

        } else if (requestStreamType == SNAPSHOT_STREAM) {
            snapStream = stream_list->streams[i];
            mCaptureWidth = stream_list->streams[i]->width;
            mCaptureHeight = stream_list->streams[i]->height;

#ifdef BOKEH_YUV_DATA_TRANSFORM
            // workaround jpeg cant handle 16-noalign issue, when jpeg fix this
            // issue, we will remove these code
            if (mCaptureHeight == 1944 && mCaptureWidth == 2592) {
                mTransformWidth = 2592;
                mTransformHeight = 1952;
            } else if (mCaptureHeight == 1836 && mCaptureWidth == 3264) {
                mTransformWidth = 3264;
                mTransformHeight = 1840;
            } else if (mCaptureHeight == 360 && mCaptureWidth == 640) {
                mTransformWidth = 640;
                mTransformHeight = 368;
            } else if (mCaptureHeight == 1080 && mCaptureWidth == 1920) {
                mTransformWidth = 1920;
                mTransformHeight = 1088;
            } else {
                mTransformWidth = 0;
                mTransformHeight = 0;
            }
#endif
            mCallbackWidth = mCaptureWidth;
            mCallbackHeight = mCaptureHeight;
            getDepthImageSize(mCaptureWidth, mCaptureHeight,
                              &mDepthSnapImageWidth, &mDepthSnapImageHeight,
                              requestStreamType);
            mMainStreams[mCaptureStreamsNum] = *stream_list->streams[i];
            mAuxStreams[mCaptureStreamsNum] = *stream_list->streams[i];
            (mAuxStreams[mCaptureStreamsNum]).width = mDepthSnapImageWidth;
            (mAuxStreams[mCaptureStreamsNum]).height = mDepthSnapImageHeight;

            pmainStreams[mCaptureStreamsNum] =
                &mMainStreams[mCaptureStreamsNum];
            mCallbackStreamsNum = REAL_BOKEH_MAX_NUM_STREAMS - 1;
            mMainStreams[mCallbackStreamsNum].max_buffers = 1;
            mMainStreams[mCallbackStreamsNum].width = mCaptureWidth;
            mMainStreams[mCallbackStreamsNum].height = mCaptureHeight;
            mMainStreams[mCallbackStreamsNum].format =
                HAL_PIXEL_FORMAT_YCbCr_420_888;
            mMainStreams[mCallbackStreamsNum].usage =
                GRALLOC_USAGE_SW_READ_OFTEN;
            mMainStreams[mCallbackStreamsNum].stream_type =
                CAMERA3_STREAM_OUTPUT;
            mMainStreams[mCallbackStreamsNum].data_space =
                stream_list->streams[i]->data_space;
            mMainStreams[mCallbackStreamsNum].rotation =
                stream_list->streams[i]->rotation;
            pmainStreams[mCallbackStreamsNum] =
                &mMainStreams[mCallbackStreamsNum];

            mAuxStreams[mCallbackStreamsNum].max_buffers = 1;
            mAuxStreams[mCallbackStreamsNum].width = mDepthSnapImageWidth;
            mAuxStreams[mCallbackStreamsNum].height = mDepthSnapImageHeight;
            mAuxStreams[mCallbackStreamsNum].format =
                HAL_PIXEL_FORMAT_YCbCr_420_888;
            mAuxStreams[mCallbackStreamsNum].usage =
                GRALLOC_USAGE_SW_READ_OFTEN;
            mAuxStreams[mCallbackStreamsNum].stream_type =
                CAMERA3_STREAM_OUTPUT;
            mAuxStreams[mCallbackStreamsNum].data_space =
                stream_list->streams[i]->data_space;
            mAuxStreams[mCallbackStreamsNum].rotation =
                stream_list->streams[i]->rotation;
            pauxStreams[mCaptureStreamsNum] = &mAuxStreams[mCallbackStreamsNum];
        } else if (requestStreamType == DEFAULT_STREAM) {
            mhasCallbackStream = true;
            (stream_list->streams[i])->max_buffers = 1;
        }
    }
    rc = allocateBuff();
    if (rc < 0) {
        HAL_LOGE("failed to allocateBuff.");
    }
    camera3_stream_configuration mainconfig;
    mainconfig = *stream_list;
    mainconfig.num_streams = REAL_BOKEH_MAX_NUM_STREAMS;
    mainconfig.streams = pmainStreams;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].hwi;

    camera3_stream_configuration auxconfig;
    auxconfig = *stream_list;
    auxconfig.num_streams = REAL_BOKEH_MAX_NUM_STREAMS - 1;
    auxconfig.streams = pauxStreams;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_DEPTH].hwi;

    rc =
        hwiAux->configure_streams(m_pPhyCamera[CAM_TYPE_DEPTH].dev, &auxconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure aux streams!!");
        return rc;
    }

    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].dev,
                                    &mainconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure main streams!!");
        return rc;
    }

    if (previewStream != NULL) {
        memcpy(previewStream, &mMainStreams[mPreviewStreamsNum],
               sizeof(camera3_stream_t));
        HAL_LOGV("previewStream  max buffers %d", previewStream->max_buffers);
        previewStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
    }
    if (snapStream != NULL) {
        memcpy(snapStream, &mMainStreams[mCaptureStreamsNum],
               sizeof(camera3_stream_t));
        snapStream->width = mCaptureWidth;
        snapStream->height = mCaptureHeight;
    }
    mCaptureThread->run(String8::format("Bokeh-Cap").string());
    mPreviewMuxerThread->run(String8::format("Bokeh-Prev").string());
    initBokehApiParams();
    if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
        initDepthApiParams();
        initBokehPrevApiParams();
    }
    mFirstSprdBokeh = true;

    HAL_LOGI("x rc%d.", rc);
    for (i = 0; i < stream_list->num_streams; i++) {
        HAL_LOGD(
            "main configurestreams, streamtype:%d, format:%d, width:%d, "
            "height:%d %p",
            stream_list->streams[i]->stream_type,
            stream_list->streams[i]->format, stream_list->streams[i]->width,
            stream_list->streams[i]->height, stream_list->streams[i]->priv);
    }
    HAL_LOGI("mum_streams:%d,%d,w,h:(%d,%d)(%d,%d)(%d,%d)(%d,%d)),"
             "mIsSupportPBokeh:%d,",
             mainconfig.num_streams, auxconfig.num_streams,
             mDepthSnapImageWidth, mDepthSnapImageHeight, mCaptureWidth,
             mCaptureHeight, mDepthPrevImageWidth, mDepthPrevImageHeight,
             mPreviewWidth, mPreviewHeight, mIsSupportPBokeh);

    return rc;
}

#ifdef CONFIG_ALTEK_ZTE_CALI
/*===========================================================================
 * FUNCTION   : createArcSoftCalibrationData
 *
 * DESCRIPTION: void
 *
 * PARAMETERS : ArcParam *pArcParam
 *
 * RETURN     :
 *                  0  : success
 *                  other: non-zero failure code
 *==========================================================================*/
int SprdCamera3RealBokeh::createArcSoftCalibrationData(unsigned char *pBuffer,
                                                       int nBufSize) {
    MRESULT hr = MOK;
    MHandle hHandle = 0;
    AltekParam stAltkeParam;
    int nRet = 0;
    memset(&mArcParam, 0, sizeof(ArcParam));

    HAL_LOGI("E");

    hr = Arc_CaliData_Init(&hHandle);
    if (hr != MOK) {
        HAL_LOGE("ArcSoft_C failed to Arc_CaliData_Init, hr:%d", (int)hr);
        return -1;
    }

    stAltkeParam.a_dInVCMStep = mVcmSteps;
    stAltkeParam.a_dInCamLayout = arcsoft_config_para.a_dInCamLayout; // type3

    hr = Arc_CaliData_ParseDualCamData(hHandle, pBuffer, nBufSize,
                                       cali_type_altek, &stAltkeParam);
    if (hr != MOK) {
        HAL_LOGE("ArcSoft_C ailed to Arc_CaliData_ParseDualCamData, hr:%d",
                 (int)hr);
        nRet = -1;
        goto error;
    }

    hr = Arc_CaliData_ParseOTPData(hHandle, pBuffer, nBufSize, otp_type_zte,
                                   NULL);
    if (hr != MOK) {
        HAL_LOGE("ArcSoft_C failed to Arc_CaliData_ParseOTPData, hr:%d",
                 (int)hr);
        nRet = -1;
        goto error;
    }

    hr = Arc_CaliData_GetArcParam(hHandle, &mArcParam, sizeof(ArcParam));
    if (hr != MOK) {
        HAL_LOGE("ArcSoft_C failed to Arc_CaliData_GetArcParam, hr:%d",
                 (int)hr);
        nRet = -1;
        goto error;
    }

    Arc_CaliData_PrintArcParam(&mArcParam);
    mRealBokeh->mCaliData.i32CalibDataSize = sizeof(mRealBokeh->mArcParam);
    mRealBokeh->mCaliData.pCalibData = &mRealBokeh->mArcParam;
    HAL_LOGI("i32CalibDataSize:%d, pCalibData: %p",
             mRealBokeh->mCaliData.i32CalibDataSize,
             mRealBokeh->mCaliData.pCalibData);
error:
    Arc_CaliData_Uninit(&hHandle);

    HAL_LOGI("X");

    return nRet;
}
#endif

/*===========================================================================
 * FUNCTION   :constructDefaultRequestSettings
 *
 * DESCRIPTION: construct Default Request Settings
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
const camera_metadata_t *SprdCamera3RealBokeh::constructDefaultRequestSettings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *fwk_metadata = NULL;

    SprdCamera3HWI *hw = m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].hwi;
    SprdCamera3HWI *hw_depth = m_pPhyCamera[CAM_TYPE_DEPTH].hwi;
    Mutex::Autolock l(mLock);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return NULL;
    }

    fwk_metadata = hw_depth->construct_default_request_settings(
        m_pPhyCamera[CAM_TYPE_DEPTH].dev, type);
    fwk_metadata = hw->construct_default_request_settings(
        m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].dev, type);
    if (!fwk_metadata) {
        HAL_LOGE("constructDefaultMetadata failed");
        return NULL;
    }
    CameraMetadata metadata;
    metadata = fwk_metadata;
    mOtpExist = false;
    if (metadata.exists(ANDROID_SPRD_OTP_DATA)) {
        uint8_t otpType;
        int otpSize;

        otpType = SprdCamera3Setting::s_setting[mRealBokeh->mCameraId]
                      .otpInfo.otp_type;
        otpSize = SprdCamera3Setting::s_setting[mRealBokeh->mCameraId]
                      .otpInfo.otp_size;
        HAL_LOGI("otpType %d, otpSize %d", otpType, otpSize);
        mOtpExist = true;
        mOtpType = otpType;
        mOtpSize = otpSize;

        if ((mRealBokeh->mApiVersion == SPRD_API_MODE) &&
            (otpType == OTP_CALI_SPRD)) {
            memcpy(mOtpData, metadata.find(ANDROID_SPRD_OTP_DATA).data.u8,
                   otpSize);
        } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
            memcpy(mArcSoftCalibData,
                   metadata.find(ANDROID_SPRD_OTP_DATA).data.u8, otpSize);

            if (otpType == OTP_CALI_ARCSOFT) {
                // it's 2303 bytes in all in arcsoft mode, but only 2048 is
                // useful, others is reserved.
                mCaliData.i32CalibDataSize = ARCSOFT_CALIB_DATA_SIZE;
                mCaliData.pCalibData = mArcSoftCalibData;
#ifdef CONFIG_ALTEK_ZTE_CALI
            } else if (otpType == OTP_CALI_ALTEK) {
                int rc;
                rc = createArcSoftCalibrationData(
                    (unsigned char *)mArcSoftCalibData, otpSize);
                if (rc) {
                    HAL_LOGE("create arcsoft calibration failed");
                }
#endif
            } else {
                HAL_LOGE("Unknown otp calibration type");
            }
        }
    }

    loadDebugOtp();
    HAL_LOGI("X");

    return fwk_metadata;
}

/*===========================================================================
 * FUNCTION   :checkOtpInfo
 *
 * DESCRIPTION: check whether Otp Info is correct
 *
 * PARAMETERS : NULL
 *
 * RETURN     :
 *                  0  : success
 *                  other: non-zero failure code
 *==========================================================================*/
int SprdCamera3RealBokeh::loadDebugOtp() {
    int rc = 0;
    if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
        mCaliData.i32CalibDataSize = ARCSOFT_CALIB_DATA_SIZE;
        mCaliData.pCalibData = mArcSoftCalibData;
        FILE *fid = fopen("/productinfo/calibration.bin", "rb");
        if (fid != NULL) {
            HAL_LOGD("open calibration file success");
            rc = fread(mCaliData.pCalibData, sizeof(char),
                       mCaliData.i32CalibDataSize, fid);
            HAL_LOGD("read otp size %d bytes", rc);
            fclose(fid);
            mOtpExist = true;
        } else {
            HAL_LOGW("no calibration file ");
            rc = -1;
        }
    } else {
        FILE *fid = fopen("/data/misc/cameraserver/calibration.txt", "rb");

        if (NULL == fid) {
            HAL_LOGD("dualotp read failed!");
            rc = -1;
        } else {
            uint32_t read_byte = 0;
            cmr_u8 *otp_data = (cmr_u8 *)mOtpData;

            while (!feof(fid)) {
                fscanf(fid, "%d\n", otp_data);
                otp_data += 4;
                read_byte += 4;
            }
            fclose(fid);
            mOtpSize = read_byte;
            mOtpExist = true;
            HAL_LOGD("dualotp read_bytes=%d ", read_byte);
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   :saveRequest
 *
 * DESCRIPTION: save buffer in request
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::saveRequest(camera3_capture_request_t *request) {
    size_t i = 0;
    camera3_stream_t *newStream = NULL;
    multi_request_saved_t currRequest;
    Mutex::Autolock l(mRequestLock);
    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        if (getStreamType(newStream) == CALLBACK_STREAM) {
            currRequest.buffer = request->output_buffers[i].buffer;
            currRequest.preview_stream = request->output_buffers[i].stream;
            currRequest.input_buffer = request->input_buffer;
            currRequest.frame_number = request->frame_number;
            HAL_LOGD("save request:id %d, ", request->frame_number);
            mSavedRequestList.push_back(currRequest);
        } else if (getStreamType(newStream) == DEFAULT_STREAM) {
            mThumbReq.buffer = request->output_buffers[i].buffer;
            mThumbReq.preview_stream = request->output_buffers[i].stream;
            mThumbReq.input_buffer = request->input_buffer;
            mThumbReq.frame_number = request->frame_number;
            HAL_LOGD("save thumb request:id %d, w= %d,h=%d",
                     request->frame_number, (mThumbReq.preview_stream)->width,
                     (mThumbReq.preview_stream)->height);
        }
    }
}

/*===========================================================================
 * FUNCTION   :processCaptureRequest
 *
 * DESCRIPTION: process Capture Request
 *
 * PARAMETERS :
 *    @device: camera3 device
 *    @request:camera3 request
 * RETURN     :
 *==========================================================================*/
int SprdCamera3RealBokeh::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;
    uint32_t i = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_DEPTH].hwi;
    CameraMetadata metaSettingsMain, metaSettingsAux;
    camera3_capture_request_t *req = request;
    camera3_capture_request_t req_main;
    camera3_capture_request_t req_aux;
    camera3_stream_t *new_stream = NULL;
    buffer_handle_t *new_buffer = NULL;
    camera3_stream_buffer_t out_streams_main[REAL_BOKEH_MAX_NUM_STREAMS];
    camera3_stream_buffer_t out_streams_aux[REAL_BOKEH_MAX_NUM_STREAMS];
    bool is_captureing = false;
    uint32_t tagCnt = 0;
    camera3_stream_t *preview_stream = NULL;
    char value[PROPERTY_VALUE_MAX] = {
        0,
    };
    bzero(out_streams_main,
          sizeof(camera3_stream_buffer_t) * REAL_BOKEH_MAX_NUM_STREAMS);
    bzero(out_streams_aux,
          sizeof(camera3_stream_buffer_t) * REAL_BOKEH_MAX_NUM_STREAMS);

    rc = validateCaptureRequest(req);
    if (rc != NO_ERROR) {
        return rc;
    }
    HAL_LOGV("frame_number:%d,num_output_buffers=%d", request->frame_number,
             request->num_output_buffers);
    metaSettingsMain = request->settings;
    metaSettingsAux = request->settings;

    for (size_t i = 0; i < req->num_output_buffers; i++) {
        int requestStreamType =
            getStreamType(request->output_buffers[i].stream);
        if (requestStreamType == SNAPSHOT_STREAM) {
            mIsCapturing = true;
            is_captureing = mIsCapturing;
            if (metaSettingsMain.exists(ANDROID_JPEG_ORIENTATION)) {
                mJpegOrientation =
                    metaSettingsMain.find(ANDROID_JPEG_ORIENTATION).data.i32[0];
            }
            HAL_LOGD("mJpegOrientation=%d", mJpegOrientation);
        } else if (requestStreamType == CALLBACK_STREAM) {
            preview_stream = (request->output_buffers[i]).stream;
            updateApiParams(metaSettingsMain, 0);
        }
    }

    tagCnt = metaSettingsMain.entryCount();
    if (tagCnt != 0) {
        if (metaSettingsMain.exists(ANDROID_SPRD_BURSTMODE_ENABLED)) {
            uint8_t sprdBurstModeEnabled = 0;
            metaSettingsMain.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                    &sprdBurstModeEnabled, 1);
            metaSettingsAux.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                   &sprdBurstModeEnabled, 1);
        }

        uint8_t sprdZslEnabled = 1;
        metaSettingsMain.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);
        metaSettingsAux.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);
    }
    saveRequest(request);
    /* save Perfectskinlevel */
    /*if (metaSettingsMain.exists(ANDROID_SPRD_UCAM_SKIN_LEVEL)) {
        mPerfectskinlevel.smoothLevel =
            metaSettingsMain.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[0];
        HAL_LOGV("smoothLevel=%d", mPerfectskinlevel.smoothLevel);
    }*/
    req_main = *req;
    req_aux = *req;
    req_main.settings = metaSettingsMain.release();
    req_aux.settings = metaSettingsAux.release();

    int main_buffer_index = 0;
    int aux_buffer_index = 0;
    for (size_t i = 0; i < req->num_output_buffers; i++) {
        int requestStreamType =
            getStreamType(request->output_buffers[i].stream);
        new_stream = (req->output_buffers[i]).stream;
        new_buffer = (req->output_buffers[i]).buffer;
        HAL_LOGV("num_output_buffers:%d, streamtype:%d",
                 req->num_output_buffers, requestStreamType);

        if (requestStreamType == SNAPSHOT_STREAM) {
            // first step: save capture request stream info
            if (NULL != mCaptureThread->mSavedCapReqsettings) {
                free_camera_metadata(mCaptureThread->mSavedCapReqsettings);
                mCaptureThread->mSavedCapReqsettings = NULL;
            }
            mCaptureThread->mSavedCapReqsettings =
                clone_camera_metadata(req_main.settings);
            mCaptureThread->mSavedOneResultBuff = NULL;
            mCaptureThread->mSavedCapRequest = *req;
            mCaptureThread->mSavedCapReqStreamBuff = req->output_buffers[i];
            mSavedCapStreams = req->output_buffers[i].stream;
            mCapFrameNumber = request->frame_number;
            mCaptureThread->mSavedResultBuff =
                request->output_buffers[i].buffer;
            mjpegSize = ADP_BUFSIZE(*new_buffer);
            // sencond step:construct callback Request
            out_streams_main[main_buffer_index] = req->output_buffers[i];
            if (!mFlushing) {
                out_streams_main[main_buffer_index].buffer =
                    (popBufferList(mLocalBufferList, SNAPSHOT_MAIN_BUFFER));
                out_streams_main[main_buffer_index].stream =
                    &mMainStreams[mCallbackStreamsNum];
            } else {
                out_streams_main[main_buffer_index].buffer =
                    (req->output_buffers[i]).buffer;
                out_streams_main[main_buffer_index].stream =
                    &mMainStreams[mCaptureStreamsNum];
            }
            HAL_LOGD("jpeg frame newtype:%d, rotation:%d ,frame_number %u",
                     out_streams_main[main_buffer_index].stream->format,
                     out_streams_main[main_buffer_index].stream->rotation,
                     req->frame_number);

            if (NULL == out_streams_main[main_buffer_index].buffer) {
                HAL_LOGE("failed, LocalBufferList is empty!");
                goto req_fail;
            }
            main_buffer_index++;

            out_streams_aux[aux_buffer_index] = req->output_buffers[i];
            if (!mFlushing) {
                out_streams_aux[aux_buffer_index].buffer =
                    (popBufferList(mLocalBufferList, SNAPSHOT_DEPTH_BUFFER));
                out_streams_aux[aux_buffer_index].stream =
                    &mAuxStreams[mCallbackStreamsNum];
            } else {
                out_streams_aux[aux_buffer_index].buffer =
                    (req->output_buffers[i]).buffer;
                out_streams_aux[aux_buffer_index].stream =
                    &mAuxStreams[mCaptureStreamsNum];
            }
            out_streams_aux[aux_buffer_index].acquire_fence = -1;
            if (NULL == out_streams_aux[aux_buffer_index].buffer) {
                HAL_LOGE("failed, LocalBufferList is empty!");
                goto req_fail;
            }
            aux_buffer_index++;

        } else if (requestStreamType == CALLBACK_STREAM) {
            out_streams_main[main_buffer_index] = req->output_buffers[i];
            out_streams_main[main_buffer_index].stream =
                &mMainStreams[mPreviewStreamsNum];
            if (mIsSupportPBokeh) {
                out_streams_main[main_buffer_index].buffer =
                    (popBufferList(mLocalBufferList, PREVIEW_MAIN_BUFFER));
                if (NULL == out_streams_main[main_buffer_index].buffer) {
                    HAL_LOGE("failed, mPrevLocalBufferList is empty!");
                    return NO_MEMORY;
                }
            }
            main_buffer_index++;

            out_streams_aux[aux_buffer_index] = req->output_buffers[i];
            out_streams_aux[aux_buffer_index].stream =
                &mAuxStreams[mPreviewStreamsNum];

            out_streams_aux[aux_buffer_index].buffer =
                (popBufferList(mLocalBufferList, PREVIEW_DEPTH_BUFFER));
            if (NULL == out_streams_aux[aux_buffer_index].buffer) {
                HAL_LOGE("failed, mDepthLocalBufferList is empty!");
                return NO_MEMORY;
            }

            out_streams_aux[aux_buffer_index].acquire_fence = -1;
            aux_buffer_index++;
        } else {
            if (mhasCallbackStream) {
                HAL_LOGD("has callback stream,num_output_buffers=%d",
                         request->num_output_buffers);
            }
        }
    }
    req_main.num_output_buffers = main_buffer_index;
    req_aux.num_output_buffers = aux_buffer_index;
    req_main.output_buffers = out_streams_main;
    req_aux.output_buffers = out_streams_aux;

    if (is_captureing) {
        struct timespec t1;
        clock_gettime(CLOCK_BOOTTIME, &t1);
        uint64_t currentmainTimestamp = (t1.tv_sec) * 1000000000LL + t1.tv_nsec;
        uint64_t currentauxTimestamp = currentmainTimestamp;
        HAL_LOGV("currentmainTimestamp=%llu,currentauxTimestamp=%llu",
                 currentmainTimestamp, currentauxTimestamp);
        hwiMain->setMultiCallBackYuvMode(true);
        hwiAux->setMultiCallBackYuvMode(true);
        if (currentmainTimestamp < currentauxTimestamp) {
            HAL_LOGD("start main, idx:%d", req_main.frame_number);
            hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP,
                                   &currentmainTimestamp, NULL);
            rc = hwiMain->process_capture_request(
                m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].dev, &req_main);
            if (rc < 0) {
                HAL_LOGE("failed, idx:%d", req_main.frame_number);
                goto req_fail;
            }
            hwiAux->camera_ioctrl(CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP,
                                  &currentmainTimestamp, NULL);
            HAL_LOGD("start sub, idx:%d", req_aux.frame_number);
            if (!mFlushing) {
                rc = hwiAux->process_capture_request(
                    m_pPhyCamera[CAM_TYPE_DEPTH].dev, &req_aux);
                if (rc < 0) {
                    HAL_LOGE("failed, idx:%d", req_aux.frame_number);
                    goto req_fail;
                }
            }
        } else {
            HAL_LOGD("start sub, idx:%d,currentauxTimestamp=%llu",
                     req_aux.frame_number, currentauxTimestamp);
            hwiAux->camera_ioctrl(CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP,
                                  &currentmainTimestamp, NULL);
            if (!mFlushing) {
                rc = hwiAux->process_capture_request(
                    m_pPhyCamera[CAM_TYPE_DEPTH].dev, &req_aux);
                if (rc < 0) {
                    HAL_LOGE("failed, idx:%d", req_aux.frame_number);
                    goto req_fail;
                }
            }
            hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP,
                                   &currentmainTimestamp, NULL);
            HAL_LOGD("start main, idx:%d", req_main.frame_number);
            rc = hwiMain->process_capture_request(
                m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].dev, &req_main);
            if (rc < 0) {
                HAL_LOGE("failed, idx:%d", req_main.frame_number);
                goto req_fail;
            }
        }
    } else {
        rc = hwiMain->process_capture_request(
            m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].dev, &req_main);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_main.frame_number);
            goto req_fail;
        }
        rc = hwiAux->process_capture_request(m_pPhyCamera[CAM_TYPE_DEPTH].dev,
                                             &req_aux);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_aux.frame_number);
            goto req_fail;
        }

        mMaxPendingCount =
            preview_stream->max_buffers - MAX_UNMATCHED_QUEUE_SIZE + 1;
        {
            Mutex::Autolock l(mPendingLock);
            size_t pendingCount = 0;
            mPendingRequest++;
            HAL_LOGV("mPendingRequest=%d, mMaxPendingCount=%d", mPendingRequest,
                     mMaxPendingCount);
            while (mPendingRequest >= mMaxPendingCount) {
                mRequestSignal.waitRelative(mPendingLock, PENDINGTIME);
                if (pendingCount > (PENDINGTIMEOUT / PENDINGTIME)) {
                    HAL_LOGD("m_PendingRequest=%d", mPendingRequest);
                    rc = -ENODEV;
                    break;
                }
                pendingCount++;
            }
        }
    }
req_fail:
    if (req_main.settings)
        free_camera_metadata((camera_metadata_t *)req_main.settings);

    if (req_aux.settings)
        free_camera_metadata((camera_metadata_t *)req_aux.settings);
    HAL_LOGV("rc. %d idx%d", rc, request->frame_number);

    return rc;
}

/*===========================================================================
 * FUNCTION   :notifyMain
 *
 * DESCRIPTION: device notify
 *
 * PARAMETERS : notify message
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::notifyMain(const camera3_notify_msg_t *msg) {
    uint32_t cur_frame_number = msg->message.shutter.frame_number;

    if (msg->type == CAMERA3_MSG_SHUTTER &&
        cur_frame_number == mCaptureThread->mSavedCapRequest.frame_number &&
        mCaptureThread->mReprocessing) {
        HAL_LOGD(" hold cap notify");
        return;
    }

    if ((!(cur_frame_number == mCapFrameNumber && cur_frame_number != 0)) &&
        mIsSupportPBokeh) {
        Mutex::Autolock l(mNotifyLockMain);
        mNotifyListMain.push_back(*msg);
    }

    mCallbackOps->notify(mCallbackOps, msg);
}

/*===========================================================================
 * FUNCTION   :thumbYuvProc
 *
 * DESCRIPTION: process thumb yuv
 *
 * PARAMETERS : process thumb yuv
 *
 * RETURN     : None
 *==========================================================================*/

int SprdCamera3RealBokeh::thumbYuvProc(buffer_handle_t *src_buffer) {
    int ret = NO_ERROR;
    int angle = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].hwi;
    struct img_frm src_img;
    struct img_frm dst_img;
    struct snp_thumb_yuv_param thumb_param;
    void *src_buffer_addr = NULL;
    void *thumb_req_addr = NULL;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    HAL_LOGI(" E");

    ret = mRealBokeh->map(src_buffer, &src_buffer_addr);
    if (ret != NO_ERROR) {
        HAL_LOGE("fail to map src buffer");
        goto fail_map_src;
    }
    ret = mRealBokeh->map(mThumbReq.buffer, &thumb_req_addr);
    if (ret != NO_ERROR) {
        HAL_LOGE("fail to map thumb buffer");
        goto fail_map_thumb;
    }

    memset(&src_img, 0, sizeof(struct img_frm));
    memset(&dst_img, 0, sizeof(struct img_frm));
    src_img.addr_phy.addr_y = 0;
    src_img.addr_phy.addr_u = src_img.addr_phy.addr_y +
                              ADP_WIDTH(*src_buffer) * ADP_HEIGHT(*src_buffer);
    src_img.addr_phy.addr_v = src_img.addr_phy.addr_u;
    src_img.addr_vir.addr_y = (cmr_uint)src_buffer_addr;
    src_img.addr_vir.addr_u = (cmr_uint)src_buffer_addr +
                              ADP_WIDTH(*src_buffer) * ADP_HEIGHT(*src_buffer);
    src_img.buf_size = ADP_BUFSIZE(*src_buffer);
    src_img.fd = ADP_BUFFD(*src_buffer);
    src_img.fmt = IMG_DATA_TYPE_YUV420;
    src_img.rect.start_x = 0;
    src_img.rect.start_y = 0;
    src_img.rect.width = ADP_WIDTH(*src_buffer);
    src_img.rect.height = ADP_HEIGHT(*src_buffer);
    src_img.size.width = ADP_WIDTH(*src_buffer);
    src_img.size.height = ADP_HEIGHT(*src_buffer);

    dst_img.addr_phy.addr_y = 0;
    dst_img.addr_phy.addr_u =
        dst_img.addr_phy.addr_y +
        ADP_WIDTH(*mThumbReq.buffer) * ADP_HEIGHT(*mThumbReq.buffer);
    dst_img.addr_phy.addr_v = dst_img.addr_phy.addr_u;

    dst_img.addr_vir.addr_y = (cmr_uint)thumb_req_addr;
    dst_img.addr_vir.addr_u =
        (cmr_uint)thumb_req_addr +
        ADP_WIDTH(*mThumbReq.buffer) * ADP_HEIGHT(*mThumbReq.buffer);
    dst_img.buf_size = ADP_BUFSIZE(*mThumbReq.buffer);
    dst_img.fd = ADP_BUFFD(*mThumbReq.buffer);
    dst_img.fmt = IMG_DATA_TYPE_YUV420;
    dst_img.rect.start_x = 0;
    dst_img.rect.start_y = 0;
    dst_img.rect.width = ADP_WIDTH(*mThumbReq.buffer);
    dst_img.rect.height = ADP_HEIGHT(*mThumbReq.buffer);
    dst_img.size.width = ADP_WIDTH(*mThumbReq.buffer);
    dst_img.size.height = ADP_HEIGHT(*mThumbReq.buffer);

    memcpy(&thumb_param.src_img, &src_img, sizeof(struct img_frm));
    memcpy(&thumb_param.dst_img, &dst_img, sizeof(struct img_frm));
    switch (mJpegOrientation) {
    case 0:
        angle = IMG_ANGLE_0;
        break;

    case 180:
        angle = IMG_ANGLE_180;
        break;

    case 90:
        angle = IMG_ANGLE_90;
        break;

    case 270:
        angle = IMG_ANGLE_270;
        break;

    default:
        angle = IMG_ANGLE_0;
        break;
    }
    thumb_param.angle = angle;

    hwiMain->camera_ioctrl(CAMERA_IOCTRL_THUMB_YUV_PROC, &thumb_param, NULL);

    property_get("persist.sys.bokeh.dump.thumb", prop, "0");
    if (atoi(prop) == 1) {
        dumpData((unsigned char *)src_img.addr_vir.addr_y, 1, src_img.buf_size,
                 src_img.rect.width, src_img.size.height, 5, "input");
        dumpData((unsigned char *)dst_img.addr_vir.addr_y, 1, dst_img.buf_size,
                 dst_img.rect.width, dst_img.size.height, 5, "output");
    }

    HAL_LOGI(" x,angle=%d JpegOrientation=%d", thumb_param.angle,
             mJpegOrientation);

    mRealBokeh->unmap(mThumbReq.buffer);
fail_map_thumb:
    mRealBokeh->unmap(src_buffer);
fail_map_src:
    return ret;
}

void SprdCamera3RealBokeh::dumpCaptureBokeh(unsigned char *result_buffer_addr,
                                            uint32_t jpeg_size) {
    char prop1[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.sys.camera.bokeh1", prop1, "0");
    if (1 == atoi(prop1)) {
        uint32_t para_size = 0;
        uint32_t depth_size = 0;
        int common_num = 0;
        int depth_width = 0, depth_height = 0;

        if (mRealBokeh->mApiVersion == SPRD_API_MODE) {
            para_size = BOKEH_REFOCUS_COMMON_PARAM_NUM * 4;
            depth_size =
                mDepthSnapOutWidth * mDepthSnapOutHeight + DEPTH_DATA_SIZE;
            common_num = BOKEH_REFOCUS_COMMON_PARAM_NUM;
            depth_width = mDepthSnapOutWidth;
            depth_height = mDepthSnapOutHeight;
        } else if (mRealBokeh->mApiVersion == ARCSOFT_API_MODE) {
            para_size = ARCSOFT_BOKEH_REFOCUS_COMMON_PARAM_NUM * 4;
            depth_size = mCaptureThread->mArcSoftDepthSize;
            common_num = ARCSOFT_BOKEH_REFOCUS_COMMON_PARAM_NUM;
            depth_width = mCaptureWidth;
            depth_height = mCaptureHeight;
        }
        uint32_t orig_yuv_size =
            mRealBokeh->mCaptureWidth * mRealBokeh->mCaptureHeight * 3 / 2;
        unsigned char *buffer_base = result_buffer_addr;
        dumpData(buffer_base, 2, jpeg_size, mCaptureWidth, mCaptureHeight, 0,
                 "bokehJpeg");

#ifdef YUV_CONVERT_TO_JPEG
        uint32_t use_size =
            para_size + depth_size + mRealBokeh->mOrigJpegSize + jpeg_size;
#else
        uint32_t use_size = para_size + depth_size + orig_yuv_size + jpeg_size;
#endif
        buffer_base += (use_size - para_size);
        dumpData(buffer_base, 3, common_num, 4, 0, 0, "parameter");
        buffer_base -= (int)(depth_size);

        dumpData(buffer_base, 1, depth_size, depth_width, depth_height, 0,
                 "depth");
#ifdef YUV_CONVERT_TO_JPEG
        buffer_base -= (int)(mRealBokeh->mOrigJpegSize);
        dumpData(buffer_base, 2, mRealBokeh->mOrigJpegSize, mCaptureWidth,
                 mCaptureHeight, 0, "origJpeg");
#else
        buffer_base -= (int)(orig_yuv_size);
        dumpData(buffer_base, 1, orig_yuv_size, mCaptureWidth, mCaptureHeight,
                 0, "origYuv");
#endif
    }
}

/*===========================================================================
 * FUNCTION   :processCaptureResultMain
 *
 * DESCRIPTION: process Capture Result from the main hwi
 *
 * PARAMETERS : capture result structure from hwi1
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::processCaptureResultMain(
    const camera3_capture_result_t *result) {
    uint64_t result_timestamp = 0;
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    CameraMetadata metadata;
    meta_save_t metadata_t;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_DEPTH].hwi;

    if (result_buffer == NULL) {
        // meta process
        if (cur_frame_number == mCapFrameNumber && cur_frame_number != 0) {
            if (mCaptureThread->mReprocessing) {
                HAL_LOGD("hold jpeg picture call bac1k, framenumber:%d",
                         result->frame_number);
            } else {
                metadata = result->result;
                mVcmSteps = metadata.find(ANDROID_SPRD_VCM_STEP).data.i32[0];
                HAL_LOGD("mVcmSteps %d", mVcmSteps);
                {
                    Mutex::Autolock l(mRealBokeh->mMetatLock);
                    metadata_t.frame_number = cur_frame_number;
                    metadata_t.metadata = clone_camera_metadata(result->result);
                    mMetadataList.push_back(metadata_t);
                }
                CallBackMetadata();
            }
            return;
        } else {

            HAL_LOGV("send  meta, framenumber:%d", cur_frame_number);
            metadata_t.frame_number = cur_frame_number;
            metadata_t.metadata = clone_camera_metadata(result->result);
            Mutex::Autolock l(mRealBokeh->mMetatLock);
            mMetadataList.push_back(metadata_t);
            return;
        }
    }
    int currStreamType = getStreamType(result_buffer->stream);
    if (mIsCapturing && currStreamType == DEFAULT_STREAM) {
        if (mhasCallbackStream && mThumbReq.frame_number) {
            thumbYuvProc(result->output_buffers->buffer);
            CallBackSnapResult();
        }
        Mutex::Autolock l(mDefaultStreamLock);
        if (NULL == mCaptureThread->mSavedOneResultBuff) {
            mCaptureThread->mSavedOneResultBuff =
                result->output_buffers->buffer;
        } else {
            capture_queue_msg_t_bokeh capture_msg;
            capture_msg.msg_type = BOKEH_MSG_DATA_PROC;
            capture_msg.combo_buff.frame_number = result->frame_number;
            capture_msg.combo_buff.buffer1 = result->output_buffers->buffer;
            capture_msg.combo_buff.buffer2 =
                mCaptureThread->mSavedOneResultBuff;
            capture_msg.combo_buff.input_buffer = result->input_buffer;
            HAL_LOGD("capture combined begin: framenumber %d",
                     capture_msg.combo_buff.frame_number);
            {
                hwiMain->setMultiCallBackYuvMode(false);
                hwiAux->setMultiCallBackYuvMode(false);
                Mutex::Autolock l(mCaptureThread->mMergequeueMutex);
                HAL_LOGD("Enqueue combo frame:%d for frame merge!",
                         capture_msg.combo_buff.frame_number);
                mCaptureThread->mCaptureMsgList.push_back(capture_msg);
                mCaptureThread->mMergequeueSignal.signal();
            }
        }
    } else if (mIsCapturing && currStreamType == SNAPSHOT_STREAM) {
        camera3_capture_result_t newResult = *result;
        camera3_stream_buffer_t newOutput_buffers = *(result->output_buffers);
        HAL_LOGD("jpeg callback: framenumber %d", cur_frame_number);
        newOutput_buffers.stream = mSavedCapStreams;
        newResult.input_buffer = NULL;
        newResult.result = NULL;
        newResult.partial_result = 0;
        newResult.output_buffers = &newOutput_buffers;
        int rc = NO_ERROR;
        unsigned char *result_buffer_addr = NULL;
        uint32_t result_buffer_size =
            ADP_BUFSIZE(*result->output_buffers->buffer);
        rc = mRealBokeh->map(result->output_buffers->buffer,
                             (void **)&result_buffer_addr);
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to map result buffer");
            return;
        }
        uint32_t jpeg_size =
            getJpegSize(result_buffer_addr, result_buffer_size);
        ADP_FAKESETBUFATTR_CAMERAONLY(
            *result->output_buffers->buffer, mjpegSize,
            ADP_WIDTH(*result->output_buffers->buffer),
            ADP_HEIGHT(*result->output_buffers->buffer));

#ifdef YUV_CONVERT_TO_JPEG
        if ((mCaptureThread->mBokehResult == true) &&
            (mRealBokeh->mOrigJpegSize > 0)) {
#else
        if (mCaptureThread->mBokehResult == true) {
#endif
            mCaptureThread->saveCaptureBokehParams(result_buffer_addr,
                                                   mjpegSize, jpeg_size);

            dumpCaptureBokeh(result_buffer_addr, jpeg_size);
        } else {
            mRealBokeh->setJpegSize((char *)result_buffer_addr, mjpegSize,
                                    jpeg_size);
        }

        mRealBokeh->pushBufferList(
            mRealBokeh->mLocalBuffer, mRealBokeh->m_pDstJpegBuffer,
            mRealBokeh->mLocalBufferNumber, mRealBokeh->mLocalBufferList);

        mCallbackOps->process_capture_result(mCallbackOps, &newResult);
        mCaptureThread->mReprocessing = false;
        mIsCapturing = false;
        mRealBokeh->unmap(result->output_buffers->buffer);
    } else if (currStreamType == CALLBACK_STREAM) {
        if (!mIsSupportPBokeh) {
            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_OK);
            return;
        }
        // process preview buffer
        {
            Mutex::Autolock l(mNotifyLockMain);
            for (List<camera3_notify_msg_t>::iterator i =
                     mNotifyListMain.begin();
                 i != mNotifyListMain.end(); i++) {
                if (i->message.shutter.frame_number == cur_frame_number) {
                    if (i->type == CAMERA3_MSG_SHUTTER) {
                        searchnotifyresult = NOTIFY_SUCCESS;
                        result_timestamp = i->message.shutter.timestamp;
                        mNotifyListMain.erase(i);
                    } else if (i->type == CAMERA3_MSG_ERROR) {
                        HAL_LOGE("Return local buffer:%d caused by error "
                                 "Notify status",
                                 result->frame_number);
                        searchnotifyresult = NOTIFY_ERROR;
                        pushBufferList(mLocalBuffer,
                                       result->output_buffers->buffer,
                                       mLocalBufferNumber, mLocalBufferList);
                        CallBackResult(cur_frame_number,
                                       CAMERA3_BUFFER_STATUS_ERROR);
                        mNotifyListMain.erase(i);
                        return;
                    }
                }
            }
        }
        if (searchnotifyresult == NOTIFY_NOT_FOUND) {
            HAL_LOGE("found no corresponding notify");
            return;
        }
        /* Process error buffer for Main camera*/
        if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
            HAL_LOGD("Return local buffer:%d caused by error Buffer status",
                     result->frame_number);
            pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_ERROR);

            return;
        }
        hwi_frame_buffer_info_t matched_frame;
        hwi_frame_buffer_info_t cur_frame;

        memset(&matched_frame, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&cur_frame, 0, sizeof(hwi_frame_buffer_info_t));
        cur_frame.frame_number = cur_frame_number;
        cur_frame.timestamp = result_timestamp;
        cur_frame.buffer = (result->output_buffers)->buffer;
        {
            Mutex::Autolock l(mUnmatchedQueueLock);
            if (MATCH_SUCCESS == matchTwoFrame(cur_frame,
                                               mUnmatchedFrameListAux,
                                               &matched_frame)) {
                muxer_queue_msg_t muxer_msg;
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number = cur_frame.frame_number;
                muxer_msg.combo_frame.buffer1 = cur_frame.buffer;
                muxer_msg.combo_frame.buffer2 = matched_frame.buffer;
                muxer_msg.combo_frame.input_buffer = result->input_buffer;
                {
                    Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
                    HAL_LOGD("Enqueue combo frame:%d for frame merge!",
                             muxer_msg.combo_frame.frame_number);
                    if (cur_frame.frame_number > 5)
                        clearFrameNeverMatched(cur_frame.frame_number,
                                               matched_frame.frame_number);
                    mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(
                        muxer_msg);
                    mPreviewMuxerThread->mMergequeueSignal.signal();
                }
            } else {
                HAL_LOGV("Enqueue newest unmatched frame:%d for Main camera",
                         cur_frame.frame_number);
                hwi_frame_buffer_info_t *discard_frame =
                    pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListMain);
                if (discard_frame != NULL) {
                    HAL_LOGD("discard frame_number %u", cur_frame_number);
                    pushBufferList(mLocalBuffer, discard_frame->buffer,
                                   mLocalBufferNumber, mLocalBufferList);
                    CallBackResult(discard_frame->frame_number,
                                   CAMERA3_BUFFER_STATUS_ERROR);
                    delete discard_frame;
                }
            }
        }
    }

    return;
}

/*===========================================================================
 * FUNCTION   :notifyAux
 *
 * DESCRIPTION: notifyAux
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::notifyAux(const camera3_notify_msg_t *msg) {
    uint32_t cur_frame_number = msg->message.shutter.frame_number;

    if ((!(cur_frame_number == mCapFrameNumber && cur_frame_number != 0)) &&
        mIsSupportPBokeh) {
        Mutex::Autolock l(mNotifyLockAux);
        HAL_LOGV("notifyAux push success frame_number %d", cur_frame_number);
        mNotifyListAux.push_back(*msg);
    }
    return;
}

/*===========================================================================
 * FUNCTION   :processCaptureResultMain
 *
 * DESCRIPTION: process Capture Result from the aux hwi
 *
 * PARAMETERS : capture result structure from hwi2
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::processCaptureResultAux(
    const camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    CameraMetadata metadata;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    metadata = result->result;
    uint64_t result_timestamp = 0;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    camera3_stream_t *newStream = NULL;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_DEPTH].hwi;

    HAL_LOGV("aux frame_number %d", cur_frame_number);
    if (result->output_buffers == NULL) {
        return;
    }
    int currStreamType = getStreamType(result_buffer->stream);
    if (mIsCapturing && currStreamType == DEFAULT_STREAM) {
        Mutex::Autolock l(mDefaultStreamLock);
        result_buffer = result->output_buffers;
        if (NULL == mCaptureThread->mSavedOneResultBuff) {
            mCaptureThread->mSavedOneResultBuff =
                result->output_buffers->buffer;
        } else {
            capture_queue_msg_t_bokeh capture_msg;

            capture_msg.msg_type = BOKEH_MSG_DATA_PROC;
            capture_msg.combo_buff.frame_number = result->frame_number;
            capture_msg.combo_buff.buffer1 =
                mCaptureThread->mSavedOneResultBuff;
            capture_msg.combo_buff.buffer2 = result->output_buffers->buffer;
            capture_msg.combo_buff.input_buffer = result->input_buffer;
            HAL_LOGD("capture combined begin: framenumber %d",
                     capture_msg.combo_buff.frame_number);
            {
                hwiMain->setMultiCallBackYuvMode(false);
                hwiAux->setMultiCallBackYuvMode(false);
                Mutex::Autolock l(mCaptureThread->mMergequeueMutex);
                HAL_LOGD("Enqueue combo frame:%d for frame merge!",
                         capture_msg.combo_buff.frame_number);
                mCaptureThread->mCaptureMsgList.push_back(capture_msg);
                mCaptureThread->mMergequeueSignal.signal();
            }
        }
    } else if (mIsCapturing && currStreamType == SNAPSHOT_STREAM) {
        HAL_LOGD("should not entry here, shutter frame:%d",
                 result->frame_number);
    } else if (currStreamType == CALLBACK_STREAM) {
        if (!mIsSupportPBokeh) {
            pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
            return;
        }
        // process preview buffer
        {
            Mutex::Autolock l(mNotifyLockAux);
            for (List<camera3_notify_msg_t>::iterator i =
                     mNotifyListAux.begin();
                 i != mNotifyListAux.end(); i++) {
                if (i->message.shutter.frame_number == cur_frame_number) {
                    if (i->type == CAMERA3_MSG_SHUTTER) {
                        searchnotifyresult = NOTIFY_SUCCESS;
                        result_timestamp = i->message.shutter.timestamp;
                        mNotifyListAux.erase(i);
                    } else if (i->type == CAMERA3_MSG_ERROR) {
                        HAL_LOGE("Return local buffer:%d caused by error "
                                 "Notify status",
                                 result->frame_number);
                        searchnotifyresult = NOTIFY_ERROR;
                        pushBufferList(mLocalBuffer,
                                       result->output_buffers->buffer,
                                       mLocalBufferNumber, mLocalBufferList);
                        mNotifyListAux.erase(i);
                        return;
                    }
                }
            }
        }
        if (searchnotifyresult == NOTIFY_NOT_FOUND) {
            HAL_LOGE("found no corresponding notify");
            return;
        }
        /* Process error buffer for Aux camera: just return local buffer*/
        if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
            HAL_LOGD("Return local buffer:%d caused by error Buffer status",
                     result->frame_number);
            pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
            return;
        }
        hwi_frame_buffer_info_t matched_frame;
        hwi_frame_buffer_info_t cur_frame;

        memset(&matched_frame, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&cur_frame, 0, sizeof(hwi_frame_buffer_info_t));
        cur_frame.frame_number = cur_frame_number;
        cur_frame.timestamp = result_timestamp;
        cur_frame.buffer = (result->output_buffers)->buffer;
        {
            Mutex::Autolock l(mUnmatchedQueueLock);
            if (MATCH_SUCCESS == matchTwoFrame(cur_frame,
                                               mUnmatchedFrameListMain,
                                               &matched_frame)) {
                muxer_queue_msg_t muxer_msg;
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number = matched_frame.frame_number;
                muxer_msg.combo_frame.buffer1 = matched_frame.buffer;
                muxer_msg.combo_frame.buffer2 = cur_frame.buffer;
                muxer_msg.combo_frame.input_buffer = result->input_buffer;
                {
                    Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
                    HAL_LOGD("Enqueue combo frame:%d for frame merge!",
                             muxer_msg.combo_frame.frame_number);
                    // we don't call clearFrameNeverMatched before five frame.
                    // for first frame meta and ok status buffer update at the
                    // same time.
                    // app need the point that the first meta updated to hide
                    // image cover
                    if (cur_frame.frame_number > 5)
                        clearFrameNeverMatched(matched_frame.frame_number,
                                               cur_frame.frame_number);
                    mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(
                        muxer_msg);
                    mPreviewMuxerThread->mMergequeueSignal.signal();
                }
            } else {
                HAL_LOGV("Enqueue newest unmatched frame:%d for Aux camera",
                         cur_frame.frame_number);
                hwi_frame_buffer_info_t *discard_frame =
                    pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListAux);
                if (discard_frame != NULL) {
                    pushBufferList(mLocalBuffer, discard_frame->buffer,
                                   mLocalBufferNumber, mLocalBufferList);
                    delete discard_frame;
                }
            }
        }
    }

    return;
}

/*===========================================================================
 * FUNCTION   :CallBackMetadata
 *
 * DESCRIPTION: CallBackMetadata
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::CallBackMetadata() {

    camera3_capture_result_t result;
    bzero(&result, sizeof(camera3_capture_result_t));
    List<meta_save_t>::iterator itor;
    {
        Mutex::Autolock l(mMetatLock);
        itor = mMetadataList.begin();
        while (itor != mMetadataList.end()) {
            result.frame_number = itor->frame_number;
            result.result = const_cast<camera_metadata_t *>(itor->metadata);
            result.num_output_buffers = 0;
            result.output_buffers = NULL;
            result.input_buffer = NULL;
            result.partial_result = 1;
            mCallbackOps->process_capture_result(mCallbackOps, &result);
            free_camera_metadata(
                const_cast<camera_metadata_t *>(result.result));
            mMetadataList.erase(itor);
            itor++;
        }
    }
}

/*===========================================================================
 * FUNCTION   :CallBackSnapResult
 *
 * DESCRIPTION: CallBackSnapResult
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::CallBackSnapResult() {

    camera3_capture_result_t result;
    camera3_stream_buffer_t result_buffers;
    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&result_buffers, sizeof(camera3_stream_buffer_t));

    result_buffers.stream = mThumbReq.preview_stream;
    result_buffers.buffer = mThumbReq.buffer;

    result_buffers.status = CAMERA3_BUFFER_STATUS_OK;
    result_buffers.acquire_fence = -1;
    result_buffers.release_fence = -1;
    result.result = NULL;
    result.frame_number = mThumbReq.frame_number;
    result.num_output_buffers = 1;
    result.output_buffers = &result_buffers;
    result.input_buffer = NULL;
    result.partial_result = 0;

    mCallbackOps->process_capture_result(mCallbackOps, &result);
    memset(&mThumbReq, 0, sizeof(multi_request_saved_t));
    HAL_LOGD("snap id:%d ", result.frame_number);
}

/*===========================================================================
 * FUNCTION   :CallBackResult
 *
 * DESCRIPTION: CallBackResult
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::CallBackResult(
    uint32_t frame_number, camera3_buffer_status_t buffer_status) {

    camera3_capture_result_t result;
    List<multi_request_saved_t>::iterator itor;
    camera3_stream_buffer_t result_buffers;
    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&result_buffers, sizeof(camera3_stream_buffer_t));

    CallBackMetadata();
    if ((frame_number != mRealBokeh->mCapFrameNumber) || (frame_number == 0)) {
        Mutex::Autolock l(mRealBokeh->mRequestLock);
        itor = mRealBokeh->mSavedRequestList.begin();
        while (itor != mRealBokeh->mSavedRequestList.end()) {
            if (itor->frame_number == frame_number) {
                HAL_LOGV("erase frame_number %u", frame_number);
                result_buffers.stream = itor->preview_stream;
                result_buffers.buffer = itor->buffer;
                mRealBokeh->mSavedRequestList.erase(itor);
                break;
            }
            itor++;
        }
        if (itor == mRealBokeh->mSavedRequestList.end()) {
            HAL_LOGE("can't find frame in mSavedRequestList %u:", frame_number);
            return;
        }
    } else {
        result_buffers.stream = mRealBokeh->mSavedCapStreams;
        result_buffers.buffer = mCaptureThread->mSavedCapReqStreamBuff.buffer;
    }

    result_buffers.status = buffer_status;
    result_buffers.acquire_fence = -1;
    result_buffers.release_fence = -1;
    result.result = NULL;
    result.frame_number = frame_number;
    result.num_output_buffers = 1;
    result.output_buffers = &result_buffers;
    result.input_buffer = NULL;
    result.partial_result = 0;

    mCallbackOps->process_capture_result(mCallbackOps, &result);
    HAL_LOGI("id:%d buffer_status %u", result.frame_number, buffer_status);
    if (!buffer_status) {
        mRealBokeh->dumpFps();
    }

    if ((frame_number != mRealBokeh->mCapFrameNumber) || (frame_number == 0)) {
        Mutex::Autolock l(mRealBokeh->mPendingLock);
        mRealBokeh->mPendingRequest--;
        if (mRealBokeh->mPendingRequest < mRealBokeh->mMaxPendingCount) {
            HAL_LOGD("signal request m_PendingRequest = %d",
                     mRealBokeh->mPendingRequest);
            mRealBokeh->mRequestSignal.signal();
        }
    }
}

/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: deconstructor of SprdCamera3RealBokeh
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::_dump(const struct camera3_device *device, int fd) {
    HAL_LOGI("E");

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :bokehThreadExit
 *
 * DESCRIPTION: preview and capture thread exit
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::bokehThreadExit(void) {

    HAL_LOGI("E");
    if (mCaptureThread != NULL) {
        if ((mRealBokeh->mApiVersion == SPRD_API_MODE) &&
            mCaptureThread->mCapDepthhandle && mDepthApi)
            mDepthApi->sprd_depth_Set_Stopflag(mCaptureThread->mCapDepthhandle,
                                               DEPTH_STOP);

        if (mCaptureThread->isRunning()) {
            mCaptureThread->requestExit();
        }
    }
    if (mPreviewMuxerThread != NULL) {
        if ((mRealBokeh->mApiVersion == SPRD_API_MODE) &&
            mPreviewMuxerThread->mPrevDepthhandle && mDepthApi)
            mDepthApi->sprd_depth_Set_Stopflag(
                mPreviewMuxerThread->mPrevDepthhandle, DEPTH_STOP);

        if (mPreviewMuxerThread->isRunning()) {
            mPreviewMuxerThread->requestExit();
        }
    }
    // wait threads quit to relese object
    mCaptureThread->join();
    mPreviewMuxerThread->join();

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :flush
 *
 * DESCRIPTION: flush
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3RealBokeh::_flush(const struct camera3_device *device) {
    int rc = 0;
    HAL_LOGI("E");
    mFlushing = true;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_DEPTH].hwi;
    rc = hwiAux->flush(m_pPhyCamera[CAM_TYPE_DEPTH].dev);

    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_BOKEH_MAIN].dev);

    bokehThreadExit();

    HAL_LOGI("X");
    return rc;
}

/*===========================================================================
 * FUNCTION   :clearFrameNeverMatched
 *
 * DESCRIPTION: clear earlier frame which will never be matched any more
 *
 * PARAMETERS : which camera queue to be clear
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RealBokeh::clearFrameNeverMatched(uint32_t main_frame_number,
                                                  uint32_t sub_frame_number) {
    List<hwi_frame_buffer_info_t>::iterator itor;
    uint32_t frame_num = 0;

    itor = mUnmatchedFrameListMain.begin();
    while (itor != mUnmatchedFrameListMain.end()) {
        if (itor->frame_number < main_frame_number) {
            pushBufferList(mLocalBuffer, itor->buffer, mLocalBufferNumber,
                           mLocalBufferList);
            HAL_LOGD("clear frame main idx:%d", itor->frame_number);
            frame_num = itor->frame_number;
            mUnmatchedFrameListMain.erase(itor);
            CallBackResult(frame_num, CAMERA3_BUFFER_STATUS_ERROR);
        }
        itor++;
    }

    itor = mUnmatchedFrameListAux.begin();
    while (itor != mUnmatchedFrameListAux.end()) {
        if (itor->frame_number < sub_frame_number) {
            pushBufferList(mLocalBuffer, itor->buffer, mLocalBufferNumber,
                           mLocalBufferList);
            HAL_LOGD("clear frame aux idx:%d", itor->frame_number);
            mUnmatchedFrameListAux.erase(itor);
        }
        itor++;
    }
}
};
