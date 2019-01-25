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
#define LOG_TAG "Cam3Blur"
#include "SprdCamera3Blur.h"

using namespace android;
namespace sprdcamera {

SprdCamera3Blur *mBlur = NULL;

// Error Check Macros
#define CHECK_BLUR()                                                           \
    if (!mBlur) {                                                              \
        HAL_LOGE("Error getting blur ");                                       \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_BLUR_ERROR()                                                     \
    if (!mBlur) {                                                              \
        HAL_LOGE("Error getting blur ");                                       \
        return -ENODEV;                                                        \
    }

#define CHECK_HWI_ERROR(hwi)                                                   \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return -ENODEV;                                                        \
    }

camera3_device_ops_t SprdCamera3Blur::mCameraCaptureOps = {
    .initialize = SprdCamera3Blur::initialize,
    .configure_streams = SprdCamera3Blur::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3Blur::construct_default_request_settings,
    .process_capture_request = SprdCamera3Blur::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3Blur::dump,
    .flush = SprdCamera3Blur::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3Blur::callback_ops_main = {
    .process_capture_result = SprdCamera3Blur::process_capture_result_main,
    .notify = SprdCamera3Blur::notifyMain};

camera3_callback_ops SprdCamera3Blur::callback_ops_aux = {
    .process_capture_result = SprdCamera3Blur::process_capture_result_aux,
    .notify = SprdCamera3Blur::notifyAux};

/*===========================================================================
 * FUNCTION   : SprdCamera3Blur
 *
 * DESCRIPTION: SprdCamera3Blur Constructor
 *
 * PARAMETERS:
 *
 *
 *==========================================================================*/
SprdCamera3Blur::SprdCamera3Blur() {
    HAL_LOGI(" E");

    memset(&m_VirtualCamera, 0, sizeof(sprd_virtual_camera_t));
    memset(&mLocalCapBuffer, 0, sizeof(new_mem_t) * BLUR_LOCAL_CAPBUFF_NUM);
    memset(&mSavedReqStreams, 0,
           sizeof(camera3_stream_t *) * BLUR_MAX_NUM_STREAMS);
    memset(&fbLevels, 0, sizeof(face_beauty_levels));

    m_VirtualCamera.id = CAM_BLUR_MAIN_ID;
    mStaticMetadata = NULL;
    mCaptureThread = new CaptureThread();
    mSavedRequestList.clear();
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mPreviewStreamsNum = 0;
    mjpegSize = 0;
#ifdef YUV_CONVERT_TO_JPEG
    mNearJpegSize = 0;
    mFarJpegSize = 0;
    m_pNearJpegBuffer = NULL;
    m_pFarJpegBuffer = NULL;
#endif
    m_pNearYuvBuffer = NULL;
    m_pFarYuvBuffer = NULL;
    weight_map = NULL;
    mCameraId = CAM_BLUR_MAIN_ID;
    mFlushing = false;
    mReqState = PREVIEW_REQUEST_STATE;
    mPerfectskinlevel = 0;
    mCoverValue = 1;
    m_pPhyCamera = NULL;
    m_nPhyCameras = 0;
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   : ~SprdCamera3Blur
 *
 * DESCRIPTION: SprdCamera3Blur Desctructor
 *
 *==========================================================================*/
SprdCamera3Blur::~SprdCamera3Blur() {
    HAL_LOGI("E");
    mCaptureThread = NULL;
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   : getCameraBlur
 *
 * DESCRIPTION: Creates Camera Blur if not created
 *
 * PARAMETERS:
 *   @pCapture               : Pointer to retrieve Camera Blur
 *
 *
 * RETURN    :  NONE
 *==========================================================================*/
void SprdCamera3Blur::getCameraBlur(SprdCamera3Blur **pBlur) {
    *pBlur = NULL;

    if (!mBlur) {
        mBlur = new SprdCamera3Blur();
    }
    CHECK_BLUR();
    *pBlur = mBlur;
    HAL_LOGV("mBlur: %p ", mBlur);

    return;
}

/*===========================================================================
 * FUNCTION   : get_camera_info
 *
 * DESCRIPTION: get logical camera info
 *
 * PARAMETERS:
 *   @camera_id     : Logical Camera ID
 *   @info              : Logical Main Camera Info
 *
 * RETURN    :
 *              NO_ERROR  : success
 *              ENODEV : Camera not found
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3Blur::get_camera_info(__unused int camera_id,
                                     struct camera_info *info) {
    int rc = NO_ERROR;

    HAL_LOGV("E");
    if (info) {
        rc = mBlur->getCameraInfo(camera_id, info);
    }
    HAL_LOGV("X, rc: %d", rc);

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
int SprdCamera3Blur::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    int rc = NO_ERROR;

    HAL_LOGV("id= %d", atoi(id));
    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    rc = mBlur->cameraDeviceOpen(atoi(id), hw_device);
    HAL_LOGV("id= %d, rc: %d", atoi(id), rc);

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
int SprdCamera3Blur::close_camera_device(__unused hw_device_t *hw_dev) {
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }

    return mBlur->closeCameraDevice();
}

/*===========================================================================
 * FUNCTION   : closeCameraDevice
 *
 * DESCRIPTION: Close the camera
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3Blur::closeCameraDevice() {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;
    HAL_LOGI("E");

    // Attempt to close all cameras regardless of unbundle results
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        sprdCam = &m_pPhyCamera[i];
        hw_device_t *dev = (hw_device_t *)(sprdCam->dev);
        if (dev == NULL) {
            continue;
        }
        HAL_LOGW("camera id:%d", i);
        rc = SprdCamera3HWI::close_camera_device(dev);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error, camera id:%d", i);
        }
        sprdCam->hwi = NULL;
        sprdCam->dev = NULL;
    }

    if (mCaptureThread != NULL) {
        if (mCaptureThread->isRunning()) {
            mCaptureThread->requestExit();
        }
    }

    freeLocalCapBuffer();
    mSavedRequestList.clear();
    mCaptureThread->unLoadBlurApi();
    if (m_pPhyCamera) {
        delete[] m_pPhyCamera;
        m_pPhyCamera = NULL;
    }

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
int SprdCamera3Blur::initialize(__unused const struct camera3_device *device,
                                const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;

    HAL_LOGV("E");
    CHECK_BLUR_ERROR();
    rc = mBlur->initialize(callback_ops);

    HAL_LOGV("X");
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
const camera_metadata_t *SprdCamera3Blur::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *rc;

    HAL_LOGV("E");
    if (!mBlur) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }
    rc = mBlur->constructDefaultRequestSettings(device, type);

    HAL_LOGV("X");
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
int SprdCamera3Blur::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;

    HAL_LOGV(" E");
    CHECK_BLUR_ERROR();

    rc = mBlur->configureStreams(device, stream_list);

    HAL_LOGV(" X");

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
int SprdCamera3Blur::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;

    HAL_LOGV("idx:%d", request->frame_number);
    CHECK_BLUR_ERROR();
    rc = mBlur->processCaptureRequest(device, request);

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3Blur
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Blur::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_BLUR();
    mBlur->processCaptureResultMain((camera3_capture_result_t *)result);
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
void SprdCamera3Blur::notifyMain(const struct camera3_callback_ops *ops,
                                 const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%d", msg->message.shutter.frame_number);
    CHECK_BLUR();
    mBlur->notifyMain(msg);
}

/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3Blur
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Blur::process_capture_result_aux(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_BLUR();
}

/*===========================================================================
 * FUNCTION   :notifyAux
 *
 * DESCRIPTION: Aux sensor  notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Blur::notifyAux(const struct camera3_callback_ops *ops,
                                const camera3_notify_msg_t *msg) {
    HAL_LOGD("idx:%d", msg->message.shutter.frame_number);
    CHECK_BLUR();
}

/*===========================================================================
 * FUNCTION   : freeLocalCapBuffer
 *
 * DESCRIPTION: free new_mem_t buffer
 *
 * PARAMETERS:
 *
 * RETURN    :  NONE
 *==========================================================================*/
void SprdCamera3Blur::freeLocalCapBuffer() {
    for (size_t i = 0; i < BLUR_LOCAL_CAPBUFF_NUM; i++) {
        new_mem_t *localBuffer = &mLocalCapBuffer[i];
        freeOneBuffer(localBuffer);
    }

    if (mCaptureThread->mOutWeightBuff != NULL) {
        free(mCaptureThread->mOutWeightBuff);
        mCaptureThread->mOutWeightBuff = NULL;
    }
    if (weight_map != NULL) {
        free(weight_map);
        weight_map = NULL;
    }
    if (m_pNearYuvBuffer != NULL) {
        free(m_pNearYuvBuffer);
        m_pNearYuvBuffer = NULL;
    }
    if (mCaptureThread->mIsGalleryBlur && (mCaptureThread->mVersion == 3)) {
        if (m_pFarYuvBuffer != NULL) {
            free(m_pFarYuvBuffer);
            m_pFarYuvBuffer = NULL;
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
int SprdCamera3Blur::cameraDeviceOpen(__unused int camera_id,
                                      struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    uint32_t phyId = 0;

    HAL_LOGI(" E");
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.sys.cam.blur.cov.id", prop, "3");

    if (camera_id == MODE_BLUR_FRONT) {
        mCameraId = CAM_BLUR_MAIN_ID_2;
        m_VirtualCamera.id = CAM_BLUR_MAIN_ID_2;
        if (atoi(prop) == 1 || atoi(prop) == 2) {
            m_nPhyCameras = 2;
        } else {
            m_nPhyCameras = 1;
        }
    } else {
        mCameraId = CAM_BLUR_MAIN_ID;
        m_VirtualCamera.id = CAM_BLUR_MAIN_ID;
        if (atoi(prop) == 0 || atoi(prop) == 2) {
            m_nPhyCameras = 2;
        } else {
            m_nPhyCameras = 1;
        }
    }
    hw_device_t *hw_dev[m_nPhyCameras];
    setupPhysicalCameras();
    // Open all physical cameras
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        phyId = m_pPhyCamera[i].id;
        HAL_LOGI("open %d", phyId);
        SprdCamera3HWI *hw = new SprdCamera3HWI((uint32_t)phyId);
        if (!hw) {
            HAL_LOGE("Allocation of hardware interface failed");
            return NO_MEMORY;
        }
        hw_dev[i] = NULL;

        hw->setMultiCameraMode(MODE_BLUR);
        rc = hw->openCamera(&hw_dev[i]);
        if (rc != NO_ERROR) {
            HAL_LOGE("failed, camera id:%d", phyId);
            delete hw;
            closeCameraDevice();
            return rc;
        }

        m_pPhyCamera[i].dev = reinterpret_cast<camera3_device_t *>(hw_dev[i]);
        m_pPhyCamera[i].hwi = hw;
    }

    m_VirtualCamera.dev.common.tag = HARDWARE_DEVICE_TAG;
    m_VirtualCamera.dev.common.version = CAMERA_DEVICE_API_VERSION_3_2;
    m_VirtualCamera.dev.common.close = close_camera_device;
    m_VirtualCamera.dev.ops = &mCameraCaptureOps;
    m_VirtualCamera.dev.priv = (void *)&m_VirtualCamera;
    *hw_device = &m_VirtualCamera.dev.common;
    mCaptureThread->loadBlurApi();

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
int SprdCamera3Blur::getCameraInfo(int blur_camera_id,
                                   struct camera_info *info) {
    int rc = NO_ERROR;
    int camera_id = 0;
    int32_t img_size = 0;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    if (blur_camera_id == MODE_BLUR_FRONT) {
        m_VirtualCamera.id = CAM_BLUR_MAIN_ID_2;
        property_get("persist.sys.cam.fr.blur.version", prop, "0");
    } else {
        m_VirtualCamera.id = CAM_BLUR_MAIN_ID;
        property_get("persist.sys.cam.ba.blur.version", prop, "0");
    }
    camera_id = m_VirtualCamera.id;
    mCaptureThread->mVersion = atoi(prop);
    HAL_LOGI("E, camera_id = %d version:%d", camera_id,
             mCaptureThread->mVersion);

    SprdCamera3Setting::initDefaultParameters(camera_id);

    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }
    CameraMetadata metadata = mStaticMetadata;
    if (atoi(prop) == 3) {
        property_get("persist.sys.gallery.blur", prop, "1");
        if (atoi(prop) == 1) {
            mCaptureThread->mIsGalleryBlur = true;
            img_size =
                SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size * 3 +
                (BLUR_REFOCUS_PARAM2_NUM * 4) + 1024;
        } else {
            mCaptureThread->mIsGalleryBlur = false;
            img_size =
                SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size * 2 +
                SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size / 6 +
                (BLUR_REFOCUS_PARAM2_NUM * 4) + 1024;
        }

    } else {
        property_get("persist.sys.cam.fr.blur.type", prop, "2");
        if (atoi(prop) == 2) {
            img_size =
                SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size * 2 +
                SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size / 48 +
                (BLUR_REFOCUS_PARAM_NUM * 4) + 1024;

        } else {
            img_size =
                SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size * 2 +
                (BLUR_REFOCUS_PARAM_NUM * 4) + 1024;
        }
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
    HAL_LOGI("X  max_size=%d", img_size);
    info->conflicting_devices_length = 0;

    return rc;
}

/*===========================================================================
 * FUNCTION   : setupPhysicalCameras
 *
 * DESCRIPTION: Creates Camera Capture if not created
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3Blur::setupPhysicalCameras() {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[m_nPhyCameras];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }
    memset(m_pPhyCamera, 0x00,
           (m_nPhyCameras * sizeof(sprdcamera_physical_descriptor_t)));
    if (mCameraId == 0) {
        m_pPhyCamera[CAM_TYPE_MAIN].id = CAM_BLUR_MAIN_ID;
        if (2 == m_nPhyCameras) {
            m_pPhyCamera[CAM_TYPE_AUX].id = CAM_BLUR_AUX_ID;
        }
    } else {
        m_pPhyCamera[CAM_TYPE_MAIN].id = CAM_BLUR_MAIN_ID_2;
        if (2 == m_nPhyCameras) {
            m_pPhyCamera[CAM_TYPE_AUX].id = CAM_BLUR_AUX_ID_2;
        }
    }

    return NO_ERROR;
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
SprdCamera3Blur::CaptureThread::CaptureThread()
    : mCallbackOps(NULL), mDevMain(NULL), mSavedResultBuff(NULL),
      mSavedCapReqsettings(NULL), mCaptureStreamsNum(0), mBlurApi2(NULL),
      mLastMinScope(0), mLastMaxScope(0), mLastAdjustRati(0),
      mCircleSizeScale(0), mFirstCapture(false), mFirstPreview(false),
      mUpdateCaptureWeightParams(false), mUpdatePreviewWeightParams(false),
      mLastFaceNum(0), mSkipFaceNum(0), mRotation(0), mLastTouchX(0),
      mLastTouchY(0), mBlurBody(true), mUpdataTouch(false), mVersion(0),
      mIsGalleryBlur(false), mIsBlurAlways(false),
#ifdef ISP_SUPPORT_MICRODEPTH
      mOutWeightBuff(NULL), mMicrodepthInfo(NULL)
#else
      mOutWeightBuff(NULL)
#endif
{
    HAL_LOGI(" E");
    memset(&mSavedCapReqstreambuff, 0, sizeof(camera3_stream_buffer_t));
    memset(&mMainStreams, 0, sizeof(camera3_stream_t) * BLUR_MAX_NUM_STREAMS);
    memset(mBlurApi, 0, sizeof(BlurAPI_t *) * BLUR_LIB_BOKEH_NUM);
    memset(mWinPeakPos, 0, sizeof(short) * BLUR_AF_WINDOW_NUM);
    memset(&mPreviewInitParams, 0, sizeof(preview_init_params_t));
    memset(&mPreviewWeightParams, 0, sizeof(preview_weight_params_t));
    memset(&mCaptureInitParams, 0, sizeof(capture_init_params_t));
    memset(&mCaptureWeightParams, 0, sizeof(capture_weight_params_t));
    memset(&mCapture2InitParams, 0, sizeof(capture2_init_params_t));
    memset(&mCapture2WeightParams, 0, sizeof(capture2_weight_params_t));
    memset(mFaceInfo, 0, sizeof(int32_t) * 4);
    memset(&mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
    memset(&mIspInfo, 0, sizeof(blur_isp_info_t));
#ifdef ISP_SUPPORT_MICRODEPTH
    memset(&mIspCapture2InitParams, 0, sizeof(bokeh_micro_depth_tune_param));
#endif
    mCaptureMsgList.clear();
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
SprdCamera3Blur::CaptureThread::~CaptureThread() {
    HAL_LOGI(" E");
    mCaptureMsgList.clear();
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
void SprdCamera3Blur::CaptureThread::BlurFaceMakeup(
    buffer_handle_t *buffer_handle, void *buffer_addr) {

    struct camera_frame_type cap_3d_frame;
    struct camera_frame_type *frame = NULL;
    int32_t origW = SprdCamera3Setting::s_setting[mBlur->mCameraId]
                        .sensor_InfoInfo.pixer_array_size[0];
    int32_t origH = SprdCamera3Setting::s_setting[mBlur->mCameraId]
                        .sensor_InfoInfo.pixer_array_size[1];
    FACE_Tag newFace = SprdCamera3Setting::s_setting[mBlur->mCameraId].faceInfo;
    bzero(&cap_3d_frame, sizeof(struct camera_frame_type));
    frame = &cap_3d_frame;
    frame->y_vir_addr = cmr_uint(buffer_addr);
    frame->width = ADP_WIDTH(*buffer_handle);
    frame->height = ADP_HEIGHT(*buffer_handle);

    for (int i = 0; i < newFace.face_num; i++) {
        newFace.face[i].rect[0] =
            newFace.face[i].rect[0] * mCaptureInitParams.width / origW;
        newFace.face[i].rect[1] =
            newFace.face[i].rect[1] * mCaptureInitParams.height / origH;
        newFace.face[i].rect[2] =
            newFace.face[i].rect[2] * mCaptureInitParams.width / origW;
        newFace.face[i].rect[3] =
            newFace.face[i].rect[3] * mCaptureInitParams.height / origH;
        HAL_LOGD(
            "blur capture face:%d sx:%d sy:%d ex:%d ey:%d angle:%d pose:%d", i,
            newFace.face[i].rect[0], newFace.face[i].rect[1],
            newFace.face[i].rect[2], newFace.face[i].rect[3], newFace.angle[i],
            newFace.pose[i]);
    }

    mBlur->doFaceMakeup2(frame, mBlur->fbLevels, &newFace,
                         0); // work mode 1 for preview, 0 for picture

    return;

map_error:
    HAL_LOGE("buffer_handle map failed");
    return;
}
#endif

/*===========================================================================
 * FUNCTION   :loadBlurApi
 *
 * DESCRIPTION: loadBlurApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3Blur::CaptureThread::loadBlurApi() {
    HAL_LOGI("E");
    const char *error = NULL;
    int i = 0;

    for (i = 0; i < BLUR_LIB_BOKEH_NUM; i++) {
        mBlurApi[i] = (BlurAPI_t *)malloc(sizeof(BlurAPI_t));
        if (mBlurApi[i] == NULL) {
            HAL_LOGE("mBlurApi malloc failed.");
            return -1;
        }
        memset(mBlurApi[i], 0, sizeof(BlurAPI_t));
        HAL_LOGD("i = %d", i);
        if (i == 0) {
            mBlurApi[i]->handle = dlopen(BLUR_LIB_BOKEH_PREVIEW, RTLD_LAZY);
            if (mBlurApi[i]->handle == NULL) {
                error = dlerror();
                HAL_LOGE("open Blur API failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothInit =
                (int (*)(void **handle, int width, int height, float min_slope,
                         float max_slope, float findex2gamma_adjust_ratio,
                         int box_filter_size))dlsym(mBlurApi[i]->handle,
                                                    "iSmoothInit");
            if (mBlurApi[i]->iSmoothInit == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothInit failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothDeinit = (int (*)(void *handle))dlsym(
                mBlurApi[i]->handle, "iSmoothDeinit");
            if (mBlurApi[i]->iSmoothDeinit == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothDeinit failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothCreateWeightMap =
                (int (*)(void *handle, preview_weight_params_t *params))dlsym(
                    mBlurApi[i]->handle, "iSmoothCreateWeightMap");
            if (mBlurApi[i]->iSmoothCreateWeightMap == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothCreateWeightMap failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothBlurImage =
                (int (*)(void *handle, unsigned char *Src_YUV,
                         unsigned char *Output_YUV))dlsym(mBlurApi[i]->handle,
                                                          "iSmoothBlurImage");
            if (mBlurApi[i]->iSmoothBlurImage == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothBlurImage failed.error = %s", error);
                return -1;
            }
        } else if (i == 1) {
            mBlurApi[i]->handle = dlopen(BLUR_LIB_BOKEH_CAPTURE, RTLD_LAZY);
            if (mBlurApi[i]->handle == NULL) {
                error = dlerror();
                HAL_LOGE("open Blur API failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothCapInit =
                (int (*)(void **handle, capture_init_params_t *params))dlsym(
                    mBlurApi[i]->handle, "iSmoothCapInit");
            if (mBlurApi[i]->iSmoothCapInit == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothCapInit failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothCap_VersionInfo_Get =
                (int (*)(void *a_pOutBuf, int a_dInBufMaxSize))dlsym(
                    mBlurApi[i]->handle, "iSmoothCap_VersionInfo_Get");
            if (mBlurApi[i]->iSmoothCap_VersionInfo_Get == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothCap_VersionInfo_Get failed.error = %s",
                         error);
                return -1;
            }
            mBlurApi[i]->iSmoothDeinit = (int (*)(void *handle))dlsym(
                mBlurApi[i]->handle, "iSmoothCapDeinit");
            if (mBlurApi[i]->iSmoothDeinit == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothDeinit failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothCapCreateWeightMap =
                (int (*)(void *handle, capture_weight_params_t *params,
                         unsigned char *Src_YUV, unsigned short *outWeightMap))
                    dlsym(mBlurApi[i]->handle, "iSmoothCapCreateWeightMap");
            if (mBlurApi[i]->iSmoothCapCreateWeightMap == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothCapCreateWeightMap failed.error = %s",
                         error);
                return -1;
            }
            mBlurApi[i]->iSmoothCapBlurImage =
                (int (*)(void *handle, unsigned char *Src_YUV,
                         unsigned short *inWeightMap,
                         capture_weight_params_t *params,
                         unsigned char *Output_YUV))
                    dlsym(mBlurApi[i]->handle, "iSmoothCapBlurImage");
            if (mBlurApi[i]->iSmoothCapBlurImage == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothBlurImage failed.error = %s", error);
                return -1;
            }
            char a_pOutBuf[256];
            mBlurApi[i]->iSmoothCap_VersionInfo_Get((void *)a_pOutBuf, 256);
            HAL_LOGD("loadsprdblurcapturelib: libbokeh_gaussian_cap.so "
                     "[support:1.0 1.1 1.2 2.0] [inner version: %s]",
                     a_pOutBuf);
        }
        mBlurApi[i]->mHandle = NULL;
    }

    mBlurApi2 = (BlurAPI2_t *)malloc(sizeof(BlurAPI2_t));
    if (mBlurApi2 == NULL) {
        HAL_LOGE("mBlurApi2 malloc failed.");
        return -1;
    }
    memset(mBlurApi2, 0, sizeof(BlurAPI2_t));
    mBlurApi2->handle = dlopen(BLUR_LIB_BOKEH_CAPTURE2, RTLD_LAZY);
    if (mBlurApi2->handle == NULL) {
        error = dlerror();
        HAL_LOGE("open Blur API2 failed.error = %s", error);
        return -1;
    }

    mBlurApi2->BokehFrames_VersionInfo_Get =
        (int (*)(char a_acOutRetbuf[256], unsigned int a_udInSize))dlsym(
            mBlurApi2->handle, "BokehFrames_VersionInfo_Get");

    if (mBlurApi2->BokehFrames_VersionInfo_Get == NULL) {
        error = dlerror();
        HAL_LOGE("sym BokehFrames_VersionInfo_Get failed.error = %s", error);
        return -1;
    }
    mBlurApi2->BokehFrames_Init =
        (int (*)(void **handle, int width, int height,
                 capture2_init_params_t *params))dlsym(mBlurApi2->handle,
                                                       "BokehFrames_Init");
    if (mBlurApi2->BokehFrames_Init == NULL) {
        error = dlerror();
        HAL_LOGE("sym BokehFrames_Init failed.error = %s", error);
        return -1;
    }
    mBlurApi2->BokehFrames_WeightMap =
        (int (*)(void *img0_src, void *img1_src, void *dis_map, void *handle))
            dlsym(mBlurApi2->handle, "BokehFrames_WeightMap");
    if (mBlurApi2->BokehFrames_WeightMap == NULL) {
        error = dlerror();
        HAL_LOGE("sym BokehFrames_WeightMap failed.error = %s", error);
        return -1;
    }
    mBlurApi2->Bokeh2Frames_Process =
        (int (*)(void *img0_src, void *img_rslt, void *dis_map, void *handle,
                 capture2_weight_params_t *params))
            dlsym(mBlurApi2->handle, "Bokeh2Frames_Process");
    if (mBlurApi2->Bokeh2Frames_Process == NULL) {
        error = dlerror();
        HAL_LOGE("sym Bokeh2Frames_Process failed.error = %s", error);
        return -1;
    }
#ifdef ISP_SUPPORT_MICRODEPTH
    mBlurApi2->BokehFrames_ParamInfo_Get =
        (int (*)(void *handle, MicrodepthBoke2Frames **microdepthInfo))dlsym(
            mBlurApi2->handle, "BokehFrames_ParamInfo_Get");
    if (mBlurApi2->BokehFrames_ParamInfo_Get == NULL) {
        error = dlerror();
        HAL_LOGE("sym BokehFrames_ParamInfo_Get failed.error = %s", error);
        return -1;
    }
#endif
    mBlurApi2->BokehFrames_Deinit =
        (int (*)(void *handle))dlsym(mBlurApi2->handle, "BokehFrames_Deinit");
    if (mBlurApi2->BokehFrames_Deinit == NULL) {
        error = dlerror();
        HAL_LOGE("sym BokehFrames_Deinit failed.error = %s", error);
        return -1;
    }
    char a_acOutRetbuf[256];
    mBlurApi2->BokehFrames_VersionInfo_Get(a_acOutRetbuf, 256);
    HAL_LOGD("loadsprdblurcapturelib: libBokeh2Frames.so [support:3.0] [inner "
             "version: %s]",
             a_acOutRetbuf);
    mBlurApi2->mHandle = NULL;

    HAL_LOGI("load blur Api succuss.");

    return 0;
}

/*===========================================================================
 * FUNCTION   :unLoadBlurApi
 *
 * DESCRIPTION: unLoadBlurApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Blur::CaptureThread::unLoadBlurApi() {
    HAL_LOGI("E");
    int i = 0;
    for (i = 0; i < BLUR_LIB_BOKEH_NUM; i++) {
        if (mBlurApi[i] != NULL) {
            if (mBlurApi[i]->mHandle != NULL) {
                mBlurApi[i]->iSmoothDeinit(mBlurApi[i]->mHandle);
                mBlurApi[i]->mHandle = NULL;
            }
            if (mBlurApi[i]->handle != NULL) {
                dlclose(mBlurApi[i]->handle);
                mBlurApi[i]->handle = NULL;
            }
            free(mBlurApi[i]);
            mBlurApi[i] = NULL;
        }
    }
    if (mBlurApi2 != NULL) {
        if (mBlurApi2->mHandle != NULL) {
            mBlurApi2->BokehFrames_Deinit(mBlurApi2->mHandle);
            mBlurApi2->mHandle = NULL;
        }
        if (mBlurApi2->handle != NULL) {
            dlclose(mBlurApi2->handle);
            mBlurApi2->handle = NULL;
        }
        free(mBlurApi2);
        mBlurApi2 = NULL;
    }
    if (mCaptureInitParams.cali_dist_seq != NULL) {
        free(mCaptureInitParams.cali_dist_seq);
        mCaptureInitParams.cali_dist_seq = NULL;
    }
    if (mCaptureInitParams.cali_dac_seq != NULL) {
        free(mCaptureInitParams.cali_dac_seq);
        mCaptureInitParams.cali_dac_seq = NULL;
    }
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :blurHandle
 *
 * DESCRIPTION: blurHandle
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3Blur::CaptureThread::blurHandle(buffer_handle_t *input1,
                                               void *input2,
                                               buffer_handle_t *output) {

    int ret = 0;
    unsigned char *srcYUV = NULL;
    unsigned char *destYUV = NULL;
    int libid = 0;
#ifdef ISP_SUPPORT_MICRODEPTH
    SprdCamera3HWI *hwiMain = mBlur->m_pPhyCamera[CAM_TYPE_MAIN].hwi;
#endif

    if (input1 != NULL) {
        ret = mBlur->map(input1, (void **)(&srcYUV));
        if (ret != NO_ERROR)
            goto map_release;
    }
    if (output != NULL) {
        ret = mBlur->map(output, (void **)(&destYUV));
        if (ret != NO_ERROR)
            goto map_release;
        libid = 1;
    }

    {
        Mutex::Autolock l(mMergequeueMutex);
        if (isBlurInitParamsChanged() || mFirstPreview) {
            mFirstPreview = false;
            if (mBlurApi[0]->mHandle != NULL) {
                int64_t deinitStart = systemTime();
                ret = mBlurApi[0]->iSmoothDeinit(mBlurApi[0]->mHandle);
                if (ret != 0) {
                    HAL_LOGE("preview iSmoothDeinit Err:%d", ret);
                }
                mBlurApi[0]->mHandle = NULL;
                HAL_LOGD("preview iSmoothDeinit cost %lld ms",
                         ns2ms(systemTime() - deinitStart));
            }
            int64_t initStart = systemTime();

            ret = mBlurApi[0]->iSmoothInit(
                &(mBlurApi[0]->mHandle), mPreviewInitParams.width,
                mPreviewInitParams.height, mPreviewInitParams.min_slope,
                mPreviewInitParams.max_slope,
                mPreviewInitParams.findex2gamma_adjust_ratio,
                mPreviewInitParams.box_filter_size);

            HAL_LOGD("preview iSmoothInit cost %lld ms",
                     ns2ms(systemTime() - initStart));
            if (ret != 0) {
                HAL_LOGE("preview iSmoothInit Err:%d", ret);
            }
        }
        if (isBlurInitParamsChanged() || mFirstCapture) {
            mFirstCapture = false;
            if (mBlurApi2->mHandle != NULL) {
                int64_t deinitStart = systemTime();
                ret = mBlurApi2->BokehFrames_Deinit(mBlurApi2->mHandle);
                if (ret != 0) {
                    HAL_LOGE("Bokeh2Frames iSmoothDeinit Err:%d", ret);
                }
                mBlurApi2->mHandle = NULL;
                HAL_LOGD("Bokeh2Frames iSmoothDeinit cost %lld ms",
                         ns2ms(systemTime() - deinitStart));
            }
            int64_t initStart = systemTime();
#ifdef ISP_SUPPORT_MICRODEPTH
            ret =
                hwiMain->camera_ioctrl(CAMERA_IOCTRL_GET_MICRODEPTH_PARAM,
                                       (void *)(&mIspCapture2InitParams), NULL);
            if (ret != 0 || mIspCapture2InitParams.tuning_exist == 0) {

                HAL_LOGD("Bokeh2Frames git isp param error,use default init "
                         "param Err:%d",
                         ret);
                ret = mBlurApi2->BokehFrames_Init(
                    &(mBlurApi2->mHandle), mCaptureInitParams.width,
                    mCaptureInitParams.height, NULL);
            } else {
                HAL_LOGD(
                    "Bokeh2Frames git isp param success,use isp init param:");
                HAL_LOGD("enable:%d", mIspCapture2InitParams.enable);
                HAL_LOGD("fir_mode:%d", mIspCapture2InitParams.fir_mode);
                HAL_LOGD("fir_len:%d", mIspCapture2InitParams.fir_len);
                HAL_LOGD("fir_channel:%d", mIspCapture2InitParams.fir_channel);
                HAL_LOGD("fir_cal_mode:%d",
                         mIspCapture2InitParams.fir_cal_mode);
                HAL_LOGD("fir_edge_factor:%d",
                         mIspCapture2InitParams.fir_edge_factor);
                HAL_LOGD("depth_mode:%d", mIspCapture2InitParams.depth_mode);
                HAL_LOGD("smooth_thr:%d", mIspCapture2InitParams.smooth_thr);
                HAL_LOGD("touch_factor:%d",
                         mIspCapture2InitParams.touch_factor);
                HAL_LOGD("scale_factor:%d",
                         mIspCapture2InitParams.scale_factor);
                HAL_LOGD("refer_len:%d", mIspCapture2InitParams.refer_len);
                HAL_LOGD("merge_factor:%d",
                         mIspCapture2InitParams.merge_factor);
                HAL_LOGD("similar_factor:%d",
                         mIspCapture2InitParams.similar_factor);
                HAL_LOGD("tmp_mode:%d", mIspCapture2InitParams.tmp_mode);
                HAL_LOGD("tmp_thr:%d", mIspCapture2InitParams.tmp_thr);
                for (uint32_t i = 0;
                     i < ARRAY_SIZE(mIspCapture2InitParams.hfir_coeff); i++) {
                    HAL_LOGD("hfir_coeff[%d]:%d", i,
                             mIspCapture2InitParams.hfir_coeff[i]);
                }
                for (uint32_t i = 0;
                     i < ARRAY_SIZE(mIspCapture2InitParams.vfir_coeff); i++) {
                    HAL_LOGD("vfir_coeff[%d]:%d", i,
                             mIspCapture2InitParams.vfir_coeff[i]);
                }
                for (uint32_t i = 0;
                     i < ARRAY_SIZE(mIspCapture2InitParams.similar_coeff);
                     i++) {
                    HAL_LOGD("similar_coeff[%d]:%d", i,
                             mIspCapture2InitParams.similar_coeff[i]);
                }
                for (uint32_t i = 0;
                     i < ARRAY_SIZE(mIspCapture2InitParams.tmp_coeff); i++) {
                    HAL_LOGD("tmp_coeff[%d]:%d", i,
                             mIspCapture2InitParams.tmp_coeff[i]);
                }

                mCapture2InitParams.enable = mIspCapture2InitParams.enable;
                mCapture2InitParams.fir_mode = mIspCapture2InitParams.fir_mode;
                mCapture2InitParams.fir_len = mIspCapture2InitParams.fir_len;
                memcpy(mCapture2InitParams.hfir_coeff,
                       mIspCapture2InitParams.hfir_coeff,
                       ARRAY_SIZE(mCapture2InitParams.hfir_coeff) *
                           sizeof(cmr_s32));
                memcpy(mCapture2InitParams.vfir_coeff,
                       mIspCapture2InitParams.vfir_coeff,
                       ARRAY_SIZE(mCapture2InitParams.vfir_coeff) *
                           sizeof(cmr_s32));
                mCapture2InitParams.fir_channel =
                    mIspCapture2InitParams.fir_channel;
                mCapture2InitParams.fir_cal_mode =
                    mIspCapture2InitParams.fir_cal_mode;
                mCapture2InitParams.fir_edge_factor =
                    mIspCapture2InitParams.fir_edge_factor;
                mCapture2InitParams.depth_mode =
                    mIspCapture2InitParams.depth_mode;
                mCapture2InitParams.smooth_thr =
                    mIspCapture2InitParams.smooth_thr;
                mCapture2InitParams.touch_factor =
                    mIspCapture2InitParams.touch_factor;
                mCapture2InitParams.scale_factor =
                    mIspCapture2InitParams.scale_factor;
                mCapture2InitParams.refer_len =
                    mIspCapture2InitParams.refer_len;
                mCapture2InitParams.merge_factor =
                    mIspCapture2InitParams.merge_factor;
                mCapture2InitParams.similar_factor =
                    mIspCapture2InitParams.similar_factor;
                memcpy(mCapture2InitParams.similar_coeff,
                       mIspCapture2InitParams.similar_coeff,
                       ARRAY_SIZE(mCapture2InitParams.similar_coeff) *
                           sizeof(cmr_u32));
                mCapture2InitParams.tmp_mode = mIspCapture2InitParams.tmp_mode;
                memcpy(mCapture2InitParams.tmp_coeff,
                       mIspCapture2InitParams.tmp_coeff,
                       ARRAY_SIZE(mCapture2InitParams.tmp_coeff) *
                           sizeof(cmr_s32));
                mCapture2InitParams.tmp_thr = mIspCapture2InitParams.tmp_thr;
                ret = mBlurApi2->BokehFrames_Init(
                    &(mBlurApi2->mHandle), mCaptureInitParams.width,
                    mCaptureInitParams.height, &mCapture2InitParams);
            }

            if (ret != 0) {
                HAL_LOGE("Bokeh2Frames iSmoothInit Err:%d", ret);
            }
            mBlurApi2->BokehFrames_ParamInfo_Get(mBlurApi2->mHandle,
                                                 &mMicrodepthInfo);
            if (ret != 0) {
                HAL_LOGE("Bokeh2Frames ParamInfo_Get Err:%d", ret);
            } else {
                HAL_LOGD("Bokeh2Frames ParamInfo_Get :%d   %p ",
                         mMicrodepthInfo->microdepth_size,
                         mMicrodepthInfo->microdepth_buffer);
                ret = hwiMain->camera_ioctrl(
                    CAMERA_IOCTRL_SET_MICRODEPTH_DEBUG_INFO,
                    (void *)mMicrodepthInfo, NULL);
                if (ret != 0) {
                    HAL_LOGE("isp set microdepth debug info Err:%d", ret);
                }
            }
#else
            ret = mBlurApi2->BokehFrames_Init(&(mBlurApi2->mHandle),
                                              mCaptureInitParams.width,
                                              mCaptureInitParams.height, NULL);
            if (ret != 0) {
                HAL_LOGE("Bokeh2Frames iSmoothInit Err:%d", ret);
            }
#endif
            HAL_LOGD("Bokeh2Frames iSmoothInit cost %lld ms",
                     ns2ms(systemTime() - initStart));

            if (mBlurApi[1]->mHandle != NULL) {
                int64_t deinitStart = systemTime();
                ret = mBlurApi[1]->iSmoothDeinit(mBlurApi[1]->mHandle);
                if (ret != 0) {
                    HAL_LOGE("capture iSmoothDeinit Err:%d", ret);
                }
                mBlurApi[1]->mHandle = NULL;
                HAL_LOGD("capture iSmoothDeinit cost %lld ms",
                         ns2ms(systemTime() - deinitStart));
            }
            initStart = systemTime();

            ret = mBlurApi[1]->iSmoothCapInit(&(mBlurApi[1]->mHandle),
                                              &mCaptureInitParams);
            HAL_LOGD("capture iSmoothInit cost %lld ms",
                     ns2ms(systemTime() - initStart));
            if (ret != 0) {
                HAL_LOGE("capture iSmoothInit Err:%d", ret);
            }
        }

        if (mUpdatePreviewWeightParams && libid == 0) {
            int64_t creatStart = systemTime();
            mUpdatePreviewWeightParams = false;

            HAL_LOGD(
                "mPreviewWeightParams:%d %d %d %d %d %d",
                mPreviewWeightParams.valid_roi, mPreviewWeightParams.roi_type,
                mPreviewWeightParams.circle_size, mPreviewWeightParams.f_number,
                mPreviewWeightParams.sel_x, mPreviewWeightParams.sel_y);
            ret = mBlurApi[0]->iSmoothCreateWeightMap(mBlurApi[0]->mHandle,
                                                      &mPreviewWeightParams);
            if (ret != 0) {
                HAL_LOGE("preview CreateWeightMap Err:%d", ret);
            }
            HAL_LOGD("preview iSmoothCreateWeightMap cost %lld ms",
                     ns2ms(systemTime() - creatStart));
        }
        if (mUpdateCaptureWeightParams && libid != 0) {
            int64_t creatStart = systemTime();
            mUpdateCaptureWeightParams = false;

            if (mVersion == 3) {
                HAL_LOGD("Bokeh2Frames mCapture2WeightParams: %d %d %d",
                         mCapture2WeightParams.f_number,
                         mCapture2WeightParams.sel_x,
                         mCapture2WeightParams.sel_y);
                ret = mBlurApi2->BokehFrames_WeightMap(
                    input2, srcYUV, mBlur->weight_map, mBlurApi2->mHandle);
            } else {
                HAL_LOGD("mCaptureWeightParams:rotate_angle:%d "
                         "roi_type:%d f_number:%d sel_x:%d sel_y:%d "
                         "circle_size:%d",
                         mCaptureWeightParams.rotate_angle,
                         mCaptureWeightParams.roi_type,
                         mCaptureWeightParams.f_number,
                         mCaptureWeightParams.sel_x, mCaptureWeightParams.sel_y,
                         mCaptureWeightParams.circle_size);

                HAL_LOGD("mCaptureWeightParams:total_roi:%d valid_roi:%d x1:%d "
                         "y1:%d x2:%d y2:%d flag:%d",
                         mCaptureWeightParams.total_roi,
                         mCaptureWeightParams.valid_roi,
                         mCaptureWeightParams.x1[0], mCaptureWeightParams.y1[0],
                         mCaptureWeightParams.x2[0], mCaptureWeightParams.y2[0],
                         mCaptureWeightParams.flag[0]);
                ret = mBlurApi[1]->iSmoothCapCreateWeightMap(
                    mBlurApi[1]->mHandle, &mCaptureWeightParams, srcYUV,
                    mOutWeightBuff);
            }

            if (ret != 0) {
                HAL_LOGE("iSmoothCapCreateWeightMap Err:%d", ret);
            }
            HAL_LOGD("iSmoothCapCreateWeightMap cost %lld ms",
                     ns2ms(systemTime() - creatStart));
        }

        int64_t blurStart = systemTime();
        if (mVersion == 3 && libid != 0) {
            ret = mBlurApi2->Bokeh2Frames_Process(
                input2, destYUV, mBlur->weight_map, mBlurApi2->mHandle,
                &mCapture2WeightParams);
        } else if (libid != 0) {
            ret = mBlurApi[libid]->iSmoothCapBlurImage(
                mBlurApi[libid]->mHandle, srcYUV, mOutWeightBuff,
                &mCaptureWeightParams, destYUV);

        } else {
            ret = mBlurApi[libid]->iSmoothBlurImage(mBlurApi[libid]->mHandle,
                                                    srcYUV, destYUV);
        }
        if (ret != 0) {
            HAL_LOGE("BlurImage Err:%d", ret);
        }
        if (libid != 0) {
            HAL_LOGD("mVersion:%d.%d capture iSmoothBlurImage cost %lld ms",
                     mVersion, mCaptureWeightParams.roi_type,
                     ns2ms(systemTime() - blurStart));
            mBlur->flushIonBuffer(ADP_BUFFD(*output), (void *)destYUV,
                                  ADP_BUFSIZE(*output));
        } else {
            HAL_LOGV("preview iSmoothBlurImage cost %lld ms",
                     ns2ms(systemTime() - blurStart));
        }
    }

map_release:
    if (input1 != NULL && srcYUV != NULL) {
        mBlur->unmap(input1);
        srcYUV = NULL;
    } else {
        HAL_LOGD("map maybe error.intput1=%p, srcYUV=%p", input1, srcYUV);
    }
    if (output != NULL && destYUV != NULL) {
        mBlur->unmap(output);
        destYUV = NULL;
    } else {
        HAL_LOGD("map maybe error. output=%p, destYUV=%p", output, destYUV);
    }

    return ret;
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
bool SprdCamera3Blur::CaptureThread::threadLoop() {
    blur_queue_msg_t capture_msg;
    int mime_type = (int)MODE_BLUR;
    HAL_LOGV("run");

    while (!mCaptureMsgList.empty()) {
        List<blur_queue_msg_t>::iterator itor1 = mCaptureMsgList.begin();
        capture_msg = *itor1;
        mCaptureMsgList.erase(itor1);
        switch (capture_msg.msg_type) {
        case BLUR_MSG_EXIT: {
            // flush queue
            memset(&mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
            memset(&mSavedCapReqstreambuff, 0, sizeof(camera3_stream_buffer_t));
            if (NULL != mSavedCapReqsettings) {
                free_camera_metadata(mSavedCapReqsettings);
                mSavedCapReqsettings = NULL;
            }
            HAL_LOGD("BLUR_MSG_EXIT");
            return false;
        } break;
        case BLUR_MSG_DATA_PROC: {
            SprdCamera3HWI *hwiMain = mBlur->m_pPhyCamera[CAM_TYPE_MAIN].hwi;
            void *combo_buff_addr = NULL;
            void *output_buff_addr = NULL;
            buffer_handle_t *const output_buffer =
                &mBlur->mLocalCapBuffer[1].native_handle;
            if (mBlur->map(capture_msg.combo_buff.buffer, &combo_buff_addr) !=
                NO_ERROR) {
                HAL_LOGE("map combo buffer(%p) failed",
                         capture_msg.combo_buff.buffer);
            }
            if (mBlur->map(output_buffer, &output_buff_addr) != NO_ERROR) {
                HAL_LOGE("map output_buffer(%p) failed", output_buffer);
            }

            HAL_LOGD("mFlushing:%d, frame idx:%d", mBlur->mFlushing,
                     capture_msg.combo_buff.frame_number);
#ifdef CONFIG_FACE_BEAUTY
            if (mBlur->isFaceBeautyOn(mBlur->fbLevels) &&
                mFaceInfo[2] - mFaceInfo[0] > 0 &&
                mFaceInfo[3] - mFaceInfo[1] > 0) {
                BlurFaceMakeup(capture_msg.combo_buff.buffer, combo_buff_addr);
            }
#endif
            if ((mVersion == 1) ||
                ((mVersion == 3) &&
                 (mBlur->mReqState == WAIT_FIRST_YUV_STATE))) {
#ifdef YUV_CONVERT_TO_JPEG
                mBlur->m_pNearJpegBuffer =
                    &mBlur->mLocalCapBuffer[2].native_handle;
                void *nearJpegBufferAddr = NULL;
                if (mBlur->map(mBlur->m_pNearJpegBuffer, &nearJpegBufferAddr) ==
                    NO_ERROR) {
                    mBlur->mNearJpegSize = mBlur->jpeg_encode_exif_simplify(
                        capture_msg.combo_buff.buffer, combo_buff_addr,
                        mBlur->m_pNearJpegBuffer, nearJpegBufferAddr, NULL,
                        NULL, mBlur->m_pPhyCamera[CAM_TYPE_MAIN].hwi);

                    mBlur->unmap(mBlur->m_pNearJpegBuffer);
                    nearJpegBufferAddr = NULL;
                } else {
                    HAL_LOGE("map m_pNearJpegBuffer(%p) failed",
                             mBlur->m_pNearJpegBuffer);
                }
#endif

                memcpy(mBlur->m_pNearYuvBuffer, combo_buff_addr,
                       mBlur->mCaptureWidth * mBlur->mCaptureHeight * 3 / 2);
            } else if ((mVersion == 3) &&
                       mBlur->mReqState == WAIT_SECOND_YUV_STATE) {
#ifdef YUV_CONVERT_TO_JPEG
                mBlur->m_pFarJpegBuffer =
                    &mBlur->mLocalCapBuffer[3].native_handle;
                void *farJpegBufferAddr = NULL;
                if (mBlur->map(mBlur->m_pFarJpegBuffer, &farJpegBufferAddr) ==
                    NO_ERROR) {
                    mBlur->mFarJpegSize = mBlur->jpeg_encode_exif_simplify(
                        capture_msg.combo_buff.buffer, combo_buff_addr,
                        mBlur->m_pFarJpegBuffer, farJpegBufferAddr, NULL, NULL,
                        mBlur->m_pPhyCamera[CAM_TYPE_MAIN].hwi);
                    mBlur->unmap(mBlur->m_pFarJpegBuffer);
                    farJpegBufferAddr = NULL;
                } else {
                    HAL_LOGE("map m_pFarJpegBuffer(%p) failed",
                             mBlur->m_pFarJpegBuffer);
                }
#endif
                if (mIsGalleryBlur) {
                    memcpy(mBlur->m_pFarYuvBuffer, combo_buff_addr,
                           mBlur->mCaptureWidth * mBlur->mCaptureHeight * 3 /
                               2);
                }
            }
            char prop1[PROPERTY_VALUE_MAX] = {
                0,
            };
            property_get("persist.sys.camera.blur1", prop1, "0");
            if (1 == atoi(prop1) && !mBlur->mFlushing) {
                unsigned char *buffer_base = (unsigned char *)combo_buff_addr;
                mBlur->dumpData(buffer_base, 1,
                                mBlur->mCaptureWidth * mBlur->mCaptureHeight *
                                    3 / 2,
                                mBlur->mCaptureWidth, mBlur->mCaptureHeight,
                                capture_msg.combo_buff.frame_number, "src");
            }
            if ((mVersion == 1) && (mCaptureWeightParams.roi_type == 2) &&
                (!mBlur->mFlushing)) {
                //                getOutWeightMap(mSavedResultBuff);
            } else if (mVersion == 2 && !mBlur->mFlushing) {
                getIspAfFullscanInfo();
            } else if (mVersion == 3 && !mBlur->mFlushing) {
                if (mBlur->mReqState == WAIT_FIRST_YUV_STATE) {
                    hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_AF_POS,
                                           &(mIspInfo.far_peak_pos), NULL);
                } else if (mBlur->mReqState == WAIT_SECOND_YUV_STATE) {
                    hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_AF_POS,
                                           &(mIspInfo.af_peak_pos), NULL);
                }
            }

            if (!mBlur->mFlushing &&
                (mVersion != 3 ||
                 (mVersion == 3 && mBlur->mReqState == WAIT_SECOND_YUV_STATE &&
                  !mIsGalleryBlur))) {
                blurHandle(capture_msg.combo_buff.buffer,
                           mBlur->m_pNearYuvBuffer, output_buffer);
                char prop2[PROPERTY_VALUE_MAX] = {
                    0,
                };
                property_get("persist.sys.camera.blur2", prop2, "0");
                if (1 == atoi(prop2)) {
                    unsigned char *buffer_base =
                        (unsigned char *)output_buff_addr;
                    mBlur->dumpData(
                        buffer_base, 1,
                        mBlur->mCaptureWidth * mBlur->mCaptureHeight * 3 / 2,
                        mBlur->mCaptureWidth, mBlur->mCaptureHeight,
                        capture_msg.combo_buff.frame_number, "blur_output");

                    buffer_base = (unsigned char *)combo_buff_addr;
                    mBlur->dumpData(
                        buffer_base, 1,
                        mBlur->mCaptureWidth * mBlur->mCaptureHeight * 3 / 2,
                        mBlur->mCaptureWidth, mBlur->mCaptureHeight,
                        capture_msg.combo_buff.frame_number, "blur_yuv");
                }
            }
#ifndef BLUR_V3_TAKE_3_YUV
            if (mVersion == 3 && mBlur->mReqState == WAIT_SECOND_YUV_STATE &&
                mIsGalleryBlur) {
                memcpy(output_buff_addr, mBlur->m_pNearYuvBuffer,
                       mBlur->mCaptureWidth * mBlur->mCaptureHeight * 3 / 2);
            }
#endif
            camera3_capture_request_t request;
            camera3_stream_buffer_t *output_buffers = NULL;
            camera3_stream_buffer_t *input_buffer = NULL;

            memset(&request, 0x00, sizeof(camera3_capture_request_t));

            output_buffers = (camera3_stream_buffer_t *)malloc(
                sizeof(camera3_stream_buffer_t));
            if (!output_buffers) {
                HAL_LOGE("failed");
                if (combo_buff_addr != NULL) {
                    mBlur->unmap(capture_msg.combo_buff.buffer);
                    combo_buff_addr = NULL;
                }
                if (output_buff_addr != NULL) {
                    mBlur->unmap(output_buffer);
                    output_buff_addr = NULL;
                }
                return false;
            }
            memset(output_buffers, 0x00, sizeof(camera3_stream_buffer_t));

            input_buffer = (camera3_stream_buffer_t *)malloc(
                sizeof(camera3_stream_buffer_t));
            if (!input_buffer) {
                HAL_LOGE("failed");
                if (output_buffers != NULL) {
                    free((void *)output_buffers);
                    output_buffers = NULL;
                }
                if (combo_buff_addr != NULL) {
                    mBlur->unmap(capture_msg.combo_buff.buffer);
                    combo_buff_addr = NULL;
                }
                if (output_buff_addr != NULL) {
                    mBlur->unmap(output_buffer);
                    output_buff_addr = NULL;
                }
                return false;
            }
            memset(input_buffer, 0x00, sizeof(camera3_stream_buffer_t));

            memcpy((void *)&request, &mSavedCapRequest,
                   sizeof(camera3_capture_request_t));
            request.settings = mSavedCapReqsettings;

            memcpy(input_buffer, &mSavedCapReqstreambuff,
                   sizeof(camera3_stream_buffer_t));
            input_buffer->stream = &mMainStreams[mCaptureStreamsNum - 1];
            input_buffer->stream->width = mBlur->mCaptureWidth;
            input_buffer->stream->height = mBlur->mCaptureHeight;
            if (mBlur->mFlushing) {
                mime_type = 0;
                input_buffer->buffer = capture_msg.combo_buff.buffer;
            } else {
                input_buffer->buffer = output_buffer;
                mime_type = (int)MODE_BLUR;
            }
            mDevMain->hwi->camera_ioctrl(CAMERA_IOCTRL_SET_MIME_TYPE,
                                         &mime_type, NULL);

            memcpy((void *)&output_buffers[0], &mSavedCapReqstreambuff,
                   sizeof(camera3_stream_buffer_t));
            output_buffers[0].stream->width = mBlur->mCaptureWidth;
            output_buffers[0].stream->height = mBlur->mCaptureHeight;
            request.output_buffers = output_buffers;
            if (mVersion == 3 && mBlur->mReqState == WAIT_FIRST_YUV_STATE &&
                !mBlur->mFlushing) {
                output_buffers[0].stream = &mMainStreams[mCaptureStreamsNum];
                output_buffers[0].buffer =
                    &mBlur->mLocalCapBuffer[0].native_handle;
                request.input_buffer = NULL;
                mBlur->mReqState = WAIT_SECOND_YUV_STATE;
#ifdef BLUR_V3_TAKE_3_YUV
            } else if (mIsGalleryBlur &&
                       mBlur->mReqState == WAIT_SECOND_YUV_STATE &&
                       !mBlur->mFlushing) {
                output_buffers[0].stream = &mMainStreams[mCaptureStreamsNum];
                output_buffers[0].buffer =
                    &mBlur->mLocalCapBuffer[0].native_handle;
                request.input_buffer = NULL;
                mBlur->mReqState = WAIT_THIRD_YUV_STATE;
            } else {
                if (mBlur->mReqState == WAIT_THIRD_YUV_STATE) {
                    input_buffer->buffer = capture_msg.combo_buff.buffer;
                }
#else
            } else {
#endif
                output_buffers[0].stream =
                    &mMainStreams[mCaptureStreamsNum - 1];
                request.input_buffer = input_buffer;
                ADP_FAKESETBUFATTR_CAMERAONLY(
                    *request.output_buffers[0].buffer,
                    ADP_BUFSIZE(*mSavedResultBuff) -
                        (mBlur->mCaptureWidth * mBlur->mCaptureHeight * 3 / 2 +
                         BLUR_REFOCUS_PARAM_NUM * 4),
                    ADP_WIDTH(*request.output_buffers[0].buffer),
                    ADP_HEIGHT(*request.output_buffers[0].buffer));
                mBlur->mReqState = REPROCESS_STATE;
            }
            request.num_output_buffers = 1;

            if (0 > mDevMain->hwi->process_capture_request(mDevMain->dev,
                                                           &request)) {
                HAL_LOGE("failed. process capture request!");
            }
            if (NULL != mSavedCapReqsettings &&
                mBlur->mReqState == REPROCESS_STATE) {
                free_camera_metadata(mSavedCapReqsettings);
                mSavedCapReqsettings = NULL;
            }
            HAL_LOGD("mReqState:%d", mBlur->mReqState);
            if (input_buffer != NULL) {
                free((void *)input_buffer);
                input_buffer = NULL;
            }
            if (output_buffers != NULL) {
                free((void *)output_buffers);
                output_buffers = NULL;
            }
            if (combo_buff_addr != NULL) {
                mBlur->unmap(capture_msg.combo_buff.buffer);
                combo_buff_addr = NULL;
            }
            if (output_buff_addr != NULL) {
                mBlur->unmap(output_buffer);
                output_buff_addr = NULL;
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
 * FUNCTION   :initBlur20Params
 *
 * DESCRIPTION: init Blur 2.0 Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3Blur::CaptureThread::initBlur20Params() {
    memset(mWinPeakPos, 0x00, sizeof(short) * BLUR_AF_WINDOW_NUM);
    unsigned short cali_dist_seq_8M[] = {30, 40, 50,  60,  70,
                                         80, 90, 100, 110, 120};
    unsigned short cali_dac_seq_8M[] = {240, 225, 220, 215, 213,
                                        211, 209, 207, 205, 180};
    unsigned short cali_dist_seq_13M[] = {30, 40, 50,  60,  70,
                                          80, 90, 100, 110, 120};
    unsigned short cali_dac_seq_13M[] = {355, 340, 325, 300, 310,
                                         305, 302, 298, 295, 262};

    mCaptureInitParams.vcm_dac_up_bound = 0;
    mCaptureInitParams.vcm_dac_low_bound = 0;
    mCaptureInitParams.vcm_dac_info = NULL;
    mCaptureInitParams.vcm_dac_gain = 127;
    mCaptureInitParams.valid_depth_clip = 32;
    mCaptureInitParams.method = 0;
    mCaptureInitParams.row_num = 3;
    mCaptureInitParams.column_num = 3;
    mCaptureInitParams.boundary_ratio = 8;
    mCaptureInitParams.sel_size = 1;
    mCaptureInitParams.valid_depth = 8;
    mCaptureInitParams.slope = 0;
    mCaptureInitParams.valid_depth_up_bound = 100;
    mCaptureInitParams.valid_depth_low_bound = 0;
    mCaptureInitParams.cali_seq_len = 10;
    mCaptureInitParams.cali_dist_seq = (unsigned short *)malloc(
        sizeof(unsigned short) * mCaptureInitParams.cali_seq_len);
    if (mCaptureInitParams.cali_dist_seq == NULL) {
        return -1;
    }
    mCaptureInitParams.cali_dac_seq = (unsigned short *)malloc(
        sizeof(unsigned short) * mCaptureInitParams.cali_seq_len);
    if (mCaptureInitParams.cali_dac_seq == NULL) {
        return -1;
    }

    if (SprdCamera3Setting::s_setting[mBlur->mCameraId]
            .sensor_InfoInfo.pixer_array_size[0] == 4160) {
        memcpy(mCaptureInitParams.cali_dist_seq, cali_dist_seq_13M,
               sizeof(unsigned short) * mCaptureInitParams.cali_seq_len);
        memcpy(mCaptureInitParams.cali_dac_seq, cali_dac_seq_13M,
               sizeof(unsigned short) * mCaptureInitParams.cali_seq_len);
    } else {
        memcpy(mCaptureInitParams.cali_dist_seq, cali_dist_seq_8M,
               sizeof(unsigned short) * mCaptureInitParams.cali_seq_len);
        memcpy(mCaptureInitParams.cali_dac_seq, cali_dac_seq_8M,
               sizeof(unsigned short) * mCaptureInitParams.cali_seq_len);
    }
    return 0;
}

/*===========================================================================
 * FUNCTION   :initBlurInitParams
 *
 * DESCRIPTION: init Blur Init Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3Blur::CaptureThread::initBlurInitParams() {
    mFirstCapture = true;
    mFirstPreview = true;
    mUpdateCaptureWeightParams = true;

    // preview 720P v1.0 v1.1
    mLastMinScope = 10;      // min_slope*10000
    mLastMaxScope = 50;      // max_slope*10000
    mLastAdjustRati = 20000; // findex2gamma_adjust_ratio*10000
    // mPreviewInitParams.SmoothWinSize = 11;
    // mPreviewInitParams.Scalingratio = 2;
    mPreviewInitParams.box_filter_size = 13;
    mPreviewInitParams.min_slope = (float)(mLastMinScope) / 10000;
    mPreviewInitParams.max_slope = (float)(mLastMaxScope) / 10000;
    mPreviewInitParams.findex2gamma_adjust_ratio =
        (float)(mLastAdjustRati) / 10000;

    // capture 5M v1.0 v1.1 v1.2
    mLastMinScope = 4;        // min_slope*10000
    mLastMaxScope = 19;       // max_slope*10000
    mLastAdjustRati = 150000; // findex2gamma_adjust_ratio*10000
    mCaptureInitParams.Scalingratio = 8;
    mCaptureInitParams.SmoothWinSize = 5;
    mCaptureInitParams.box_filter_size = 0;
    mCaptureInitParams.min_slope = (float)(mLastMinScope) / 10000;
    mCaptureInitParams.max_slope = (float)(mLastMaxScope) / 10000;
    mCaptureInitParams.findex2gamma_adjust_ratio =
        (float)(mLastAdjustRati) / 10000;

    return initBlur20Params();
}

/*===========================================================================
 * FUNCTION   :initBlurWeightParams
 *
 * DESCRIPTION: init Blur Weight Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
void SprdCamera3Blur::CaptureThread::initBlurWeightParams() {
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    if (mBlur->mCameraId == CAM_BLUR_MAIN_ID) {
        mVersion = 3;
        mCaptureWeightParams.version = 2;
        mPreviewWeightParams.roi_type = 0;
        mCaptureWeightParams.roi_type = 0;
        mCaptureWeightParams.rear_cam_en = true;
        property_get("persist.sys.cam.ba.blur.version", prop, "0");
    } else {
        mVersion = 1;
        mCaptureWeightParams.version = 1;
        mPreviewWeightParams.roi_type = 1;
        mCaptureWeightParams.roi_type = 1;
        mCaptureWeightParams.rear_cam_en = false;
        property_get("persist.sys.cam.fr.blur.version", prop, "0");
    }
    if (atoi(prop) == 1 || atoi(prop) == 2 || atoi(prop) == 3) {
        mVersion = atoi(prop);
    }
    if (atoi(prop) == 1 || atoi(prop) == 2) {
        mCaptureWeightParams.version = atoi(prop);
    }
    mIsGalleryBlur = false;
    mIsBlurAlways = false;
    if (mVersion == 3) {
        property_get("persist.sys.gallery.blur", prop, "1");
        if (atoi(prop) == 1) {
            mIsGalleryBlur = true;
        }
        property_get("persist.sys.cam.blur.always", prop, "1");
        if (atoi(prop) == 1) {
            mIsBlurAlways = true;
            mCaptureWeightParams.version = 1;
        }
    } else if (mVersion == 1) {
        property_get("persist.sys.cam.fr.blur.type", prop, "2");
        if (atoi(prop) == 0 || atoi(prop) == 1 || atoi(prop) == 2) {
            mPreviewWeightParams.roi_type = atoi(prop);
            mCaptureWeightParams.roi_type = atoi(prop);
            if (atoi(prop) == 2) {
                mPreviewWeightParams.roi_type = 1;
            }
        }
    }
    HAL_LOGD("Camera:%d,lib1v:%d,lib2v:%d", mBlur->mCameraId,
             mCaptureWeightParams.version, mVersion);

    HAL_LOGD("roi_type:%d, mIsGalleryBlur:%d, mIsBlurAlways:%d",
             mCaptureWeightParams.roi_type, mIsGalleryBlur, mIsBlurAlways);

    property_get("persist.sys.cam.blur.cirlscal", prop, "50");
    if (atoi(prop) != 0) {
        mCircleSizeScale = atoi(prop);
    }
    mLastFaceNum = 0;
    mSkipFaceNum = 0;
    mRotation = 0;
    mLastTouchX = 0;
    mLastTouchY = 0;
    mBlurBody = true;
    mUpdataTouch = false;
    // preview weight params
    mPreviewWeightParams.f_number = 1;
    mPreviewWeightParams.valid_roi = 0;
    memset(mPreviewWeightParams.x1, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mPreviewWeightParams.y1, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mPreviewWeightParams.x2, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mPreviewWeightParams.y2, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mPreviewWeightParams.flag, 0x00, sizeof(int) * BLUR_MAX_ROI);
    mUpdatePreviewWeightParams = true;

    // capture weight params
    mCaptureWeightParams.f_number = 1;
    mCaptureWeightParams.valid_roi = 0;
    mCaptureWeightParams.total_roi = 0;
    mCaptureWeightParams.rotate_angle = 0;
    mCaptureWeightParams.win_peak_pos = mWinPeakPos;
    memset(mCaptureWeightParams.x1, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mCaptureWeightParams.y1, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mCaptureWeightParams.x2, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mCaptureWeightParams.y2, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mCaptureWeightParams.flag, 0x00, sizeof(int) * BLUR_MAX_ROI);

    // capture weight params
    mCapture2WeightParams.f_number = 1;
}

/*===========================================================================
 * FUNCTION   :isBlurInitParamsChanged
 *
 * DESCRIPTION: user change  blur init params by adb shell setprop
 *
 * PARAMETERS :
 *
 *
 * RETURN     : ret
 *==========================================================================*/
bool SprdCamera3Blur::CaptureThread::isBlurInitParamsChanged() {
    bool ret = false;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };

    property_get("persist.sys.camera.blur.conf", prop, "0");
    if (1 != atoi(prop)) {
        return false;
    }

    property_get("persist.sys.camera.blur.min", prop, "0");
    if (0 != atoi(prop) && mLastMinScope != atoi(prop)) {
        mLastMinScope = atoi(prop);
        mPreviewInitParams.min_slope = (float)(mLastMinScope) / 10000;
        mCaptureInitParams.min_slope = (float)(mLastMinScope) / 10000;
        ret = true;
    }

    property_get("persist.sys.camera.blur.max", prop, "0");
    if (0 != atoi(prop) && mLastMaxScope != atoi(prop)) {
        mLastMaxScope = atoi(prop);
        mPreviewInitParams.max_slope = (float)(mLastMaxScope) / 10000;
        mCaptureInitParams.max_slope = (float)(mLastMaxScope) / 10000;
        ret = true;
    }

    property_get("persist.sys.camera.blur.rati", prop, "0");
    if (0 != atoi(prop) && mLastAdjustRati != atoi(prop)) {
        mLastAdjustRati = atoi(prop);
        mPreviewInitParams.findex2gamma_adjust_ratio =
            (float)(mLastAdjustRati) / 10000;
        mCaptureInitParams.findex2gamma_adjust_ratio =
            (float)(mLastAdjustRati) / 10000;
        ret = true;
    }

    property_get("persist.sys.camera.blur.scal", prop, "0");
    if (0 != atoi(prop) && mCaptureInitParams.Scalingratio != atoi(prop)) {
        mCaptureInitParams.Scalingratio = atoi(prop);

        if (mOutWeightBuff != NULL)
            free(mOutWeightBuff);

        int buff_size =
            (mCaptureInitParams.width / mCaptureInitParams.Scalingratio) *
            (mCaptureInitParams.height / mCaptureInitParams.Scalingratio) *
            sizeof(unsigned short);
        mOutWeightBuff = (unsigned short *)malloc(buff_size);
        memset(mOutWeightBuff, 0, buff_size);

        ret = true;
    }

    property_get("persist.sys.camera.blur.win", prop, "0");
    if (0 != atoi(prop) && mCaptureInitParams.SmoothWinSize != atoi(prop)) {
        mCaptureInitParams.SmoothWinSize = atoi(prop);
        ret = true;
    }

    property_get("persist.sys.camera.blur.filt", prop, "0");
    if (0 != atoi(prop) && mCaptureInitParams.box_filter_size != atoi(prop)) {
        mCaptureInitParams.box_filter_size = atoi(prop);
        ret = true;
    }

    property_get("persist.sys.camera.blur.upb", prop, "0");
    if (0 != atoi(prop) && mCaptureInitParams.vcm_dac_up_bound != atoi(prop)) {
        mCaptureInitParams.vcm_dac_up_bound = atoi(prop);
        ret = true;
    }

    property_get("persist.sys.camera.blur.lob", prop, "0");
    if (0 != atoi(prop) && mCaptureInitParams.vcm_dac_low_bound != atoi(prop)) {
        mCaptureInitParams.vcm_dac_low_bound = atoi(prop);
        ret = true;
    }

    property_get("persist.sys.camera.blur.gain", prop, "0");
    if (0 != atoi(prop) && mCaptureInitParams.vcm_dac_gain != atoi(prop)) {
        mCaptureInitParams.vcm_dac_gain = atoi(prop);
        ret = true;
    }

    property_get("persist.sys.camera.blur.clip", prop, "0");
    if (0 != atoi(prop) && mCaptureInitParams.valid_depth_clip != atoi(prop)) {
        mCaptureInitParams.valid_depth_clip = atoi(prop);
        ret = true;
    }

    property_get("persist.sys.camera.blur.meth", prop, "0");
    if (0 != atoi(prop) && mCaptureInitParams.method != atoi(prop)) {
        mCaptureInitParams.method = atoi(prop);
        ret = true;
    }

    property_get("persist.sys.camera.blur.row", prop, "0");
    if (0 != atoi(prop) && mCaptureInitParams.row_num != atoi(prop)) {
        mCaptureInitParams.row_num = atoi(prop);
        ret = true;
    }

    property_get("persist.sys.camera.blur.col", prop, "0");
    if (0 != atoi(prop) && mCaptureInitParams.column_num != atoi(prop)) {
        mCaptureInitParams.column_num = atoi(prop);
        ret = true;
    }

    property_get("persist.sys.camera.blur.rtio", prop, "0");
    if (0 != atoi(prop) && mCaptureInitParams.boundary_ratio != atoi(prop)) {
        mCaptureInitParams.boundary_ratio = atoi(prop);
        ret = true;
    }

    property_get("persist.sys.camera.blur.sel", prop, "0");
    if (0 != atoi(prop) && mCaptureInitParams.sel_size != atoi(prop)) {
        mCaptureInitParams.sel_size = atoi(prop);
        ret = true;
    }

    property_get("persist.sys.camera.blur.dep", prop, "0");
    if (0 != atoi(prop) && mCaptureInitParams.valid_depth != atoi(prop)) {
        mCaptureInitParams.valid_depth = atoi(prop);
        ret = true;
    }

    property_get("persist.sys.camera.blur.slop", prop, "0");
    if (0 != atoi(prop) && mCaptureInitParams.slope != atoi(prop)) {
        mCaptureInitParams.slope = atoi(prop);
        ret = true;
    }

    property_get("persist.sys.camera.blur.vupb", prop, "0");
    if (0 != atoi(prop) &&
        mCaptureInitParams.valid_depth_up_bound != atoi(prop)) {
        mCaptureInitParams.valid_depth_up_bound = atoi(prop);
        ret = true;
    }

    property_get("persist.sys.camera.blur.vlob", prop, "0");
    if (0 != atoi(prop) &&
        mCaptureInitParams.valid_depth_low_bound != atoi(prop)) {
        mCaptureInitParams.valid_depth_low_bound = atoi(prop);
        ret = true;
    }

    return ret;
}

/*===========================================================================
 * FUNCTION   :updateBlurWeightParams
 *
 * DESCRIPTION: update Blur Weight Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
void SprdCamera3Blur::CaptureThread::updateBlurWeightParams(
    CameraMetadata metaSettings, int type) {
    uint8_t face_num = 0;
    uint8_t i = 0;
    uint8_t k = 0;
    int32_t x1 = 0;
    int32_t x2 = 0;
    int32_t max = 0;
    int32_t origW = SprdCamera3Setting::s_setting[mBlur->mCameraId]
                        .sensor_InfoInfo.pixer_array_size[0];
    int32_t origH = SprdCamera3Setting::s_setting[mBlur->mCameraId]
                        .sensor_InfoInfo.pixer_array_size[1];
    unsigned short savePreviewX = mPreviewWeightParams.sel_x;
    unsigned short savePreviewY = mPreviewWeightParams.sel_y;
    uint32_t orientation =
        SprdCamera3Setting::s_setting[mBlur->mCameraId].jpgInfo.orientation;

    mCaptureWeightParams.rotate_angle = orientation;
    if (origW == 0 || origH == 0) {
        return;
    }

    // always get f_num and orientattion in request
    if (type == 0) {
        if (metaSettings.exists(ANDROID_SPRD_BLUR_F_NUMBER)) {
            int fnum =
                metaSettings.find(ANDROID_SPRD_BLUR_F_NUMBER).data.i32[0];
            if (fnum < MIN_F_FUMBER) {
                fnum = MIN_F_FUMBER;
            } else if (fnum > MAX_F_FUMBER) {
                fnum = MAX_F_FUMBER;
            }
            fnum = (fnum)*MAX_BLUR_F_FUMBER / MAX_F_FUMBER;
            if (mPreviewWeightParams.f_number != fnum) {
                mPreviewWeightParams.f_number = fnum;
                mCaptureWeightParams.f_number = fnum;
                mCapture2WeightParams.f_number = fnum;
                mUpdatePreviewWeightParams = true;
            }
        }
        if (metaSettings.exists(ANDROID_SPRD_SENSOR_ROTATION_FOR_FRONT_BLUR)) {
            mRotation =
                metaSettings.find(ANDROID_SPRD_SENSOR_ROTATION_FOR_FRONT_BLUR)
                    .data.i32[0];
            if (mBlur->mCameraId == CAM_BLUR_MAIN_ID) {
                if (mRotation == 0) {
                    mRotation = 180;
                } else if (mRotation == 180) {
                    mRotation = 0;
                }
            }
            HAL_LOGD("rotation %d", mRotation);
        }
        if (metaSettings.exists(ANDROID_CONTROL_AF_REGIONS)) {
            uint32_t left =
                metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[0];
            uint32_t top =
                metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[1];
            uint32_t right =
                metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[2];
            uint32_t bottom =
                metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[3];
            int32_t x = left, y = top;
            HAL_LOGV("ANDROID_CONTROL_AF_REGIONS (%d,%d,%d,%d)", left, top,
                     right, bottom);
            if (!(left == 0 && top == 0 && right == 0 && bottom == 0)) {
                x = left + (right - left) / 2;
                y = top + (bottom - top) / 2;
                if (mBlur->mCameraId == CAM_BLUR_MAIN_ID) {
                    x = x * mPreviewInitParams.width / origW;
                    y = y * mPreviewInitParams.height / origH;
                    if (x != mPreviewWeightParams.sel_x ||
                        y != mPreviewWeightParams.sel_y) {
                        mPreviewWeightParams.sel_x = x;
                        mPreviewWeightParams.sel_y = y;
                        mUpdatePreviewWeightParams = true;
                    }
                } else {
                    if (mLastFaceNum > 0 &&
                        (x != mLastTouchX || y != mLastTouchY)) {
                        mLastTouchX = x;
                        mLastTouchY = y;
                        mUpdataTouch = true;
                        mUpdatePreviewWeightParams = true;
                        HAL_LOGD("mLastTouch (%d,%d)", mLastTouchX,
                                 mLastTouchY);
                    }
                }
            }
        } else {
            if (mPreviewWeightParams.sel_x != mPreviewInitParams.width / 2 &&
                mPreviewWeightParams.sel_y != mPreviewInitParams.height / 2) {
                if (mPreviewWeightParams.roi_type == 0) {
                    CONTROL_Tag controlInfo;
                    mBlur->m_pPhyCamera[CAM_TYPE_MAIN]
                        .hwi->mSetting->getCONTROLTag(&controlInfo);
                    if (controlInfo.af_state ==
                            ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN &&
                        controlInfo.af_mode ==
                            ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE) {
                        mPreviewWeightParams.sel_x =
                            mPreviewInitParams.width / 2;
                        mPreviewWeightParams.sel_y =
                            mPreviewInitParams.height / 2;
                        mUpdatePreviewWeightParams = true;
                        HAL_LOGD("autofocus and blur center");
                    }
                }
            }
        }
    }
    // back camera get other WeightParams in request
    if (type == 0 && mBlur->mCameraId == CAM_BLUR_MAIN_ID) {
        if (metaSettings.exists(ANDROID_SPRD_BLUR_CIRCLE_SIZE)) {
            int circle =
                metaSettings.find(ANDROID_SPRD_BLUR_CIRCLE_SIZE).data.i32[0];
            if (circle != 0) {
                int max = mPreviewInitParams.height * mCircleSizeScale / 100;
                if (mPreviewInitParams.width < mPreviewInitParams.height) {
                    max = mPreviewInitParams.width * mCircleSizeScale / 100;
                }
                circle = max - (max - BLUR_CIRCLE_VALUE_MIN) * circle / 255;
                if (mPreviewWeightParams.circle_size != circle) {
                    mPreviewWeightParams.circle_size = circle;
                    mUpdatePreviewWeightParams = true;
                }
            }
        }
    }

    // get WeightParams in result
    if (type == 1 && mVersion == 1) {
        face_num =
            SprdCamera3Setting::s_setting[mBlur->mCameraId].faceInfo.face_num;
        if (mLastFaceNum > 0 && face_num <= 0 && mSkipFaceNum < 10) {
            HAL_LOGV("mSkipFaceNum:%d", mSkipFaceNum);
            mSkipFaceNum++;
            return;
        }
        HAL_LOGV("mLastFaceNum:%d,face_num:%d", mLastFaceNum, face_num);
        if (face_num <= 0 && mLastFaceNum <= 0) {
            return;
        }
        mLastFaceNum = face_num;
        mSkipFaceNum = 0;

        if (face_num <= 0) {
            if (mIsBlurAlways) {
                HAL_LOGD("blur3.0-->blur1.0");
                return;
            }
            mPreviewWeightParams.sel_x = mPreviewInitParams.width / 2;
            mPreviewWeightParams.sel_y = mPreviewInitParams.height / 2;
            mPreviewWeightParams.circle_size =
                mPreviewInitParams.height * mCircleSizeScale / 100 / 2;
            mUpdatePreviewWeightParams = true;
            mPreviewWeightParams.valid_roi = 0;
            memset(mPreviewWeightParams.x1, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mPreviewWeightParams.y1, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mPreviewWeightParams.x2, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mPreviewWeightParams.y2, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mPreviewWeightParams.flag, 0x00, sizeof(int) * BLUR_MAX_ROI);
            mCaptureWeightParams.valid_roi = 0;
            mCaptureWeightParams.total_roi = 0;
            memset(mCaptureWeightParams.x1, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.y1, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.x2, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.y2, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.flag, 0x00, sizeof(int) * BLUR_MAX_ROI);
            HAL_LOGD("no face");
            memset(mFaceInfo, 0, sizeof(int32_t) * 4);
        } else if (metaSettings.exists(ANDROID_STATISTICS_FACE_RECTANGLES)) {
            if (mIsBlurAlways && mBlur->mReqState == WAIT_FIRST_YUV_STATE) {
                mPreviewWeightParams.roi_type = 1;
                mCaptureWeightParams.roi_type = 2;
                HAL_LOGD("blur3.0-->blur1.2");
            }
            HAL_LOGV("roi_type:%d,face_num:%d mUpdataTouch:%d",
                     mPreviewWeightParams.roi_type, face_num, mUpdataTouch);

            for (i = 0; i < face_num; i++) {
                x1 = metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                         .data.i32[i * 4 + 0];
                x2 = metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                         .data.i32[i * 4 + 2];
                if (x2 - x1 > max) {
                    k = i;
                    max = x2 - x1;
                }
            }
            mFaceInfo[0] =
                (metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                     .data.i32[k * 4 + 0]) *
                mPreviewInitParams.width / origW;
            mFaceInfo[1] =
                (metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                     .data.i32[k * 4 + 1]) *
                mPreviewInitParams.height / origH;
            mFaceInfo[2] =
                (metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                     .data.i32[k * 4 + 2]) *
                mPreviewInitParams.width / origW;
            mFaceInfo[3] =
                (metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                     .data.i32[k * 4 + 3]) *
                mPreviewInitParams.height / origH;

            if (k >= BLUR_MAX_ROI / 2 && mPreviewWeightParams.roi_type != 0) {
                metaSettings.update(ANDROID_STATISTICS_FACE_RECTANGLES,
                                    mFaceInfo, 4);
            }
            if (mPreviewWeightParams.roi_type == 0) {
                unsigned short sel_x;
                unsigned short sel_y;
                int circle;

                circle = (mFaceInfo[2] - mFaceInfo[0]) * 4 / 5;
                if (mPreviewWeightParams.circle_size != circle) {
                    mPreviewWeightParams.circle_size = circle;
                    mUpdatePreviewWeightParams = true;
                }
                sel_x = (mFaceInfo[0] + mFaceInfo[2]) / 2 + circle / 8;
                if (mPreviewWeightParams.sel_x != sel_x) {
                    mPreviewWeightParams.sel_x = sel_x;
                    mUpdatePreviewWeightParams = true;
                }

                sel_y = (mFaceInfo[1] + mFaceInfo[3]) / 2;
                if (mPreviewWeightParams.sel_y != sel_y) {
                    mPreviewWeightParams.sel_y = sel_y;
                    mUpdatePreviewWeightParams = true;
                }
            } else {
                int32_t bodyInfo[4];
                int32_t faceInfo[4];
                bool touchInBody = false;
                char prop1[PROPERTY_VALUE_MAX] = {
                    0,
                };
                char prop2[PROPERTY_VALUE_MAX] = {
                    0,
                };
                char prop3[PROPERTY_VALUE_MAX] = {
                    0,
                };
                char prop4[PROPERTY_VALUE_MAX] = {
                    0,
                };
                char prop5[PROPERTY_VALUE_MAX] = {
                    0,
                };
                char prop6[PROPERTY_VALUE_MAX] = {
                    0,
                };

                // Don't blur which face acreage less than max acreage width x%
                property_get("persist.sys.cam.blur.face.prop1", prop1, "50");

                // The face width increase by x%
                property_get("persist.sys.cam.blur.face.prop2", prop2, "30");

                // The face height increase by x% on top
                property_get("persist.sys.cam.blur.face.prop3", prop3, "70");

                // The width of the body is the width of the face increased by
                // x%
                property_get("persist.sys.cam.blur.face.prop4", prop4, "130");

                // The upper side of the body is at x% of the face position
                property_get("persist.sys.cam.blur.face.prop5", prop5, "0");

                // The face height increase by x% on bottom
                property_get("persist.sys.cam.blur.face.prop6", prop6, "20");

                memset(bodyInfo, 0x00, sizeof(int32_t) * 4);
                memset(faceInfo, 0x00, sizeof(int32_t) * 4);
                memset(mPreviewWeightParams.flag, 0x00,
                       sizeof(int) * BLUR_MAX_ROI);

                if (face_num > BLUR_MAX_ROI / 2) {
                    face_num = BLUR_MAX_ROI / 2;
                }
                k = 0;

                if (mCaptureWeightParams.roi_type == 2) {
                    memset(mCaptureWeightParams.x1, 0x00,
                           sizeof(int) * BLUR_MAX_ROI);
                    memset(mCaptureWeightParams.y1, 0x00,
                           sizeof(int) * BLUR_MAX_ROI);
                    memset(mCaptureWeightParams.x2, 0x00,
                           sizeof(int) * BLUR_MAX_ROI);
                    memset(mCaptureWeightParams.y2, 0x00,
                           sizeof(int) * BLUR_MAX_ROI);
                    memset(mCaptureWeightParams.flag, 0x00,
                           sizeof(int) * BLUR_MAX_ROI);
                }
                for (i = 0; i < face_num; i++) {
                    faceInfo[0] =
                        metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 0];
                    faceInfo[1] =
                        metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 1];
                    faceInfo[2] =
                        metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 2];
                    faceInfo[3] =
                        metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 3];

                    if (mCaptureWeightParams.roi_type == 2) {
                        if (mRotation == 0) {
                            mCaptureWeightParams.x1[i] =
                                faceInfo[2] * mCaptureInitParams.width / origW;
                            mCaptureWeightParams.y1[i] =
                                faceInfo[1] * mCaptureInitParams.height / origH;
                            mCaptureWeightParams.x2[i] =
                                faceInfo[0] * mCaptureInitParams.width / origW;
                            mCaptureWeightParams.y2[i] =
                                faceInfo[3] * mCaptureInitParams.height / origH;
                        } else if (mRotation == 90) {
                            mCaptureWeightParams.x1[i] =
                                faceInfo[2] * mCaptureInitParams.width / origW;
                            mCaptureWeightParams.y1[i] =
                                faceInfo[3] * mCaptureInitParams.height / origH;
                            mCaptureWeightParams.x2[i] =
                                faceInfo[0] * mCaptureInitParams.width / origW;
                            mCaptureWeightParams.y2[i] =
                                faceInfo[1] * mCaptureInitParams.height / origH;
                        } else if (mRotation == 180) {
                            mCaptureWeightParams.x1[i] =
                                faceInfo[0] * mCaptureInitParams.width / origW;
                            mCaptureWeightParams.y1[i] =
                                faceInfo[3] * mCaptureInitParams.height / origH;
                            mCaptureWeightParams.x2[i] =
                                faceInfo[2] * mCaptureInitParams.width / origW;
                            mCaptureWeightParams.y2[i] =
                                faceInfo[1] * mCaptureInitParams.height / origH;
                        } else {
                            mCaptureWeightParams.x1[i] =
                                faceInfo[0] * mCaptureInitParams.width / origW;
                            mCaptureWeightParams.y1[i] =
                                faceInfo[1] * mCaptureInitParams.height / origH;
                            mCaptureWeightParams.x2[i] =
                                faceInfo[2] * mCaptureInitParams.width / origW;
                            mCaptureWeightParams.y2[i] =
                                faceInfo[3] * mCaptureInitParams.height / origH;
                        }
                        mCaptureWeightParams.total_roi = face_num;
                    }
                    if ((faceInfo[2] - faceInfo[0]) < max * atoi(prop1) / 100) {
                        if (mPreviewWeightParams.valid_roi == face_num * 2) {
                            mUpdatePreviewWeightParams = true;
                        }
                        k++;
                        continue;
                    }

                    if (mRotation == 270) {
                        int w = faceInfo[2] - faceInfo[0];
                        int h = faceInfo[3] - faceInfo[1];

                        faceInfo[0] -= w * atoi(prop2) / 200;
                        if (faceInfo[0] < 0) {
                            faceInfo[0] = 0;
                        }

                        faceInfo[2] += w * atoi(prop2) / 200;
                        if (faceInfo[2] > origW) {
                            faceInfo[2] = origW;
                        }

                        faceInfo[1] -= h * atoi(prop3) / 100;
                        if (faceInfo[1] < 0) {
                            faceInfo[1] = 0;
                        }

                        faceInfo[3] += h * atoi(prop6) / 100;
                        if (faceInfo[3] > origH) {
                            faceInfo[3] = origH;
                        }

                        bodyInfo[0] =
                            faceInfo[0] -
                            (faceInfo[2] - faceInfo[0]) * atoi(prop4) / 200;
                        if (bodyInfo[0] < 0) {
                            bodyInfo[0] = 0;
                        }

                        bodyInfo[1] =
                            faceInfo[3] -
                            (faceInfo[3] - faceInfo[1]) * atoi(prop5) / 100;

                        bodyInfo[2] =
                            faceInfo[2] +
                            (faceInfo[2] - faceInfo[0]) * atoi(prop4) / 200;
                        if (bodyInfo[2] > origW) {
                            bodyInfo[2] = origW;
                        }

                        bodyInfo[3] = origH;
                    } else if (mRotation == 90) {
                        int w = faceInfo[2] - faceInfo[0];
                        int h = faceInfo[3] - faceInfo[1];

                        faceInfo[0] -= w * atoi(prop2) / 200;
                        if (faceInfo[0] < 0) {
                            faceInfo[0] = 0;
                        }

                        faceInfo[2] += w * atoi(prop2) / 200;
                        if (faceInfo[2] > origW) {
                            faceInfo[2] = origW;
                        }

                        faceInfo[3] += h * atoi(prop3) / 100;
                        if (faceInfo[3] > origH) {
                            faceInfo[3] = origH;
                        }

                        faceInfo[1] -= h * atoi(prop6) / 100;
                        if (faceInfo[1] < 0) {
                            faceInfo[1] = 0;
                        }

                        bodyInfo[0] =
                            faceInfo[0] -
                            (faceInfo[2] - faceInfo[0]) * atoi(prop4) / 200;
                        if (bodyInfo[0] < 0) {
                            bodyInfo[0] = 0;
                        }

                        bodyInfo[1] = 0;

                        bodyInfo[2] =
                            faceInfo[2] +
                            (faceInfo[2] - faceInfo[0]) * atoi(prop4) / 200;
                        if (bodyInfo[2] > origW) {
                            bodyInfo[2] = origW;
                        }

                        bodyInfo[3] =
                            faceInfo[1] +
                            (faceInfo[3] - faceInfo[1]) * atoi(prop5) / 100;
                    } else if (mRotation == 180) {
                        int w = faceInfo[3] - faceInfo[1];
                        int h = faceInfo[2] - faceInfo[0];

                        faceInfo[1] -= w * atoi(prop2) / 200;
                        if (faceInfo[1] < 0) {
                            faceInfo[1] = 0;
                        }

                        faceInfo[3] += w * atoi(prop2) / 200;
                        if (faceInfo[3] > origH) {
                            faceInfo[3] = origH;
                        }

                        faceInfo[0] -= h * atoi(prop3) / 100;
                        if (faceInfo[0] < 0) {
                            faceInfo[0] = 0;
                        }

                        faceInfo[2] += h * atoi(prop6) / 100;
                        if (faceInfo[2] > origW) {
                            faceInfo[2] = origW;
                        }

                        bodyInfo[0] =
                            faceInfo[2] -
                            (faceInfo[2] - faceInfo[0]) * atoi(prop5) / 100;

                        bodyInfo[1] =
                            faceInfo[1] -
                            (faceInfo[3] - faceInfo[1]) * atoi(prop4) / 200;
                        if (bodyInfo[1] < 0) {
                            bodyInfo[1] = 0;
                        }

                        bodyInfo[2] = origW;

                        bodyInfo[3] =
                            faceInfo[3] +
                            (faceInfo[3] - faceInfo[1]) * atoi(prop4) / 200;
                        if (bodyInfo[3] > origH) {
                            bodyInfo[3] = origH;
                        }
                    } else {
                        int w = faceInfo[3] - faceInfo[1];
                        int h = faceInfo[2] - faceInfo[0];

                        faceInfo[1] -= w * atoi(prop2) / 200;
                        if (faceInfo[1] < 0) {
                            faceInfo[1] = 0;
                        }

                        faceInfo[3] += w * atoi(prop2) / 200;
                        if (faceInfo[3] > origH) {
                            faceInfo[3] = origH;
                        }

                        faceInfo[2] += h * atoi(prop3) / 100;
                        if (faceInfo[2] > origW) {
                            faceInfo[2] = origW;
                        }

                        faceInfo[0] -= h * atoi(prop6) / 100;
                        if (faceInfo[0] < 0) {
                            faceInfo[0] = 0;
                        }

                        bodyInfo[0] = 0;

                        bodyInfo[1] =
                            faceInfo[1] -
                            (faceInfo[3] - faceInfo[1]) * atoi(prop4) / 200;
                        if (bodyInfo[1] < 0) {
                            bodyInfo[1] = 0;
                        }

                        bodyInfo[2] =
                            faceInfo[0] +
                            (faceInfo[2] - faceInfo[0]) * atoi(prop5) / 100;

                        bodyInfo[3] =
                            faceInfo[3] +
                            (faceInfo[3] - faceInfo[1]) * atoi(prop4) / 200;
                        if (bodyInfo[3] > origH) {
                            bodyInfo[3] = origH;
                        }
                    }
                    if (mUpdataTouch == true && touchInBody == false) {
                        if ((mLastTouchX > faceInfo[0] &&
                             mLastTouchX < faceInfo[2] &&
                             mLastTouchY > faceInfo[1] &&
                             mLastTouchY < faceInfo[3]) ||
                            (mLastTouchX > bodyInfo[0] &&
                             mLastTouchX < bodyInfo[2] &&
                             mLastTouchY > bodyInfo[1] &&
                             mLastTouchY < bodyInfo[3])) {
                            mBlurBody = true;
                            touchInBody = true;
                            HAL_LOGD("in body");
                        } else {
                            mBlurBody = false;
                            HAL_LOGD("out body");
                        }
                    }

                    if (mPreviewWeightParams.x1[2 * (i - k)] !=
                        faceInfo[0] * mPreviewInitParams.width / origW) {
                        mPreviewWeightParams.x1[2 * (i - k)] =
                            faceInfo[0] * mPreviewInitParams.width / origW;
                        mUpdatePreviewWeightParams = true;
                    }
                    if (mPreviewWeightParams.y1[2 * (i - k)] !=
                        faceInfo[1] * mPreviewInitParams.height / origH) {
                        mPreviewWeightParams.y1[2 * (i - k)] =
                            faceInfo[1] * mPreviewInitParams.height / origH;
                        mUpdatePreviewWeightParams = true;
                    }
                    if (mPreviewWeightParams.x2[2 * (i - k)] !=
                        faceInfo[2] * mPreviewInitParams.width / origW) {
                        mPreviewWeightParams.x2[2 * (i - k)] =
                            faceInfo[2] * mPreviewInitParams.width / origW;
                        mUpdatePreviewWeightParams = true;
                    }
                    if (mPreviewWeightParams.y2[2 * (i - k)] !=
                        faceInfo[3] * mPreviewInitParams.height / origH) {
                        mPreviewWeightParams.y2[2 * (i - k)] =
                            faceInfo[3] * mPreviewInitParams.height / origH;
                        mUpdatePreviewWeightParams = true;
                    }
                    mPreviewWeightParams.x1[2 * (i - k) + 1] =
                        bodyInfo[0] * mPreviewInitParams.width / origW;
                    mPreviewWeightParams.y1[2 * (i - k) + 1] =
                        bodyInfo[1] * mPreviewInitParams.height / origH;
                    mPreviewWeightParams.x2[2 * (i - k) + 1] =
                        bodyInfo[2] * mPreviewInitParams.width / origW;
                    mPreviewWeightParams.y2[2 * (i - k) + 1] =
                        bodyInfo[3] * mPreviewInitParams.height / origH;
                    mPreviewWeightParams.flag[2 * (i - k)] = 0;
                    mPreviewWeightParams.flag[2 * (i - k) + 1] = 1;
                }
                mPreviewWeightParams.valid_roi = (face_num - k) * 2;
                if (mCaptureWeightParams.roi_type == 2) {
                    mCaptureWeightParams.valid_roi = face_num - k;
                }
                if (mUpdataTouch == true) {
                    mPreviewWeightParams.sel_x =
                        mLastTouchX * mPreviewInitParams.width / origW;
                    mPreviewWeightParams.sel_y =
                        mLastTouchY * mPreviewInitParams.height / origH;
                    mUpdataTouch = false;
                }
            }
        } else {
            if (mUpdataTouch) {
                int i = 0;
                int32_t x = mLastTouchX * mPreviewInitParams.width / origW;
                int32_t y = mLastTouchY * mPreviewInitParams.height / origH;
                for (i = 0; i < mPreviewWeightParams.valid_roi; i++) {
                    if (x > mPreviewWeightParams.x1[i] &&
                        x < mPreviewWeightParams.x2[i] &&
                        y > mPreviewWeightParams.y1[i] &&
                        y < mPreviewWeightParams.y2[i]) {
                        mBlurBody = true;
                        break;
                    } else {
                        mBlurBody = false;
                    }
                }
                mPreviewWeightParams.sel_x =
                    mLastTouchX * mPreviewInitParams.width / origW;
                mPreviewWeightParams.sel_y =
                    mLastTouchY * mPreviewInitParams.height / origH;
                mUpdataTouch = false;
            }
        }
    }

    if (mUpdatePreviewWeightParams) {
        mUpdateCaptureWeightParams = true;
        mCaptureWeightParams.circle_size = mPreviewWeightParams.circle_size *
                                           mCaptureInitParams.height /
                                           mPreviewInitParams.height;
        mCaptureWeightParams.sel_x = mPreviewWeightParams.sel_x *
                                     mCaptureInitParams.width /
                                     mPreviewInitParams.width;
        mCaptureWeightParams.sel_y = mPreviewWeightParams.sel_y *
                                     mCaptureInitParams.height /
                                     mPreviewInitParams.height;
        mCapture2WeightParams.sel_x = mCaptureWeightParams.sel_x;
        mCapture2WeightParams.sel_y = mCaptureWeightParams.sel_y;
        if (mCaptureWeightParams.roi_type == 1 &&
            mPreviewWeightParams.valid_roi > 0) {
            mCaptureWeightParams.valid_roi = mPreviewWeightParams.valid_roi;
            memset(mCaptureWeightParams.x1, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.y1, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.x2, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.y2, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.flag, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memcpy(mCaptureWeightParams.flag, mPreviewWeightParams.flag,
                   sizeof(int) * BLUR_MAX_ROI);
            for (i = 0; i < mCaptureWeightParams.valid_roi / 2; i++) {
                mCaptureWeightParams.x1[2 * i] =
                    mPreviewWeightParams.x1[2 * i] * mCaptureInitParams.width /
                    mPreviewInitParams.width;
                mCaptureWeightParams.y1[2 * i] =
                    mPreviewWeightParams.y1[2 * i] * mCaptureInitParams.height /
                    mPreviewInitParams.height;
                mCaptureWeightParams.x2[2 * i] =
                    mPreviewWeightParams.x2[2 * i] * mCaptureInitParams.width /
                    mPreviewInitParams.width;
                mCaptureWeightParams.y2[2 * i] =
                    mPreviewWeightParams.y2[2 * i] * mCaptureInitParams.height /
                    mPreviewInitParams.height;
                mCaptureWeightParams.x1[2 * i + 1] =
                    mPreviewWeightParams.x1[2 * i + 1] *
                    mCaptureInitParams.width / mPreviewInitParams.width;
                mCaptureWeightParams.y1[2 * i + 1] =
                    mPreviewWeightParams.y1[2 * i + 1] *
                    mCaptureInitParams.height / mPreviewInitParams.height;
                mCaptureWeightParams.x2[2 * i + 1] =
                    mPreviewWeightParams.x2[2 * i + 1] *
                    mCaptureInitParams.width / mPreviewInitParams.width;
                mCaptureWeightParams.y2[2 * i + 1] =
                    mPreviewWeightParams.y2[2 * i + 1] *
                    mCaptureInitParams.height / mPreviewInitParams.height;
            }
        }
        if (mIsBlurAlways && mVersion == 1) {
            mUpdatePreviewWeightParams = false;
            mPreviewWeightParams.roi_type = 0;
            mPreviewWeightParams.sel_x = savePreviewX;
            mPreviewWeightParams.sel_y = savePreviewY;
            mCapture2WeightParams.sel_x = mPreviewWeightParams.sel_x *
                                          mCaptureInitParams.width /
                                          mPreviewInitParams.width;
            mCapture2WeightParams.sel_y = mPreviewWeightParams.sel_y *
                                          mCaptureInitParams.height /
                                          mPreviewInitParams.height;
        }
    }
}

/*===========================================================================
 * FUNCTION   :getOutWeightMap
 *
 * DESCRIPTION: storage output weight map temporary
 *
 * PARAMETERS : near yuv addr
 *
 *
 * RETURN     : none
 *==========================================================================*/
// void SprdCamera3Blur::CaptureThread::getOutWeightMap(
//    buffer_handle_t *output_buffer) {

//    //only for mSavedResultBuff
//    if(mOutWeightMap == NULL && mSavedResultBuff == output_buffer) {
//        unsigned char *buffer_base = NULL;
//        mBlur->map(output_buffer, (void **)(&buffer_base));
//        mOutWeightMap = (unsigned short *)(buffer_base + mBlur->mjpegSize -
//                                       ((mCaptureInitParams.width /
//                                         mCaptureInitParams.Scalingratio) *
//                                        (mCaptureInitParams.height /
//                                         mCaptureInitParams.Scalingratio) *
//                                        sizeof(unsigned short)));
//    } else {
//        HAL_LOGE("mOutWeightMap %p, not unmap or not mSavedResultBuff",
//        mOutWeightMap);
//    }
//}

/*===========================================================================
 * FUNCTION   :saveCaptureBlurParams
 *
 * DESCRIPTION: save Capture Blur Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
void SprdCamera3Blur::CaptureThread::saveCaptureBlurParams(
    buffer_handle_t *result_buff, uint32_t jpeg_size) {
    uint32_t i = 0;
    void *buffer_base = NULL;
    if (mBlur->map(result_buff, &buffer_base) != NO_ERROR) {
        HAL_LOGE("map result_buff(%p) error", result_buff);
        return;
    }

    unsigned char *buffer = (unsigned char *)buffer_base;
    uint32_t buffer_size = ADP_BUFSIZE(*result_buff);
    uint32_t yuv_size1 =
        mCaptureInitParams.width * mCaptureInitParams.height * 3 / 2;
    uint32_t yuv_size2 =
        mCaptureInitParams.width * mCaptureInitParams.height * 3 / 2;
    uint32_t output_weight_map =
        (mCaptureInitParams.width / mCaptureInitParams.Scalingratio) *
        (mCaptureInitParams.height / mCaptureInitParams.Scalingratio) *
        sizeof(unsigned short);
    uint32_t para_size = 0;
    uint32_t use_size = 0;
#ifdef YUV_CONVERT_TO_JPEG
    uint32_t near_jpeg_size = mBlur->mNearJpegSize;
    uint32_t far_jpeg_size = mBlur->mFarJpegSize;
#endif
    uint32_t weight_map_size = 0;

    HAL_LOGD("usesprdblurcapturelib: %d.%d", mVersion,
             mCaptureWeightParams.roi_type);

    if (mVersion == 3) {
#ifdef ISP_SUPPORT_MICRODEPTH
        uint32_t hfir_coeff_size =
            ARRAY_SIZE(mCapture2InitParams.hfir_coeff) * 4;
        uint32_t vfir_coeff_size =
            ARRAY_SIZE(mCapture2InitParams.vfir_coeff) * 4;
        uint32_t similar_coeff_size =
            ARRAY_SIZE(mCapture2InitParams.similar_coeff) * 4;
        uint32_t tmp_coeff_size = ARRAY_SIZE(mCapture2InitParams.tmp_coeff) * 4;

        para_size += BLUR3_REFOCUS_COMMON_PARAM_NUM * 4 + hfir_coeff_size +
                     vfir_coeff_size + similar_coeff_size + tmp_coeff_size;
#else
        para_size += BLUR3_REFOCUS_COMMON_PARAM_NUM * 4;
#endif
    } else {
        para_size += BLUR_REFOCUS_COMMON_PARAM_NUM * 4 +
                     BLUR_REFOCUS_2_PARAM_NUM * 4 + BLUR_AF_WINDOW_NUM * 4 +
                     BLUR_MAX_ROI * 5 * 4 +
                     mCaptureInitParams.cali_seq_len * 4 * 2;
    }

    if ((mVersion == 3) && (!mIsGalleryBlur)) {
        weight_map_size =
            mCaptureInitParams.width * mCaptureInitParams.height / 4;
    }
    if (!mIsGalleryBlur) {
#ifdef YUV_CONVERT_TO_JPEG
        far_jpeg_size = 0;
#endif
        yuv_size2 = 0;
    }
#ifdef YUV_CONVERT_TO_JPEG
    if (mVersion == 3) {
        use_size = para_size + near_jpeg_size + far_jpeg_size +
                   weight_map_size + jpeg_size;
    } else if ((mVersion == 1) && (mCaptureWeightParams.roi_type == 2)) {
        use_size = para_size + near_jpeg_size + jpeg_size + output_weight_map;
    } else {
        use_size = para_size + near_jpeg_size + jpeg_size;
    }
#else
    if (mVersion == 3) {
        use_size =
            para_size + yuv_size1 + yuv_size2 + weight_map_size + jpeg_size;
    } else if ((mVersion == 1) && (mCaptureWeightParams.roi_type == 2)) {
        use_size = para_size + yuv_size1 + jpeg_size + output_weight_map;
    } else {
        use_size = para_size + yuv_size1 + jpeg_size;
    }
#endif
    if (mVersion == 3) {
        uint32_t orientation = mCaptureWeightParams.rotate_angle;
        uint32_t width = mBlur->mCaptureWidth;
        uint32_t height = mBlur->mCaptureHeight;
        uint32_t FNum = mCapture2WeightParams.f_number;
        uint32_t SelCoordX = mCapture2WeightParams.sel_x;
        uint32_t SelCoordY = mCapture2WeightParams.sel_y;
        uint32_t isGalleryBlur = (uint32_t)mIsGalleryBlur;
        uint32_t version = mVersion;
        unsigned char BlurFlag[] = {'B', 'L', 'U', 'R'};
#ifdef ISP_SUPPORT_MICRODEPTH
        // blur3.0, use a new lib
        uint32_t enable = mCapture2InitParams.enable;
        uint32_t fir_mode = mCapture2InitParams.fir_mode;
        uint32_t fir_len = mCapture2InitParams.fir_len;
        uint32_t fir_channel = mCapture2InitParams.fir_channel;
        uint32_t fir_cal_mode = mCapture2InitParams.fir_cal_mode;
        uint32_t fir_edge_factor = mCapture2InitParams.fir_edge_factor;
        uint32_t depth_mode = mCapture2InitParams.depth_mode;
        uint32_t smooth_thr = mCapture2InitParams.smooth_thr;
        uint32_t touch_factor = mCapture2InitParams.touch_factor;
        uint32_t scale_factor = mCapture2InitParams.scale_factor;
        uint32_t refer_len = mCapture2InitParams.refer_len;
        uint32_t merge_factor = mCapture2InitParams.merge_factor;
        uint32_t similar_factor = mCapture2InitParams.similar_factor;
        uint32_t tmp_mode = mCapture2InitParams.tmp_mode;
        uint32_t tmp_thr = mCapture2InitParams.tmp_thr;
        uint32_t tuning_exist = mIspCapture2InitParams.tuning_exist;

        unsigned char *p1[] = {
            (unsigned char *)&enable,         (unsigned char *)&fir_mode,
            (unsigned char *)&fir_len,        (unsigned char *)&fir_channel,
            (unsigned char *)&fir_cal_mode,   (unsigned char *)&fir_edge_factor,
            (unsigned char *)&depth_mode,     (unsigned char *)&smooth_thr,
            (unsigned char *)&touch_factor,   (unsigned char *)&scale_factor,
            (unsigned char *)&refer_len,      (unsigned char *)&merge_factor,
            (unsigned char *)&similar_factor, (unsigned char *)&tmp_mode,
            (unsigned char *)&tmp_thr,        (unsigned char *)&orientation,
#ifdef YUV_CONVERT_TO_JPEG
            (unsigned char *)&near_jpeg_size, (unsigned char *)&far_jpeg_size,
#endif
            (unsigned char *)&width,          (unsigned char *)&height,
            (unsigned char *)&FNum,           (unsigned char *)&SelCoordX,
            (unsigned char *)&SelCoordY,      (unsigned char *)&isGalleryBlur,
            (unsigned char *)&tuning_exist,   (unsigned char *)&version,
            (unsigned char *)&BlurFlag};

        buffer += (use_size - BLUR3_REFOCUS_COMMON_PARAM_NUM * 4);
        for (i = 0; i < BLUR3_REFOCUS_COMMON_PARAM_NUM; i++) {
            memcpy(buffer + i * 4, p1[i], 4);
        }

        buffer -= ARRAY_SIZE(mCapture2InitParams.hfir_coeff) * 4;
        for (i = 0; i < ARRAY_SIZE(mCapture2InitParams.hfir_coeff); i++) {
            memcpy(buffer + i * 4, mCapture2InitParams.hfir_coeff + i, 4);
        }
        buffer -= ARRAY_SIZE(mCapture2InitParams.vfir_coeff) * 4;
        for (i = 0; i < ARRAY_SIZE(mCapture2InitParams.vfir_coeff); i++) {
            memcpy(buffer + i * 4, mCapture2InitParams.vfir_coeff + i, 4);
        }
        buffer -= ARRAY_SIZE(mCapture2InitParams.similar_coeff) * 4;
        for (i = 0; i < ARRAY_SIZE(mCapture2InitParams.similar_coeff); i++) {
            memcpy(buffer + i * 4, mCapture2InitParams.similar_coeff + i, 4);
        }
        buffer -= ARRAY_SIZE(mCapture2InitParams.tmp_coeff) * 4;
        for (i = 0; i < ARRAY_SIZE(mCapture2InitParams.tmp_coeff); i++) {
            memcpy(buffer + i * 4, mCapture2InitParams.tmp_coeff + i, 4);
        }
#else
        unsigned char *p1[] = {
            (unsigned char *)&orientation,
#ifdef YUV_CONVERT_TO_JPEG
            (unsigned char *)&near_jpeg_size, (unsigned char *)&far_jpeg_size,
#endif
            (unsigned char *)&width,          (unsigned char *)&height,
            (unsigned char *)&FNum,           (unsigned char *)&SelCoordX,
            (unsigned char *)&SelCoordY,      (unsigned char *)&isGalleryBlur,
            (unsigned char *)&version,        (unsigned char *)&BlurFlag};

        buffer += (use_size - BLUR3_REFOCUS_COMMON_PARAM_NUM * 4);
        for (i = 0; i < BLUR3_REFOCUS_COMMON_PARAM_NUM; i++) {
            memcpy(buffer + i * 4, p1[i], 4);
        }
#endif
    } else {
        // blur1.0 and blur2.0 commom
        uint32_t orientation = mCaptureWeightParams.rotate_angle;
        uint32_t MainWidthData = mBlur->mCaptureWidth;
        uint32_t MainHeightData = mBlur->mCaptureHeight;
        uint32_t rear_cam_en = mCaptureWeightParams.rear_cam_en;
        uint32_t roi_type = mCaptureWeightParams.roi_type;
        uint32_t FNum = mCaptureWeightParams.f_number;
        uint32_t circle = mCaptureWeightParams.circle_size;
        uint32_t valid_roi = mCaptureWeightParams.valid_roi;
        uint32_t total_roi = mCaptureWeightParams.total_roi;
        uint32_t MinScope = mLastMinScope;
        uint32_t MaxScope = mLastMaxScope;
        uint32_t AdjustRati = mLastAdjustRati;
        uint32_t SelCoordX = mCaptureWeightParams.sel_x;
        uint32_t SelCoordY = mCaptureWeightParams.sel_y;
        uint32_t scaleSmoothWidth = MainWidthData / BLUR_SMOOTH_SIZE_SCALE;
        uint32_t scaleSmoothHeight = MainHeightData / BLUR_SMOOTH_SIZE_SCALE;
        uint32_t box_filter_size = mCaptureInitParams.box_filter_size;
        uint32_t version = mCaptureWeightParams.version;

        char prop1[PROPERTY_VALUE_MAX] = {
            0,
        };

        HAL_LOGD("jpegorientation=%d, blur=%d", orientation,
                 mCaptureWeightParams.rotate_angle);
        unsigned char BlurFlag[] = {'B', 'L', 'U', 'R'};
        unsigned char *p1[] = {(unsigned char *)&orientation,
                               (unsigned char *)&MainWidthData,
                               (unsigned char *)&MainHeightData,
#ifdef YUV_CONVERT_TO_JPEG
                               (unsigned char *)&near_jpeg_size,
#else
                               (unsigned char *)&yuv_size1,
#endif
                               (unsigned char *)&rear_cam_en,
                               (unsigned char *)&roi_type,
                               (unsigned char *)&FNum,
                               (unsigned char *)&circle,
                               (unsigned char *)&valid_roi,
                               (unsigned char *)&total_roi,
                               (unsigned char *)&MinScope,
                               (unsigned char *)&MaxScope,
                               (unsigned char *)&AdjustRati,
                               (unsigned char *)&SelCoordX,
                               (unsigned char *)&SelCoordY,
                               (unsigned char *)&scaleSmoothWidth,
                               (unsigned char *)&scaleSmoothHeight,
                               (unsigned char *)&box_filter_size,
                               (unsigned char *)&version,
                               (unsigned char *)&BlurFlag};

        // cpoy common param to tail
        buffer += (use_size - BLUR_REFOCUS_COMMON_PARAM_NUM * 4);
        HAL_LOGD("common param base=%p", buffer);
        for (i = 0; i < BLUR_REFOCUS_COMMON_PARAM_NUM; i++) {
            memcpy(buffer + i * 4, p1[i], 4);
        }

        // blur2.0 use only,always save now
        uint32_t scaling_ratio = mCaptureInitParams.Scalingratio;
        uint32_t smooth_winSize = mCaptureInitParams.SmoothWinSize;
        uint32_t vcm_dac_up_bound = mCaptureInitParams.vcm_dac_up_bound;
        uint32_t vcm_dac_low_bound = mCaptureInitParams.vcm_dac_low_bound;
        uint32_t vcm_dac_info = 0; //*mCaptureInitParams.vcm_dac_info;
        uint32_t vcm_dac_gain = mCaptureInitParams.vcm_dac_gain;
        uint32_t valid_depth_clip = mCaptureInitParams.valid_depth_clip;
        uint32_t method = mCaptureInitParams.method;
        uint32_t row_num = mCaptureInitParams.row_num;
        uint32_t column_num = mCaptureInitParams.column_num;
        uint32_t boundary_ratio = mCaptureInitParams.boundary_ratio;
        uint32_t sel_size = mCaptureInitParams.sel_size;
        uint32_t valid_depth = mCaptureInitParams.valid_depth;
        uint32_t slope = mCaptureInitParams.slope;
        uint32_t valid_depth_up_bound = mCaptureInitParams.valid_depth_up_bound;
        uint32_t valid_depth_low_bound =
            mCaptureInitParams.valid_depth_low_bound;
        uint32_t cali_seq_len = mCaptureInitParams.cali_seq_len;
        unsigned char *p2[] = {(unsigned char *)&scaling_ratio,
                               (unsigned char *)&smooth_winSize,
                               (unsigned char *)&vcm_dac_up_bound,
                               (unsigned char *)&vcm_dac_low_bound,
                               (unsigned char *)&vcm_dac_info,
                               (unsigned char *)&vcm_dac_gain,
                               (unsigned char *)&valid_depth_clip,
                               (unsigned char *)&method,
                               (unsigned char *)&row_num,
                               (unsigned char *)&column_num,
                               (unsigned char *)&boundary_ratio,
                               (unsigned char *)&sel_size,
                               (unsigned char *)&valid_depth,
                               (unsigned char *)&slope,
                               (unsigned char *)&valid_depth_up_bound,
                               (unsigned char *)&valid_depth_low_bound,
                               (unsigned char *)&cali_seq_len};

        buffer -= BLUR_REFOCUS_2_PARAM_NUM * 4;
        HAL_LOGD("blur2.0 param base=%p", buffer);
        for (i = 0; i < BLUR_REFOCUS_2_PARAM_NUM; i++) {
            memcpy(buffer + i * 4, p2[i], 4);
        }
        buffer -= BLUR_AF_WINDOW_NUM * 4;
        for (i = 0; i < BLUR_AF_WINDOW_NUM; i++) {
            memcpy(buffer + i * 4, mCaptureWeightParams.win_peak_pos + i, 2);
        }

        buffer -= BLUR_MAX_ROI * 4 * 5;
        for (i = 0; i < BLUR_MAX_ROI; i++) {
            memcpy(buffer + i * 4, mCaptureWeightParams.x1 + i, 4);
        }
        for (i = 0; i < BLUR_MAX_ROI; i++) {
            memcpy((buffer + BLUR_MAX_ROI * 4) + i * 4,
                   mCaptureWeightParams.y1 + i, 4);
        }
        for (i = 0; i < BLUR_MAX_ROI; i++) {
            memcpy((buffer + BLUR_MAX_ROI * 8) + i * 4,
                   mCaptureWeightParams.x2 + i, 4);
        }
        for (i = 0; i < BLUR_MAX_ROI; i++) {
            memcpy((buffer + BLUR_MAX_ROI * 12) + i * 4,
                   mCaptureWeightParams.y2 + i, 4);
        }
        for (i = 0; i < BLUR_MAX_ROI; i++) {
            memcpy((buffer + BLUR_MAX_ROI * 16) + i * 4,
                   mCaptureWeightParams.flag + i, 4);
        }

        buffer -= cali_seq_len * 4;
        for (i = 0; i < cali_seq_len; i++) {
            memcpy(buffer + i * 4, mCaptureInitParams.cali_dac_seq + i, 2);
        }
        buffer -= cali_seq_len * 4;
        for (i = 0; i < cali_seq_len; i++) {
            memcpy(buffer + i * 4, mCaptureInitParams.cali_dist_seq + i, 2);
        }
    }

#ifdef YUV_CONVERT_TO_JPEG
    // cpoy near jpeg1
    unsigned char *orig_jpeg_data1 = NULL;
    if (mBlur->map(mBlur->m_pNearJpegBuffer, (void **)(&orig_jpeg_data1)) ==
        NO_ERROR) {
        buffer -= near_jpeg_size;
        memcpy(buffer, orig_jpeg_data1, near_jpeg_size);

        mBlur->unmap(mBlur->m_pNearJpegBuffer);
        orig_jpeg_data1 = NULL;
    } else {
        HAL_LOGE("map m_pNearJpegBuffer(%p) failed", mBlur->m_pNearJpegBuffer);
    }

    if (mVersion == 3) {
        if (mIsGalleryBlur) {
            // cpoy far jpeg2
            unsigned char *orig_jpeg_data2 = NULL;
            if (mBlur->map(mBlur->m_pFarJpegBuffer,
                           (void **)(&orig_jpeg_data2)) == NO_ERROR) {
                buffer -= far_jpeg_size;
                memcpy(buffer, orig_jpeg_data2, far_jpeg_size);

                mBlur->unmap(mBlur->m_pFarJpegBuffer);
                orig_jpeg_data2 = NULL;
            } else {
                HAL_LOGE("map m_pFarJpegBuffer(%p) failed",
                         mBlur->m_pFarJpegBuffer);
            }
        } else {
            // cpoy weight map
            buffer -= weight_map_size;
            memcpy(buffer, (unsigned char *)mBlur->weight_map, weight_map_size);
        }
    }
#else
    // cpoy near yuv1
    unsigned char *orig_yuv_data1 = (unsigned char *)(mBlur->m_pNearYuvBuffer);

    buffer -= yuv_size1;
    memcpy(buffer, orig_yuv_data1, yuv_size1);

    if (mVersion == 3) {
        if (mIsGalleryBlur) {
            // cpoy far yuv2
            unsigned char *orig_yuv_data2 =
                (unsigned char *)(mBlur->m_pFarYuvBuffer);

            buffer -= yuv_size2;
            memcpy(buffer, orig_yuv_data2, yuv_size2);
        } else {
            // cpoy weight map
            buffer -= weight_map_size;
            memcpy(buffer, (unsigned char *)mBlur->weight_map, weight_map_size);
        }
    }
#endif
    // copy output weight map for blur1.2
    if ((mVersion == 1) && (mCaptureWeightParams.roi_type == 2)) {
        buffer -= output_weight_map;
        memcpy(buffer, (unsigned char *)mOutWeightBuff, output_weight_map);
    }

    // blob to indicate all image size(use_size)
    buffer = (unsigned char *)buffer_base;
    mBlur->setJpegSize((char *)buffer, buffer_size, use_size);

    char prop1[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.sys.camera.blur.dump", prop1, "0");
    if (1 == atoi(prop1)) {
        dumpSaveImages(buffer_base, use_size, jpeg_size);
    }

    if (buffer_base != NULL) {
        mBlur->unmap(result_buff);
        buffer_base = NULL;
    }
}

void SprdCamera3Blur::CaptureThread::dumpSaveImages(void *result_buff,
                                                    uint32_t use_size,
                                                    uint32_t jpeg_size) {
    uint32_t para_num = 0;
    unsigned char *buffer_base = (unsigned char *)result_buff;
    uint32_t yuv_size =
        mCaptureInitParams.width * mCaptureInitParams.height * 3 / 2;
    uint32_t weight_map_size =
        mCaptureInitParams.width * mCaptureInitParams.height / 4;
    uint32_t output_weight_map =
        (mCaptureInitParams.width / mCaptureInitParams.Scalingratio) *
        (mCaptureInitParams.height / mCaptureInitParams.Scalingratio) *
        sizeof(unsigned short);
    uint32_t para_size = 0;
#ifdef YUV_CONVERT_TO_JPEG
    uint32_t near_jpeg_size = mBlur->mNearJpegSize;
    uint32_t far_jpeg_size = mBlur->mFarJpegSize;
#endif

    if (mVersion == 3) {
#ifdef ISP_SUPPORT_MICRODEPTH
        uint32_t hfir_coeff_size =
            ARRAY_SIZE(mCapture2InitParams.hfir_coeff) * 4;
        uint32_t vfir_coeff_size =
            ARRAY_SIZE(mCapture2InitParams.vfir_coeff) * 4;
        uint32_t similar_coeff_size =
            ARRAY_SIZE(mCapture2InitParams.similar_coeff) * 4;
        uint32_t tmp_coeff_size = ARRAY_SIZE(mCapture2InitParams.tmp_coeff) * 4;

        para_size += BLUR3_REFOCUS_COMMON_PARAM_NUM * 4 + hfir_coeff_size +
                     vfir_coeff_size + similar_coeff_size + tmp_coeff_size;
#else
        para_num += BLUR3_REFOCUS_COMMON_PARAM_NUM;
        para_size = para_num * 4;
#endif
    } else {
        para_num += BLUR_REFOCUS_COMMON_PARAM_NUM + BLUR_REFOCUS_2_PARAM_NUM +
                    BLUR_AF_WINDOW_NUM + BLUR_MAX_ROI * 5 +
                    mCaptureInitParams.cali_seq_len * 2;
        para_size = para_num * 4;
    }
    // dump jpeg
    mBlur->dumpData(buffer_base, 2, jpeg_size, mBlur->mCaptureWidth,
                    mBlur->mCaptureHeight, 0, "jpeg");
    // dump para
    buffer_base += (use_size - para_size);
    mBlur->dumpData(buffer_base, 3, para_num, 4, 0, 0, "parameter");
#ifdef YUV_CONVERT_TO_JPEG
    // dump near jpeg
    buffer_base -= near_jpeg_size;
    mBlur->dumpData(buffer_base, 2, near_jpeg_size, mBlur->mCaptureWidth,
                    mBlur->mCaptureHeight, 0, "nearJpeg");
    // dump near yuv
    mBlur->dumpData((unsigned char *)(mBlur->m_pNearYuvBuffer), 1, yuv_size,
                    mCaptureInitParams.width, mCaptureInitParams.height, 0,
                    "nearYuv");
    if (mVersion == 3) {
        if (mIsGalleryBlur) {
            // dump far jpeg
            buffer_base -= far_jpeg_size;
            mBlur->dumpData(buffer_base, 2, far_jpeg_size, mBlur->mCaptureWidth,
                            mBlur->mCaptureHeight, 0, "farJpeg");
            // dump far yuv
            mBlur->dumpData((unsigned char *)(mBlur->m_pFarYuvBuffer), 1,
                            yuv_size, mCaptureInitParams.width,
                            mCaptureInitParams.height, 0, "farYuv");
        } else {
            // dump weigth map
            buffer_base -= weight_map_size;
            mBlur->dumpData(buffer_base, 1, weight_map_size,
                            mCaptureInitParams.width, mCaptureInitParams.height,
                            0, "weightMap");
        }
    }
#else
    // dump near yuv
    buffer_base -= yuv_size;
    mBlur->dumpData(buffer_base, 1, yuv_size, mCaptureInitParams.width,
                    mCaptureInitParams.height, 0, "nearYuv");
    if (mVersion == 3) {
        if (mIsGalleryBlur) {
            // dump far yuv
            buffer_base -= yuv_size;
            mBlur->dumpData(buffer_base, 1, yuv_size, mCaptureInitParams.width,
                            mCaptureInitParams.height, 0, "farYuv");
        } else {
            // dump weight map
            buffer_base -= weight_map_size;
            mBlur->dumpData(buffer_base, 1, yuv_size, mCaptureInitParams.width,
                            mCaptureInitParams.height, 0, "weightMap");
        }
    }
#endif
    if ((mVersion == 1) && (mCaptureWeightParams.roi_type == 2)) {
        // dump output weight map
        buffer_base -= output_weight_map;
        mBlur->dumpData(
            buffer_base, 1, output_weight_map,
            mCaptureInitParams.width / mCaptureInitParams.Scalingratio,
            mCaptureInitParams.height / mCaptureInitParams.Scalingratio, 0,
            "outWeightMap");
    }
}

/*===========================================================================
 * FUNCTION   :getIspAfFullscanInfo
 *
 * DESCRIPTION: get Af Fullscan Info
 *
 * PARAMETERS :
 *
 *
 * RETURN     : result
 *==========================================================================*/
uint8_t SprdCamera3Blur::CaptureThread::getIspAfFullscanInfo() {
    int rc = 0;
    struct isp_af_fullscan_info af_fullscan_info;
    SprdCamera3HWI *hwiMain = mBlur->m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->camera_ioctrl(CAMERA_IOCTRL_GET_FULLSCAN_INFO,
                                &af_fullscan_info, &mVersion);
    if (rc < 0) {
        HAL_LOGE("read sub sensor failed");
        return rc;
    }
    HAL_LOGD("getIspAfFullscanInfo %d %d %d %d", af_fullscan_info.af_peak_pos,
             af_fullscan_info.near_peak_pos, af_fullscan_info.far_peak_pos,
             af_fullscan_info.distance_reminder);
    mIspInfo.af_peak_pos = af_fullscan_info.af_peak_pos;
    mIspInfo.near_peak_pos = af_fullscan_info.near_peak_pos;
    mIspInfo.far_peak_pos = af_fullscan_info.far_peak_pos;
    mIspInfo.distance_reminder = af_fullscan_info.distance_reminder;

    if (af_fullscan_info.win_peak_pos != NULL && af_fullscan_info.row_num > 0 &&
        af_fullscan_info.column_num > 0) {
        char prop[PROPERTY_VALUE_MAX] = {
            0,
        };

        property_get("persist.sys.camera.blur.conf", prop, "0");

        if (1 != atoi(prop) && mFirstCapture) {
            mCaptureInitParams.row_num = af_fullscan_info.row_num;
            mCaptureInitParams.column_num = af_fullscan_info.column_num;
            mCaptureInitParams.vcm_dac_up_bound =
                af_fullscan_info.vcm_dac_up_bound;
            mCaptureInitParams.vcm_dac_low_bound =
                af_fullscan_info.vcm_dac_low_bound;
            mCaptureInitParams.boundary_ratio = af_fullscan_info.boundary_ratio;
#if BLUR_GET_SEQ_FROM_AF
            mCaptureInitParams.valid_depth_up_bound =
                af_fullscan_info.valid_depth_up_bound;
            mCaptureInitParams.valid_depth_low_bound =
                af_fullscan_info.valid_depth_low_bound;
            mCaptureInitParams.cali_seq_len = af_fullscan_info.cali_seq_len;
            for (int i = 0; i < mCaptureInitParams.cali_seq_len; i++) {
                if (mCaptureInitParams.cali_dist_seq[i] !=
                        af_fullscan_info.cali_dist_seq[i] ||
                    mCaptureInitParams.cali_dac_seq[i] !=
                        af_fullscan_info.cali_dac_seq[i]) {
                    mCaptureInitParams.cali_dist_seq[i] =
                        af_fullscan_info.cali_dist_seq[i];
                    mCaptureInitParams.cali_dac_seq[i] =
                        af_fullscan_info.cali_dac_seq[i];
                }
            }
#endif
        }

        for (int i = 0;
             i < af_fullscan_info.row_num * af_fullscan_info.column_num; i++) {
            if (*(af_fullscan_info.win_peak_pos + i) != mWinPeakPos[i]) {
                mWinPeakPos[i] = *(af_fullscan_info.win_peak_pos + i);
                mUpdateCaptureWeightParams = true;
            }
            HAL_LOGD("win_peak_pos:%d", mWinPeakPos[i]);
        }
    }
    if (mIsBlurAlways && 0 != mIspInfo.distance_reminder) {
        mVersion = 1;
    }
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
void SprdCamera3Blur::CaptureThread::requestExit() {
    blur_queue_msg_t blur_msg;
    blur_msg.msg_type = BLUR_MSG_EXIT;
    mCaptureMsgList.push_back(blur_msg);
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
void SprdCamera3Blur::CaptureThread::waitMsgAvailable() {
    // TODO:what to do for timeout
    while (mCaptureMsgList.empty()) {
        if (mBlur->mFlushing) {
            Mutex::Autolock l(mBlur->mMergequeueFinishMutex);
            mBlur->mMergequeueFinishSignal.signal();
            HAL_LOGD("send mMergequeueFinishSignal.signal");
        }
        {
            Mutex::Autolock l(mMergequeueMutex);
            mMergequeueSignal.waitRelative(mMergequeueMutex,
                                           BLUR_THREAD_TIMEOUT);
        }
    }
}

/*======================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: dump fd
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Blur::dump(const struct camera3_device *device, int fd) {
    HAL_LOGV("E");
    CHECK_BLUR();

    mBlur->_dump(device, fd);

    HAL_LOGV("X");
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
int SprdCamera3Blur::flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGV(" E");
    CHECK_BLUR_ERROR();

    rc = mBlur->_flush(device);

    HAL_LOGV(" X");

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
int SprdCamera3Blur::initialize(const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;

    HAL_LOGI("E");
    CHECK_HWI_ERROR(hwiMain);

    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mPreviewStreamsNum = 0;
    mCaptureThread->mCaptureStreamsNum = 0;
    mCaptureThread->mSavedResultBuff = NULL;
    mCaptureThread->mSavedCapReqsettings = NULL;
    mjpegSize = 0;
    m_pNearYuvBuffer = NULL;
    m_pFarYuvBuffer = NULL;
    weight_map = NULL;
#ifdef YUV_CONVERT_TO_JPEG
    mNearJpegSize = 0;
    mFarJpegSize = 0;
    m_pNearJpegBuffer = NULL;
    m_pFarJpegBuffer = NULL;
#endif
    mFlushing = false;
    mReqState = PREVIEW_REQUEST_STATE;
    SprdCamera3MultiBase::initialize(MODE_BLUR);

    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }
    if (m_nPhyCameras == 2) {
        sprdCam = m_pPhyCamera[CAM_TYPE_AUX];
        SprdCamera3HWI *hwiAux = sprdCam.hwi;
        CHECK_HWI_ERROR(hwiAux);

        rc = hwiAux->initialize(sprdCam.dev, &callback_ops_aux);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error aux camera while initialize !! ");
            return rc;
        }
        int on_off = STREAM_ON;
        rc = hwiAux->camera_ioctrl(CAMERA_IOCTRL_COVERED_SENSOR_STREAM_CTRL,
                                   &on_off, NULL);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error while aux camera streamon !! ");
            return rc;
        }
    }
    memset(mLocalCapBuffer, 0, sizeof(new_mem_t) * BLUR_LOCAL_CAPBUFF_NUM);
    memset(&mCaptureThread->mSavedCapRequest, 0,
           sizeof(camera3_capture_request_t));
    memset(&mCaptureThread->mSavedCapReqstreambuff, 0,
           sizeof(camera3_stream_buffer_t));
    mCaptureThread->mCallbackOps = callback_ops;
    mCaptureThread->mDevMain = &m_pPhyCamera[CAM_TYPE_MAIN];
    mCaptureThread->run(String8::format("Blur").string(), -7);
    rc = mCaptureThread->initBlurInitParams();
    mCaptureThread->initBlurWeightParams();
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
int SprdCamera3Blur::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGV("E");

    int rc = 0;
    camera3_stream_t *pMainStreams[BLUR_MAX_NUM_STREAMS];
    size_t i = 0;
    size_t j = 0;
    int w = 0;
    int h = 0;
    struct stream_info_s stream_info;

    Mutex::Autolock l(mLock);

    HAL_LOGD("configurestreams, stream num:%d", stream_list->num_streams);
    for (size_t i = 0; i < stream_list->num_streams; i++) {
        int requestStreamType = getStreamType(stream_list->streams[i]);
        if (requestStreamType == PREVIEW_STREAM) {
            mPreviewStreamsNum = i;
            mCaptureThread->mPreviewInitParams.width =
                stream_list->streams[i]->width;
            mCaptureThread->mPreviewInitParams.height =
                stream_list->streams[i]->height;
            if (mCaptureThread->mFirstPreview) {
                mCaptureThread->mPreviewWeightParams.sel_x =
                    mCaptureThread->mPreviewInitParams.width / 2;
                mCaptureThread->mPreviewWeightParams.sel_y =
                    mCaptureThread->mPreviewInitParams.height / 2;
                mCaptureThread->mPreviewWeightParams.circle_size =
                    mCaptureThread->mPreviewInitParams.height *
                    mCaptureThread->mCircleSizeScale / 100 / 2;
            }
            HAL_LOGD("config preview stream num: %d, size: %dx%d", i,
                     stream_list->streams[i]->width,
                     stream_list->streams[i]->height);
        } else if (requestStreamType == SNAPSHOT_STREAM) {
            w = stream_list->streams[i]->width;
            h = stream_list->streams[i]->height;

            // workaround jpeg cant handle 16-noalign issue, when jpeg fix this
            // issue, we will remove these code
            if (h == 1944 && w == 2592) {
                h = 1952;
            }

            if (mCaptureWidth != w && mCaptureHeight != h) {
                freeLocalCapBuffer();
                for (size_t j = 0; j < BLUR_LOCAL_CAPBUFF_NUM; j++) {
                    if (0 > allocateOne(w, h, &(mLocalCapBuffer[j]), YUV420)) {
                        HAL_LOGE("request one buf failed.");
                        continue;
                    }
                }
                int buff_size =
                    (w / mCaptureThread->mCaptureInitParams.Scalingratio) *
                    (h / mCaptureThread->mCaptureInitParams.Scalingratio) *
                    sizeof(unsigned short);
                mCaptureThread->mOutWeightBuff =
                    (unsigned short *)malloc(buff_size);
                memset(mCaptureThread->mOutWeightBuff, 0, buff_size);
                weight_map = (void *)malloc(w * h / 4);
                memset(weight_map, 0, w * h / 4);
            }
            mCaptureWidth = w;
            mCaptureHeight = h;
            mCaptureThread->mCaptureInitParams.width = w;
            mCaptureThread->mCaptureInitParams.height = h;

            m_pNearYuvBuffer = (void *)malloc(w * h * 3 / 2);
            memset(m_pNearYuvBuffer, 0, w * h * 3 / 2);

            if (mCaptureThread->mIsGalleryBlur &&
                (mCaptureThread->mVersion == 3)) {
                m_pFarYuvBuffer = (void *)malloc(w * h * 3 / 2);
                memset(m_pFarYuvBuffer, 0, w * h * 3 / 2);
            }

            if (mCaptureThread->mFirstCapture) {
                mCaptureThread->mCaptureWeightParams.sel_x =
                    mCaptureThread->mCaptureInitParams.width / 2;
                mCaptureThread->mCaptureWeightParams.sel_y =
                    mCaptureThread->mCaptureInitParams.height / 2;
                mCaptureThread->mCaptureWeightParams.circle_size =
                    mCaptureThread->mCaptureInitParams.height *
                    mCaptureThread->mCircleSizeScale / 100 / 2;
                mCaptureThread->mCapture2WeightParams.sel_x =
                    mCaptureThread->mCaptureInitParams.width / 2;
                mCaptureThread->mCapture2WeightParams.sel_y =
                    mCaptureThread->mCaptureInitParams.height / 2;
            }
            mCaptureThread->mCaptureStreamsNum = stream_list->num_streams;
            mCaptureThread->mMainStreams[stream_list->num_streams].max_buffers =
                1;
            mCaptureThread->mMainStreams[stream_list->num_streams].width = w;
            mCaptureThread->mMainStreams[stream_list->num_streams].height = h;
            mCaptureThread->mMainStreams[stream_list->num_streams].format =
                HAL_PIXEL_FORMAT_YCbCr_420_888;
            mCaptureThread->mMainStreams[stream_list->num_streams].usage =
                stream_list->streams[i]->usage;
            mCaptureThread->mMainStreams[stream_list->num_streams].stream_type =
                CAMERA3_STREAM_OUTPUT;
            mCaptureThread->mMainStreams[stream_list->num_streams].data_space =
                stream_list->streams[i]->data_space;
            mCaptureThread->mMainStreams[stream_list->num_streams].rotation =
                stream_list->streams[i]->rotation;
            pMainStreams[stream_list->num_streams] =
                &mCaptureThread->mMainStreams[stream_list->num_streams];
        }
        mCaptureThread->mMainStreams[i] = *stream_list->streams[i];
        pMainStreams[i] = &mCaptureThread->mMainStreams[i];
    }

    camera3_stream_configuration mainconfig;
    mainconfig = *stream_list;
    mainconfig.num_streams = stream_list->num_streams + 1;
    mainconfig.streams = pMainStreams;

    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                    &mainconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure main streams!!");
        return rc;
    }

    if (mainconfig.num_streams == 3) {
        HAL_LOGD("push back to streamlist");
        memcpy(stream_list->streams[0], &mCaptureThread->mMainStreams[0],
               sizeof(camera3_stream_t));
        memcpy(stream_list->streams[1], &mCaptureThread->mMainStreams[1],
               sizeof(camera3_stream_t));
        stream_list->streams[1]->width = mCaptureWidth;
        stream_list->streams[1]->height = mCaptureHeight;
    }
    for (i = 0; i < stream_list->num_streams; i++) {
        HAL_LOGD("main configurestreams, streamtype:%d, format:%d, width:%d, "
                 "height:%d",
                 stream_list->streams[i]->stream_type,
                 stream_list->streams[i]->format,
                 stream_list->streams[i]->width,
                 stream_list->streams[i]->height);
    }

    HAL_LOGV("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :constructDefaultRequestSettings
 *
 * DESCRIPTION: construct Default Request Settings
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
const camera_metadata_t *SprdCamera3Blur::constructDefaultRequestSettings(
    const struct camera3_device *device, int type) {
    HAL_LOGV("E");
    const camera_metadata_t *fwk_metadata = NULL;

    SprdCamera3HWI *hw = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    Mutex::Autolock l(mLock);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return NULL;
    }

    fwk_metadata = hw->construct_default_request_settings(
        m_pPhyCamera[CAM_TYPE_MAIN].dev, type);
    if (!fwk_metadata) {
        HAL_LOGE("constructDefaultMetadata failed");
        return NULL;
    }
    HAL_LOGV("X");
    return fwk_metadata;
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
void SprdCamera3Blur::saveRequest(camera3_capture_request_t *request) {
    size_t i = 0;
    camera3_stream_t *newStream = NULL;
    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        if (getStreamType(newStream) == CALLBACK_STREAM) {
            request_saved_blur_t currRequest;
            HAL_LOGV("save request %d", request->frame_number);
            Mutex::Autolock l(mRequestLock);
            currRequest.frame_number = request->frame_number;
            currRequest.buffer = request->output_buffers[i].buffer;
            currRequest.stream = request->output_buffers[i].stream;
            currRequest.input_buffer = request->input_buffer;
            mSavedRequestList.push_back(currRequest);
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
int SprdCamera3Blur::processCaptureRequest(const struct camera3_device *device,
                                           camera3_capture_request_t *request) {
    int rc = 0;
    uint32_t i = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    CameraMetadata metaSettings;
    camera3_capture_request_t *req = request;
    camera3_capture_request_t req_main;
    camera3_stream_buffer_t *out_streams_main = NULL;
    uint32_t tagCnt = 0;
    int snap_stream_num = 2;
    int af_bypass = 0;
    int fb_on = 0;

    memset(&req_main, 0x00, sizeof(camera3_capture_request_t));
    rc = validateCaptureRequest(req);
    if (rc != NO_ERROR) {
        return rc;
    }
    metaSettings = request->settings;
    saveRequest(req);

    tagCnt = metaSettings.entryCount();
    if (tagCnt != 0) {
        uint8_t sprdBurstModeEnabled = 0;
        metaSettings.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                            &sprdBurstModeEnabled, 1);
        uint8_t sprdZslEnabled = 1;
        metaSettings.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);
    }
    /* save Perfectskinlevel */
    if (metaSettings.exists(ANDROID_SPRD_UCAM_SKIN_LEVEL)) {
        mBlur->fbLevels.blemishLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[0];
        mBlur->fbLevels.smoothLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[1];
        mBlur->fbLevels.skinColor =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[2];
        mBlur->fbLevels.skinLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[3];
        mBlur->fbLevels.brightLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[4];
        mBlur->fbLevels.lipColor =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[5];
        mBlur->fbLevels.lipLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[6];
        mBlur->fbLevels.slimLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[7];
        mBlur->fbLevels.largeLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[8];
    }
    if (metaSettings.exists(ANDROID_CONTROL_AE_TARGET_FPS_RANGE)) {
        int32_t aeTargetFpsRange[2] = {25, 30};
        metaSettings.update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
                            aeTargetFpsRange, ARRAY_SIZE(aeTargetFpsRange));
    }
    /*config main camera*/
    req_main = *req;
    out_streams_main = (camera3_stream_buffer_t *)malloc(
        sizeof(camera3_stream_buffer_t) * (req_main.num_output_buffers));
    if (!out_streams_main) {
        HAL_LOGE("failed");
        return NO_MEMORY;
    }
    memset(out_streams_main, 0x00,
           (sizeof(camera3_stream_buffer_t)) * (req_main.num_output_buffers));

    for (size_t i = 0; i < req->num_output_buffers; i++) {
        int requestStreamType =
            getStreamType(request->output_buffers[i].stream);
        out_streams_main[i] = req->output_buffers[i];
        HAL_LOGV("num_output_buffers:%d, streamtype:%d",
                 req->num_output_buffers, requestStreamType);
        if (requestStreamType == SNAPSHOT_STREAM) {
            if (mCaptureThread->mVersion == 3) {
                mCaptureThread->getIspAfFullscanInfo();
            }

            mCaptureThread->mSavedResultBuff =
                request->output_buffers[i].buffer;
            mjpegSize = ADP_BUFSIZE(*request->output_buffers[i].buffer);
            memcpy(&mCaptureThread->mSavedCapRequest, req,
                   sizeof(camera3_capture_request_t));
            memcpy(&mCaptureThread->mSavedCapReqstreambuff,
                   &req->output_buffers[i], sizeof(camera3_stream_buffer_t));
            mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1] =
                req->output_buffers[i].stream;
            HAL_LOGD("mFlushing:%d,frame_number:%d", mFlushing,
                     request->frame_number);

            if (!mFlushing && mCoverValue == 1 &&
                !(mCaptureThread->mVersion == 3 &&
                  0 != mCaptureThread->mIspInfo.distance_reminder) &&
                !(!mCaptureThread->mIsBlurAlways &&
                  mCaptureThread->mCaptureWeightParams.total_roi == 0)) {
                if (mCaptureThread->mVersion == 3) {
                    af_bypass = 1;
                    hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_3A_BYPASS,
                                           &af_bypass, NULL);
                    hwiMain->camera_ioctrl(
                        CAMERA_IOCTRL_SET_AF_POS,
                        &mCaptureThread->mIspInfo.near_peak_pos, NULL);
                }
                snap_stream_num = 2;
                out_streams_main[i].buffer = &mLocalCapBuffer[0].native_handle;
                fb_on = 0;
                hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_CAPTURE_FACE_BEAUTIFY,
                                       &fb_on, NULL);
            } else {
                snap_stream_num = 1;
                fb_on = 1;
                out_streams_main[i].buffer = (req->output_buffers[i]).buffer;
                hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_CAPTURE_FACE_BEAUTIFY,
                                       &fb_on, NULL);
            }
            out_streams_main[i].stream =
                &mCaptureThread->mMainStreams[snap_stream_num];

            mReqState = WAIT_FIRST_YUV_STATE;
        } else {
            mSavedReqStreams[mPreviewStreamsNum] =
                req->output_buffers[i].stream;
            out_streams_main[i].stream =
                &mCaptureThread->mMainStreams[mPreviewStreamsNum];
            mCaptureThread->updateBlurWeightParams(metaSettings, 0);
        }
    }
    req_main.output_buffers = out_streams_main;
    req_main.settings = metaSettings.release();
    for (size_t i = 0; i < req->num_output_buffers; i++) {
        if (getStreamType(request->output_buffers[i].stream) ==
            SNAPSHOT_STREAM) {
            if (NULL != mCaptureThread->mSavedCapReqsettings) {
                free_camera_metadata(mCaptureThread->mSavedCapReqsettings);
                mCaptureThread->mSavedCapReqsettings = NULL;
            }
            mCaptureThread->mSavedCapReqsettings =
                clone_camera_metadata(req_main.settings);
        }
    }
    rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                          &req_main);
    if (rc < 0) {
        HAL_LOGE("failed, mReqState:%d  idx:%d", mReqState,
                 req_main.frame_number);
        goto req_fail;
    }

req_fail:
    if (req_main.settings != NULL) {
        free_camera_metadata(
            const_cast<camera_metadata_t *>(req_main.settings));
        req_main.settings = NULL;
    }
    if (req_main.output_buffers != NULL) {
        free((void *)req_main.output_buffers);
        req_main.output_buffers = NULL;
    }

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
void SprdCamera3Blur::notifyMain(const camera3_notify_msg_t *msg) {
    if (msg->type == CAMERA3_MSG_SHUTTER &&
        (msg->message.shutter.frame_number ==
         mCaptureThread->mSavedCapRequest.frame_number) &&
        mCaptureThread->mSavedCapRequest.frame_number != 0) {
        if (mReqState != WAIT_FIRST_YUV_STATE) {
            HAL_LOGD(" hold cap notify");
            return;
        }
        HAL_LOGD(" cap notify");
    }
    mCaptureThread->mCallbackOps->notify(mCaptureThread->mCallbackOps, msg);
}

/*===========================================================================
 * FUNCTION   :processCaptureResultMain
 *
 * DESCRIPTION: process Capture Result from the main hwi
 *
 * PARAMETERS : capture result structure from hwi
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Blur::processCaptureResultMain(
    camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    CameraMetadata metadata;
    metadata = result->result;

    /* Direclty pass preview buffer and meta result for Main camera */
    if (result_buffer == NULL && result->result != NULL) {
        if (result->frame_number ==
                mCaptureThread->mSavedCapRequest.frame_number &&
            0 != result->frame_number) {
            if (mReqState != WAIT_FIRST_YUV_STATE) {
                HAL_LOGD("hold yuv picture call back, framenumber:%d",
                         result->frame_number);
                return;
            } else {
                mCaptureThread->mCallbackOps->process_capture_result(
                    mCaptureThread->mCallbackOps, result);
                return;
            }
        }

        if (mReqState != PREVIEW_REQUEST_STATE) {
            if (mCaptureThread->mVersion == 3 &&
                1 == mCaptureThread->mIspInfo.distance_reminder) {
                mCoverValue = 6;
            }
        } else {
            mCaptureThread->updateBlurWeightParams(metadata, 1);
            if (2 == m_nPhyCameras && cur_frame_number > 2) {
                SprdCamera3HWI *hwiSub = m_pPhyCamera[CAM_TYPE_AUX].hwi;
                SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
                mCoverValue = getCoveredValue(metadata, hwiSub, hwiMain,
                                              m_pPhyCamera[CAM_TYPE_AUX].id);
            } else {
                char prop[PROPERTY_VALUE_MAX] = {
                    0,
                };
                property_get("persist.sys.camera.blur.prop7", prop, "1");
                if (mCaptureThread->mVersion == 3) {
                    mCoverValue = atoi(prop);
                }
            }
        }

        metadata.update(ANDROID_SPRD_BLUR_COVERED, &mCoverValue, 1);
        camera3_capture_result_t new_result = *result;
        new_result.result = metadata.release();
        mCaptureThread->mCallbackOps->process_capture_result(
            mCaptureThread->mCallbackOps, &new_result);
        free_camera_metadata(
            const_cast<camera_metadata_t *>(new_result.result));

        HAL_LOGV("cur_frame_number:%d mCoverValue:%d mReqState:%d",
                 cur_frame_number, mCoverValue, mReqState);
        return;
    }

    if (result_buffer == NULL) {
        HAL_LOGE("result_buffer = result->output_buffers is NULL");
        return;
    }

    int currStreamType = getStreamType(result_buffer->stream);
    if (mReqState != PREVIEW_REQUEST_STATE &&
        currStreamType == DEFAULT_STREAM) {
        HAL_LOGD("framenumber:%d, receive yuv:%d", cur_frame_number, mReqState);
        blur_queue_msg_t capture_msg;
        capture_msg.msg_type = BLUR_MSG_DATA_PROC;
        capture_msg.combo_buff.frame_number = result->frame_number;
        capture_msg.combo_buff.buffer = result->output_buffers->buffer;
        capture_msg.combo_buff.input_buffer = result->input_buffer;
        {
            hwiMain->setMultiCallBackYuvMode(false);
            Mutex::Autolock l(mCaptureThread->mMergequeueMutex);
            mCaptureThread->mCaptureMsgList.push_back(capture_msg);
            mCaptureThread->mMergequeueSignal.signal();
        }
        HAL_LOGI("mVersion %d,mReqState %d", mCaptureThread->mVersion,
                 mBlur->mReqState);
    } else if (mReqState != PREVIEW_REQUEST_STATE &&
               currStreamType == SNAPSHOT_STREAM) {
        camera3_capture_result_t newResult = *result;
        camera3_stream_buffer_t newOutput_buffers = *(result->output_buffers);
        HAL_LOGI("jpeg callback framenumber:%d, mReqState:%d",
                 result->frame_number, mReqState);

        memcpy(mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1],
               result->output_buffers[0].stream, sizeof(camera3_stream_t));
        newOutput_buffers.stream =
            mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1];
        newOutput_buffers.buffer = result->output_buffers->buffer;
        if (mFlushing) {
            newOutput_buffers.status = CAMERA3_BUFFER_STATUS_ERROR;
        }

        newResult.output_buffers = &newOutput_buffers;
        newResult.input_buffer = NULL;
        newResult.result = NULL;
        newResult.partial_result = 0;
        uint32_t jpeg_size = 0;
        uint8_t *jpeg_addr = NULL;
        if (mBlur->map(result->output_buffers->buffer, (void **)(&jpeg_addr)) ==
            NO_ERROR) {
            jpeg_size = getJpegSize(
                jpeg_addr, ADP_BUFSIZE(*result->output_buffers->buffer));
            ADP_FAKESETBUFATTR_CAMERAONLY(
                *result->output_buffers->buffer, mjpegSize,
                ADP_WIDTH(*result->output_buffers->buffer),
                ADP_HEIGHT(*result->output_buffers->buffer));
            mBlur->unmap(result->output_buffers->buffer);
            jpeg_addr = NULL;
        } else {
            HAL_LOGE("map buffer(%p) failed", result->output_buffers->buffer);
        }

#ifdef YUV_CONVERT_TO_JPEG
        if (mCaptureThread->mVersion == 3) {
            if (mNearJpegSize > 0 && mFarJpegSize > 0)
                mCaptureThread->saveCaptureBlurParams(
                    result->output_buffers->buffer, jpeg_size);
        } else {
            if (mNearJpegSize > 0)
                mCaptureThread->saveCaptureBlurParams(
                    result->output_buffers->buffer, jpeg_size);
        }
#else
        mCaptureThread->saveCaptureBlurParams(result->output_buffers->buffer,
                                              jpeg_size);
#endif
        mCaptureThread->mCallbackOps->process_capture_result(
            mCaptureThread->mCallbackOps, &newResult);
        if (mCaptureThread->mVersion == 3 &&
            1 != mCaptureThread->mIspInfo.distance_reminder) {
            int af_bypass = 0;
            hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_3A_BYPASS, &af_bypass,
                                   NULL);
        }
        if (mCaptureThread->mIsBlurAlways && mCaptureThread->mVersion == 1) {
            mCaptureThread->mVersion = 3;
            mCaptureThread->mCaptureWeightParams.roi_type = 0;
            mCaptureThread->mPreviewWeightParams.roi_type = 0;
        }
        mReqState = PREVIEW_REQUEST_STATE;
    } else {
        camera3_capture_result_t newResult;
        camera3_stream_buffer_t newOutput_buffers;
        memset(&newResult, 0, sizeof(camera3_capture_result_t));
        memset(&newOutput_buffers, 0, sizeof(camera3_stream_buffer_t));
        {
            Mutex::Autolock l(mRequestLock);
            List<request_saved_blur_t>::iterator i = mSavedRequestList.begin();
            while (i != mSavedRequestList.end()) {
                if (i->frame_number == result->frame_number) {
                    if (result->output_buffers->status !=
                            CAMERA3_BUFFER_STATUS_ERROR &&
                        mReqState == PREVIEW_REQUEST_STATE &&
                        mCoverValue == 1) {
                        if (!mFlushing &&
                            !(mCameraId == CAM_BLUR_MAIN_ID_2 &&
                              mCaptureThread->mLastFaceNum <= 0 &&
                              mCaptureThread->mSkipFaceNum >= 10)) {
                            mCaptureThread->blurHandle(
                                result->output_buffers->buffer, NULL, NULL);
                        }
                    }
                    memcpy(&newResult, result,
                           sizeof(camera3_capture_result_t));
                    memcpy(&newOutput_buffers, &result->output_buffers[0],
                           sizeof(camera3_stream_buffer_t));
                    newOutput_buffers.stream = i->stream;
                    memcpy(newOutput_buffers.stream,
                           result->output_buffers[0].stream,
                           sizeof(camera3_stream_t));

                    if (mReqState != PREVIEW_REQUEST_STATE)
                        newOutput_buffers.status = CAMERA3_BUFFER_STATUS_ERROR;
                    newResult.output_buffers = &newOutput_buffers;
                    mCaptureThread->mCallbackOps->process_capture_result(
                        mCaptureThread->mCallbackOps, &newResult);
                    mSavedRequestList.erase(i);
                    dumpFps();
                    HAL_LOGV("find preview frame %d", result->frame_number);
                    break;
                }
                i++;
            }
        }
    }

    return;
}

/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Blur::_dump(const struct camera3_device *device, int fd) {
    HAL_LOGI(" E");

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
int SprdCamera3Blur::_flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGI("E flush, mCaptureMsgList.size=%zu, mSavedRequestList.size:%zu",
             mCaptureThread->mCaptureMsgList.size(), mSavedRequestList.size());
    mFlushing = true;

    while (mReqState != PREVIEW_REQUEST_STATE) {
        Mutex::Autolock l(mMergequeueFinishMutex);
        mMergequeueFinishSignal.waitRelative(mMergequeueFinishMutex,
                                             BLUR_THREAD_TIMEOUT);
    }
    HAL_LOGD("wait until mCaptureMsgList.empty");

    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_MAIN].dev);
    if (2 == m_nPhyCameras) {
        SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
        rc = hwiAux->flush(m_pPhyCamera[CAM_TYPE_AUX].dev);
    }
    if (mCaptureThread != NULL) {
        if (mCaptureThread->isRunning()) {
            mCaptureThread->requestExit();
        }
    }
    HAL_LOGI("X");

    return rc;
}
};
