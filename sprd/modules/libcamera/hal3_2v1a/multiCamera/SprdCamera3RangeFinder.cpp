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

#define LOG_TAG "Cam3RangeFinder"
#include "SprdCamera3RangeFinder.h"

using namespace android;
namespace sprdcamera {

SprdCamera3RangeFinder *gRangeFinder = NULL;

#define IMG_DUMP_DEBUG 0

#define NULL_CHECKER(expr, ret, args...)                                       \
    if ((expr) == NULL) {                                                      \
        HAL_LOGE("failed,NULL pointer detected: " #expr ":" args);             \
        return ret;                                                            \
    }

// Error Check Macros
#define CHECK_FINDER()                                                         \
    if (!gRangeFinder) {                                                       \
        HAL_LOGE("Error getting finder ");                                     \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_RANGEFINDER_ERR()                                                \
    if (!gRangeFinder) {                                                       \
        HAL_LOGE("Error getting finder ");                                     \
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

camera3_device_ops_t SprdCamera3RangeFinder::mCameraFinderOps = {
    .initialize = SprdCamera3RangeFinder::initialize,
    .configure_streams = SprdCamera3RangeFinder::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3RangeFinder::construct_default_request_settings,
    .process_capture_request = SprdCamera3RangeFinder::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3RangeFinder::dump,
    .flush = SprdCamera3RangeFinder::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3RangeFinder::callback_ops_main = {
    .process_capture_result =
        SprdCamera3RangeFinder::process_capture_result_main,
    .notify = SprdCamera3RangeFinder::notifyMain};

camera3_callback_ops SprdCamera3RangeFinder::callback_ops_aux = {
    .process_capture_result =
        SprdCamera3RangeFinder::process_capture_result_aux,
    .notify = SprdCamera3RangeFinder::notifyAux};

/*===========================================================================
 * FUNCTION         : SprdCamera3RangeFinder
 *
 * DESCRIPTION     : SprdCamera3RangeFinder Constructor
 *
 * PARAMETERS:
 *   @num_of_cameras  : Number of Physical Cameras on device
 *
 *==========================================================================*/
SprdCamera3RangeFinder::SprdCamera3RangeFinder() {
    HAL_LOGI(" E");
    m_nPhyCameras = 2; // m_nPhyCameras should always be 2 with dual camera mode

    mStaticMetadata = NULL;
    mSyncThread = new SyncThread();
    mMeasureThread = new MeasureThread();
    mMeasureThread->mMaxLocalBufferNum = MAX_FINDER_QEQUEST_BUF;
    mMeasureThread->mLocalBuffer = NULL;
    setupPhysicalCameras();
    mLastWidth = 0;
    mLastHeight = 0;
    m_VirtualCamera.id = 0; // hardcode main camera id here
    memset(&m_VirtualCamera, 0, sizeof(sprd_virtual_camera_t));
    mDepthWidth = 0;
    mDepthHeight = 0;
    memset(mAuxStreams, 0, sizeof(camera3_stream_t) * 3);
    m_pMainDepthStream = NULL;
    mConfigStreamNum = 0;
    mPreviewStreamsNum = 0;
    mCurrentState = STATE_IDLE;
    mVcmSteps = 0;
    mUwDepth = 0;
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : ~SprdCamera3RangeFinder
 *
 * DESCRIPTION     : SprdCamera3RangeFinder Desctructor
 *
 *==========================================================================*/
SprdCamera3RangeFinder::~SprdCamera3RangeFinder() {
    HAL_LOGI("E");
    mSyncThread = NULL;
    mNotifyListAux.clear();
    mNotifyListMain.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mLastWidth = 0;
    mLastHeight = 0;
    if (m_pPhyCamera) {
        delete[] m_pPhyCamera;
        m_pPhyCamera = NULL;
    }

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : getCameraRangeFinder
 *
 * DESCRIPTION     : Creates Camera RangeFinder if not created
 *
 * PARAMETERS:
 *   @pFinder               : Pointer to retrieve Camera RangeFinder
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera3RangeFinder::getCameraRangeFinder(
    SprdCamera3RangeFinder **pFinder) {
    *pFinder = NULL;
    if (!gRangeFinder) {
        gRangeFinder = new SprdCamera3RangeFinder();
    }
    CHECK_FINDER();
    *pFinder = gRangeFinder;
    HAL_LOGD("gRangeFinder: %p ", gRangeFinder);

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
int SprdCamera3RangeFinder::get_camera_info(__unused int camera_id,
                                            struct camera_info *info) {

    int rc = NO_ERROR;

    HAL_LOGI("E");
    if (info) {
        rc = gRangeFinder->getCameraInfo(info);
    }
    HAL_LOGI("X, rc: %d", rc);

    return rc;
}
/*===========================================================================
 * FUNCTION         : getDepthImageSize
 *
 * DESCRIPTION     : getDepthImageSize
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

void SprdCamera3RangeFinder::getDepthImageSize(int inputWidth, int inputHeight,
                                               int *outWidth, int *outHeight) {
    *outWidth = inputWidth;
    *outHeight = inputHeight;
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
    *outWidth = 320;
    *outHeight = 240;
    HAL_LOGD("iw:%d,ih:%d,ow:%d,oh:%d", inputWidth, inputHeight, *outWidth,
             *outHeight);
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
int SprdCamera3RangeFinder::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {

    int rc = NO_ERROR;

    HAL_LOGD("id= %d", atoi(id));
    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    rc = gRangeFinder->cameraDeviceOpen(atoi(id), hw_device);
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
int SprdCamera3RangeFinder::close_camera_device(__unused hw_device_t *hw_dev) {
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }

    return gRangeFinder->closeCameraDevice();
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
int SprdCamera3RangeFinder::closeCameraDevice() {

    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;

    // Attempt to close all cameras regardless of unbundle results
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        sprdCam = &m_pPhyCamera[i];
        hw_device_t *dev = (hw_device_t *)(sprdCam->dev);
        if (dev == NULL)
            continue;

        HAL_LOGD("camera id:%d, dev addr %p", i, dev);
        rc = SprdCamera3HWI::close_camera_device(dev);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error, camera id:%d", i);
        }
        sprdCam->hwi = NULL;
        sprdCam->dev = NULL;
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
int SprdCamera3RangeFinder::initialize(
    __unused const struct camera3_device *device,
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;

    HAL_LOGI("E");
    CHECK_RANGEFINDER_ERR();
    rc = gRangeFinder->initialize(callback_ops);

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
SprdCamera3RangeFinder::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *rc;

    HAL_LOGI("E");
    if (!gRangeFinder) {
        HAL_LOGE("Error getting finder");
        return NULL;
    }
    rc = gRangeFinder->constructDefaultRequestSettings(device, type);

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
int SprdCamera3RangeFinder::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;

    HAL_LOGI(" E");
    CHECK_RANGEFINDER_ERR();

    rc = gRangeFinder->configureStreams(device, stream_list);

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
int SprdCamera3RangeFinder::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;

    HAL_LOGV("idx:%d", request->frame_number);
    CHECK_RANGEFINDER_ERR();
    rc = gRangeFinder->processCaptureRequest(device, request);

    return rc;
}
/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of process_capture_result_main
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3RangeFinder::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGD("idx:%d", result->frame_number);
    CHECK_FINDER();
    gRangeFinder->processCaptureResultMain((camera3_capture_result_t *)result);
}
/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of process_capture_result_aux
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3RangeFinder::process_capture_result_aux(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGD("idx:%d", result->frame_number);
    CHECK_FINDER();
    gRangeFinder->processCaptureResultAux(result);
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
void SprdCamera3RangeFinder::notifyMain(const struct camera3_callback_ops *ops,
                                        const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%d", msg->message.shutter.frame_number);
    CHECK_FINDER();
    gRangeFinder->notifyMain(msg);
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
void SprdCamera3RangeFinder::notifyAux(const struct camera3_callback_ops *ops,
                                       const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%d", msg->message.shutter.frame_number);
    CHECK_FINDER();
    gRangeFinder->notifyAux(msg);
}

/*===========================================================================
 * FUNCTION   :popRequestList
 *
 * DESCRIPTION: popRequestList
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
buffer_handle_t *
SprdCamera3RangeFinder::popRequestList(List<buffer_handle_t *> &list) {
    buffer_handle_t *ret = NULL;

    if (list.empty()) {
        return NULL;
    }
    List<buffer_handle_t *>::iterator j = list.begin();
    ret = *j;
    list.erase(j);

    return ret;
}
/*===========================================================================
 * FUNCTION   :validateCaptureRequest
 *
 * DESCRIPTION: validate request received from framework
 *
 * PARAMETERS :
 *  @request:   request received from framework
 *
 * RETURN     :
 *  NO_ERROR if the request is normal
 *  BAD_VALUE if the request if improper
 *==========================================================================*/
int SprdCamera3RangeFinder::validateCaptureRequest(
    camera3_capture_request_t *request) {
    size_t idx = 0;
    const camera3_stream_buffer_t *b = NULL;

    /* Sanity check the request */
    if (request == NULL) {
        HAL_LOGE("NULL capture request");
        return BAD_VALUE;
    }

    uint32_t frameNumber = request->frame_number;
    if (request->num_output_buffers < 1 || request->output_buffers == NULL) {
        HAL_LOGE("Request %d: No output buffers provided!", frameNumber);
        return BAD_VALUE;
    }

    if (request->input_buffer != NULL) {
        b = request->input_buffer;
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            HAL_LOGE("Request %d: Buffer %d: Status not OK!", frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            HAL_LOGE("Request %d: Buffer %d: Has a release fence!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
        if (b->buffer == NULL) {
            HAL_LOGE("Request %d: Buffer %d: NULL buffer handle!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
    }

    // Validate all output buffers
    b = request->output_buffers;
    do {
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            HAL_LOGE("Request %d: Buffer %d: Status not OK!", frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            HAL_LOGE("Request %d: Buffer %d: Has a release fence!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
        if (b->buffer == NULL) {
            HAL_LOGE("Request %d: Buffer %d: NULL buffer handle!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
        idx++;
        b = request->output_buffers + idx;
    } while (idx < (size_t)request->num_output_buffers);

    return NO_ERROR;
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
int SprdCamera3RangeFinder::cameraDeviceOpen(__unused int camera_id,
                                             struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    uint32_t phyId = 0;
    char version[256];

    rc = mMeasureThread->depthAlgoSanityCheck();
    if (rc != NO_ERROR) {
        HAL_LOGE("depthAlgoSanityCheck failed");
        return rc;
    }
    mMeasureThread->mDepthEngineApi->alSDE2_VersionInfo_Get(version, 256);
    HAL_LOGE("version:%s", version);
    hw_device_t *hw_dev[m_nPhyCameras];
    // Open all physical cameras
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        phyId = m_pPhyCamera[i].id;
        SprdCamera3HWI *hw = new SprdCamera3HWI((uint32_t)phyId);
        if (!hw) {
            HAL_LOGE("Allocation of hardware interface failed");
            return NO_MEMORY;
        }
        hw_dev[i] = NULL;

        hw->setMultiCameraMode((multiCameraMode)camera_id);
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
    m_VirtualCamera.dev.ops = &mCameraFinderOps;
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
int SprdCamera3RangeFinder::getCameraInfo(struct camera_info *info) {
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
    HAL_LOGI("X");
    return rc;
}
/*===========================================================================
 * FUNCTION         : setupPhysicalCameras
 *
 * DESCRIPTION     : Creates Camera RangeFinder if not created
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3RangeFinder::setupPhysicalCameras() {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[m_nPhyCameras];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }
    memset(m_pPhyCamera, 0x00,
           (m_nPhyCameras * sizeof(sprdcamera_physical_descriptor_t)));
    m_pPhyCamera[RANGE_CAM_TYPE_MAIN].id = RANGE_CAM_MAIN_ID;
    m_pPhyCamera[RANGE_CAM_TYPE_AUX].id = RANGE_CAM_AUX_ID;

    return NO_ERROR;
}
/*===========================================================================
 * FUNCTION   :getStreamType
 *
 * DESCRIPTION: return stream type
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3RangeFinder::getStreamType(camera3_stream_t *new_stream) {
    int stream_type = 0;
    int format = new_stream->format;
    if (new_stream->stream_type == CAMERA3_STREAM_OUTPUT) {
        if (format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
            format = HAL_PIXEL_FORMAT_YCrCb_420_SP;

        switch (format) {
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            if (new_stream->usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
                stream_type = VIDEO_STREAM;
            } else if (new_stream->usage & GRALLOC_USAGE_SW_READ_OFTEN) {
                stream_type = CALLBACK_STREAM;
            } else {
                stream_type = PREVIEW_STREAM;
            }
            break;
        case HAL_PIXEL_FORMAT_YV12:
        case HAL_PIXEL_FORMAT_YCbCr_420_888:
            if (new_stream->usage & GRALLOC_USAGE_SW_READ_OFTEN) {
                stream_type = DEFAULT_STREAM;
            }
            break;
        case HAL_PIXEL_FORMAT_BLOB:
            stream_type = SNAPSHOT_STREAM;
            break;
        default:
            stream_type = DEFAULT_STREAM;
            break;
        }
    } else if (new_stream->stream_type == CAMERA3_STREAM_BIDIRECTIONAL) {
        stream_type = CALLBACK_STREAM;
    }

    return stream_type;
}

/*===========================================================================
 * FUNCTION   :matchTwoFrame
 *
 * DESCRIPTION: matchTwoFrame
 *
 * PARAMETERS : result1: success match  result.
              list_type: main camera or aux camera list
 *              result2: output data,match  result with result1.
 *
 * RETURN     : MATCH_FAILED
 *              MATCH_SUCCESS
 *==========================================================================*/

bool SprdCamera3RangeFinder::matchTwoFrame(hwi_frame_buffer_info_t result1,
                                           List<hwi_frame_buffer_info_t> &list,
                                           hwi_frame_buffer_info_t *result2) {
    List<hwi_frame_buffer_info_t>::iterator itor2;
    if (list.empty()) {
        HAL_LOGV("match failed for idx:%d, unmatched queue is empty",
                 result1.frame_number);
        return MATCH_FAILED;
    } else {
        itor2 = list.begin();
        while (itor2 != list.end()) {
            int64_t tmp = result1.timestamp - itor2->timestamp;
            if (abs((cmr_s32)tmp) <= FINDER_TIME_DIFF) {
                *result2 = *itor2;
                list.erase(itor2);
                return MATCH_SUCCESS;
            }
            itor2++;
        }
        HAL_LOGV("match failed for idx:%d, could not find matched frame",
                 result1.frame_number);
        return MATCH_FAILED;
    }
}

/*===========================================================================
 * FUNCTION   :SyncThread
 *
 * DESCRIPTION: constructor of SyncThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3RangeFinder::SyncThread::SyncThread() {
    HAL_LOGI("E");
    mSyncMsgList.clear();
    mGetNewestFrameForMeasure = false;
    mVFps = 0;
    mVFrameCount = 0;
    mVLastFpsTime = 0;
    mVLastFrameCount = 0;
    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :~~SyncThread
 *
 * DESCRIPTION: deconstructor of SyncThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3RangeFinder::SyncThread::~SyncThread() {
    HAL_LOGI("E");
    mSyncMsgList.clear();
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

bool SprdCamera3RangeFinder::SyncThread::threadLoop() {
    frame_matched_msg_t sync_msg;

    while (!mSyncMsgList.empty()) {
        List<frame_matched_msg_t>::iterator itor1 = mSyncMsgList.begin();
        sync_msg = *itor1;
        mSyncMsgList.erase(itor1);
        switch (sync_msg.msg_type) {
        case MSG_EXIT: {
            return false;
        }
        case MSG_DATA_PROC: {
            if (mGetNewestFrameForMeasure) {
                Mutex::Autolock l(
                    gRangeFinder->mMeasureThread->mMeasureQueueMutex);
                HAL_LOGD("Measurement for frame:%d",
                         sync_msg.combo_frame.frame_number);
                gRangeFinder->mMeasureThread->mMeasureMsgList.push_back(
                    sync_msg);
                gRangeFinder->mMeasureThread->mMeasureQueueSignal.signal();
                mGetNewestFrameForMeasure = false;
            } else {
                gRangeFinder->mMeasureThread->mLocalBufferList.push_back(
                    sync_msg.combo_frame.buffer1);
                gRangeFinder->mMeasureThread->mLocalBufferList.push_back(
                    sync_msg.combo_frame.buffer2);
            }

        } break;
        default:
            break;
        }
    }
    waitMsgAvailable();

    return true;
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
void SprdCamera3RangeFinder::SyncThread::requestExit() {
    frame_matched_msg_t sync_msg;
    sync_msg.msg_type = MSG_EXIT;
    mSyncMsgList.push_back(sync_msg);
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
void SprdCamera3RangeFinder::SyncThread::waitMsgAvailable() {
    // TODO:what to do for timeout
    while (mSyncMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, THREAD_TIMEOUT);
    }
}

/*===========================================================================
 * FUNCTION   :dumpFps
 *
 * DESCRIPTION: dump frame rate
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RangeFinder::SyncThread::dumpFps() {
    mVFrameCount++;
    int64_t now = systemTime();
    int64_t diff = now - mVLastFpsTime;
    if (diff > ms2ns(250)) {
        mVFps =
            (((double)(mVFrameCount - mVLastFrameCount)) * (double)(s2ns(1))) /
            (double)diff;
        HAL_LOGD("[KPI Perf]: PROFILE_3D_VIDEO_FRAMES_PER_SECOND: %.4f ",
                 mVFps);
        mVLastFpsTime = now;
        mVLastFrameCount = mVFrameCount;
    }
}

/*===========================================================================
 * FUNCTION   :MeasureThread
 *
 * DESCRIPTION: MeasureThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3RangeFinder::MeasureThread::MeasureThread() {
    HAL_LOGI("E");
    mCurUwcoods.uwInX1 = 0;
    mCurUwcoods.uwInY1 = 0;
    mCurUwcoods.uwInX2 = 0;
    mCurUwcoods.uwInY2 = 0;
    mIommuEnabled = 0;
    mMeasureMsgList.clear();
    mLocalBufferList.clear();
    mDepthEngineApi = (depth_engine_api_t *)malloc(sizeof(depth_engine_api_t));
    if (mDepthEngineApi == NULL) {
        HAL_LOGE("mDepthEngineApi malloc failed.");
    } else {
        memset(mDepthEngineApi, 0, sizeof(depth_engine_api_t));
        if (loadDepthEngine() < 0) {
            HAL_LOGE("load DepthEngine API failed.");
        }
    }

    mLocalBuffer = NULL;
    mMaxLocalBufferNum = 0;
    memset(mNativeBuffer, 0,
           sizeof(native_handle_t *) * MAX_FINDER_QEQUEST_BUF);
    memset(&mLastPreCoods, 0, sizeof(uw_Coordinate));
    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :~MeasureThread
 *
 * DESCRIPTION: deconstructor of ~MeasureThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3RangeFinder::MeasureThread::~MeasureThread() {
    HAL_LOGI("E");
    mMeasureMsgList.clear();
    mLocalBufferList.clear();
    if (mDepthEngineApi != NULL) {
        free(mDepthEngineApi);
        mDepthEngineApi = NULL;
    }
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :allocateOne
 *
 * DESCRIPTION: deconstructor of allocateOne
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3RangeFinder::MeasureThread::allocateOne(int w, int h,
                                                       uint32_t is_cache,
                                                       new_mem_t *new_mem) {

    int result = 0;
    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;
    private_handle_t *buffer;

    mem_size = w * h * 3 / 2;
    // to make it page size aligned
    //  mem_size = (mem_size + 4095U) & (~4095U);

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

    buffer =
        new private_handle_t(private_handle_t::PRIV_FLAGS_USES_ION, 0x130,
                             mem_size, (unsigned char *)pHeapIon->getBase(), 0);

    buffer->share_fd = pHeapIon->getHeapID();
    buffer->format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
    buffer->byte_stride = w;
    buffer->internal_format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
    buffer->width = w;
    buffer->height = h;
    buffer->stride = w;
    buffer->internalWidth = w;
    buffer->internalHeight = h;

    new_mem->native_handle = buffer;
    new_mem->pHeapIon = pHeapIon;

    HAL_LOGI("X");

    return result;

getpmem_fail:
    delete pHeapIon;

    return -1;
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
void SprdCamera3RangeFinder::MeasureThread::requestExit() {
    Mutex::Autolock l(mMeasureQueueMutex);
    frame_matched_msg_t sync_msg;
    sync_msg.msg_type = MSG_EXIT;
    mMeasureMsgList.push_back(sync_msg);

    mMeasureQueueSignal.signal();
}

/*===========================================================================
 * FUNCTION         : freeLocalBuffer
 *
 * DESCRIPTION     : free new_mem_t buffer
 *
 * PARAMETERS:
 *   @new_mem_t      : Pointer to struct new_mem_t buffer
 *
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera3RangeFinder::MeasureThread::freeLocalBuffer(
    new_mem_t *mLocalBuffer) {

    mLocalBufferList.clear();
    for (size_t i = 0; i < mMaxLocalBufferNum; i++) {
        if (mLocalBuffer[i].native_handle != NULL) {
            delete ((private_handle_t *)*(&(mLocalBuffer[i].native_handle)));
            delete mLocalBuffer[i].pHeapIon;
            mLocalBuffer[i].native_handle = NULL;
        }
    }
    free(mLocalBuffer);
}

/*===========================================================================
 * FUNCTION   :threadLoop
 *
 * DESCRIPTION:
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/

bool SprdCamera3RangeFinder::MeasureThread::threadLoop() {
    int rc = NO_ERROR;
    frame_matched_msg_t sync_msg;

    while (!mMeasureMsgList.empty()) {
        List<frame_matched_msg_t>::iterator itor1 = mMeasureMsgList.begin();
        sync_msg = *itor1;
        mMeasureMsgList.erase(itor1);
        if (sync_msg.msg_type == MSG_EXIT) {
            freeLocalBuffer(mLocalBuffer);
            mMeasureMsgList.clear();
            return false;
        }

        rc = calculateDepthValue(&sync_msg.combo_frame);
        if (rc != NO_ERROR) {
            HAL_LOGE("calculateDepthValue failed, rc=%d", rc);
        }
        gRangeFinder->mMeasureThread->mLocalBufferList.push_back(
            sync_msg.combo_frame.buffer1);
        gRangeFinder->mMeasureThread->mLocalBufferList.push_back(
            sync_msg.combo_frame.buffer2);
    }
    gRangeFinder->mCurrentState = STATE_IDLE;
    waitMsgAvailable();
    return true;
}

/*===========================================================================
 * FUNCTION   :waitMsgAvailable
 *
 * DESCRIPTION: waitMsgAvailable
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/

void SprdCamera3RangeFinder::MeasureThread::waitMsgAvailable() {
    while (mMeasureMsgList.empty()) {
        Mutex::Autolock l(mMeasureQueueMutex);
        mMeasureQueueSignal.wait(mMeasureQueueMutex);
    }
}

/*===========================================================================
 * FUNCTION   :loadDepthEngine
 *
 * DESCRIPTION: loadDepthEngine
 *
 * PARAMETERS : none
 *
 * RETURN     : NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3RangeFinder::MeasureThread::loadDepthEngine() {

    HAL_LOGI(" E");
    mDepthEngineApi->handle = dlopen(DEPTH_ENGINE_PATH, RTLD_NOW);
    if (mDepthEngineApi->handle == NULL) {
        HAL_LOGE("open depth engine API failed.error");
        return -1;
    }

    /* will do sanity check when open camera */
    mDepthEngineApi->alSDE2_VersionInfo_Get =
        (alDE_ERR_CODE (*)(char *, unsigned int))dlsym(
            mDepthEngineApi->handle, "alSDE2_VersionInfo_Get");
    mDepthEngineApi->alSDE2_Init =
        (alDE_ERR_CODE (*)(void *, int, int, int, void *, int))dlsym(
            mDepthEngineApi->handle, "alSDE2_Init");
    mDepthEngineApi->alSDE2_Run =
        (alDE_ERR_CODE (*)(void *, void *, void *, int))dlsym(
            mDepthEngineApi->handle, "alSDE2_Run");
    mDepthEngineApi->alSDE2_Abort =
        (alDE_ERR_CODE (*)(void))dlsym(mDepthEngineApi->handle, "alSDE2_Abort");
    mDepthEngineApi->alSDE2_Close =
        (void (*)(void))dlsym(mDepthEngineApi->handle, "alSDE2_Close");
    mDepthEngineApi->alSDE2_DistanceMeasurement =
        (alDE_ERR_CODE (*)(double *, unsigned short *, int, int, unsigned short,
                           unsigned short, unsigned short, unsigned short,
                           int))dlsym(mDepthEngineApi->handle,
                                      "alSDE2_DistanceMeasurement");
    mDepthEngineApi->alSDE2_HolelessPolish =
        (alDE_ERR_CODE (*)(unsigned short *a_puwInOutDisparity,
                           unsigned short a_uwInImageWidth,
                           unsigned short a_uwInImageHeight))
            dlsym(mDepthEngineApi->handle, "alSDE2_HolelessPolish");

    HAL_LOGI("load depth engine Api succuss.");

    return 0;
}

/*===========================================================================
 * FUNCTION   :depthAlgoSanityCheck
 *
 * DESCRIPTION: depthAlgoSanityCheck
 *
 * PARAMETERS : none
 *
 * RETURN     : NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3RangeFinder::MeasureThread::depthAlgoSanityCheck() {
    NULL_CHECKER(mDepthEngineApi->handle, -EINVAL, "dlopen failed to load %s",
                 DEPTH_ENGINE_PATH);
    NULL_CHECKER(mDepthEngineApi->alSDE2_VersionInfo_Get, -EINVAL,
                 "alSDE2_VersionInfo_Get failed");
    NULL_CHECKER(mDepthEngineApi->alSDE2_Init, -EINVAL, "alSDE2_Init failed");
    NULL_CHECKER(mDepthEngineApi->alSDE2_Run, -EINVAL, "alSDE2_Run failed");
    NULL_CHECKER(mDepthEngineApi->alSDE2_Abort, -EINVAL, "alSDE2_Abort failed");
    NULL_CHECKER(mDepthEngineApi->alSDE2_Close, -EINVAL, "alSDE2_Close failed");

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   :depthEngineInit
 *
 * DESCRIPTION: depthEngineInit
 *
 * PARAMETERS :
 *
 * RETURN     : NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3RangeFinder::MeasureThread::depthEngineInit() {
    int rc = NO_ERROR;
    gRangeFinder->mCurrentState = STATE_IDLE;
    gRangeFinder->mMeasureThread->run(NULL);
    gRangeFinder->mSyncThread->run(NULL);

    return rc;
}
/*===========================================================================
 * FUNCTION   :measurePointChanged
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
bool SprdCamera3RangeFinder::MeasureThread::measurePointChanged(
    camera3_capture_request_t *request) {
    uw_Coordinate Coords;
    CameraMetadata metadata;
    int preview_width = request->output_buffers->stream->width;
    int preview_height = request->output_buffers->stream->height;
    metadata = request->settings;

    if (metadata.exists(ANDROID_SPRD_3D_RANGEFINDER_POINTS)) {
        for (size_t sprd_i = 0; sprd_i < 4; sprd_i++) {
            HAL_LOGD("coords:%d",
                     metadata.find(ANDROID_SPRD_3D_RANGEFINDER_POINTS)
                         .data.i32[sprd_i]);
        }

        Coords.uwInX1 =
            metadata.find(ANDROID_SPRD_3D_RANGEFINDER_POINTS).data.i32[0];
        Coords.uwInY1 =
            metadata.find(ANDROID_SPRD_3D_RANGEFINDER_POINTS).data.i32[1];
        Coords.uwInX2 =
            metadata.find(ANDROID_SPRD_3D_RANGEFINDER_POINTS).data.i32[2];
        Coords.uwInY2 =
            metadata.find(ANDROID_SPRD_3D_RANGEFINDER_POINTS).data.i32[3];

        if ((mLastPreCoods.uwInX1 != Coords.uwInX1) ||
            (mLastPreCoods.uwInY1 != Coords.uwInY1) ||
            (mLastPreCoods.uwInX2 != Coords.uwInX2) ||
            (mLastPreCoods.uwInY2 != Coords.uwInY2)) {
            mLastPreCoods.uwInX1 = Coords.uwInX1;
            mLastPreCoods.uwInY1 = Coords.uwInY1;
            mLastPreCoods.uwInX2 = Coords.uwInX2;
            mLastPreCoods.uwInY2 = Coords.uwInY2;
            convertCoordinate(preview_height, preview_width,
                              gRangeFinder->mDepthHeight,
                              gRangeFinder->mDepthWidth, Coords);
            return true;
        }
    }

    return false;
}

/*===========================================================================
 * FUNCTION   :convertCoordinate
 *
 * DESCRIPTION: convert coordinate value
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RangeFinder::MeasureThread::convertCoordinate(
    int oldWidth, int oldHeight, int newWidth, int newHeight,
    uw_Coordinate oldCoords) {
    if (oldWidth == newWidth && oldHeight == newWidth) {
        mCurUwcoods.uwInX1 = oldCoords.uwInX1;
        mCurUwcoods.uwInY1 = oldCoords.uwInY1;
        mCurUwcoods.uwInX2 = oldCoords.uwInX2;
        mCurUwcoods.uwInY2 = oldCoords.uwInY2;
        return;
    }

    mCurUwcoods.uwInY1 = (float)oldCoords.uwInX1 / (float)oldWidth * newWidth;
    mCurUwcoods.uwInX1 = (float)oldCoords.uwInY1 / (float)oldHeight * newHeight;
    mCurUwcoods.uwInY1 = newWidth - mCurUwcoods.uwInY1;
    mCurUwcoods.uwInY2 = (float)oldCoords.uwInX2 / (float)oldWidth * newWidth;
    mCurUwcoods.uwInX2 = (float)oldCoords.uwInY2 / (float)oldHeight * newHeight;
    mCurUwcoods.uwInY2 = newWidth - mCurUwcoods.uwInY2;

    HAL_LOGD("before.(w=%d.h=%d),after.(w=%d,h=%d)", oldWidth, oldHeight,
             newWidth, newHeight);
    HAL_LOGD("oldcoord:X1:Y1(%d,%d),X2,Y2(%d,%d)", oldCoords.uwInX1,
             oldCoords.uwInY1, oldCoords.uwInX2, oldCoords.uwInY2);
    HAL_LOGD("newcoord:X1:Y1(%d,%d),X2,Y2(%d,%d)", mCurUwcoods.uwInX1,
             mCurUwcoods.uwInY1, mCurUwcoods.uwInX2, mCurUwcoods.uwInY2);
}

/*===========================================================================
 * FUNCTION   :calculateDepthValue
 *
 * DESCRIPTION: calculateDepthValue
 *
 * PARAMETERS :
 *
 * RETURN     : NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3RangeFinder::MeasureThread::calculateDepthValue(
    frame_matched_info_t *combDepthResult) {
    int rc = NO_ERROR;
    buffer_handle_t *input_buf1 = combDepthResult->buffer1;
    buffer_handle_t *input_buf2 = combDepthResult->buffer2;
    double eOutDistance;
    int vcmSteps = 0;
    void *addr;
    int size;
    unsigned short uwInX1 = 0, uwInY1 = 0, uwInX2 = 0, uwInY2 = 0;

    unsigned short *puwDisparityBuf, *disparityRotate;
    unsigned char *mainRotate, *auxRotate;

    if (input_buf1 == NULL || input_buf2 == NULL) {
        HAL_LOGE("Error, null buffer detected! input_buf1:%p input_buf2:%p",
                 input_buf1, input_buf2);
        return BAD_VALUE;
    }
    HAL_LOGD("calculate depth value for frame number:%d",
             combDepthResult->frame_number);

    int dPVImgW = ((private_handle_t *)*(input_buf1))->width;
    int dPVImgH = ((private_handle_t *)*(input_buf1))->height;

    vcmSteps = combDepthResult->vcmSteps;
    void *pucBufMain_YCC420NV21 =
        (void *)((private_handle_t *)*(input_buf1))->base;
    void *pucBufSub_YCC420NV21 =
        (void *)((private_handle_t *)*(input_buf2))->base;

    puwDisparityBuf = new unsigned short[dPVImgW * dPVImgH * 2];
    if (puwDisparityBuf == NULL) {
        rc = NO_MEMORY;
        goto getpmem_fail;
    }

    mainRotate = new unsigned char[dPVImgW * dPVImgH * 3 / 2];
    if (mainRotate == NULL) {
        rc = NO_MEMORY;
        goto MEM_MAIN_FAILED;
    }

    auxRotate = new unsigned char[dPVImgW * dPVImgH * 3 / 2];
    if (auxRotate == NULL) {
        rc = NO_MEMORY;
        goto MEM_AUX_FAILED;
    }

    disparityRotate = new unsigned short[dPVImgW * dPVImgH * 2];
    if (disparityRotate == NULL) {
        rc = NO_MEMORY;
        goto MEM_DISPROT_FAILED;
    }

    uwInX1 = mCurUwcoods.uwInX1;
    uwInY1 = mCurUwcoods.uwInY1;
    uwInX2 = mCurUwcoods.uwInX2;
    uwInY2 = mCurUwcoods.uwInY2;

    HAL_LOGD("uwInX1=%d,uwInY1=%d,uwInX2=%d,uwInY2=%d", uwInX1, uwInY1, uwInX2,
             uwInY2);
    HAL_LOGD("width=%d height=%d pucBufSub_YCC420NV21=%p VCM=%d ", dPVImgW,
             dPVImgH, pucBufSub_YCC420NV21, vcmSteps);

#ifdef CONFIG_DUAL_CAMERA_HORIZONTAL
    rc = mDepthEngineApi->alSDE2_Init(NULL, 0, dPVImgH, dPVImgW, mOtpData,
                                      SPRD_DUAL_OTP_SIZE);
#else
    rc = mDepthEngineApi->alSDE2_Init(NULL, 0, dPVImgW, dPVImgH, mOtpData,
                                      SPRD_DUAL_OTP_SIZE);
#endif
    if (rc != 0) {
        HAL_LOGE("alSDE2_Init failed, rc=%d", rc);
        goto ALSDE_FAILED;
    }

#ifdef CONFIG_DUAL_CAMERA_HORIZONTAL
    rc = gRangeFinder->NV21Rotate90((unsigned char *)mainRotate,
                                    (unsigned char *)pucBufMain_YCC420NV21,
                                    dPVImgW, dPVImgH, 0);
    if (rc != true) {
        HAL_LOGE("main rotate fail  rc = %d", rc);
        goto ALSDE_FAILED;
    }

    rc = gRangeFinder->NV21Rotate90((unsigned char *)auxRotate,
                                    (unsigned char *)pucBufSub_YCC420NV21,
                                    dPVImgW, dPVImgH, 0);
    if (rc != true) {
        HAL_LOGE("aux rotate fail  rc = %d", rc);
        goto ALSDE_FAILED;
    }
#else
    rc = gRangeFinder->NV21Rotate180((unsigned char *)mainRotate,
                                     (unsigned char *)pucBufMain_YCC420NV21,
                                     dPVImgW, dPVImgH, 0);
    if (rc != true) {
        HAL_LOGE("main rotate fail  rc = %d", rc);
        goto ALSDE_FAILED;
    }

    rc = gRangeFinder->NV21Rotate180((unsigned char *)auxRotate,
                                     (unsigned char *)pucBufSub_YCC420NV21,
                                     dPVImgW, dPVImgH, 0);
    if (rc != true) {
        HAL_LOGE("aux rotate fail  rc = %d", rc);
        goto ALSDE_FAILED;
    }
#endif

    rc = mDepthEngineApi->alSDE2_Run((void *)puwDisparityBuf, (void *)auxRotate,
                                     (void *)mainRotate, vcmSteps);
    if (rc != 0) {
        HAL_LOGE("alSDE2_Run failed .rc = %d", rc);
        goto ALSDE_FAILED;
    }

#ifdef CONFIG_DUAL_CAMERA_HORIZONTAL
    rc =
        gRangeFinder->DepthRotateCCW90(disparityRotate, puwDisparityBuf,
                                       dPVImgH, dPVImgW, dPVImgW * dPVImgH * 2);
    if (rc != true) {
        HAL_LOGE("disparityRotate fail rc = %d", rc);
        goto ALSDE_FAILED;
    }
#else
    rc = gRangeFinder->DepthRotateCCW180(disparityRotate, puwDisparityBuf,
                                         dPVImgW, dPVImgH,
                                         dPVImgW * dPVImgH * 2);
    if (rc != true) {
        HAL_LOGE("disparityRotate fail rc = %d", rc);
        goto ALSDE_FAILED;
    }
#endif

    rc = mDepthEngineApi->alSDE2_HolelessPolish(disparityRotate, dPVImgW,
                                                dPVImgH);
    if (rc != 0) {
        HAL_LOGE("alSDE2_HolelessPolish failed .rc = %d", rc);
        goto ALSDE_FAILED;
    }

    if (uwInY1 == UWINY1_MAX) {
        uwInY1 = UWINY1_MAX - 1;
    }
    if (uwInY2 == UWINY2_MAX) {
        uwInY2 = UWINY2_MAX - 1;
    }

#ifdef CONFIG_DUAL_CAMERA_HORIZONTAL
    rc = mDepthEngineApi->alSDE2_DistanceMeasurement(
        &eOutDistance, disparityRotate, dPVImgH, dPVImgW, uwInX1, uwInY1,
        uwInX2, uwInY2, vcmSteps);
#else
    rc = mDepthEngineApi->alSDE2_DistanceMeasurement(
        &eOutDistance, disparityRotate, dPVImgW, dPVImgH, uwInX1, uwInY1,
        uwInX2, uwInY2, vcmSteps);
#endif

    if (rc != 0) {
        HAL_LOGE("alSDE2_DistanceMeasurement failed .rc = %d", rc);
        goto ALSDE_FAILED;
    }

    HAL_LOGD("eOutDistance=%f", eOutDistance);
    if (eOutDistance > 500) {
        eOutDistance = 500;
        HAL_LOGD("distance too large. set to 500");
    }
    {
        Mutex::Autolock l(gRangeFinder->mDepthVauleLock);
        gRangeFinder->mUwDepth = (int64_t)eOutDistance;
    }

#if IMG_DUMP_DEBUG
    addr =
        (void *)((struct private_handle_t *)*(combDepthResult->buffer1))->base;

    size = ((struct private_handle_t *)*(combDepthResult->buffer1))->size;
    gRangeFinder->dumpImg(addr, size, combDepthResult->frame_number, 1);

    addr =
        (void *)((struct private_handle_t *)*(combDepthResult->buffer2))->base;
    size = ((struct private_handle_t *)*(combDepthResult->buffer2))->size;
    gRangeFinder->dumpImg(addr, size, combDepthResult->frame_number, 2);

    addr = (void *)mainRotate;
    size = ((struct private_handle_t *)*(combDepthResult->buffer2))->size;
    gRangeFinder->dumpImg(addr, size, combDepthResult->frame_number, 3);

    addr = (void *)auxRotate;
    size = ((struct private_handle_t *)*(combDepthResult->buffer2))->size;
    gRangeFinder->dumpImg(addr, size, combDepthResult->frame_number, 4);
#endif

ALSDE_FAILED:
    mDepthEngineApi->alSDE2_Close();
    delete[] disparityRotate;
MEM_DISPROT_FAILED:
    delete[] auxRotate;
MEM_AUX_FAILED:
    delete[] mainRotate;
MEM_MAIN_FAILED:
    delete[] puwDisparityBuf;
getpmem_fail:
    return rc;
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
void SprdCamera3RangeFinder::dump(const struct camera3_device *device, int fd) {
    HAL_LOGI("E");
    CHECK_FINDER();

    gRangeFinder->_dump(device, fd);

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
int SprdCamera3RangeFinder::flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGI(" E");
    CHECK_RANGEFINDER_ERR();

    rc = gRangeFinder->_flush(device);

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
int SprdCamera3RangeFinder::initialize(
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t sprdCam =
        m_pPhyCamera[RANGE_CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;

    HAL_LOGI("E");
    CHECK_HWI_ERROR(hwiMain);

    mNotifyListAux.clear();
    mNotifyListMain.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mLastWidth = 0;
    mLastHeight = 0;
    mDepthWidth = 0;
    mDepthWidth = 0;
    mPreviewStreamsNum = 0;
    mConfigStreamNum = 0;
    mVcmSteps = 0;
    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }

    sprdCam = m_pPhyCamera[RANGE_CAM_TYPE_AUX];
    SprdCamera3HWI *hwiAux = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiAux);

    rc = hwiAux->initialize(sprdCam.dev, &callback_ops_aux);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error aux camera while initialize !! ");
        return rc;
    }

    // init buffer_handle_t
    mMeasureThread->mLocalBuffer = (new_mem_t *)malloc(
        sizeof(new_mem_t) * mMeasureThread->mMaxLocalBufferNum);
    if (NULL == mMeasureThread->mLocalBuffer) {
        HAL_LOGE("fatal error! mLocalBuffer pointer is null.");
    }
    memset(mMeasureThread->mLocalBuffer, 0,
           sizeof(new_mem_t) * mMeasureThread->mMaxLocalBufferNum);

    mCallbackOps = callback_ops;

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
int SprdCamera3RangeFinder::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGI("E");

    int rc = NO_ERROR;
    camera3_stream_t *newStream = NULL;
    camera3_stream_t *previewStream = NULL;
    camera3_stream_t *pAuxStreams[MAX_NUM_STREAMS];
    size_t i = 0;
    size_t j = 0;
    int w = 0;
    int h = 0;

    Mutex::Autolock l(mLock1);

    mConfigStreamNum = stream_list->num_streams;
    // allocate mMeasureThread->mMaxLocalBufferNum buf
    for (size_t i = 0; i < stream_list->num_streams; i++) {
        newStream = stream_list->streams[i];
        if (getStreamType(newStream) == PREVIEW_STREAM) {
            previewStream = stream_list->streams[i];
            w = previewStream->width;
            h = previewStream->height;
            if ((mLastWidth == w) && (mLastHeight == h)) {
                continue;
            }
            if ((mLastWidth != w || mLastHeight != h) &&
                (mLastWidth != 0 || mLastHeight != 0)) {
                HAL_LOGE("free Local Buffer");
                mMeasureThread->freeLocalBuffer(mMeasureThread->mLocalBuffer);
            }
            getDepthImageSize(w, h, &mDepthWidth, &mDepthHeight);
            for (size_t j = 0; j < mMeasureThread->mMaxLocalBufferNum;) {
                int tmp = mMeasureThread->allocateOne(
                    mDepthWidth, mDepthHeight, 1,
                    &(mMeasureThread->mLocalBuffer[j]));
                if (tmp < 0) {
                    HAL_LOGE("request one buf failed.");
                    continue;
                }
                mMeasureThread->mLocalBufferList.push_back(
                    &mMeasureThread->mLocalBuffer[j].native_handle);
                j++;
            }
            mLastWidth = w;
            mLastHeight = h;
            mPreviewStreamsNum = i;
        }

        mAuxStreams[i] = *newStream;
        pAuxStreams[i] = &mAuxStreams[i];
    }

    if (stream_list->streams[1] == NULL) {
        HAL_LOGE("stream_list->streams[1] == NULL");
        stream_list->streams[1] =
            (camera3_stream_t *)malloc(sizeof(camera3_stream_t));
    }

    // convert snapshot stream to video stream
    stream_list->streams[1]->max_buffers = 1;
    stream_list->streams[1]->width = mDepthWidth;
    stream_list->streams[1]->height = mDepthHeight;
    stream_list->streams[1]->format = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
    stream_list->streams[1]->usage = GRALLOC_USAGE_HW_VIDEO_ENCODER;
    stream_list->streams[1]->stream_type = CAMERA3_STREAM_OUTPUT;
    stream_list->streams[1]->data_space = HAL_DATASPACE_BT709;
    stream_list->streams[1]->rotation = 0;

    SprdCamera3HWI *hwiMain = m_pPhyCamera[RANGE_CAM_TYPE_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[RANGE_CAM_TYPE_AUX].hwi;

    camera3_stream_configuration config;
    config = *stream_list;
    config.num_streams = 1;
    config.streams = pAuxStreams;

    rc = hwiMain->configure_streams(m_pPhyCamera[RANGE_CAM_TYPE_MAIN].dev,
                                    stream_list);
    if (rc < 0) {
        HAL_LOGE("failed. configure main streams!!");
        return rc;
    }

    m_pMainDepthStream = stream_list->streams[1];

    (config.streams[0])->width = mDepthWidth;
    (config.streams[0])->height = mDepthHeight;
    rc = hwiAux->configure_streams(m_pPhyCamera[RANGE_CAM_TYPE_AUX].dev,
                                   &config);
    if (rc < 0) {
        HAL_LOGE("failed. configure aux streams!!");
        return rc;
    }

    stream_list->streams[1]->width = w;
    stream_list->streams[1]->height = h;

    rc = mMeasureThread->depthEngineInit();
    if (rc != NO_ERROR) {
        HAL_LOGE("depthEngineInit failed!");
        return rc;
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
const camera_metadata_t *
SprdCamera3RangeFinder::constructDefaultRequestSettings(
    const struct camera3_device *device, int type) {
    HAL_LOGI("E");
    const camera_metadata_t *fwk_metadata = NULL;

    SprdCamera3HWI *hw = m_pPhyCamera[RANGE_CAM_TYPE_MAIN].hwi;
    Mutex::Autolock l(mLock1);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return NULL;
    }

    fwk_metadata = hw->construct_default_request_settings(
        m_pPhyCamera[RANGE_CAM_TYPE_MAIN].dev, type);
    if (!fwk_metadata) {
        HAL_LOGE("constructDefaultMetadata failed");
        return NULL;
    }

    CameraMetadata metadata;
    metadata = fwk_metadata;
    if (metadata.exists(ANDROID_SPRD_OTP_DATA)) {
        memcpy(mMeasureThread->mOtpData,
               metadata.find(ANDROID_SPRD_OTP_DATA).data.u8,
               SPRD_DUAL_OTP_SIZE);
        checkOtpInfo();
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
int SprdCamera3RangeFinder::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;
    uint32_t i = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[RANGE_CAM_TYPE_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[RANGE_CAM_TYPE_AUX].hwi;
    camera3_capture_request_t *req = request;
    camera3_capture_request_t req_main;
    camera3_capture_request_t req_aux;
    camera3_stream_t *new_stream = NULL;
    camera3_stream_buffer_t *out_streams_main = NULL;
    camera3_stream_buffer_t *out_streams_aux = NULL;

    if ((mCurrentState == STATE_IDLE) &&
        mMeasureThread->measurePointChanged(request)) {
        mCurrentState = STATE_BUSY;
        mSyncThread->mGetNewestFrameForMeasure = true;
    }

    /*config main camera*/
    req_main = *req;
    req_main.num_output_buffers = 2;
    out_streams_main = (camera3_stream_buffer_t *)malloc(
        sizeof(camera3_stream_buffer_t) * (req_main.num_output_buffers));
    if (!out_streams_main) {
        HAL_LOGE("failed");
        goto mem_fail;
    }
    memset(out_streams_main, 0x00,
           (sizeof(camera3_stream_buffer_t)) * (req_main.num_output_buffers));
    out_streams_main[0] = req->output_buffers[0];

    out_streams_main[1].stream = m_pMainDepthStream;
    out_streams_main[1].stream->width = mDepthWidth;
    out_streams_main[1].stream->height = mDepthHeight;
    out_streams_main[1].buffer =
        popRequestList(mMeasureThread->mLocalBufferList);
    out_streams_main[1].status = CAMERA3_BUFFER_STATUS_OK;
    out_streams_main[1].acquire_fence = -1;
    out_streams_main[1].release_fence = -1;
    if (NULL == out_streams_main[1].buffer) {
        HAL_LOGE("failed, LocalBufferList is empty!");
        goto mem_fail;
    }

    req_main.output_buffers = out_streams_main;
    getStreamType(out_streams_main[0].stream);
    getStreamType(out_streams_main[1].stream);

    /*config aux camera*/
    req_aux = *req;

    out_streams_aux =
        (camera3_stream_buffer_t *)malloc(sizeof(camera3_stream_buffer_t));
    if (!out_streams_aux) {
        HAL_LOGE("failed");
        goto mem_fail;
    }
    memset(out_streams_aux, 0x00,
           (sizeof(camera3_stream_buffer_t)) * (req_aux.num_output_buffers));

    out_streams_aux[0] = req->output_buffers[0];
    out_streams_aux[0].buffer =
        popRequestList(mMeasureThread->mLocalBufferList);
    out_streams_aux[0].stream = &mAuxStreams[mPreviewStreamsNum];
    out_streams_aux[0].acquire_fence = -1;
    if (NULL == out_streams_aux[0].buffer) {
        HAL_LOGE("failed, LocalBufferList is empty!");
        goto mem_fail;
    }

    req_aux.output_buffers = out_streams_aux;
    rc = hwiMain->process_capture_request(m_pPhyCamera[RANGE_CAM_TYPE_MAIN].dev,
                                          &req_main);

    if (rc < 0) {
        HAL_LOGD("request main failed");
        mMeasureThread->mLocalBufferList.push_back(
            (req_main.output_buffers)[0].buffer);
        goto req_fail;
    }
    out_streams_main[1].stream->width = out_streams_main[0].stream->width;
    out_streams_main[1].stream->height = out_streams_main[0].stream->height;

    rc = hwiAux->process_capture_request(m_pPhyCamera[RANGE_CAM_TYPE_AUX].dev,
                                         &req_aux);

    if (rc < 0) {
        mMeasureThread->mLocalBufferList.push_back(
            (req_aux.output_buffers)[0].buffer);
        HAL_LOGE("failed, idx:%d", req_aux.frame_number);
        goto req_fail;
    }

req_fail:

    if (req_aux.output_buffers != NULL) {
        free((void *)req_aux.output_buffers);
        req_aux.output_buffers = NULL;
    }

    if (req_main.output_buffers != NULL) {
        free((void *)req_main.output_buffers);
        req_main.output_buffers = NULL;
    }

    return rc;

mem_fail:
    if (NULL != out_streams_main) {
        free((void *)out_streams_main);
        out_streams_main = NULL;
    }

    if (NULL != out_streams_aux) {
        free((void *)out_streams_aux);
        out_streams_aux = NULL;
    }
    return NO_MEMORY;
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
void SprdCamera3RangeFinder::notifyMain(const camera3_notify_msg_t *msg) {
    {
        Mutex::Autolock l(mNotifyLockMain);
        mNotifyListMain.push_back(*msg);
    }
    mCallbackOps->notify(mCallbackOps, msg);
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
int SprdCamera3RangeFinder::checkOtpInfo() { return 0; }

/*===========================================================================
 * FUNCTION   :processCaptureResultMain
 *
 * DESCRIPTION: process Capture Result from the main hwi
 *
 * PARAMETERS : capture result structure from hwi1
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RangeFinder::processCaptureResultMain(
    camera3_capture_result_t *result) {
    uint64_t result_timestamp = 0;
    uint32_t cur_frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;

    int rc = NO_ERROR;
    int test = 0;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;

    if (result_buffer == NULL) {
        CameraMetadata metadata;
        metadata = result->result;
        {
            Mutex::Autolock l(mVcmLockAux);
            HAL_LOGD("step:%d",
                     metadata.find(ANDROID_SPRD_VCM_STEP).data.i32[0]);
            mVcmSteps = metadata.find(ANDROID_SPRD_VCM_STEP).data.i32[0];
        }

        if (mUwDepth != 0) {

            metadata.update(ANDROID_SPRD_3D_RANGEFINDER_DISTANCE, &mUwDepth, 1);
            camera3_capture_result_t new_result = *result;
            new_result.result = metadata.release();
            mCallbackOps->process_capture_result(mCallbackOps, &new_result);
            {
                Mutex::Autolock l(mDepthVauleLock);
                mUwDepth = 0;
            }
            free_camera_metadata(
                const_cast<camera_metadata_t *>(new_result.result));
            return;
        }

        mCallbackOps->process_capture_result(mCallbackOps, result);
        return;
    }

    if (getStreamType(result->output_buffers->stream) != VIDEO_STREAM) {
        HAL_LOGE("return preview frame to fw");
        mCallbackOps->process_capture_result(mCallbackOps, result);
        return;
    }

    cur_frame_number = result->frame_number;
    {
        Mutex::Autolock l(mNotifyLockMain);
        for (List<camera3_notify_msg_t>::iterator i = mNotifyListMain.begin();
             i != mNotifyListMain.end(); i++) {
            if (i->message.shutter.frame_number == cur_frame_number) {
                if (i->type == CAMERA3_MSG_SHUTTER) {
                    searchnotifyresult = NOTIFY_SUCCESS;
                    result_timestamp = i->message.shutter.timestamp;
                    mNotifyListMain.erase(i);
                } else if (i->type == CAMERA3_MSG_ERROR) {
                    searchnotifyresult = NOTIFY_ERROR;
                    mMeasureThread->mLocalBufferList.push_back(
                        result->output_buffers->buffer);
                    mNotifyListMain.erase(i);
                    return;
                }
            }
        }
    }

    if (searchnotifyresult == NOTIFY_NOT_FOUND) {
        // add new error management which will free local buffer
        HAL_LOGE("found no corresponding notify");
        HAL_LOGE("return when missing notify");
        return;
    }

    /* Process error buffer for Main camera:
     * 1.construct error result and return to FW
     * 2.return local buffer
     */
    if (result_buffer->status == CAMERA3_BUFFER_STATUS_ERROR) {
        HAL_LOGE("Return error buffer:%d caused by error Buffer status",
                 result->frame_number);
        mMeasureThread->mLocalBufferList.push_back(
            result->output_buffers->buffer);
        return;
    }

    hwi_frame_buffer_info_t matched_frame;
    hwi_frame_buffer_info_t cur_frame;
    cur_frame.frame_number = cur_frame_number;
    cur_frame.timestamp = result_timestamp;
    cur_frame.buffer = result->output_buffers->buffer;
    {
        Mutex::Autolock l(mVcmLockAux);
        cur_frame.vcmSteps = mVcmSteps;
    }
    {
        Mutex::Autolock l(mUnmatchedQueueLock);
        if (MATCH_SUCCESS ==
            matchTwoFrame(cur_frame, mUnmatchedFrameListAux, &matched_frame)) {
            frame_matched_msg_t sync_msg;
            sync_msg.msg_type = MSG_DATA_PROC;
            sync_msg.combo_frame.frame_number = cur_frame.frame_number;
            sync_msg.combo_frame.buffer1 = cur_frame.buffer;
            sync_msg.combo_frame.buffer2 = matched_frame.buffer;
            sync_msg.combo_frame.vcmSteps = cur_frame.vcmSteps;
            {
                Mutex::Autolock l(mSyncThread->mMergequeueMutex);
                HAL_LOGD("Enqueue combo frame:%d for frame merge!",
                         sync_msg.combo_frame.frame_number);
                mSyncThread->mSyncMsgList.push_back(sync_msg);
                mSyncThread->mMergequeueSignal.signal();
            }
        } else {
            HAL_LOGE("Enqueue newest unmatched frame:%d for Main camera",
                     cur_frame.frame_number);
            hwi_frame_buffer_info_t *discard_frame =
                pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListMain);
            if (discard_frame != NULL) {
                HAL_LOGE("Discard oldest unmatched frame:%d for Main camera",
                         cur_frame_number);
                mMeasureThread->mLocalBufferList.push_back(
                    discard_frame->buffer);
                delete discard_frame;
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
void SprdCamera3RangeFinder::notifyAux(const camera3_notify_msg_t *msg) {
    Mutex::Autolock l(mNotifyLockAux);
    mNotifyListAux.push_back(*msg);
}
/*===========================================================================
 * FUNCTION   :processCaptureResultAux
 *
 * DESCRIPTION: process Capture Result from the aux hwi
 *
 * PARAMETERS : capture result structure from aux hwi
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RangeFinder::processCaptureResultAux(
    const camera3_capture_result_t *result) {
    uint64_t result_timestamp = 0;
    uint32_t cur_frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;

    /* Direclty pass meta result for Aux camera */
    if (result->num_output_buffers == 0) {
        return;
    }
    cur_frame_number = result->frame_number;
    {
        Mutex::Autolock l(mNotifyLockAux);
        for (List<camera3_notify_msg_t>::iterator i = mNotifyListAux.begin();
             i != mNotifyListAux.end(); i++) {
            if (i->message.shutter.frame_number == cur_frame_number) {
                if (i->type == CAMERA3_MSG_SHUTTER) {
                    searchnotifyresult = NOTIFY_SUCCESS;
                    result_timestamp = i->message.shutter.timestamp;
                    mNotifyListAux.erase(i);
                } else if (i->type == CAMERA3_MSG_ERROR) {
                    HAL_LOGE(
                        "Return local buffer:%d caused by error Notify status",
                        result->frame_number);
                    searchnotifyresult = NOTIFY_ERROR;
                    mMeasureThread->mLocalBufferList.push_back(
                        result->output_buffers->buffer);
                    mNotifyListAux.erase(i);
                    return;
                }
            }
        }
    }

    if (searchnotifyresult == NOTIFY_NOT_FOUND) {
        HAL_LOGE("found no corresponding notify");
        // add new error management
        HAL_LOGE("return when missing notify");
        return;
    }

    /* Process error buffer for Aux camera: just return local buffer*/
    if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
        HAL_LOGE("Return local buffer:%d caused by error Buffer status",
                 result->frame_number);
        mMeasureThread->mLocalBufferList.push_back(
            result->output_buffers->buffer);
        return;
    }

    hwi_frame_buffer_info_t matched_frame;
    hwi_frame_buffer_info_t cur_frame;
    cur_frame.frame_number = cur_frame_number;
    cur_frame.timestamp = result_timestamp;
    cur_frame.buffer = (result->output_buffers)->buffer;
    {
        Mutex::Autolock l(mUnmatchedQueueLock);
        if (MATCH_SUCCESS ==
            matchTwoFrame(cur_frame, mUnmatchedFrameListMain, &matched_frame)) {
            frame_matched_msg_t sync_msg;
            sync_msg.msg_type = MSG_DATA_PROC;
            sync_msg.combo_frame.frame_number = matched_frame.frame_number;
            sync_msg.combo_frame.buffer1 = matched_frame.buffer;
            sync_msg.combo_frame.buffer2 = cur_frame.buffer;
            sync_msg.combo_frame.vcmSteps = matched_frame.vcmSteps;
            {
                Mutex::Autolock l(mSyncThread->mMergequeueMutex);
                HAL_LOGD("Enqueue combo frame:%d for frame merge!",
                         sync_msg.combo_frame.frame_number);
                mSyncThread->mSyncMsgList.push_back(sync_msg);
                mSyncThread->mMergequeueSignal.signal();
            }
        } else {
            HAL_LOGD("Enqueue newest unmatched frame:%d for Aux camera",
                     cur_frame.frame_number);
            hwi_frame_buffer_info_t *discard_frame =
                pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListAux);
            if (discard_frame != NULL) {
                HAL_LOGE("Discard oldest unmatched frame:%d for Aux camera",
                         discard_frame->frame_number);
                mMeasureThread->mLocalBufferList.push_back(
                    discard_frame->buffer);
                delete discard_frame;
            }
        }
    }

    return;
}

/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: deconstructor of SprdCamera3RangeFinder
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RangeFinder::_dump(const struct camera3_device *device,
                                   int fd) {

    HAL_LOGI(" E");

    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: dump image data
 *
 * PARAMETERS : void*
 *                        int
 *                        int
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3RangeFinder::dumpImg(void *addr, int size, int frameId,
                                     int flag) {

    HAL_LOGI(" E");
    char name[128];
    int count;
    snprintf(name, sizeof(name), "/data/misc/media/%d_%d_%d.yuv", size, frameId,
             flag);

    FILE *file_fd = fopen(name, "w");

    if (file_fd == NULL) {
        HAL_LOGE("open yuv file fail!\n");
        goto exit;
    }
    count = fwrite(addr, 1, size, file_fd);
    if (count != size) {
        HAL_LOGE("write dst.yuv fail\n");
    }
    fclose(file_fd);

exit:
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
int SprdCamera3RangeFinder::_flush(const struct camera3_device *device) {
    int rc = 0;

    SprdCamera3HWI *hwiMain = m_pPhyCamera[RANGE_CAM_TYPE_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[RANGE_CAM_TYPE_MAIN].dev);

    SprdCamera3HWI *hwiAux = m_pPhyCamera[RANGE_CAM_TYPE_AUX].hwi;
    rc = hwiAux->flush(m_pPhyCamera[RANGE_CAM_TYPE_AUX].dev);

    if (mSyncThread != NULL) {
        if (mSyncThread->isRunning()) {
            mSyncThread->requestExit();
        }
    }
    if (mMeasureThread != NULL) {
        if (mMeasureThread->isRunning()) {
            mMeasureThread->requestExit();
        }
    }
    return rc;
}
/*===========================================================================
 * FUNCTION   :pushToUnmatchedQueue
 *
 * DESCRIPTION:
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
hwi_frame_buffer_info_t *SprdCamera3RangeFinder::pushToUnmatchedQueue(
    hwi_frame_buffer_info_t new_buffer_info,
    List<hwi_frame_buffer_info_t> &queue) {
    hwi_frame_buffer_info_t *pushout = NULL;
    HAL_LOGD("pushToUnmatchedQueue list size=%d", queue.size());
    if (queue.size() == MAX_UNMATCHED_QUEUE_SIZE) {
        pushout = new hwi_frame_buffer_info_t;
        List<hwi_frame_buffer_info_t>::iterator i = queue.begin();
        *pushout = *i;
        queue.erase(i);
    }
    queue.push_back(new_buffer_info);

    return pushout;
}

bool SprdCamera3RangeFinder::DepthRotateCCW90(uint16_t *a_uwDstBuf,
                                              uint16_t *a_uwSrcBuf,
                                              uint16_t a_uwSrcWidth,
                                              uint16_t a_uwSrcHeight,
                                              uint32_t a_udFileSize) {
    int x, y, nw, nh;
    uint16_t *dst;
    if (!a_uwSrcBuf || !a_uwDstBuf || a_uwSrcWidth <= 0 || a_uwSrcHeight <= 0)
        return false;

    nw = a_uwSrcHeight;
    nh = a_uwSrcWidth;
    for (x = 0; x < nw; x++) {
        dst = a_uwDstBuf + (nh - 1) * nw + x;
        for (y = 0; y < nh; y++, dst -= nw) {
            *dst = *a_uwSrcBuf++;
        }
    }
    return true;
}

bool SprdCamera3RangeFinder::DepthRotateCCW180(uint16_t *a_uwDstBuf,
                                               uint16_t *a_uwSrcBuf,
                                               uint16_t a_uwSrcWidth,
                                               uint16_t a_uwSrcHeight,
                                               uint32_t a_udFileSize) {
    int x, y, nw, nh;
    uint16_t *dst;
    int n = 0;
    if (!a_uwSrcBuf || !a_uwDstBuf || a_uwSrcWidth <= 0 || a_uwSrcHeight <= 0)
        return false;

    for (int j = a_uwSrcHeight - 1; j >= 0; j--) {
        for (int i = a_uwSrcWidth - 1; i >= 0; i--) {
            a_uwDstBuf[n++] = a_uwSrcBuf[a_uwSrcWidth * j + i];
        }
    }

    return true;
}

bool SprdCamera3RangeFinder::NV21Rotate90(uint8_t *a_ucDstBuf,
                                          uint8_t *a_ucSrcBuf,
                                          uint16_t a_uwSrcWidth,
                                          uint16_t a_uwSrcHeight,
                                          uint32_t a_udFileSize) {
    int k, x, y, nw, nh;
    uint8_t *dst;

    nw = a_uwSrcHeight;
    nh = a_uwSrcWidth;
    // rotate Y
    k = 0;
    for (x = nw - 1; x >= 0; x--) {
        dst = a_ucDstBuf + x;
        for (y = 0; y < nh; y++, dst += nw) {
            *dst = a_ucSrcBuf[k++];
        }
    }

    // rotate cbcr
    k = nw * nh * 3 / 2 - 1;
    for (x = a_uwSrcWidth - 1; x > 0; x = x - 2) {
        for (y = 0; y < a_uwSrcHeight / 2; y++) {
            a_ucDstBuf[k] = a_ucSrcBuf[(a_uwSrcWidth * a_uwSrcHeight) +
                                       (y * a_uwSrcWidth) + x];
            k--;
            a_ucDstBuf[k] = a_ucSrcBuf[(a_uwSrcWidth * a_uwSrcHeight) +
                                       (y * a_uwSrcWidth) + (x - 1)];
            k--;
        }
    }

    return true;
}

bool SprdCamera3RangeFinder::NV21Rotate180(uint8_t *a_ucDstBuf,
                                           uint8_t *a_ucSrcBuf,
                                           uint16_t a_uwSrcWidth,
                                           uint16_t a_uwSrcHeight,
                                           uint32_t a_udFileSize) {
    int n = 0;
    int hw = a_uwSrcWidth / 2;
    int hh = a_uwSrcHeight / 2;
    // copy y
    for (int j = a_uwSrcHeight - 1; j >= 0; j--) {
        for (int i = a_uwSrcWidth - 1; i >= 0; i--) {
            a_ucDstBuf[n++] = a_ucSrcBuf[a_uwSrcWidth * j + i];
        }
    }

    // copy uv
    unsigned char *ptemp = a_ucSrcBuf + a_uwSrcWidth * a_uwSrcHeight;
    for (int j = hh - 1; j >= 0; j--) {
        for (int i = a_uwSrcWidth - 1; i >= 0; i--) {
            if (i & 0x01) {
                a_ucDstBuf[n + 1] = ptemp[a_uwSrcWidth * j + i];
            } else {
                a_ucDstBuf[n] = ptemp[a_uwSrcWidth * j + i];
                n += 2;
            }
        }
    }
    return true;
}
};
