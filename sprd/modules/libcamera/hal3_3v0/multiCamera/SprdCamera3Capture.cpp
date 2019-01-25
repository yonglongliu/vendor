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
#define LOG_TAG "SprdCamera3Capture"
//#define LOG_NDEBUG 0
#include "SprdCamera3Capture.h"

using namespace android;
namespace sprdcamera {

SprdCamera3Capture *mCapture = NULL;

// Error Check Macros
#define CHECK_CAPTURE()                                                        \
    if (!mCapture) {                                                           \
        HAL_LOGE("Error getting capture ");                                    \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_CAPTURE_ERROR()                                                  \
    if (!mCapture) {                                                           \
        HAL_LOGE("Error getting capture ");                                    \
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

camera3_device_ops_t SprdCamera3Capture::mCameraCaptureOps = {
    .initialize = SprdCamera3Capture::initialize,
    .configure_streams = SprdCamera3Capture::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3Capture::construct_default_request_settings,
    .process_capture_request = SprdCamera3Capture::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3Capture::dump,
    .flush = SprdCamera3Capture::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3Capture::callback_ops_main = {
    .process_capture_result = SprdCamera3Capture::process_capture_result_main,
    .notify = SprdCamera3Capture::notifyMain};

camera3_callback_ops SprdCamera3Capture::callback_ops_aux = {
    .process_capture_result = SprdCamera3Capture::process_capture_result_aux,
    .notify = SprdCamera3Capture::notifyAux};

/*===========================================================================
 * FUNCTION         : SprdCamera3Capture
 *
 * DESCRIPTION     : SprdCamera3Capture Constructor
 *
 * PARAMETERS:
 *   @num_of_cameras  : Number of Physical Cameras on device
 *
 *==========================================================================*/
SprdCamera3Capture::SprdCamera3Capture() {
    HAL_LOGD(" E");
    m_nPhyCameras = 2; // m_nPhyCameras should always be 2 with dual camera mode
    m_VirtualCamera.id = 1; // hardcode left front camera here
    mStaticMetadata = NULL;
    mCaptureThread = new CaptureThread();
    mIommuEnabled = 0;
    mLocalBuffer = NULL;
    mLocalCapBuffer = NULL;
    mLocalBufferList.clear();
    mSavedRequestList.clear();
    setupPhysicalCameras();
    mLastWidth = 0;
    mLastHeight = 0;
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mIsCapturing = false;
    HAL_LOGD("X");
}

/*===========================================================================
 * FUNCTION         : ~SprdCamera3Capture
 *
 * DESCRIPTION     : SprdCamera3Capture Desctructor
 *
 *==========================================================================*/
SprdCamera3Capture::~SprdCamera3Capture() {
    HAL_LOGD("E");
    mCaptureThread = NULL;
    if (m_pPhyCamera) {
        delete[] m_pPhyCamera;
        m_pPhyCamera = NULL;
    }
    HAL_LOGD("X");
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
void SprdCamera3Capture::getCameraCapture(SprdCamera3Capture **pCapture) {

    *pCapture = NULL;
    if (!mCapture) {
        mCapture = new SprdCamera3Capture();
    }
    CHECK_CAPTURE();
    *pCapture = mCapture;
    HAL_LOGV("mCapture: %p ", mCapture);

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
int SprdCamera3Capture::get_camera_info(__unused int camera_id,
                                        struct camera_info *info) {

    int rc = NO_ERROR;

    HAL_LOGV("E");
    if (info) {
        rc = mCapture->getCameraInfo(info);
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
int SprdCamera3Capture::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {

    int rc = NO_ERROR;

    HAL_LOGV("id= %d", atoi(id));
    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    rc = mCapture->cameraDeviceOpen(atoi(id), hw_device);
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
int SprdCamera3Capture::close_camera_device(__unused hw_device_t *hw_dev) {
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }
    return mCapture->closeCameraDevice();
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
int SprdCamera3Capture::closeCameraDevice() {

    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;
    HAL_LOGD("E");

    // Attempt to close all cameras regardless of unbundle results
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        sprdCam = &m_pPhyCamera[i];
        hw_device_t *dev = (hw_device_t *)(sprdCam->dev);
        if (dev == NULL)
            continue;

        HAL_LOGW("camera id:%d", i);
        rc = SprdCamera3HWI::close_camera_device(dev);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error, camera id:%d", i);
        }
        sprdCam->hwi = NULL;
        sprdCam->dev = NULL;
    }
    mLocalBufferList.clear();
    freeLocalBuffer(mLocalBuffer);
    free(mLocalBuffer);
    mLocalBuffer = NULL;
    freeLocalCapBuffer(mLocalCapBuffer);
    free(mLocalCapBuffer);
    mLocalCapBuffer = NULL;

    mSavedRequestList.clear();

    if (mCaptureThread->mGpuApi != NULL) {
        mCaptureThread->unLoadGpuApi();
        free(mCaptureThread->mGpuApi);
        mCaptureThread->mGpuApi = NULL;
    }
    HAL_LOGD("X, rc: %d", rc);

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
int SprdCamera3Capture::initialize(__unused const struct camera3_device *device,
                                   const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;

    HAL_LOGV("E");
    CHECK_CAPTURE_ERROR();

    rc = mCapture->initialize(callback_ops);

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
const camera_metadata_t *SprdCamera3Capture::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *rc;

    HAL_LOGV("E");
    if (!mCapture) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }
    rc = mCapture->constructDefaultRequestSettings(device, type);

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
int SprdCamera3Capture::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;

    HAL_LOGV(" E");
    CHECK_CAPTURE_ERROR();

    rc = mCapture->configureStreams(device, stream_list);

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
int SprdCamera3Capture::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;

    HAL_LOGV("idx:%d", request->frame_number);
    CHECK_CAPTURE_ERROR();
    rc = mCapture->processCaptureRequest(device, request);

    return rc;
}
/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3Capture
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Capture::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_CAPTURE();
    mCapture->processCaptureResultMain(result);
}
/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3Capture
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Capture::process_capture_result_aux(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_CAPTURE();
    mCapture->processCaptureResultAux(result);
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
void SprdCamera3Capture::notifyMain(const struct camera3_callback_ops *ops,
                                    const camera3_notify_msg_t *msg) {
    HAL_LOGD("idx:%d", msg->message.shutter.frame_number);
    CHECK_CAPTURE();
    mCapture->notifyMain(msg);
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
void SprdCamera3Capture::notifyAux(const struct camera3_callback_ops *ops,
                                   const camera3_notify_msg_t *msg) {
    HAL_LOGD("idx:%d", msg->message.shutter.frame_number);
    CHECK_CAPTURE();
    mCapture->notifyAux(msg);
}

/*===========================================================================
 * FUNCTION   :popRequestList
 *
 * DESCRIPTION: deconstructor of SprdCamera3Capture
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
buffer_handle_t *
SprdCamera3Capture::popRequestList(List<buffer_handle_t *> &list) {
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
 * FUNCTION   :allocateOne
 *
 * DESCRIPTION: deconstructor of SprdCamera3Capture
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3Capture::allocateOne(int w, int h, uint32_t is_cache,
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

    if (new_mem == NULL) {
        HAL_LOGE("error new_mem is null.");
        goto getpmem_fail;
    }

    buffer =
        new private_handle_t(private_handle_t::PRIV_FLAGS_USES_ION, 0x130,
                             mem_size, (unsigned char *)pHeapIon->getBase(), 0);
    if (buffer == NULL) {
        HAL_LOGE("error buffer is null.");
        goto getpmem_fail;
    }

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

    HAL_LOGD("fd=0x%x, size=%zu, heap=%p", buffer->share_fd, mem_size,
             pHeapIon);
    HAL_LOGV("X");

    return result;

getpmem_fail:
    delete pHeapIon;

    return -1;
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
void SprdCamera3Capture::freeLocalBuffer(new_mem_t *pLocalBuffer) {
    for (size_t i = 0; i < MAX_QEQUEST_BUF; i++) {
        if (pLocalBuffer[i].native_handle != NULL) {
            delete ((private_handle_t *)*(&(pLocalBuffer[i].native_handle)));
            pLocalBuffer[i].native_handle = NULL;
        }
        if (pLocalBuffer[i].pHeapIon != NULL) {
            HAL_LOGD("fd=0x%x, heap=%p", pLocalBuffer[i].pHeapIon->getHeapID(),
                     pLocalBuffer[i].pHeapIon);
            delete pLocalBuffer[i].pHeapIon;
            pLocalBuffer[i].pHeapIon = NULL;
        }
    }
}
/*===========================================================================
 * FUNCTION         : freeLocalCapBuffer
 *
 * DESCRIPTION     : free new_mem_t buffer
 *
 * PARAMETERS:
 *   @new_mem_t      : Pointer to struct new_mem_t buffer
 *
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera3Capture::freeLocalCapBuffer(new_mem_t *pLocalCapBuffer) {
    for (size_t i = 0; i < LOCAL_CAPBUFF_NUM; i++) {
        if (pLocalCapBuffer[i].native_handle != NULL) {
            delete ((private_handle_t *)*(&(pLocalCapBuffer[i].native_handle)));
            pLocalCapBuffer[i].native_handle = NULL;
        }
        if (pLocalCapBuffer[i].pHeapIon != NULL) {
            HAL_LOGD("fd=0x%x, heap=%p",
                     pLocalCapBuffer[i].pHeapIon->getHeapID(),
                     pLocalCapBuffer[i].pHeapIon);
            delete pLocalCapBuffer[i].pHeapIon;
            pLocalCapBuffer[i].pHeapIon = NULL;
        }
    }
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
int SprdCamera3Capture::validateCaptureRequest(
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
            HAL_LOGE("Request %d: Buffer %zu: Status not OK!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            HAL_LOGE("Request %d: Buffer %zu: Has a release fence!",
                     frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->buffer == NULL) {
            HAL_LOGE("Request %d: Buffer %zu: NULL buffer handle!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
    }

    // Validate all output buffers
    b = request->output_buffers;
    do {
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            HAL_LOGE("Request %d: Buffer %zu: Status not OK!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            HAL_LOGE("Request %d: Buffer %zu: Has a release fence!",
                     frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->buffer == NULL) {
            HAL_LOGE("Request %d: Buffer %zu: NULL buffer handle!", frameNumber,
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
int SprdCamera3Capture::cameraDeviceOpen(__unused int camera_id,
                                         struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    uint32_t phyId = 0;

    HAL_LOGD(" E");
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
    m_VirtualCamera.dev.ops = &mCameraCaptureOps;
    m_VirtualCamera.dev.priv = (void *)&m_VirtualCamera;
    *hw_device = &m_VirtualCamera.dev.common;

    mCaptureThread->mGpuApi = (GPUAPI_t *)malloc(sizeof(GPUAPI_t));
    if (mCaptureThread->mGpuApi == NULL) {
        HAL_LOGE("mGpuApi malloc failed.");
        closeCameraDevice();
        return NO_MEMORY;
    }
    memset(mCaptureThread->mGpuApi, 0, sizeof(GPUAPI_t));

    if (mCaptureThread->loadGpuApi() < 0) {
        HAL_LOGE("load gpu api failed.");
        closeCameraDevice();
        return UNKNOWN_ERROR;
    }

    HAL_LOGD("X");
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
int SprdCamera3Capture::getCameraInfo(struct camera_info *info) {
    int rc = NO_ERROR;
    int camera_id = m_VirtualCamera.id;
    HAL_LOGV("E, camera_id = %d", camera_id);

    SprdCamera3Setting::initDefaultParameters(camera_id);

    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }

    SprdCamera3Setting::getCameraInfo(camera_id, info);

    info->device_version =
        CAMERA_DEVICE_API_VERSION_3_2; // CAMERA_DEVICE_API_VERSION_3_0;
    info->static_camera_characteristics = mStaticMetadata;
    HAL_LOGV("X");
    return rc;
}
/*===========================================================================
 * FUNCTION         : get3DCaptureSize
 *
 * DESCRIPTION     : get3DCaptureSize
 *
 *==========================================================================*/
void SprdCamera3Capture::get3DCaptureSize(int *pWidth, int *pHeight) {
    SPRD_DEF_Tag sprddefInfo;
    if (NULL == pWidth || NULL == pHeight || NULL == m_pPhyCamera) {
        HAL_LOGE("parameter error!");
        return;
    }
    m_pPhyCamera[CAM_TYPE_MAIN].hwi->mSetting->getSPRDDEFTag(&sprddefInfo);
    HAL_LOGD("sensor full size, w:%d, h:%d",
             sprddefInfo.sprd_3dcalibration_cap_size[0],
             sprddefInfo.sprd_3dcalibration_cap_size[1]);
    *pWidth = sprddefInfo.sprd_3dcalibration_cap_size[0] >> 1;
    *pHeight = sprddefInfo.sprd_3dcalibration_cap_size[1] >> 1;
    HAL_LOGD("3d capture size, w:%d, h:%d", *pWidth, *pHeight);
}
/*===========================================================================
 * FUNCTION         : set3DCaptureMode
 *
 * DESCRIPTION     : set3DCaptureMode
 *
 *==========================================================================*/
void SprdCamera3Capture::set3DCaptureMode() {
    SPRD_DEF_Tag sprddefInfo;
    if (NULL == m_pPhyCamera) {
        HAL_LOGE("parameter error!");
        return;
    }
    m_pPhyCamera[CAM_TYPE_MAIN].hwi->mSetting->getSPRDDEFTag(&sprddefInfo);
    sprddefInfo.sprd_3dcapture_enabled = true;
    m_pPhyCamera[CAM_TYPE_MAIN].hwi->mSetting->setSPRDDEFTag(sprddefInfo);

    m_pPhyCamera[CAM_TYPE_AUX].hwi->mSetting->getSPRDDEFTag(&sprddefInfo);
    sprddefInfo.sprd_3dcapture_enabled = true;
    m_pPhyCamera[CAM_TYPE_AUX].hwi->mSetting->setSPRDDEFTag(sprddefInfo);
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
int SprdCamera3Capture::setupPhysicalCameras() {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[m_nPhyCameras];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }
    memset(m_pPhyCamera, 0x00,
           (m_nPhyCameras * sizeof(sprdcamera_physical_descriptor_t)));
    m_pPhyCamera[CAM_TYPE_MAIN].id = CAM_MAIN_ID;
    m_pPhyCamera[CAM_TYPE_AUX].id = CAM_AUX_ID;

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
SprdCamera3Capture::CaptureThread::CaptureThread() {
    HAL_LOGD(" E");
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
SprdCamera3Capture::CaptureThread::~CaptureThread() {
    HAL_LOGD(" E");
    mCaptureMsgList.clear();
}

/*===========================================================================
 * FUNCTION   :cap_3d_convert_face_info_from_preview2cap
 *
 * DESCRIPTION:
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
#ifdef CONFIG_FACE_BEAUTY
void SprdCamera3Capture::CaptureThread::
    cap_3d_convert_face_info_from_preview2cap(int *ptr_cam_face_inf, int width,
                                              int height) {
    float zoomWidth, zoomHeight;
    uint32_t fdWidth, fdHeight, sensorOrgW, sensorOrgH;
    int *w = NULL;
    int *h = NULL;

    sensorOrgW = m3DCaptureWidth * 2;
    sensorOrgH = m3DCaptureHeight * 2;
    fdWidth = ptr_cam_face_inf[2] - ptr_cam_face_inf[0];
    fdHeight = ptr_cam_face_inf[3] - ptr_cam_face_inf[1];
    if (fdWidth == 0 || fdHeight == 0) {
        HAL_LOGE("parameters error.");
        return;
    }
    zoomWidth = (float)width / (float)sensorOrgW;
    zoomHeight = (float)height / (float)sensorOrgH;
    ptr_cam_face_inf[0] = ptr_cam_face_inf[0] * zoomWidth / 2;
    ptr_cam_face_inf[1] = ptr_cam_face_inf[1] * zoomHeight;
    ptr_cam_face_inf[2] = ptr_cam_face_inf[2] * zoomWidth / 2;
    ptr_cam_face_inf[3] = ptr_cam_face_inf[3] * zoomHeight;
    HAL_LOGV("ptr_cam_face_inf 0 %d 1 %d 2%d 3 %d", ptr_cam_face_inf[0],
             ptr_cam_face_inf[1], ptr_cam_face_inf[2], ptr_cam_face_inf[3]);

    return;
}

/*===========================================================================
 * FUNCTION   :cap_3d_doFaceMakeup
 *
 * DESCRIPTION:
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Capture::CaptureThread::cap_3d_doFaceMakeup(
    private_handle_t *private_handle, int perfect_level, int *face_info) {
    // init the parameters table. save the value until the process is restart or
    // the device is restart.
    /*
    int tab_skinWhitenLevel[10] = {0, 15, 25, 35, 45, 55, 65, 75, 85, 95};
    int tab_skinCleanLevel[10] = {0, 25, 45, 50, 55, 60, 70, 80, 85, 95};
    struct camera_frame_type cap_3d_frame;
    bzero(&cap_3d_frame, sizeof(struct camera_frame_type));
    struct camera_frame_type *frame = &cap_3d_frame;
    frame->y_vir_addr = (cmr_uint)private_handle->base;
    frame->width = private_handle->width;
    frame->height = private_handle->height;

    TSRect Tsface;
    YuvFormat yuvFormat = TSFB_FMT_NV21;
    if (face_info[0] != 0 || face_info[1] != 0 || face_info[2] != 0 ||
        face_info[3] != 0) {
        cap_3d_convert_face_info_from_preview2cap(face_info, frame->width,
                                                  frame->height);
        Tsface.left = face_info[0];
        Tsface.top = face_info[1];
        Tsface.right = face_info[2];
        Tsface.bottom = face_info[3];
        HAL_LOGD("FACE_BEAUTY rect:%ld-%ld-%ld-%ld", Tsface.left, Tsface.top,
                 Tsface.right, Tsface.bottom);

        int level = perfect_level;
        int skinWhitenLevel = 0;
        int skinCleanLevel = 0;
        int level_num = 0;
        // convert the skin_level set by APP to skinWhitenLevel & skinCleanLevel
        // according to the table saved.
        level = (level < 0) ? 0 : ((level > 90) ? 90 : level);
        level_num = level / 10;
        skinWhitenLevel = tab_skinWhitenLevel[level_num];
        skinCleanLevel = tab_skinCleanLevel[level_num];
        HAL_LOGD("UCAM skinWhitenLevel is %d, skinCleanLevel is %d "
                 "frame->height %d frame->width %d",
                 skinWhitenLevel, skinCleanLevel, frame->height, frame->width);

        TSMakeupData inMakeupData;
        unsigned char *yBuf = (unsigned char *)(frame->y_vir_addr);
        unsigned char *uvBuf =
            (unsigned char *)(frame->y_vir_addr) + frame->width * frame->height;

        inMakeupData.frameWidth = frame->width;
        inMakeupData.frameHeight = frame->height;
        inMakeupData.yBuf = yBuf;
        inMakeupData.uvBuf = uvBuf;

        if (frame->width > 0 && frame->height > 0) {
            int ret_val =
                ts_face_beautify(&inMakeupData, &inMakeupData, skinCleanLevel,
                                 skinWhitenLevel, &Tsface, 0, yuvFormat);
            if (ret_val != TS_OK) {
                HAL_LOGE("UCAM ts_face_beautify ret is %d", ret_val);
            } else {
                HAL_LOGD("UCAM ts_face_beautify return OK");
            }
        } else {
            HAL_LOGE("No face beauty! frame size. If size is not zero, then "
                     "outMakeupData.yBuf is null!");
        }
    } else {
        HAL_LOGD("Not detect face!");
    }
    */
}

/*===========================================================================
 * FUNCTION   :reProcessFrame face makeup
 *
 * DESCRIPTION: reprocess frame
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Capture::CaptureThread::reProcessFrame(
    const buffer_handle_t *frame_buffer, int cur_frameid) {
    int32_t perfectskinlevel = 0;
    int32_t face_info[4];
    perfectskinlevel = mCapture->mPerfectskinlevel;
    face_info[0] = mCapture->g_face_info[0];
    face_info[1] = mCapture->g_face_info[1];
    face_info[2] = mCapture->g_face_info[2];
    face_info[3] = mCapture->g_face_info[3];
    HAL_LOGD("perfectskinlevel=%d face:sx %d sy %d ex %d ey %d",
             perfectskinlevel, face_info[0], face_info[1], face_info[2],
             face_info[3]);
    private_handle_t *private_handle =
        (struct private_handle_t *)(*frame_buffer);
    if (perfectskinlevel > 0)
        cap_3d_doFaceMakeup(private_handle, perfectskinlevel, face_info);

    return;
}
#endif

/*===========================================================================
 * FUNCTION   :loadGpuApi
 *
 * DESCRIPTION: loadGpuApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3Capture::CaptureThread::loadGpuApi() {
    HAL_LOGD(" E");
    const char *error = NULL;

    if (mGpuApi == NULL) {
        error = dlerror();
        HAL_LOGE("mGpuApi is null. error = %s", error);
        return -1;
    }

    mGpuApi->handle = dlopen(LIB_GPU_PATH, RTLD_LAZY);
    if (mGpuApi->handle == NULL) {
        error = dlerror();
        HAL_LOGE("open GPU API failed.error = %s", error);
        return -1;
    }

    mGpuApi->initRenderContext =
        (int (*)(struct stream_info_s *, float *, int))dlsym(
            mGpuApi->handle, "initRenderContext");
    if (mGpuApi->initRenderContext == NULL) {
        error = dlerror();
        HAL_LOGE("sym initRenderContext failed.error = %s", error);
        return -1;
    }

    mGpuApi->imageStitchingWithGPU = (void (*)(dcam_info_t *dcam))dlsym(
        mGpuApi->handle, "imageStitchingWithGPU");
    if (mGpuApi->imageStitchingWithGPU == NULL) {
        error = dlerror();
        HAL_LOGE("sym imageStitchingWithGPU failed.error = %s", error);
        return -1;
    }

    mGpuApi->destroyRenderContext =
        (void (*)(void))dlsym(mGpuApi->handle, "destroyRenderContext");
    if (mGpuApi->destroyRenderContext == NULL) {
        error = dlerror();
        HAL_LOGE("sym destroyRenderContext failed.error = %s", error);
        return -1;
    }
    isInitRenderContest = false;

    HAL_LOGD("load Gpu Api succuss.");

    return 0;
}
/*===========================================================================
 * FUNCTION   :unLoadGpuApi
 *
 * DESCRIPTION: unLoadGpuApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Capture::CaptureThread::unLoadGpuApi() {
    HAL_LOGD("E");

    if (mGpuApi->handle != NULL) {
        dlclose(mGpuApi->handle);
        mGpuApi->handle = NULL;
    }

    HAL_LOGD("X");
}
/*===========================================================================
 * FUNCTION   :initGpuData
 *
 * DESCRIPTION: init gpu data
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Capture::CaptureThread::initGpuData() {
    pt_stream_info.dst_height = mCapture->mCaptureHeight;
    pt_stream_info.dst_width = mCapture->mCaptureWidth;
    pt_stream_info.src_height = m3DCaptureHeight;
    pt_stream_info.src_width = m3DCaptureWidth;
    pt_stream_info.rot_angle = 0;
    HAL_LOGD("src_width = %d, dst_height=%d, dst_width=%d, dst_height=%d, "
             "rotation:%d",
             pt_stream_info.src_width, pt_stream_info.src_height,
             pt_stream_info.dst_width, pt_stream_info.dst_height,
             pt_stream_info.rot_angle);
    float buff[768];
    float H_left[9], H_right[9];
    FILE *fid =
        fopen("/productinfo/sprd_3d_calibration/calibration.data", "rb");
    if (fid != NULL) {
        HAL_LOGD("open calibration file success");
        fread(buff, sizeof(float), 768, fid);
        fclose(fid);
        memcpy(H_left, buff + 750, sizeof(float) * 9);
        memcpy(H_right, buff + 759, sizeof(float) * 9);

        memcpy(pt_line_buf.homography_matrix, H_left, sizeof(float) * 9);
        memcpy(pt_line_buf.homography_matrix + 9, H_right, sizeof(float) * 9);
    } else {
        HAL_LOGD("can not open calibration file, using default ");
        pt_line_buf.homography_matrix[0] = 1.0;
        pt_line_buf.homography_matrix[1] = 0.0;
        pt_line_buf.homography_matrix[2] = 0.0;
        pt_line_buf.homography_matrix[3] = 0.0;
        pt_line_buf.homography_matrix[4] = 1.0;
        pt_line_buf.homography_matrix[5] = 0.0;
        pt_line_buf.homography_matrix[6] = 0.0;
        pt_line_buf.homography_matrix[7] = 0.0;
        pt_line_buf.homography_matrix[8] = 1.0;
        pt_line_buf.homography_matrix[9] = 1.0;
        pt_line_buf.homography_matrix[10] = 0.0;
        pt_line_buf.homography_matrix[11] = 0.0;
        pt_line_buf.homography_matrix[12] = 0.0;
        pt_line_buf.homography_matrix[13] = 1.0;
        pt_line_buf.homography_matrix[14] = 0.0;
        pt_line_buf.homography_matrix[15] = 0.0;
        pt_line_buf.homography_matrix[16] = 0.0;
        pt_line_buf.homography_matrix[17] = 1.0;
    }
    if (CONTEXT_FAIL ==
        mGpuApi->initRenderContext(&pt_stream_info,
                                   pt_line_buf.homography_matrix, 18)) {
        HAL_LOGE("initRenderContext fail");
    } else {
        isInitRenderContest = true;
    }
    HAL_LOGD("using following homography_matrix data:\n");
    HAL_LOGD("left:\t%8f  %8f  %8f    right:\t%8f  %8f  %8f",
             pt_line_buf.homography_matrix[0], pt_line_buf.homography_matrix[1],
             pt_line_buf.homography_matrix[2], pt_line_buf.homography_matrix[9],
             pt_line_buf.homography_matrix[10],
             pt_line_buf.homography_matrix[11]);
    HAL_LOGD(
        "\t\t%8f  %8f  %8f    \t%8f  %8f  %8f",
        pt_line_buf.homography_matrix[3], pt_line_buf.homography_matrix[4],
        pt_line_buf.homography_matrix[5], pt_line_buf.homography_matrix[12],
        pt_line_buf.homography_matrix[13], pt_line_buf.homography_matrix[14]);
    HAL_LOGD(
        "\t\t%8f  %8f  %8f    \t%8f  %8f  %8f",
        pt_line_buf.homography_matrix[6], pt_line_buf.homography_matrix[7],
        pt_line_buf.homography_matrix[8], pt_line_buf.homography_matrix[15],
        pt_line_buf.homography_matrix[16], pt_line_buf.homography_matrix[17]);
}

static void save_yuv_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width,
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

    strcpy(file_name, "/data/misc/media/");
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
            return;
        }

        fwrite((void *)addr->addr_y, 1, width * height, fp);
        fclose(fp);

        strcpy(file_name, "/data/misc/media/");
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
            return;
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
            return;
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
            return;
        }

        fwrite((void *)addr->addr_y, 1, (uint32_t)(width * height * 5 / 4), fp);
        fclose(fp);
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

bool SprdCamera3Capture::CaptureThread::threadLoop() {
    buffer_handle_t *output_buffer = NULL;
    capture_queue_msg_t capture_msg;

    HAL_LOGD("E: isInitRenderContest:%d", isInitRenderContest);
    if (!isInitRenderContest)
        initGpuData();

    while (!mCaptureMsgList.empty()) {
        List<capture_queue_msg_t>::iterator itor1 = mCaptureMsgList.begin();
        capture_msg = *itor1;
        mCaptureMsgList.erase(itor1);
        switch (capture_msg.msg_type) {
        case CAPTURE_MSG_EXIT: {
            // flush queue
            memset(&mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
            memset(&mSavedCapReqstreambuff, 0, sizeof(camera3_stream_buffer_t));
            if (NULL != mSavedCapReqsettings) {
                free_camera_metadata(mSavedCapReqsettings);
                mSavedCapReqsettings = NULL;
            }
            if (isInitRenderContest) {
                mGpuApi->destroyRenderContext();
                isInitRenderContest = false;
            }
            return false;
        } break;
        case CAPTURE_MSG_DATA_PROC: {
            HAL_LOGD("CAPTURE_MSG_DATA_PROC for frame idx:%d",
                     capture_msg.combo_buff.frame_number);
            if (combineTwoPicture(output_buffer, capture_msg.combo_buff.buffer1,
                                  capture_msg.combo_buff.buffer2) != NO_ERROR) {
                HAL_LOGE("Error happen when merging for frame idx:%d",
                         capture_msg.combo_buff.frame_number);
            } else {
#ifdef CONFIG_FACE_BEAUTY
                reProcessFrame(output_buffer,
                               capture_msg.combo_buff.frame_number);
#endif
                CameraMetadata meta;
                camera3_capture_request_t request;
                camera3_stream_buffer_t *output_buffers = NULL;
                camera3_stream_buffer_t *input_buffer = NULL;

                memset(&request, 0x00, sizeof(camera3_capture_request_t));

                output_buffers = (camera3_stream_buffer_t *)malloc(
                    sizeof(camera3_stream_buffer_t));
                if (!output_buffers) {
                    HAL_LOGE("failed");
                    return false;
                }
                memset(output_buffers, 0x00, sizeof(camera3_stream_buffer_t));

                input_buffer = (camera3_stream_buffer_t *)malloc(
                    sizeof(camera3_stream_buffer_t));
                if (!input_buffer) {
                    HAL_LOGE("failed");
                    return false;
                }
                memset(input_buffer, 0x00, sizeof(camera3_stream_buffer_t));

                memcpy((void *)&request, &mSavedCapRequest,
                       sizeof(camera3_capture_request_t));
                meta.append(mSavedCapReqsettings);
                if (meta.exists(ANDROID_JPEG_ORIENTATION)) {
                    int32_t jpeg_orientation =
                        meta.find(ANDROID_JPEG_ORIENTATION).data.i32[0];
                    HAL_LOGD("find jpeg orientation %d", jpeg_orientation);
                    jpeg_orientation = 0;
                    meta.update(ANDROID_JPEG_ORIENTATION, &jpeg_orientation, 1);
                }
                request.settings = meta.release();

                memcpy(input_buffer, &mSavedCapReqstreambuff,
                       sizeof(camera3_stream_buffer_t));
                input_buffer->stream = &mMainStreams[mCaptureStreamsNum - 1];
                input_buffer->stream->width = mCapture->mCaptureWidth;
                input_buffer->stream->height = mCapture->mCaptureHeight;
                input_buffer->buffer = output_buffer;

                memcpy((void *)&output_buffers[0], &mSavedCapReqstreambuff,
                       sizeof(camera3_stream_buffer_t));
                output_buffers[0].stream =
                    &mMainStreams[mCaptureStreamsNum - 1];
                output_buffers[0].stream->width = mCapture->mCaptureWidth;
                output_buffers[0].stream->height = mCapture->mCaptureHeight;

                request.output_buffers = output_buffers;
                request.input_buffer = input_buffer;
                mReprocessing = true;

                HAL_LOGD("capture combined success: framenumber %d",
                         request.frame_number);
                HAL_LOGD("capture combined success: cmbbuff %p", output_buffer);

                HAL_LOGD(
                    "reprocess request input buff %p, stream:%p, yaddr_v:%p, "
                    "width:%d, height:%d",
                    request.input_buffer->buffer, request.input_buffer->stream,
                    ((struct private_handle_t *)(*request.input_buffer->buffer))
                        ->base,
                    input_buffer->stream->width, input_buffer->stream->height);
                HAL_LOGD("reprocess request output buff %p, stream:%p, "
                         "yaddr_v:%p, width:%d, height:%d",
                         request.output_buffers[0].buffer,
                         request.output_buffers[0].stream,
                         ((struct private_handle_t *)(*request.output_buffers[0]
                                                           .buffer))
                             ->base,
                         request.output_buffers[0].stream->width,
                         request.output_buffers[0].stream->height);
                if (0 > mDevMain->hwi->process_capture_request(mDevMain->dev,
                                                               &request)) {
                    HAL_LOGE("failed. process capture request!");
                }
                if (input_buffer != NULL) {
                    free((void *)input_buffer);
                    input_buffer = NULL;
                }
                if (output_buffers != NULL) {
                    free((void *)output_buffers);
                    output_buffers = NULL;
                }
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
 * FUNCTION   :requestExit
 *
 * DESCRIPTION: request thread exit
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Capture::CaptureThread::requestExit() {
    capture_queue_msg_t capture_msg;
    capture_msg.msg_type = CAPTURE_MSG_EXIT;
    mCaptureMsgList.push_back(capture_msg);
    mMergequeueSignal.signal();
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
int SprdCamera3Capture::getStreamType(camera3_stream_t *new_stream) {
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
 * FUNCTION   :waitMsgAvailable
 *
 * DESCRIPTION: wait util two list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Capture::CaptureThread::waitMsgAvailable() {
    // TODO:what to do for timeout
    while (mCaptureMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, THREAD_TIMEOUT);
    }
}

/*===========================================================================
 * FUNCTION   :combineTwoPicture
 *
 * DESCRIPTION: combineTwoPicture
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3Capture::CaptureThread::combineTwoPicture(
    buffer_handle_t *&output_buf, buffer_handle_t *input_buf1,
    buffer_handle_t *input_buf2) {
    int rc = NO_ERROR;
    CameraMetadata metaSettings;
    if (input_buf1 == NULL || input_buf2 == NULL) {
        HAL_LOGE("Error, null buffer detected! input_buf1:%p input_buf2:%p",
                 input_buf1, input_buf2);
        return BAD_VALUE;
    }
    HAL_LOGD("combine two picture:%p, %p", input_buf1, input_buf2);

    output_buf = &(mCapture->mLocalCapBuffer[mCaptureStreamsNum].native_handle);
    dcam_info_t dcam;

    dcam.left_buf = (struct private_handle_t *)*input_buf1;
    dcam.right_buf = (struct private_handle_t *)*input_buf2;
    dcam.dst_buf = (struct private_handle_t *)*output_buf;
    dcam.rot_angle = ROT_90;

    metaSettings = mSavedCapReqsettings;
    if (metaSettings.exists(ANDROID_JPEG_ORIENTATION)) {
        int32_t jpeg_orientation =
            metaSettings.find(ANDROID_JPEG_ORIENTATION).data.i32[0];
        HAL_LOGD("find jpeg orientation %d", jpeg_orientation);
        switch (jpeg_orientation) {
        case 0:
            dcam.rot_angle = ROT_0;
            break;
        case 90:
            dcam.rot_angle = ROT_90;
            break;
        case 180:
            dcam.rot_angle = ROT_180;
            break;
        case 270:
            dcam.rot_angle = ROT_270;
            break;
        default:
            dcam.rot_angle = ROT_270;
            break;
        }
    }
    HAL_LOGD(" combine rot_angle:%d", dcam.rot_angle);
    HAL_LOGD(" before cmb yaddr_v:%p",
             ((struct private_handle_t *)output_buf)->base);
    if (mGpuApi->imageStitchingWithGPU == NULL) {
        HAL_LOGE("mGpuApi imageStitchingWithGPU is null.");
        return -1;
    }
    mGpuApi->imageStitchingWithGPU(&dcam);
    HAL_LOGD(" after cmb yaddr_v:%p",
             ((struct private_handle_t *)output_buf)->base);
    {
        char prop[PROPERTY_VALUE_MAX] = {
            0,
        };
        property_get("debug.camera.3dcap.savefile", prop, "0");
        if (1 == atoi(prop)) {
            static int nCont = 0;
            struct img_addr imgadd;
            bzero(&imgadd, sizeof(img_addr));
            imgadd.addr_y =
                (cmr_uint)(((struct private_handle_t *)*input_buf1)->base);
            imgadd.addr_u = imgadd.addr_y + m3DCaptureWidth * m3DCaptureHeight;
            save_yuv_to_file(100 + nCont, IMG_DATA_TYPE_YUV420, m3DCaptureWidth,
                             m3DCaptureHeight, &imgadd);

            imgadd.addr_y =
                (cmr_uint)(((struct private_handle_t *)*input_buf2)->base);
            imgadd.addr_u = imgadd.addr_y + 1632 * 1224;
            save_yuv_to_file(200 + nCont, IMG_DATA_TYPE_YUV420, m3DCaptureWidth,
                             m3DCaptureHeight, &imgadd);

            imgadd.addr_y =
                (cmr_uint)(((struct private_handle_t *)*output_buf)->base);
            imgadd.addr_u = imgadd.addr_y + 1920 * 1080;
            save_yuv_to_file(300 + nCont, IMG_DATA_TYPE_YUV420,
                             mCapture->mCaptureWidth, mCapture->mCaptureHeight,
                             &imgadd);
            nCont = nCont % 100 + 1;
        }
    }
    return rc;
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
void SprdCamera3Capture::CaptureThread::dumpFps() {
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
/*======================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: dump fd
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Capture::dump(const struct camera3_device *device, int fd) {
    HAL_LOGV("E");
    CHECK_CAPTURE();

    mCapture->_dump(device, fd);

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
int SprdCamera3Capture::flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGV(" E");
    CHECK_CAPTURE_ERROR();

    rc = mCapture->_flush(device);

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
int SprdCamera3Capture::initialize(const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;

    HAL_LOGD("E");
    CHECK_HWI_ERROR(hwiMain);

    mLastWidth = 0;
    mLastHeight = 0;
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mPreviewStreamsNum = 0;
    mPreviewID = 0;
    mCaptureThread->mCaptureStreamsNum = 0;
    mCaptureThread->mReprocessing = false;
    mWaitFrameNumber = 0;
    mHasSendFrameNumber = 0;
    mLastShowPreviewDeviceId = 0;

    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }

    sprdCam = m_pPhyCamera[CAM_TYPE_AUX];
    SprdCamera3HWI *hwiAux = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiAux);

    rc = hwiAux->initialize(sprdCam.dev, &callback_ops_aux);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error aux camera while initialize !! ");
        return rc;
    }

    // init buffer_handle_t
    mLocalBuffer = (new_mem_t *)malloc(sizeof(new_mem_t) * MAX_QEQUEST_BUF);
    if (NULL == mLocalBuffer) {
        HAL_LOGE("fatal error! mLocalBuffer pointer is null.");
        return NO_MEMORY;
    }
    memset(mLocalBuffer, 0, sizeof(new_mem_t) * MAX_QEQUEST_BUF);

    mLocalCapBuffer =
        (new_mem_t *)malloc(sizeof(new_mem_t) * LOCAL_CAPBUFF_NUM);
    if (NULL == mLocalCapBuffer) {
        HAL_LOGE("fatal error! mLocalCapBuffer pointer is null.");
        free(mLocalBuffer);
        mLocalBuffer = NULL;
        return NO_MEMORY;
    }
    memset(mLocalCapBuffer, 0, sizeof(new_mem_t) * LOCAL_CAPBUFF_NUM);

    mCaptureThread->mCallbackOps = callback_ops;
    mCaptureThread->mDevMain = &m_pPhyCamera[CAM_TYPE_MAIN];
    HAL_LOGD("X");
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
int SprdCamera3Capture::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGV("E");

    int rc = 0;
    camera3_stream_t *pMainStreams[MAX_NUM_STREAMS];
    camera3_stream_t *pAuxStreams[MAX_NUM_STREAMS];
    size_t i = 0;
    size_t j = 0;
    int w = 0;
    int h = 0;
    struct stream_info_s stream_info;

    Mutex::Autolock l(mLock1);
    get3DCaptureSize(&mCaptureThread->m3DCaptureWidth,
                     &mCaptureThread->m3DCaptureHeight);
    set3DCaptureMode();

    HAL_LOGD("configurestreams, stream num:%d", stream_list->num_streams);
    for (size_t i = 0; i < stream_list->num_streams; i++) {
        int requestStreamType = getStreamType(stream_list->streams[i]);
        HAL_LOGD("configurestreams, org streamtype:%d, out streamtype:%d",
                 stream_list->streams[i]->stream_type, requestStreamType);
        if (requestStreamType == PREVIEW_STREAM) {
            w = stream_list->streams[i]->width;
            h = stream_list->streams[i]->height;
            HAL_LOGD("preview width:%d, height:%d", w, h);
            if (mLastWidth != w && mLastHeight != h) {

                freeLocalBuffer(mLocalBuffer);

                for (size_t j = 0; j < MAX_QEQUEST_BUF; j++) {
                    int tmp = allocateOne(w, h, 1, &(mLocalBuffer[j]));
                    if (tmp < 0) {
                        HAL_LOGE("request one buf failed.");
                        continue;
                    }
                    mLocalBufferList.push_back(&mLocalBuffer[j].native_handle);
                }
            }
            mLastWidth = w;
            mLastHeight = h;
            mPreviewStreamsNum = i;
            HAL_LOGD("configurestreams, mPreviewStreamsNum:%d",
                     mPreviewStreamsNum);
        } else if (requestStreamType == SNAPSHOT_STREAM) {
            w = stream_list->streams[i]->width;
            h = stream_list->streams[i]->height;
            HAL_LOGD("capture width:%d, height:%d", w, h);
            if (mCaptureWidth != w && mCaptureHeight != h) {

                freeLocalCapBuffer(mLocalCapBuffer);

                for (size_t j = 0; j < LOCAL_CAPBUFF_NUM - 1; j++) {
                    if (0 > allocateOne(mCaptureThread->m3DCaptureWidth,
                                        mCaptureThread->m3DCaptureHeight, 1,
                                        &(mLocalCapBuffer[j]))) {
                        HAL_LOGE("request one buf failed.");
                        continue;
                    }
                }
                if (0 >
                    allocateOne(w, h, 1,
                                &(mLocalCapBuffer[stream_list->num_streams]))) {
                    HAL_LOGE("request one buf failed.");
                    continue;
                }
            }
            mCaptureWidth = w;
            mCaptureHeight = h;
            mCaptureThread->mCaptureStreamsNum = stream_list->num_streams;
            HAL_LOGD("configurestreams, mCaptureThread->mCaptureStreamsNum:%d",
                     mCaptureThread->mCaptureStreamsNum);

            stream_list->streams[i]->width = mCaptureThread->m3DCaptureWidth;
            stream_list->streams[i]->height = mCaptureThread->m3DCaptureHeight;
            // mCaptureThread->mMainStreams[stream_list->num_streams] =
            // &mCallBackStream;
            mCaptureThread->mMainStreams[stream_list->num_streams].max_buffers =
                1;
            mCaptureThread->mMainStreams[stream_list->num_streams].width =
                mCaptureThread->m3DCaptureWidth;
            mCaptureThread->mMainStreams[stream_list->num_streams].height =
                mCaptureThread->m3DCaptureHeight;
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
            // stream_list->num_streams++;
            mCaptureThread->mAuxStreams[stream_list->num_streams].max_buffers =
                1;
            mCaptureThread->mAuxStreams[stream_list->num_streams].width =
                mCaptureThread->m3DCaptureWidth;
            mCaptureThread->mAuxStreams[stream_list->num_streams].height =
                mCaptureThread->m3DCaptureHeight;
            mCaptureThread->mAuxStreams[stream_list->num_streams].format =
                HAL_PIXEL_FORMAT_YCbCr_420_888;
            mCaptureThread->mAuxStreams[stream_list->num_streams].usage =
                stream_list->streams[i]->usage;
            mCaptureThread->mAuxStreams[stream_list->num_streams].stream_type =
                CAMERA3_STREAM_OUTPUT;
            mCaptureThread->mAuxStreams[stream_list->num_streams].data_space =
                stream_list->streams[i]->data_space;
            mCaptureThread->mAuxStreams[stream_list->num_streams].rotation =
                stream_list->streams[i]->rotation;
            pAuxStreams[stream_list->num_streams] =
                &mCaptureThread->mAuxStreams[stream_list->num_streams];
        }
        mCaptureThread->mMainStreams[i] = *stream_list->streams[i];
        mCaptureThread->mAuxStreams[i] = *stream_list->streams[i];
        pMainStreams[i] = &mCaptureThread->mMainStreams[i];
        pAuxStreams[i] = &mCaptureThread->mAuxStreams[i];
    }

    camera3_stream_configuration mainconfig;
    mainconfig = *stream_list;
    mainconfig.num_streams = stream_list->num_streams + 1;
    mainconfig.streams = pMainStreams;
    for (i = 0; i < mainconfig.num_streams; i++) {
        HAL_LOGD("main configurestreams, streamtype:%d, format:%d, width:%d, "
                 "height:%d",
                 pMainStreams[i]->stream_type, pMainStreams[i]->format,
                 pMainStreams[i]->width, pMainStreams[i]->height);
    }
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                    &mainconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure main streams!!");
        return rc;
    }

    camera3_stream_configuration subconfig;
    subconfig = *stream_list;
    subconfig.num_streams = stream_list->num_streams + 1;
    subconfig.streams = pAuxStreams;
    for (i = 0; i < subconfig.num_streams; i++) {
        HAL_LOGD("aux configurestreams, streamtype:%d, format:%d, width:%d, "
                 "height:%d",
                 pAuxStreams[i]->stream_type, pAuxStreams[i]->format,
                 pAuxStreams[i]->width, pAuxStreams[i]->height);
    }
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    rc = hwiAux->configure_streams(m_pPhyCamera[CAM_TYPE_AUX].dev, &subconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure aux streams!!");
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

    mCaptureThread->run(NULL);

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
const camera_metadata_t *SprdCamera3Capture::constructDefaultRequestSettings(
    const struct camera3_device *device, int type) {
    HAL_LOGV("E");
    const camera_metadata_t *fwk_metadata = NULL;

    SprdCamera3HWI *hw = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    Mutex::Autolock l(mLock1);
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
void SprdCamera3Capture::saveRequest(camera3_capture_request_t *request,
                                     uint32_t preview_id) {
    size_t i = 0;
    camera3_stream_t *newStream = NULL;
    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        if (getStreamType(newStream) == CALLBACK_STREAM) {
            request_saved_t currRequest;
            HAL_LOGD("save preview request %d", request->frame_number);
            Mutex::Autolock l(mRequestLock);
            currRequest.frame_number = request->frame_number;
            currRequest.preview_id = preview_id;
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
int SprdCamera3Capture::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;
    uint32_t i = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    CameraMetadata metaSettings;
    camera3_capture_request_t *req = request;
    camera3_capture_request_t req_main;
    camera3_capture_request_t req_aux;
    camera3_stream_t *new_stream = NULL;
    camera3_stream_buffer_t *out_streams_main = NULL;
    camera3_stream_buffer_t *out_streams_aux = NULL;
    bool is_captureing = false;

    rc = validateCaptureRequest(req);
    if (rc != NO_ERROR) {
        return rc;
    }

    metaSettings = request->settings;
    if (metaSettings.exists(ANDROID_SPRD_MULTI_CAM3_PREVIEW_ID)) {
        int nPreviewId =
            metaSettings.find(ANDROID_SPRD_MULTI_CAM3_PREVIEW_ID).data.i32[0];
        HAL_LOGV("find preview id %d, current preview id:%d", nPreviewId,
                 mPreviewID);
        if (mPreviewID != nPreviewId) {
            HAL_LOGD("preview id changed to %d, current preview id:%d",
                     nPreviewId, mPreviewID);
            mPreviewID = nPreviewId;
        }
    } else {
        char prop[PROPERTY_VALUE_MAX] = {
            0,
        };
        property_get("debug.camera.3dcap.preview", prop, "0");
        int nPreviewId = atoi(prop);
        HAL_LOGV("no preview id setting find, prop preview id %d, current "
                 "preview id:%d",
                 nPreviewId, mPreviewID);
        if (nPreviewId & 0x03 && mPreviewID != nPreviewId) {
            HAL_LOGD("preview id changed to %d by prop, current preview id:%d",
                     nPreviewId, mPreviewID);
            mPreviewID = nPreviewId;
        }
    }
    /* save Perfectskinlevel */
    if (metaSettings.exists(ANDROID_SPRD_UCAM_SKIN_LEVEL)) {
        mPerfectskinlevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[0];
        HAL_LOGD("perfectskinlevel=%d", mPerfectskinlevel);
    }

    saveRequest(req, mPreviewID);
    if (mLastShowPreviewDeviceId != mPreviewID &&
        mHasSendFrameNumber < request->frame_number - 1) {
        mWaitFrameNumber = request->frame_number - 1;
        HAL_LOGD("change showPreviewid %d to %d,mWaitFrameNumber=%d",
                 mLastShowPreviewDeviceId, mPreviewID, mWaitFrameNumber);
        Mutex::Autolock l(mWaitFrameMutex);
        mWaitFrameSignal.waitRelative(mWaitFrameMutex, WAIT_FRAME_TIMEOUT);
        HAL_LOGD("wait succeed.");
    }
    mLastShowPreviewDeviceId = mPreviewID;

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
        new_stream = (req->output_buffers[i]).stream;
        HAL_LOGD("num_output_buffers:%d, streamtype:%d",
                 req->num_output_buffers, requestStreamType);
        if (requestStreamType == SNAPSHOT_STREAM) {
            CameraMetadata meta;
            HAL_LOGD(" orgtype:%d", req->output_buffers[i].stream->format);
            mCaptureThread->mSavedResultBuff = NULL;
            HAL_LOGD("org snp request output buff %p, stream:%p, yaddr_v:%p",
                     request->output_buffers[i].buffer,
                     request->output_buffers[i].stream,
                     ((struct private_handle_t *)(*request->output_buffers[i]
                                                       .buffer))
                         ->base);
            memcpy(&mCaptureThread->mSavedCapRequest, req,
                   sizeof(camera3_capture_request_t));
            memcpy(&mCaptureThread->mSavedCapReqstreambuff,
                   &req->output_buffers[i], sizeof(camera3_stream_buffer_t));
            if (NULL != mCaptureThread->mSavedCapReqsettings) {
                free_camera_metadata(mCaptureThread->mSavedCapReqsettings);
                mCaptureThread->mSavedCapReqsettings = NULL;
            }
            mCaptureThread->mSavedCapReqsettings =
                clone_camera_metadata(req_main.settings);
            req_main.settings = mCaptureThread->mSavedCapReqsettings;
            mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1] =
                req->output_buffers[i].stream;
            out_streams_main[i].stream =
                &mCaptureThread
                     ->mMainStreams[mCaptureThread->mCaptureStreamsNum];
            out_streams_main[i].stream->width = mCaptureThread->m3DCaptureWidth;
            out_streams_main[i].stream->height =
                mCaptureThread->m3DCaptureHeight;
            out_streams_main[i].buffer = &mLocalCapBuffer[0].native_handle;
            HAL_LOGD("[caputre]start.caputre frame num=%d",
                     request->frame_number);
            HAL_LOGD("newtype:%d, rotation:%d",
                     out_streams_main[i].stream->format,
                     out_streams_main[i].stream->rotation);

            mIsCapturing = true;
            is_captureing = true;
        } else {
            mSavedReqStreams[mPreviewStreamsNum] =
                req->output_buffers[i].stream;
            out_streams_main[i].stream =
                &mCaptureThread->mMainStreams[mPreviewStreamsNum];
            if (CAM_TYPE_AUX == mPreviewID) {
                HAL_LOGV("hide main preview");
                out_streams_main[i].buffer = popRequestList(mLocalBufferList);
            }
        }
    }
    req_main.output_buffers = out_streams_main;

    /*config aux camera * recreate a config without preview stream */
    req_aux = *req;
    out_streams_aux =
        (camera3_stream_buffer_t *)malloc(sizeof(camera3_stream_buffer_t));
    if (!out_streams_aux) {
        HAL_LOGE("failed");
        goto req_fail;
    }
    memset(out_streams_aux, 0,
           (sizeof(camera3_stream_buffer_t)) * (req_aux.num_output_buffers));

    for (size_t i = 0; i < req->num_output_buffers; i++) {
        new_stream = (req->output_buffers[i]).stream;
        HAL_LOGD("num_output_buffers:%d, , streamtype:%d",
                 req->num_output_buffers, getStreamType(new_stream));
        if (getStreamType(new_stream) == SNAPSHOT_STREAM) {
            req_aux.settings = mCaptureThread->mSavedCapReqsettings;
            out_streams_aux[i] = req->output_buffers[i];
            out_streams_aux[i].buffer = &mLocalCapBuffer[1].native_handle;
            out_streams_aux[i].stream =
                &mCaptureThread
                     ->mAuxStreams[mCaptureThread->mCaptureStreamsNum];
            out_streams_aux[i].stream->width = mCaptureThread->m3DCaptureWidth;
            out_streams_aux[i].stream->height =
                mCaptureThread->m3DCaptureHeight;
            out_streams_aux[i].acquire_fence = -1;
            HAL_LOGD("mCaptureThread->mCaptureStreamsNum:%d, stream:%p",
                     mCaptureThread->mCaptureStreamsNum,
                     out_streams_aux[i].stream);
            if (NULL == out_streams_aux[i].buffer) {
                HAL_LOGE("failed, LocalBufferList is empty!");
                goto req_fail;
            }
        } else {
            out_streams_aux[i] = req->output_buffers[i];
            out_streams_aux[i].stream =
                &mCaptureThread->mAuxStreams[mPreviewStreamsNum];
            if (CAM_TYPE_MAIN == mPreviewID) {
                HAL_LOGV("hide sub preview");
                out_streams_aux[i].buffer = popRequestList(mLocalBufferList);
            }
            HAL_LOGD("mPreviewStreamsNum:%d, stream:%p", mPreviewStreamsNum,
                     out_streams_aux[i].stream);
            if (NULL == out_streams_aux[i].buffer) {
                HAL_LOGE("failed, LocalBufferList is empty!");
                goto req_fail;
            }
            out_streams_aux[i].acquire_fence = -1;
        }
    }
    req_aux.output_buffers = out_streams_aux;

    if (req_main.output_buffers[0].stream->format == HAL_PIXEL_FORMAT_BLOB) {
        HAL_LOGD("capture request, idx:%d", req_main.frame_number);
        req_main.output_buffers[0].stream->format =
            HAL_PIXEL_FORMAT_YCbCr_420_888;
        req_aux.output_buffers[0].stream->format =
            HAL_PIXEL_FORMAT_YCbCr_420_888;
    }
    HAL_LOGD("mIsCapturing:%d, framenumber=%d", mIsCapturing,
             request->frame_number);

    if (is_captureing) {
        uint64_t currentMainTimestamp = hwiMain->getZslBufferTimestamp();
        uint64_t currentAuxTimestamp = hwiAux->getZslBufferTimestamp();
        hwiMain->setMultiCallBackYuvMode(true);
        hwiAux->setMultiCallBackYuvMode(true);
        HAL_LOGD("currentMainTimestamp:%" PRId64
                 ", currentAuxTimestamp=%" PRId64,
                 currentMainTimestamp, currentAuxTimestamp);
        if (currentMainTimestamp < currentAuxTimestamp) {
            HAL_LOGV("start main, idx:%d", req_main.frame_number);
            hwiMain->setZslBufferTimestamp(currentMainTimestamp);
            rc = hwiMain->process_capture_request(
                m_pPhyCamera[CAM_TYPE_MAIN].dev, &req_main);
            if (rc < 0) {
                HAL_LOGE("failed, idx:%d", req_main.frame_number);
                goto req_fail;
            }
            hwiAux->setZslBufferTimestamp(currentMainTimestamp);
            HAL_LOGV("start sub, idx:%d", req_aux.frame_number);
            rc = hwiAux->process_capture_request(m_pPhyCamera[CAM_TYPE_AUX].dev,
                                                 &req_aux);
            if (rc < 0) {
                HAL_LOGE("failed, idx:%d", req_aux.frame_number);
                goto req_fail;
            }
        } else {
            HAL_LOGV("start sub, idx:%d", req_aux.frame_number);
            hwiAux->setZslBufferTimestamp(currentAuxTimestamp);
            rc = hwiAux->process_capture_request(m_pPhyCamera[CAM_TYPE_AUX].dev,
                                                 &req_aux);
            if (rc < 0) {
                HAL_LOGE("failed, idx:%d", req_aux.frame_number);
                goto req_fail;
            }
            hwiMain->setZslBufferTimestamp(currentAuxTimestamp);
            HAL_LOGV("start main, idx:%d", req_main.frame_number);
            rc = hwiMain->process_capture_request(
                m_pPhyCamera[CAM_TYPE_MAIN].dev, &req_main);
            if (rc < 0) {
                HAL_LOGE("failed, idx:%d", req_main.frame_number);
                goto req_fail;
            }
        }
    } else {
        HAL_LOGV("start main, idx:%d", req_main.frame_number);
        rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                              &req_main);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_main.frame_number);
            goto req_fail;
        }
        HAL_LOGV("start sub, idx:%d", req_aux.frame_number);
        rc = hwiAux->process_capture_request(m_pPhyCamera[CAM_TYPE_AUX].dev,
                                             &req_aux);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_aux.frame_number);
            goto req_fail;
        }
    }

req_fail:
    HAL_LOGD("rc, d%d", rc);

    if (req_aux.output_buffers != NULL) {
        free((void *)req_aux.output_buffers);
        req_aux.output_buffers = NULL;
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
void SprdCamera3Capture::notifyMain(const camera3_notify_msg_t *msg) {
    if (msg->type == CAMERA3_MSG_SHUTTER &&
        msg->message.shutter.frame_number ==
            mCaptureThread->mSavedCapRequest.frame_number) {
        if (mCaptureThread->mReprocessing) {
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
 * PARAMETERS : capture result structure from hwi1
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Capture::processCaptureResultMain(
    const camera3_capture_result_t *result) {
    uint64_t result_timestamp;
    uint32_t cur_frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    /*save face info for 3d capture */
    CameraMetadata metadata;
    metadata = result->result;
    if ((result->result) &&
        metadata.exists(ANDROID_STATISTICS_FACE_RECTANGLES)) {
        g_face_info[0] =
            metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[0];
        g_face_info[1] =
            metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[1];
        g_face_info[2] =
            metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[2];
        g_face_info[3] =
            metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[3];
        HAL_LOGD("sx %d sy %d ex %d ey %d", g_face_info[0], g_face_info[1],
                 g_face_info[2], g_face_info[3]);
    }

    /* Direclty pass preview buffer and meta result for Main camera */
    if (result_buffer == NULL) {
        if (result->frame_number ==
                mCaptureThread->mSavedCapRequest.frame_number &&
            0 != result->frame_number) {
            if (mCaptureThread->mReprocessing) {
                HAL_LOGD("hold yuv picture call back, framenumber:%d",
                         result->frame_number);
                if (NULL != mCaptureThread->mSavedCapReqsettings) {
                    free_camera_metadata(mCaptureThread->mSavedCapReqsettings);
                    mCaptureThread->mSavedCapReqsettings = NULL;
                }
                return;
            } else {
                mCaptureThread->mCallbackOps->process_capture_result(
                    mCaptureThread->mCallbackOps, result);
                return;
            }
        }
        mCaptureThread->mCallbackOps->process_capture_result(
            mCaptureThread->mCallbackOps, result);
        return;
    }

    int currStreamType = getStreamType(result_buffer->stream);
    if (mIsCapturing && currStreamType == DEFAULT_STREAM) {
        HAL_LOGD("framenumber:%d, currStreamType:%d, stream:%p, buffer:%p, "
                 "format:%d",
                 result->frame_number, currStreamType, result_buffer->stream,
                 result_buffer->buffer, result_buffer->stream->format);
        HAL_LOGD("main yuv picture: format:%d, width:%d, height:%d, buff:%p",
                 result_buffer->stream->format, result_buffer->stream->width,
                 result_buffer->stream->height, result_buffer->buffer);
        HAL_LOGD("main capture result frame:%d, mSavedResult:%p",
                 result->frame_number, mCaptureThread->mSavedResultBuff);
        if (NULL == mCaptureThread->mSavedResultBuff) {
            mCaptureThread->mSavedResultBuff = result->output_buffers->buffer;
        } else {
            capture_queue_msg_t capture_msg;
            capture_msg.msg_type = CAPTURE_MSG_DATA_PROC;
            capture_msg.combo_buff.frame_number = result->frame_number;
            capture_msg.combo_buff.buffer1 = result->output_buffers->buffer;
            capture_msg.combo_buff.buffer2 = mCaptureThread->mSavedResultBuff;
            capture_msg.combo_buff.input_buffer = result->input_buffer;
            HAL_LOGD("capture combined begin: framenumber %d",
                     capture_msg.combo_buff.frame_number);
            HAL_LOGD("capture combined begin: buff1 %p",
                     capture_msg.combo_buff.buffer1);
            HAL_LOGD("capture combined begin: buff2 %p",
                     capture_msg.combo_buff.buffer2);
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
        camera3_capture_result_t newResult;
        camera3_stream_buffer_t newOutput_buffers;

        memset(&newResult, 0, sizeof(camera3_capture_result_t));
        memset(&newOutput_buffers, 0, sizeof(camera3_stream_buffer_t));

        HAL_LOGD("result framenumber:%d, result num_output_buffers:%d, result "
                 "output_buffers:%p",
                 result->frame_number, result->num_output_buffers,
                 &result->output_buffers[0]);
        memcpy(&newResult, result, sizeof(camera3_capture_result_t));
        memcpy(&newOutput_buffers, &result->output_buffers[0],
               sizeof(camera3_stream_buffer_t));

        HAL_LOGD("result stream:%p, result buffer:%p, savedreqstream:%p",
                 result->output_buffers[0].stream,
                 result->output_buffers[0].buffer,
                 mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1]);
        memcpy(mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1],
               result->output_buffers[0].stream, sizeof(camera3_stream_t));
        newOutput_buffers.stream =
            mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1];

        memcpy((void *)&newResult.output_buffers[0], &newOutput_buffers,
               sizeof(camera3_stream_buffer_t));
        newResult.input_buffer = NULL;
        newResult.result = NULL;
        newResult.partial_result = 0;
        HAL_LOGD("framenumber:%d, buffer:%p, stream:%p, streamtype:%d",
                 newResult.frame_number, newResult.output_buffers[0].buffer,
                 newResult.output_buffers[0].stream,
                 newResult.output_buffers[0].stream->stream_type);
        mCaptureThread->mCallbackOps->process_capture_result(
            mCaptureThread->mCallbackOps, &newResult);
        mCaptureThread->mReprocessing = false;
        mIsCapturing = false;
    } else {
        camera3_capture_result_t newResult;
        camera3_stream_buffer_t newOutput_buffers;
        memset(&newResult, 0, sizeof(camera3_capture_result_t));
        memset(&newOutput_buffers, 0, sizeof(camera3_stream_buffer_t));

        int request_preview_id = -1;
        {
            Mutex::Autolock l(mRequestLock);
            List<request_saved_t>::iterator i = mSavedRequestList.begin();
            while (i != mSavedRequestList.end()) {
                if (i->frame_number == result->frame_number) {
                    request_preview_id = i->preview_id;
                    if (CAM_TYPE_MAIN == request_preview_id) {
                        memcpy(&newResult, result,
                               sizeof(camera3_capture_result_t));
                        memcpy(&newOutput_buffers, &result->output_buffers[0],
                               sizeof(camera3_stream_buffer_t));
                        newOutput_buffers.stream = i->stream;
                        memcpy(newOutput_buffers.stream,
                               result->output_buffers[0].stream,
                               sizeof(camera3_stream_t));
                        newResult.output_buffers = &newOutput_buffers;
                        mCaptureThread->mCallbackOps->process_capture_result(
                            mCaptureThread->mCallbackOps, &newResult);
                        mSavedRequestList.erase(i);
                        mHasSendFrameNumber = result->frame_number;
                        if (result->frame_number == mWaitFrameNumber &&
                            mWaitFrameNumber != 0) {
                            mWaitFrameSignal.signal();
                            mWaitFrameNumber = 0;
                            HAL_LOGD("wake up request");
                        }
                    }
                    HAL_LOGD("find preview frame %d, show preview id:%d",
                             result->frame_number, request_preview_id);
                    break;
                }
                i++;
            }
        }
        if (CAM_TYPE_MAIN != request_preview_id) {
            HAL_LOGD("not in capture, just discard frame:%d",
                     result->frame_number);
            mLocalBufferList.push_back(result->output_buffers[0].buffer);
        }
    }

    return;
}
/*===========================================================================
 * FUNCTION   :~SprdCamera3Capture
 *
 * DESCRIPTION: deconstructor of SprdCamera3Capture
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Capture::notifyAux(const camera3_notify_msg_t *msg) { return; }
/*===========================================================================
 * FUNCTION   :processCaptureResultMain
 *
 * DESCRIPTION: process Capture Result from the aux hwi
 *
 * PARAMETERS : capture result structure from hwi2
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Capture::processCaptureResultAux(
    const camera3_capture_result_t *result) {
    uint64_t result_timestamp;
    uint32_t cur_frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    /* Direclty pass meta result for Aux camera */
    if (result->num_output_buffers == 0) {
        return;
    }
    int currStreamType = getStreamType(result->output_buffers->stream);
    if (mIsCapturing && currStreamType == DEFAULT_STREAM) {
        const camera3_stream_buffer_t *result_buffer = result->output_buffers;

        HAL_LOGD("framenumber:%d, currStreamType:%d, stream:%p, buffer:%p, "
                 "format:%d",
                 result->frame_number, currStreamType,
                 result->output_buffers->stream, result->output_buffers->buffer,
                 result->output_buffers->stream->format);
        HAL_LOGD("aux yuv picture: format:%d, width:%d, height:%d, buff:%p",
                 result_buffer->stream->format, result_buffer->stream->width,
                 result_buffer->stream->height, result_buffer->buffer);
        HAL_LOGD("aux capture result:%d, mSavedResult:%p", result->frame_number,
                 mCaptureThread->mSavedResultBuff);
        if (NULL == mCaptureThread->mSavedResultBuff) {
            mCaptureThread->mSavedResultBuff = result->output_buffers->buffer;
        } else {
            capture_queue_msg_t capture_msg;
            capture_msg.msg_type = CAPTURE_MSG_DATA_PROC;
            capture_msg.combo_buff.frame_number = result->frame_number;
            capture_msg.combo_buff.buffer1 = mCaptureThread->mSavedResultBuff;
            capture_msg.combo_buff.buffer2 = result->output_buffers->buffer;
            capture_msg.combo_buff.input_buffer = result->input_buffer;
            HAL_LOGD("capture combined begin: framenumber %d",
                     capture_msg.combo_buff.frame_number);
            HAL_LOGD("capture combined begin: buff1 %p",
                     capture_msg.combo_buff.buffer1);
            HAL_LOGD("capture combined begin: buff2 %p",
                     capture_msg.combo_buff.buffer2);
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
    } else {
        camera3_capture_result_t newResult;
        camera3_stream_buffer_t newOutput_buffers;
        memset(&newResult, 0, sizeof(camera3_capture_result_t));
        memset(&newOutput_buffers, 0, sizeof(camera3_stream_buffer_t));

        int request_preview_id = -1;
        {
            Mutex::Autolock l(mRequestLock);
            List<request_saved_t>::iterator i = mSavedRequestList.begin();
            while (i != mSavedRequestList.end()) {
                if (i->frame_number == result->frame_number) {
                    request_preview_id = i->preview_id;
                    if (CAM_TYPE_AUX == request_preview_id) {
                        memcpy(&newResult, result,
                               sizeof(camera3_capture_result_t));
                        memcpy(&newOutput_buffers, &result->output_buffers[0],
                               sizeof(camera3_stream_buffer_t));
                        newOutput_buffers.stream = i->stream;
                        memcpy(newOutput_buffers.stream,
                               result->output_buffers[0].stream,
                               sizeof(camera3_stream_t));
                        newResult.output_buffers = &newOutput_buffers;
                        mCaptureThread->mCallbackOps->process_capture_result(
                            mCaptureThread->mCallbackOps, &newResult);
                        mSavedRequestList.erase(i);
                        mHasSendFrameNumber = result->frame_number;
                        if (result->frame_number == mWaitFrameNumber &&
                            mWaitFrameNumber != 0) {
                            mWaitFrameSignal.signal();
                            mWaitFrameNumber = 0;
                            HAL_LOGD("wake up request");
                        }
                    }
                    HAL_LOGD("find preview frame %d, show preview id:%d",
                             result->frame_number, request_preview_id);
                    break;
                }
                i++;
            }
        }
        if (CAM_TYPE_AUX != request_preview_id) {
            HAL_LOGD("not in capture, just discard frame:%d",
                     result->frame_number);
            mLocalBufferList.push_back(result->output_buffers[0].buffer);
        }
    }
    return;
}
/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: deconstructor of SprdCamera3Capture
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Capture::_dump(const struct camera3_device *device, int fd) {

    HAL_LOGD(" E");

    HAL_LOGD("X");
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
void SprdCamera3Capture::dumpImg(void *addr, int size, int frameId) {

    HAL_LOGD(" E");
    char name[128];
    snprintf(name, sizeof(name), "/data/misc/media/%d_%d.yuv", size, frameId);

    FILE *file_fd = fopen(name, "w");

    if (file_fd == NULL) {
        HAL_LOGE("open yuv file fail!\n");
    }
    int count = fwrite(addr, 1, size, file_fd);
    if (count != size) {
        HAL_LOGE("write dst.yuv fail\n");
    }
    fclose(file_fd);

    HAL_LOGD("X");
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
int SprdCamera3Capture::_flush(const struct camera3_device *device) {
    int rc = 0;
    HAL_LOGD("E");

    HAL_LOGD("flush, mCaptureMsgList.size=%zu, mSavedRequestList.size:%zu",
             mCaptureThread->mCaptureMsgList.size(), mSavedRequestList.size());

    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_MAIN].dev);

    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    rc = hwiAux->flush(m_pPhyCamera[CAM_TYPE_AUX].dev);

    if (mCaptureThread != NULL) {
        if (mCaptureThread->isRunning()) {
            mCaptureThread->requestExit();
        }
    }
    HAL_LOGD("X");
    return rc;
}
};
