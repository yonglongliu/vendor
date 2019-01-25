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
#define LOG_TAG "Cam3PageTurn"
//#define LOG_NDEBUG 0
#include "SprdCamera3PageTurn.h"
#include <ui/Fence.h>

using namespace android;
namespace sprdcamera {

SprdCamera3PageTurn *mPageTurn = NULL;

// Error Check Macros
#define CHECK_MUXER()                                                          \
    if (!mPageTurn) {                                                          \
        HAL_LOGE("Error getting muxer ");                                      \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_MUXER_ERROR()                                                    \
    if (!mPageTurn) {                                                          \
        HAL_LOGE("Error getting muxer ");                                      \
        return -ENODEV;                                                        \
    }

#define CHECK_HWI(hwi)                                                         \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return;                                                                \
    }

#define CHECK_HWI_ERROR(hwi)                                                   \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return -ENODEV;                                                        \
    }

#define CHECK_CAMERA(sprdCam)                                                  \
    if (!sprdCam) {                                                            \
        HAL_LOGE("Error getting physical camera");                             \
        return;                                                                \
    }

#define CHECK_CAMERA_ERROR(sprdCam)                                            \
    if (!sprdCam) {                                                            \
        HAL_LOGE("Error getting physical camera");                             \
        return -ENODEV;                                                        \
    }

camera3_device_ops_t SprdCamera3PageTurn::mCameraMuxerOps = {
    .initialize = SprdCamera3PageTurn::initialize,
    .configure_streams = SprdCamera3PageTurn::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3PageTurn::construct_default_request_settings,
    .process_capture_request = SprdCamera3PageTurn::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3PageTurn::dump,
    .flush = SprdCamera3PageTurn::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3PageTurn::callback_ops_main = {
    .process_capture_result = SprdCamera3PageTurn::process_capture_result_main,
    .notify = SprdCamera3PageTurn::notifyMain};

/*===========================================================================
 * FUNCTION         : SprdCamera3PageTurn
 *
 * DESCRIPTION     : SprdCamera3PageTurn Constructor
 *
 * PARAMETERS:
 *   @num_of_cameras  : Number of Physical Cameras on device
 *
 *==========================================================================*/
SprdCamera3PageTurn::SprdCamera3PageTurn() {
    HAL_LOGI(" E");
    m_nPhyCameras = 1;

    mStaticMetadata = NULL;
    setupPhysicalCameras();
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : ~SprdCamera3PageTurn
 *
 * DESCRIPTION     : SprdCamera3PageTurn Desctructor
 *
 *==========================================================================*/
SprdCamera3PageTurn::~SprdCamera3PageTurn() {
    HAL_LOGI("E");
    if (m_pPhyCamera) {
        delete[] m_pPhyCamera;
        m_pPhyCamera = NULL;
    }
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : getCameraMuxer
 *
 * DESCRIPTION     : Creates Camera Muxer if not created
 *
 * PARAMETERS:
 *   @pMuxer               : Pointer to retrieve Camera Muxer
 *
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera3PageTurn::getCameraMuxer(SprdCamera3PageTurn **pMuxer) {
    *pMuxer = NULL;
    if (!mPageTurn) {
        mPageTurn = new SprdCamera3PageTurn();
    }
    CHECK_MUXER();
    *pMuxer = mPageTurn;
    HAL_LOGD("mPageTurn: %p ", mPageTurn);

    return;
}

/*===========================================================================
 * FUNCTION         : get_camera_info
 *
 * DESCRIPTION     : get logical camera info
 *
 * PARAMETERS:
 *   @camera_id     : Logical Camera ID
 *   @info              : Logical Main Camera Info
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              ENODEV : Camera not found
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3PageTurn::get_camera_info(__unused int camera_id,
                                         struct camera_info *info) {
    int rc = NO_ERROR;

    HAL_LOGI("E");
    if (info) {
        rc = mPageTurn->getCameraInfo(info);
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
int SprdCamera3PageTurn::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    int rc = NO_ERROR;

    HAL_LOGI("id= %d", atoi(id));
    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    rc = mPageTurn->cameraDeviceOpen(atoi(id), hw_device);
    HAL_LOGI("id= %d, rc: %d", atoi(id), rc);

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
int SprdCamera3PageTurn::close_camera_device(__unused hw_device_t *hw_dev) {
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }

    return mPageTurn->closeCameraDevice();
}

/*===========================================================================
 * FUNCTION   : closeCameraDevice
 *
 * DESCRIPTION: Close the camera
 *
 * PARAMETERS :
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3PageTurn::closeCameraDevice() {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;

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
    timer_stop();

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
int SprdCamera3PageTurn::initialize(
    __unused const struct camera3_device *device,
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;

    HAL_LOGI("E");
    CHECK_MUXER_ERROR();

    rc = mPageTurn->initialize(callback_ops);

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
SprdCamera3PageTurn::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *rc;

    HAL_LOGI("E");
    if (!mPageTurn) {
        HAL_LOGE("Error getting muxer ");
        return NULL;
    }
    rc = mPageTurn->constructDefaultRequestSettings(device, type);

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
int SprdCamera3PageTurn::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;

    HAL_LOGI(" E");
    CHECK_MUXER_ERROR();
    rc = mPageTurn->configureStreams(device, stream_list);

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
int SprdCamera3PageTurn::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;

    HAL_LOGV("idx:%d", request->frame_number);
    CHECK_MUXER_ERROR();
    rc = mPageTurn->processCaptureRequest(device, request);

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3PageTurn
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3PageTurn::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_MUXER();
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
void SprdCamera3PageTurn::notifyMain(const struct camera3_callback_ops *ops,
                                     const camera3_notify_msg_t *msg) {
    CHECK_MUXER();
}

/*===========================================================================
 * FUNCTION   : cameraDeviceOpen
 *
 * DESCRIPTION: open a camera device with its ID
 *
 * PARAMETERS :
 *   @camera_id : camera ID
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3PageTurn::cameraDeviceOpen(__unused int camera_id,
                                          struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    uint32_t phyId = 0;

    HAL_LOGI(" E");
    hw_device_t *hw_dev[m_nPhyCameras];
    // Open all physical cameras
    phyId = m_pPhyCamera[0].id;
    SprdCamera3HWI *hw = new SprdCamera3HWI((uint32_t)phyId);
    if (!hw) {
        HAL_LOGE("Allocation of hardware interface failed");
        return NO_MEMORY;
    }
    hw_dev[0] = NULL;

    hw->setMultiCameraMode((multiCameraMode)camera_id);
    rc = hw->openCamera(&hw_dev[0]);
    if (rc != NO_ERROR) {
        HAL_LOGE("failed, camera id:%d", phyId);
        delete hw;
        closeCameraDevice();
        return rc;
    }
    m_pPhyCamera[0].dev = reinterpret_cast<camera3_device_t *>(hw_dev[0]);
    m_pPhyCamera[0].hwi = hw;

    m_VirtualCamera.dev.common.tag = HARDWARE_DEVICE_TAG;
    m_VirtualCamera.dev.common.version = CAMERA_DEVICE_API_VERSION_3_2;
    m_VirtualCamera.dev.common.close = close_camera_device;
    m_VirtualCamera.dev.ops = &mCameraMuxerOps;
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
 *   @info      : ptr to camera info struct
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3PageTurn::getCameraInfo(struct camera_info *info) {
    int rc = NO_ERROR;
    int camera_id = m_VirtualCamera.id;
    HAL_LOGI("E, camera_id = %d", camera_id);

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
    HAL_LOGI("X");

    return rc;
}

/*===========================================================================
 * FUNCTION         : setupPhysicalCameras
 *
 * DESCRIPTION     : Creates Camera Muxer if not created
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3PageTurn::setupPhysicalCameras() {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[m_nPhyCameras];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }
    memset(m_pPhyCamera, 0x00,
           (m_nPhyCameras * sizeof(sprdcamera_physical_descriptor_t)));
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.sys.cam.blur.cov.id", prop, "3");
    if (atoi(prop) == 0) {
        m_pPhyCamera[CAM_TYPE_MAIN].id = CAM_BLUR_AUX_ID;
    } else {
        m_pPhyCamera[CAM_TYPE_MAIN].id = CAM_BLUR_AUX_ID_2;
    }
    m_VirtualCamera.id = m_pPhyCamera[CAM_TYPE_MAIN].id;

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
void SprdCamera3PageTurn::dump(const struct camera3_device *device, int fd) {
    HAL_LOGI("E");
    CHECK_MUXER();

    mPageTurn->_dump(device, fd);

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
int SprdCamera3PageTurn::flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGI(" E");
    CHECK_MUXER_ERROR();

    rc = mPageTurn->_flush(device);

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
int SprdCamera3PageTurn::initialize(
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;
    int on_off = STREAM_ON;

    HAL_LOGI("E");
    CHECK_HWI_ERROR(hwiMain);

    SprdCamera3MultiBase::initialize(MODE_PAGE_TURN);
    hwiMain->camera_ioctrl(CAMERA_IOCTRL_COVERED_SENSOR_STREAM_CTRL, &on_off,
                           NULL);
    mCoveredValue = 0;
    mCallbackOps = callback_ops;
    mStartPreviewTime = systemTime();

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
int SprdCamera3PageTurn::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGI("E");

    int rc = 0;
    int streamNum = stream_list->num_streams;

    for (uint32_t i = 0; i < stream_list->num_streams; i++) {
        (stream_list->streams[i])->max_buffers = 4;
    }
    mOldRequestList.clear();
    struct sigevent se;
    mPageTurnPrvTimerID = NULL;
    if (mPageTurnPrvTimerID == NULL) {
        memset(&se, 0, sizeof(struct sigevent));
        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = NULL;
        se.sigev_notify_function = timer_handler;
        se.sigev_notify_attributes = NULL;

        int status = timer_create(CLOCK_MONOTONIC, &se, &mPageTurnPrvTimerID);
        if (status != 0) {
            HAL_LOGE("time create failed");
            return status;
        }
        HAL_LOGD("time create ok");
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
const camera_metadata_t *SprdCamera3PageTurn::constructDefaultRequestSettings(
    const struct camera3_device *device, int type) {
    HAL_LOGI("E");
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
    HAL_LOGI("X");

    return fwk_metadata;
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
int SprdCamera3PageTurn::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;
    int status = 0;
    HAL_LOGI("E");
    old_request newRequest;
    bzero(&newRequest, sizeof(old_request));
    newRequest.frame_number = request->frame_number;
    newRequest.buffer = request->output_buffers->buffer;
    newRequest.stream = request->output_buffers->stream;

    CameraMetadata metadata;

    metadata = request->settings;
    if (metadata.exists(ANDROID_REQUEST_ID)) {
        mOldRequestId = metadata.find(ANDROID_REQUEST_ID).data.i32[0];
    }
    newRequest.request_id = mOldRequestId;
    HAL_LOGV("frame_number=%u,newRequest.buffer =%p,newRequest.stream=%p",
             newRequest.frame_number, newRequest.buffer, newRequest.stream);
    if (request->num_output_buffers > 1) {
        HAL_LOGD("failed .pageturn feature num_output_buffers should be > "
                 "1,app is error");
    }
    for (size_t i = 0; i < request->num_output_buffers; i++) {
        const camera3_stream_buffer_t &output = request->output_buffers[i];
        sp<Fence> acquireFence = new Fence(output.acquire_fence);

        rc = acquireFence->wait(Fence::TIMEOUT_NEVER);
        if (rc != OK) {
            HAL_LOGE("%s: fence wait failed %d ", __func__, rc);
            return rc;
        }

        acquireFence = NULL;
    }
    {
        Mutex::Autolock l(mLock);
        mOldRequestList.push_back(newRequest);
    }
    if (request->frame_number == 0)
        return rc;

    struct itimerspec ts;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("debug.camera.pageturn.timer", prop, "30");
    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = atoi(prop) * 1000000;
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    status = timer_settime(mPageTurnPrvTimerID, 0, &ts, NULL);
    {
        Mutex::Autolock l(mWaitFrameMutex);
        mWaitFrameSignal.waitRelative(mWaitFrameMutex, 1000e6);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   :convertToRegions
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3PageTurn::convertToRegions(int32_t *rect, int32_t *region,
                                           int weight) {
    region[0] = rect[0];
    region[1] = rect[1];
    region[2] = rect[2];
    region[3] = rect[3];
    if (weight > -1) {
        region[4] = weight;
    }
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
void SprdCamera3PageTurn::processCaptureResultMain() {
    HAL_LOGI("E");
    camera3_capture_result_t result;
    List<old_request>::iterator i;
    camera3_stream_buffer_t output_buffers;
    uint32_t cur_frame_number = 0;
    buffer_handle_t *cur_buffer = NULL;
    camera3_stream_t *cur_stream = NULL;

    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&output_buffers, sizeof(camera3_stream_buffer_t));
    {
        Mutex::Autolock l(mLock);
        i = mOldRequestList.begin();
        if (i != mOldRequestList.end()) {
            cur_frame_number = i->frame_number;
            cur_stream = i->stream;
            cur_buffer = i->buffer;
            mOldRequestList.erase(i);
        }
    }

    HAL_LOGD("cur_frame_number=%u,cur_stream=%p,cur_buffer=%p",
             cur_frame_number, cur_stream, cur_buffer);
    // send notify

    camera3_notify_msg_t notify_msg;
    bzero(&notify_msg, sizeof(camera3_notify_msg_t));
    int64_t capture_time = systemTime();
    notify_msg.type = CAMERA3_MSG_SHUTTER;
    notify_msg.message.shutter.frame_number = cur_frame_number;
    notify_msg.message.shutter.timestamp = capture_time;
    mCallbackOps->notify(mCallbackOps, &notify_msg);
    // send meta
    CameraMetadata metadata;
    mCoveredValue = getCoveredValue(metadata, m_pPhyCamera[CAM_TYPE_MAIN].hwi,
                                    NULL, m_pPhyCamera[CAM_TYPE_MAIN].id);
    if (ns2ms(systemTime() - mStartPreviewTime) > 1000) {
        metadata.update(ANDROID_SPRD_BLUR_COVERED, &mCoveredValue, 1);
    }
    metadata.update(ANDROID_SENSOR_TIMESTAMP, &(capture_time), 1);
    metadata.update(ANDROID_REQUEST_ID, &(mOldRequestId), 1);
    uint8_t capture_intet = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
    metadata.update(ANDROID_CONTROL_CAPTURE_INTENT, &(capture_intet), 1);

    result.result = metadata.release();
    result.frame_number = cur_frame_number;
    result.input_buffer = NULL;
    result.num_output_buffers = 0;
    result.partial_result = 1;
    mCallbackOps->process_capture_result(mCallbackOps, &result);

    free_camera_metadata((camera_metadata_t *)result.result);
    // send buffer
    bzero(&result, sizeof(camera3_capture_result_t));
    output_buffers.stream = cur_stream;
    output_buffers.buffer = cur_buffer;
    output_buffers.status = CAMERA3_BUFFER_STATUS_OK;
    output_buffers.acquire_fence = -1;
    output_buffers.release_fence = -1;

    result.frame_number = cur_frame_number;
    result.output_buffers = &output_buffers;
    result.result = NULL;
    result.num_output_buffers = 1;
    result.input_buffer = NULL;
    result.partial_result = 0;

    mCallbackOps->process_capture_result(mCallbackOps, &result);
    mWaitFrameSignal.signal();
}

/*===========================================================================
 * FUNCTION   :timer_stop
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3PageTurn::timer_stop() {
    HAL_LOGI("E");

    if (mPageTurnPrvTimerID) {
        timer_delete(mPageTurnPrvTimerID);
        mPageTurnPrvTimerID = NULL;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   :timer_handler
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/

void SprdCamera3PageTurn::timer_handler(union sigval arg) {
    HAL_LOGI("E");
    mPageTurn->processCaptureResultMain();
    HAL_LOGI("X");
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
void SprdCamera3PageTurn::_dump(const struct camera3_device *device, int fd) {
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
int SprdCamera3PageTurn::_flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGI("E");

    return rc;
}
};
