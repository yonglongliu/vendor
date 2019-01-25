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
#define LOG_TAG "Cam3DualTuning"
//#define LOG_NDEBUG 0
#include "SprdDualCamera3Tuning.h"

using namespace android;
namespace sprdcamera {

SprdDualCamera3Tuning *gTuning = NULL;

// Error Check Macros
#define CHECK_CAPTURE()                                                        \
    if (!gTuning) {                                                            \
        HAL_LOGE("Error getting capture ");                                    \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_CAPTURE_ERROR()                                                  \
    if (!gTuning) {                                                            \
        HAL_LOGE("Error getting capture ");                                    \
        return -ENODEV;                                                        \
    }

#define CHECK_HWI_ERROR(hwi)                                                   \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return -ENODEV;                                                        \
    }

camera3_device_ops_t SprdDualCamera3Tuning::mCameraCaptureOps = {
    .initialize = SprdDualCamera3Tuning::initialize,
    .configure_streams = SprdDualCamera3Tuning::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdDualCamera3Tuning::construct_default_request_settings,
    .process_capture_request = SprdDualCamera3Tuning::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdDualCamera3Tuning::dump,
    .flush = SprdDualCamera3Tuning::flush,
    .reserved = {0},
};

camera3_callback_ops SprdDualCamera3Tuning::callback_ops_main = {
    .process_capture_result =
        SprdDualCamera3Tuning::process_capture_result_main,
    .notify = SprdDualCamera3Tuning::notifyMain};

camera3_callback_ops SprdDualCamera3Tuning::callback_ops_aux = {
    .process_capture_result = SprdDualCamera3Tuning::process_capture_result_aux,
    .notify = SprdDualCamera3Tuning::notifyAux};

/*===========================================================================
 * FUNCTION         : SprdDualCamera3Tuning
 *
 * DESCRIPTION     : SprdDualCamera3Tuning Constructor
 *
 * PARAMETERS:
 *   @num_of_cameras  : Number of Physical Cameras on device
 *
 *==========================================================================*/
SprdDualCamera3Tuning::SprdDualCamera3Tuning() {
    HAL_LOGI(" E");
    m_nPhyCameras = 2; // m_nPhyCameras should always be 2 with dual camera mode
    bzero(&m_VirtualCamera, sizeof(sprd_virtual_camera_t));
    m_VirtualCamera.id = (uint8_t)CAM_TMAIN_ID;
    mStaticMetadata = NULL;
    m_pPhyCamera = NULL;
    mCallbackOps = NULL;
    mSaveSnapStream = NULL;
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mPreviewWidth = 0;
    mPreviewHeight = 0;
    mCaptureStreamsNum = 0;
    mPreviewStreamsNum = 0;
    mCapFrameNumber = 0;
    mhasCallbackStream = false;
    mSavedRequestList.clear();

    setupPhysicalCameras();
    mLocalBufferNumber = LOCAL_TPREVIEW_NUM + 1;

    memset(mLocalBuffer, 0, sizeof(new_mem_t) * mLocalBufferNumber);
    memset(mAuxStreams, 0, sizeof(camera3_stream_t) * MAX_NUM_TUNING_STREAMS);
    memset(mMainStreams, 0, sizeof(camera3_stream_t) * MAX_NUM_TUNING_STREAMS);
    memset(&mThumbReq, 0, sizeof(multi_request_saved_t));

    mLocalBufferList.clear();

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : ~SprdDualCamera3Tuning
 *
 * DESCRIPTION     : SprdDualCamera3Tuning Desctructor
 *
 *==========================================================================*/
SprdDualCamera3Tuning::~SprdDualCamera3Tuning() {
    HAL_LOGI("E");
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
void SprdDualCamera3Tuning::getTCamera(SprdDualCamera3Tuning **pTuningCam) {
    *pTuningCam = NULL;
    if (!gTuning) {
        gTuning = new SprdDualCamera3Tuning();
    }
    CHECK_CAPTURE();
    *pTuningCam = gTuning;
    HAL_LOGV("gTuning: %p ", gTuning);

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
int SprdDualCamera3Tuning::get_camera_info(__unused int camera_id,
                                           struct camera_info *info) {

    int rc = NO_ERROR;

    HAL_LOGI("E");
    if (info) {
        rc = gTuning->getCameraInfo(info);
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
int SprdDualCamera3Tuning::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    int rc = NO_ERROR;

    HAL_LOGD("id= %d", atoi(id));
    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    rc = gTuning->cameraDeviceOpen(atoi(id), hw_device);
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
int SprdDualCamera3Tuning::close_camera_device(__unused hw_device_t *hw_dev) {
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }
    return gTuning->closeCameraDevice();
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
int SprdDualCamera3Tuning::closeCameraDevice() {

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

    freeLocalBuffer();
    mSavedRequestList.clear();

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
int SprdDualCamera3Tuning::initialize(
    __unused const struct camera3_device *device,
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;

    HAL_LOGI("E");
    CHECK_CAPTURE_ERROR();

    rc = gTuning->initialize(callback_ops);

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
SprdDualCamera3Tuning::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *rc;

    HAL_LOGI("E");
    if (!gTuning) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }
    rc = gTuning->constructDefaultRequestSettings(device, type);

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
int SprdDualCamera3Tuning::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;

    HAL_LOGI(" E");
    CHECK_CAPTURE_ERROR();

    rc = gTuning->configureStreams(device, stream_list);

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
int SprdDualCamera3Tuning::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;

    HAL_LOGV("idx:%d", request->frame_number);
    CHECK_CAPTURE_ERROR();
    rc = gTuning->processCaptureRequest(device, request);

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdDualCamera3Tuning
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdDualCamera3Tuning::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_CAPTURE();
    gTuning->processCaptureResultMain(result);
}

/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdDualCamera3Tuning
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdDualCamera3Tuning::process_capture_result_aux(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_CAPTURE();
    gTuning->processCaptureResultAux(result);
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
void SprdDualCamera3Tuning::notifyMain(const struct camera3_callback_ops *ops,
                                       const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%d", msg->message.shutter.frame_number);
    gTuning->notifyMain(msg);
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
void SprdDualCamera3Tuning::notifyAux(const struct camera3_callback_ops *ops,
                                      const camera3_notify_msg_t *msg) {
    CHECK_CAPTURE();
    return;
}

/*===========================================================================
 * FUNCTION   :allocatebuf
 *
 * DESCRIPTION: deconstructor of SprdDualCamera3Tuning
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdDualCamera3Tuning::allocateBuff() {
    int rc = 0;
    size_t preview_num = LOCAL_TPREVIEW_NUM;
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
    }
    if (0 > allocateOne(mCaptureWidth, mCaptureHeight,
                        &(mLocalBuffer[preview_num]), YUV420)) {
        HAL_LOGE("request one buf failed.");

        goto mem_fail;
    }

    mLocalBuffer[preview_num].type = SNAPSHOT_MAIN_BUFFER;
    mLocalBufferList.push_back(&(mLocalBuffer[preview_num]));

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
void SprdDualCamera3Tuning::freeLocalBuffer() {
    for (int i = 0; i < mLocalBufferNumber; i++) {
        freeOneBuffer(&mLocalBuffer[i]);
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
int SprdDualCamera3Tuning::cameraDeviceOpen(__unused int camera_id,
                                            struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    uint8_t phyId = 0;

    HAL_LOGI(" E");
    hw_device_t *hw_dev[m_nPhyCameras];
    m_VirtualCamera.id = (uint8_t)CAM_TMAIN_ID;
    // Open all physical cameras
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        phyId = m_pPhyCamera[i].id;
        SprdCamera3HWI *hw = new SprdCamera3HWI((int)phyId);
        if (!hw) {
            HAL_LOGE("Allocation of hardware interface failed");
            return NO_MEMORY;
        }
        hw_dev[i] = NULL;

        hw->setMultiCameraMode(MODE_TUNING);
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
int SprdDualCamera3Tuning::getCameraInfo(struct camera_info *info) {
    int rc = NO_ERROR;
    int camera_id = 0;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };

    HAL_LOGI("E, camera_id = %d", camera_id);
    m_VirtualCamera.id = CAM_TMAIN_ID;
    camera_id = (int)m_VirtualCamera.id;
    SprdCamera3Setting::initDefaultParameters(camera_id);
    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }

    SprdCamera3Setting::getCameraInfo(camera_id, info);

    info->device_version =
        CAMERA_DEVICE_API_VERSION_3_2; // CAMERA_DEVICE_API_VERSION_3_0;
    info->static_camera_characteristics = mStaticMetadata;
    info->conflicting_devices_length = 0;
    HAL_LOGI("X rc=%d", rc);

    return rc;
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
int SprdDualCamera3Tuning::setupPhysicalCameras() {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[m_nPhyCameras];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }
    memset(m_pPhyCamera, 0x00,
           (m_nPhyCameras * sizeof(sprdcamera_physical_descriptor_t)));
    m_pPhyCamera[CAM_TYPE_TMAIN].id = (uint8_t)CAM_TMAIN_ID;
    m_pPhyCamera[CAM_TYPE_TAUX].id = (uint8_t)CAM_TAUX_ID;

    return NO_ERROR;
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
void SprdDualCamera3Tuning::dump(const struct camera3_device *device, int fd) {
    HAL_LOGI("E");

    CHECK_CAPTURE();

    gTuning->_dump(device, fd);

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
int SprdDualCamera3Tuning::flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGI(" E");
    CHECK_CAPTURE_ERROR();

    rc = gTuning->_flush(device);

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
int SprdDualCamera3Tuning::initialize(
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;
    HAL_LOGI("E");
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mPreviewStreamsNum = 0;
    mCaptureStreamsNum = 1;
    mCapFrameNumber = 0;
    mCallbackOps = callback_ops;
    mLocalBufferList.clear();
    mhasCallbackStream = false;
    mSavedRequestList.clear();

    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_TMAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiMain);

    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }

    sprdCam = m_pPhyCamera[CAM_TYPE_TAUX];
    SprdCamera3HWI *hwiAux = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiAux);

    rc = hwiAux->initialize(sprdCam.dev, &callback_ops_aux);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error aux camera while initialize !! ");
        return rc;
    }
    memset(mAuxStreams, 0, sizeof(camera3_stream_t) * MAX_NUM_TUNING_STREAMS);
    memset(mMainStreams, 0, sizeof(camera3_stream_t) * MAX_NUM_TUNING_STREAMS);

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
int SprdDualCamera3Tuning::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGI("E");
    int rc = 0;
    size_t i = 0;
    size_t j = 0;
    camera3_stream_t *pmainStreams[MAX_NUM_TUNING_STREAMS];
    camera3_stream_t *pauxStreams[MAX_NUM_TUNING_STREAMS];
    camera3_stream_t *newStream = NULL;
    camera3_stream_t *previewStream = NULL;
    camera3_stream_t *snapStream = NULL;

    memset(pmainStreams, 0,
           sizeof(camera3_stream_t *) * MAX_NUM_TUNING_STREAMS);
    memset(pauxStreams, 0, sizeof(camera3_stream_t *) * MAX_NUM_TUNING_STREAMS);

    Mutex::Autolock l(mLock);

    for (size_t i = 0; i < stream_list->num_streams; i++) {
        int requestStreamType = getStreamType(stream_list->streams[i]);
        HAL_LOGD("configurestreams, org streamtype:%d",
                 stream_list->streams[i]->stream_type);
        if (requestStreamType == PREVIEW_STREAM) {
            previewStream = stream_list->streams[i];

            mPreviewWidth = stream_list->streams[i]->width;
            mPreviewHeight = stream_list->streams[i]->height;

            mMainStreams[0] = *stream_list->streams[i];
            mAuxStreams[0] = *stream_list->streams[i];
            pmainStreams[0] = &mMainStreams[0];
            pauxStreams[0] = &mAuxStreams[0];

        } else if (requestStreamType == SNAPSHOT_STREAM) {
            snapStream = stream_list->streams[i];
            mCaptureWidth = stream_list->streams[i]->width;
            mCaptureHeight = stream_list->streams[i]->height;
            mMainStreams[1] = *stream_list->streams[i];
            mAuxStreams[1] = *stream_list->streams[i];
            pmainStreams[1] = &mMainStreams[1];
            pauxStreams[1] = &mAuxStreams[1];
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
    mainconfig.num_streams = MAX_NUM_TUNING_STREAMS;
    mainconfig.streams = pmainStreams;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_TMAIN].hwi;

    camera3_stream_configuration auxconfig;
    auxconfig = *stream_list;
    auxconfig.num_streams = MAX_NUM_TUNING_STREAMS;
    auxconfig.streams = pauxStreams;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_TAUX].hwi;

    rc = hwiAux->configure_streams(m_pPhyCamera[CAM_TYPE_TAUX].dev, &auxconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure aux streams!!");
        return rc;
    }

    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_TMAIN].dev,
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
    HAL_LOGE("mLocalBufferList size=%d", mLocalBufferList.size());
    HAL_LOGI("x rc:%d.", rc);
    for (i = 0; i < stream_list->num_streams; i++) {
        HAL_LOGD(
            "main configurestreams, streamtype:%d, format:%d, width:%d, "
            "height:%d %p",
            stream_list->streams[i]->stream_type,
            stream_list->streams[i]->format, stream_list->streams[i]->width,
            stream_list->streams[i]->height, stream_list->streams[i]->priv);
    }

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
const camera_metadata_t *SprdDualCamera3Tuning::constructDefaultRequestSettings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *fwk_metadata = NULL;

    SprdCamera3HWI *hw = m_pPhyCamera[CAM_TYPE_TMAIN].hwi;
    SprdCamera3HWI *hw_depth = m_pPhyCamera[CAM_TYPE_TAUX].hwi;
    Mutex::Autolock l(mLock);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return NULL;
    }

    fwk_metadata = hw_depth->construct_default_request_settings(
        m_pPhyCamera[CAM_TYPE_TAUX].dev, type);
    fwk_metadata = hw->construct_default_request_settings(
        m_pPhyCamera[CAM_TYPE_TMAIN].dev, type);
    if (!fwk_metadata) {
        HAL_LOGE("constructDefaultMetadata failed");
        return NULL;
    }

    HAL_LOGI("X");

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
void SprdDualCamera3Tuning::saveRequest(camera3_capture_request_t *request) {

    size_t i = 0;
    camera3_stream_t *newStream = NULL;

    Mutex::Autolock l(mRequestLock);
    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        if (getStreamType(newStream) == CALLBACK_STREAM) {
            multi_request_saved_t currRequest;
            memset(&currRequest, 0, sizeof(multi_request_saved_t));
            currRequest.buffer = request->output_buffers[i].buffer;
            currRequest.preview_stream = request->output_buffers[i].stream;
            currRequest.frame_number = request->frame_number;

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
int SprdDualCamera3Tuning::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;
    uint32_t i = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_TMAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_TAUX].hwi;
    CameraMetadata metaSettingsMain, metaSettingsAux;
    camera3_capture_request_t *req = request;
    camera3_capture_request_t req_main;
    camera3_capture_request_t req_aux;
    camera3_stream_t *new_stream = NULL;
    buffer_handle_t *new_buffer = NULL;
    camera3_stream_buffer_t out_streams_main[MAX_NUM_TUNING_STREAMS];
    camera3_stream_buffer_t out_streams_aux[MAX_NUM_TUNING_STREAMS];
    bool is_captureing = false;
    uint32_t tagCnt = 0;
    camera3_stream_t *preview_stream = NULL;
    char value[PROPERTY_VALUE_MAX] = {
        0,
    };
    bzero(out_streams_main,
          sizeof(camera3_stream_buffer_t) * MAX_NUM_TUNING_STREAMS);
    bzero(out_streams_aux,
          sizeof(camera3_stream_buffer_t) * MAX_NUM_TUNING_STREAMS);

    rc = validateCaptureRequest(req);
    if (rc != NO_ERROR) {
        return rc;
    }
    HAL_LOGD("frame_number:%d,num_output_buffers=%d", request->frame_number,
             request->num_output_buffers);

    metaSettingsMain = clone_camera_metadata(request->settings);
    metaSettingsAux = clone_camera_metadata(request->settings);

    tagCnt = metaSettingsMain.entryCount();
    if (tagCnt != 0) {
        if (metaSettingsMain.exists(ANDROID_SPRD_BURSTMODE_ENABLED)) {
            uint8_t sprdBurstModeEnabled = 0;
            metaSettingsMain.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                    &sprdBurstModeEnabled, 1);
            metaSettingsAux.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                   &sprdBurstModeEnabled, 1);
        }
    }

    saveRequest(request);
    HAL_LOGE("mLocalBufferList size=%d", mLocalBufferList.size());

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
        HAL_LOGD("num_output_buffers:%d, streamtype:%d",
                 req->num_output_buffers, requestStreamType);

        if (requestStreamType == SNAPSHOT_STREAM) {
            mSaveSnapStream = new_stream;
            mCapFrameNumber = request->frame_number;
            // sencond step:construct callback Request
            out_streams_main[main_buffer_index] = req->output_buffers[i];
            out_streams_main[main_buffer_index].stream = &mMainStreams[1];
            main_buffer_index++;
            out_streams_aux[aux_buffer_index] = req->output_buffers[i];
            out_streams_aux[aux_buffer_index].stream = &mAuxStreams[1];
            out_streams_aux[aux_buffer_index].buffer =
                popBufferList(mLocalBufferList, SNAPSHOT_MAIN_BUFFER);
            aux_buffer_index++;
            HAL_LOGD("snap req.mCapFrameNumber=%d", mCapFrameNumber);

        } else if (requestStreamType == CALLBACK_STREAM) {
            out_streams_main[main_buffer_index] = req->output_buffers[i];
            out_streams_main[main_buffer_index].stream =
                &mMainStreams[mPreviewStreamsNum];
            main_buffer_index++;

            out_streams_aux[aux_buffer_index] = req->output_buffers[i];
            out_streams_aux[aux_buffer_index].stream =
                &mAuxStreams[mPreviewStreamsNum];
            out_streams_aux[aux_buffer_index].buffer =
                popBufferList(mLocalBufferList, PREVIEW_MAIN_BUFFER);

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
    HAL_LOGE("start main, idx:%d", req_main.frame_number);

    rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_TMAIN].dev,
                                          &req_main);
    if (rc < 0) {
        HAL_LOGE("failed, idx:%d", req_main.frame_number);
        goto req_fail;
    }

    HAL_LOGE("start aux, idx:%d", req_main.frame_number);
    rc = hwiAux->process_capture_request(m_pPhyCamera[CAM_TYPE_TAUX].dev,
                                         &req_aux);
    if (rc < 0) {
        HAL_LOGE("failed, idx:%d", req_aux.frame_number);
        goto req_fail;
    }

req_fail:
    free_camera_metadata((camera_metadata_t *)req_main.settings);
    free_camera_metadata((camera_metadata_t *)req_aux.settings);
    HAL_LOGD("rc. %d idx%d", rc, request->frame_number);

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
void SprdDualCamera3Tuning::notifyMain(const camera3_notify_msg_t *msg) {

    mCallbackOps->notify(mCallbackOps, msg);
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
void SprdDualCamera3Tuning::processCaptureResultMain(
    const camera3_capture_result_t *result) {
    uint64_t result_timestamp = 0;
    uint32_t cur_frame_number = result->frame_number;
    HAL_LOGD("main frame_number %d", cur_frame_number);
    camera3_capture_result_t newResult;
    camera3_stream_buffer_t newOutput_buffers;

    memset(&newResult, 0, sizeof(camera3_capture_result_t));
    memset(&newOutput_buffers, 0, sizeof(camera3_stream_buffer_t));
    newResult = *result;

    if (result->result == NULL && result->output_buffers) {

        int currStreamType = getStreamType(result->output_buffers->stream);
        memcpy(&newOutput_buffers, &result->output_buffers[0],
               sizeof(camera3_stream_buffer_t));

        if (currStreamType == SNAPSHOT_STREAM) {
            newOutput_buffers.stream = mSaveSnapStream;
            if (mhasCallbackStream) {
                CallBackSnapResult();
            }
        } else {
            Mutex::Autolock l(mRequestLock);
            List<multi_request_saved_t>::iterator itor =
                mSavedRequestList.begin();
            while (itor != mSavedRequestList.end()) {
                if (itor->frame_number == cur_frame_number) {
                    HAL_LOGD("erase frame_number %u", cur_frame_number);
                    newOutput_buffers.stream = itor->preview_stream;
                    mSavedRequestList.erase(itor);
                    break;
                }
                itor++;
            }
            if (itor == mSavedRequestList.end()) {
                HAL_LOGE("can't find frame in mSavedRequestList %u:",
                         cur_frame_number);
                return;
            }
        }

        newResult.output_buffers = &newOutput_buffers;
    }

    mCallbackOps->process_capture_result(mCallbackOps, &newResult);

    return;
}

/*===========================================================================
 * FUNCTION   :processCaptureResultAux
 *
 * DESCRIPTION: process Capture Result from the aux hwi
 *
 * PARAMETERS : capture result structure from hwi2
 *
 * RETURN     : None
 *==========================================================================*/
void SprdDualCamera3Tuning::processCaptureResultAux(
    const camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;

    HAL_LOGD("aux frame_number %d", cur_frame_number);
    if (result->result == NULL && result->output_buffers) {
        pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                       mLocalBufferNumber, mLocalBufferList);
    }

    HAL_LOGD("aux frame_number x%d", cur_frame_number);

    return;
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
void SprdDualCamera3Tuning::CallBackSnapResult() {

    camera3_capture_result_t result;
    camera3_stream_buffer_t result_buffers;
    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&result_buffers, sizeof(camera3_stream_buffer_t));
    if (mThumbReq.buffer != NULL) {
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
}

/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: deconstructor of SprdDualCamera3Tuning
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdDualCamera3Tuning::_dump(const struct camera3_device *device, int fd) {
    HAL_LOGI("E");

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
int SprdDualCamera3Tuning::_flush(const struct camera3_device *device) {
    int rc = 0;
    HAL_LOGI("E");
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_TAUX].hwi;
    rc = hwiAux->flush(m_pPhyCamera[CAM_TYPE_TAUX].dev);

    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_TMAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_TMAIN].dev);

    HAL_LOGI("X");
    return rc;
}
};
