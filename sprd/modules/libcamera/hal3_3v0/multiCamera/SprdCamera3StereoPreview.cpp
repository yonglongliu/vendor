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
#define LOG_TAG "SprdCamera3StereoPreview"
//#define LOG_NDEBUG 0
#include "SprdCamera3StereoPreview.h"

using namespace android;
namespace sprdcamera {

SprdCamera3StereoPreview *mPreviewMuxer = NULL;

// Error Check Macros
#define CHECK_MUXER()                                                          \
    if (!mPreviewMuxer) {                                                      \
        HAL_LOGE("Error getting muxer ");                                      \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_MUXER_ERROR()                                                    \
    if (!mPreviewMuxer) {                                                      \
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

camera3_device_ops_t SprdCamera3StereoPreview::mCameraMuxerOps = {
    .initialize = SprdCamera3StereoPreview::initialize,
    .configure_streams = SprdCamera3StereoPreview::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3StereoPreview::construct_default_request_settings,
    .process_capture_request =
        SprdCamera3StereoPreview::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3StereoPreview::dump,
    .flush = SprdCamera3StereoPreview::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3StereoPreview::callback_ops_main = {
    .process_capture_result =
        SprdCamera3StereoPreview::process_capture_result_main,
    .notify = SprdCamera3StereoPreview::notifyMain};

camera3_callback_ops SprdCamera3StereoPreview::callback_ops_aux = {
    .process_capture_result =
        SprdCamera3StereoPreview::process_capture_result_aux,
    .notify = SprdCamera3StereoPreview::notifyAux};

/*===========================================================================
 * FUNCTION         : SprdCamera3StereoPreview
 *
 * DESCRIPTION     : SprdCamera3StereoPreview Constructor
 *
 * PARAMETERS:
 *   @num_of_cameras  : Number of Physical Cameras on device
 *
 *==========================================================================*/
SprdCamera3StereoPreview::SprdCamera3StereoPreview() {
    HAL_LOGD(" E");

    m_nPhyCameras = 2; // m_nPhyCameras should always be 2 with dual camera mode
    mStaticMetadata = NULL;
    mPreviewMuxerThread = new MuxerThread();
    mMaxLocalBufferNum = MAX_PREVIEW_QEQUEST_BUF;
    mLocalBuffer = NULL;
    mLocalBufferList.clear();
    mOldPreviewRequestList.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    setupPhysicalCameras();
    m_VirtualCamera.id = CAM_MAIN_ID; // hardcode left front camera id here

    HAL_LOGD("X");
}

/*===========================================================================
 * FUNCTION         : ~SprdCamera3StereoPreview
 *
 * DESCRIPTION     : SprdCamera3StereoPreview Desctructor
 *
 *==========================================================================*/
SprdCamera3StereoPreview::~SprdCamera3StereoPreview() {
    HAL_LOGD("E");
    mNotifyListAux.clear();
    mNotifyListMain.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mLocalBufferList.clear();
    mOldPreviewRequestList.clear();
    if (m_pPhyCamera) {
        delete[] m_pPhyCamera;
        m_pPhyCamera = NULL;
    }
    HAL_LOGD("X");
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
void SprdCamera3StereoPreview::freeLocalBuffer(
    new_mem_t *LocalBuffer, List<buffer_handle_t *> &bufferList,
    int bufferNum) {
    HAL_LOGD("free local buffer,bufferNum=%d", bufferNum);
    bufferList.clear();
    if (LocalBuffer != NULL) {
        for (int i = 0; i < bufferNum; i++) {
            if (LocalBuffer[i].native_handle != NULL) {
                delete ((private_handle_t *)*(&(LocalBuffer[i].native_handle)));
                LocalBuffer[i].native_handle = NULL;
            }
            if (LocalBuffer[i].pHeapIon != NULL) {
                delete LocalBuffer[i].pHeapIon;
                LocalBuffer[i].pHeapIon = NULL;
            }
        }
    } else {
        HAL_LOGD("Not allocated, No need to free");
    }
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
void SprdCamera3StereoPreview::getCameraMuxer(
    SprdCamera3StereoPreview **pMuxer) {
    *pMuxer = NULL;
    if (!mPreviewMuxer) {
        mPreviewMuxer = new SprdCamera3StereoPreview();
    }
    CHECK_MUXER();
    *pMuxer = mPreviewMuxer;
    HAL_LOGD("mPreviewMuxer: %p ", mPreviewMuxer);

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
int SprdCamera3StereoPreview::get_camera_info(__unused int camera_id,
                                              struct camera_info *info) {

    int rc = NO_ERROR;

    HAL_LOGD("E");
    if (info) {
        rc = mPreviewMuxer->getCameraInfo(info);
    }
    HAL_LOGD("X, rc: %d", rc);

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
int SprdCamera3StereoPreview::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    HAL_LOGD("id= %d", atoi(id));
    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }
    rc = mPreviewMuxer->cameraDeviceOpen(atoi(id), hw_device);
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
int SprdCamera3StereoPreview::close_camera_device(
    __unused hw_device_t *hw_dev) {
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }
    return mPreviewMuxer->closeCameraDevice();
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
int SprdCamera3StereoPreview::closeCameraDevice() {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;

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
    mUnmatchedFrameListAux.clear();
    mUnmatchedFrameListMain.clear();
    mOldPreviewRequestList.clear();
    freeLocalBuffer(mLocalBuffer, mLocalBufferList, MAX_PREVIEW_QEQUEST_BUF);
    if (mLocalBuffer != NULL) {
        free(mLocalBuffer);
        mLocalBuffer = NULL;
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
int SprdCamera3StereoPreview::initialize(
    __unused const struct camera3_device *device,
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;
    HAL_LOGD("E");
    CHECK_MUXER_ERROR();
    rc = mPreviewMuxer->initialize(callback_ops);
    HAL_LOGD("X");
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
SprdCamera3StereoPreview::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *rc;
    HAL_LOGD("E");
    if (!mPreviewMuxer) {
        HAL_LOGE("Error getting muxer ");
        return NULL;
    }
    rc = mPreviewMuxer->constructDefaultRequestSettings(device, type);
    HAL_LOGD("X");
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
int SprdCamera3StereoPreview::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;
    HAL_LOGD(" E");
    CHECK_MUXER_ERROR();
    rc = mPreviewMuxer->configureStreams(device, stream_list);
    HAL_LOGD(" X");
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
int SprdCamera3StereoPreview::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;
    HAL_LOGV("idx:%d", request->frame_number);
    CHECK_MUXER_ERROR();
    rc = mPreviewMuxer->processCaptureRequest(device, request);
    return rc;
}
/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3StereoPreview
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3StereoPreview::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_MUXER();
    mPreviewMuxer->processCaptureResultMain(result);
}
/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3StereoPreview
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3StereoPreview::process_capture_result_aux(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_MUXER();
    mPreviewMuxer->processCaptureResultAux(result);
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
void SprdCamera3StereoPreview::notifyMain(
    const struct camera3_callback_ops *ops, const camera3_notify_msg_t *msg) {
    CHECK_MUXER();
    mPreviewMuxer->notifyMain(msg);
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
void SprdCamera3StereoPreview::notifyAux(const struct camera3_callback_ops *ops,
                                         const camera3_notify_msg_t *msg) {
    CHECK_MUXER();
    mPreviewMuxer->notifyAux(msg);
}

/*===========================================================================
 * FUNCTION   :popRequestList
 *
 * DESCRIPTION: deconstructor of SprdCamera3StereoPreview
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
buffer_handle_t *
SprdCamera3StereoPreview::popRequestList(List<buffer_handle_t *> &list) {
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
 * DESCRIPTION: deconstructor of SprdCamera3StereoPreview
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3StereoPreview::allocateOne(int w, int h, uint32_t is_cache,
                                          new_mem_t *new_mem) {
    int result = 0;
    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;
    private_handle_t *buffer;

    HAL_LOGD("E");
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

    HAL_LOGD("X");

    return result;

getpmem_fail:
    delete pHeapIon;

    return -1;
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
int SprdCamera3StereoPreview::validateCaptureRequest(
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
            HAL_LOGE("Request: Buffer: Status not OK!");
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            HAL_LOGE("Request: Buffer: Has a release fence!");
            return BAD_VALUE;
        }
        if (b->buffer == NULL) {
            HAL_LOGE("Request: Buffer: NULL buffer handle!");
            return BAD_VALUE;
        }
    }

    // Validate all output buffers
    b = request->output_buffers;
    do {
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            HAL_LOGE("Request: Buffer: Status not OK!");
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            HAL_LOGE("Request: Buffer: Has a release fence!");
            return BAD_VALUE;
        }
        if (b->buffer == NULL) {
            HAL_LOGE("Request: Buffer: NULL buffer handle!");
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
int SprdCamera3StereoPreview::cameraDeviceOpen(__unused int camera_id,
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
    m_VirtualCamera.dev.ops = &mCameraMuxerOps;
    m_VirtualCamera.dev.priv = (void *)&m_VirtualCamera;
    *hw_device = &m_VirtualCamera.dev.common;

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
int SprdCamera3StereoPreview::getCameraInfo(struct camera_info *info) {
    int rc = NO_ERROR;
    int camera_id = m_VirtualCamera.id;
    HAL_LOGD("E, camera_id = %d", camera_id);

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
    HAL_LOGD("X");
    return rc;
}
/*===========================================================================
 * FUNCTION         : get3DPreviewSize
 *
 * DESCRIPTION     : get3DPreviewSize
 *
 *==========================================================================*/
void SprdCamera3StereoPreview::get3DPreviewSize(int *pWidth, int *pHeight) {
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

    HAL_LOGD("3d preview size, w:%d, h:%d", *pWidth, *pHeight);
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
int SprdCamera3StereoPreview::setupPhysicalCameras() {
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
 * FUNCTION   :ReProcessThread
 *
 * DESCRIPTION: constructor of ReProcessThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3StereoPreview::ReProcessThread::ReProcessThread() {
    HAL_LOGD(" E");
    mVFrameCount = 0;
    mVLastFpsTime = 0;
    mReprocessMsgList.clear();

    HAL_LOGD("X");
}
/*===========================================================================
 * FUNCTION   :~ReProcessThread
 *
 * DESCRIPTION: deconstructor of ReProcessThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3StereoPreview::ReProcessThread::~ReProcessThread() {
    HAL_LOGD(" E");

    mReprocessMsgList.clear();

    HAL_LOGD("X");
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

bool SprdCamera3StereoPreview::ReProcessThread::threadLoop() {
    muxer_queue_msg_t muxer_msg;

    while (!mReprocessMsgList.empty()) {
        List<muxer_queue_msg_t>::iterator it;
        {
            Mutex::Autolock l(mMutex);
            it = mReprocessMsgList.begin();
            muxer_msg = *it;
            mReprocessMsgList.erase(it);
        }
        switch (muxer_msg.msg_type) {
        case MUXER_MSG_EXIT:
            if (mReprocessMsgList.size() != 0) {
                for (List<muxer_queue_msg_t>::iterator i =
                         mReprocessMsgList.begin();
                     i != mReprocessMsgList.end(); i++) {
                    muxer_msg = *i;
                    CallBackResult(&muxer_msg.combo_frame);
                }
            }
            return false;
            break;
        case MUXER_MSG_DATA_PROC: {
#ifdef CONFIG_FACE_BEAUTY
            if (reProcessFrame(muxer_msg.combo_frame.buffer1,
                               muxer_msg.combo_frame.frame_number) !=
                NO_ERROR) {
                HAL_LOGD("frameid: reprocess frame failed!");
            }
#endif
            CallBackResult(&muxer_msg.combo_frame);
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
void SprdCamera3StereoPreview::ReProcessThread::requestExit() {

    Mutex::Autolock l(mMutex);
    muxer_queue_msg_t muxer_msg;
    muxer_msg.msg_type = MUXER_MSG_EXIT;
    mReprocessMsgList.push_back(muxer_msg);
    mSignal.signal();
}

#ifdef CONFIG_FACE_BEAUTY
/*===========================================================================
 * FUNCTION   :preview_3d_convert_face_info
 *
 * DESCRIPTION:
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoPreview::ReProcessThread::preview_3d_convert_face_info(
    int *ptr_cam_face_inf, int width, int height) {
    float zoomWidth, zoomHeight;
    uint32_t fdWidth, fdHeight, sensorOrgW, sensorOrgH;

    sensorOrgW = mPreviewMuxer->mPreviewSize.stereoPreviewWidth * 2;
    sensorOrgH = mPreviewMuxer->mPreviewSize.stereoPreviewHeight * 2;
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

    return;
}

/*===========================================================================
 * FUNCTION   :preview_3d_doFaceMakeup
 *
 * DESCRIPTION:
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoPreview::ReProcessThread::preview_3d_doFaceMakeup(
    private_handle_t *private_handle, int perfect_level, int *face_info) {
    // init the parameters table. save the value until the process is restart or
    // the device is restart.
    /*
    int tab_skinWhitenLevel[10] = {0, 15, 25, 35, 45, 55, 65, 75, 85, 95};
    int tab_skinCleanLevel[10] = {0, 25, 45, 50, 55, 60, 70, 80, 85, 95};
    struct camera_frame_type preview_3d_frame;
    bzero(&preview_3d_frame, sizeof(struct camera_frame_type));
    struct camera_frame_type *frame = &preview_3d_frame;
    frame->y_vir_addr = (cmr_uint)private_handle->base;
    frame->width = private_handle->width;
    frame->height = private_handle->height;

    TSRect Tsface;
    YuvFormat yuvFormat = TSFB_FMT_NV21;
    if (face_info[0] != 0 || face_info[1] != 0 || face_info[2] != 0 ||
        face_info[3] != 0) {
        preview_3d_convert_face_info(face_info, frame->width, frame->height);
        Tsface.left = face_info[0];
        Tsface.top = face_info[1];
        Tsface.right = face_info[2];
        Tsface.bottom = face_info[3];
        // HAL_LOGD("FACE_BEAUTY
        // rect:%d-%d-%d-%d",Tsface.left,Tsface.top,Tsface.right,Tsface.bottom);

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
            HAL_LOGE("No face beauty!  If size is not zero, then "
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
int SprdCamera3StereoPreview::ReProcessThread::reProcessFrame(
    const buffer_handle_t *frame_buffer, uint32_t cur_frameid) {
    int rc = 0;
    int32_t perfectskinlevel = 0;
    int32_t face_info[4];
    List<old_request>::iterator itor;

    {
        Mutex::Autolock l(mPreviewMuxer->mRequest);
        itor = mPreviewMuxer->mOldPreviewRequestList.begin();
        while (itor != mPreviewMuxer->mOldPreviewRequestList.end()) {
            if (itor->frame_number == cur_frameid) {
                perfectskinlevel = itor->perfectskinlevel;
                face_info[0] = itor->g_face_info[0];
                face_info[1] = itor->g_face_info[1];
                face_info[2] = itor->g_face_info[2];
                face_info[3] = itor->g_face_info[3];
                HAL_LOGD("perfectskinlevel=%d face:sx %d sy %d ex %d ey %d",
                         perfectskinlevel, face_info[0], face_info[1],
                         face_info[2], face_info[3]);
                break;
            }
            itor++;
        }
    }
    private_handle_t *private_handle =
        (struct private_handle_t *)(*frame_buffer);
    //if (perfectskinlevel > 0)
        //preview_3d_doFaceMakeup(private_handle, perfectskinlevel, face_info);

    return rc;
}
#endif

/*===========================================================================
 * FUNCTION   :Video_CallBack_Result
 *
 * DESCRIPTION: Video_CallBack_Result
 *
 * PARAMETERS : buffer: the old request buffer from framework
        muxer_msg.combo_frame:the node of list
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoPreview::ReProcessThread::CallBackResult(
    frame_matched_info_t *combPreviewResult) {

    camera3_buffer_status_t buffer_status = CAMERA3_BUFFER_STATUS_OK;
    camera3_capture_result_t result;
    List<old_request>::iterator itor;
    camera3_stream_buffer_t result_buffers;
    {
        Mutex::Autolock l(mPreviewMuxer->mRequest);
        itor = mPreviewMuxer->mOldPreviewRequestList.begin();
        while (itor != mPreviewMuxer->mOldPreviewRequestList.end()) {
            if (itor->frame_number == combPreviewResult->frame_number) {
                if (itor->invalid == true) {
                    return;
                }
                itor->invalid = true;
                break;
            }
            itor++;
        }
    }

    if (itor->stream && itor->buffer) {
        result_buffers.stream = itor->stream;
        result_buffers.buffer = combPreviewResult->buffer1;
        result_buffers.status = buffer_status;
        result_buffers.acquire_fence = -1;
        result_buffers.release_fence = -1;
        result.result = NULL;
        result.frame_number = combPreviewResult->frame_number;
        result.num_output_buffers = 1;
        result.output_buffers = &result_buffers;
        result.input_buffer = itor->input_buffer;
        result.partial_result = 0;
        dumpFps();
        HAL_LOGD(":%d. return preview stream=%p,buffer=%p", result.frame_number,
                 result_buffers.stream, result_buffers.buffer);
        mPreviewMuxer->mCallbackOps->process_capture_result(
            mPreviewMuxer->mCallbackOps, &result);
    }
    if (itor->callback_stream && itor->callback_buffer) {
        camera3_capture_result_t callback_result;
        camera3_stream_buffer_t callback_result_buffers;

        if (itor->stream && itor->buffer) {
            memcpy(((struct private_handle_t *)(*itor->callback_buffer))->base,
                   ((struct private_handle_t *)(*result_buffers.buffer))->base,
                   ((struct private_handle_t *)(*result_buffers.buffer))->size);
            callback_result_buffers.buffer = itor->callback_buffer;
        } else {
            result_buffers.buffer = combPreviewResult->buffer1;
        }
        callback_result_buffers.stream = itor->callback_stream;
        callback_result_buffers.status = buffer_status;
        callback_result_buffers.acquire_fence = -1;
        callback_result_buffers.release_fence = -1;
        callback_result.result = NULL;
        callback_result.frame_number = combPreviewResult->frame_number;
        callback_result.num_output_buffers = 1;
        callback_result.output_buffers = &callback_result_buffers;
        callback_result.input_buffer = itor->input_buffer;
        callback_result.partial_result = 0;

        mPreviewMuxer->mCallbackOps->process_capture_result(
            mPreviewMuxer->mCallbackOps, &callback_result);
        HAL_LOGD(":%d return callbackstream=%p,buffer=%p", result.frame_number,
                 callback_result_buffers.stream,
                 callback_result_buffers.buffer);
    }
    {
        Mutex::Autolock l(mPreviewMuxer->mRequestLock);
        mPreviewMuxer->mPendingRequest--;
        if (mPreviewMuxer->mPendingRequest < mPreviewMuxer->mMaxPendingCount) {
            HAL_LOGV("signal request m_PendingRequest = %d",
                     mPreviewMuxer->mPendingRequest);
            mPreviewMuxer->mRequestSignal.signal();
        }
    }
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
void SprdCamera3StereoPreview::ReProcessThread::waitMsgAvailable() {
    while (mReprocessMsgList.empty()) {
        Mutex::Autolock l(mMutex);
        mSignal.waitRelative(mMutex, THREAD_TIMEOUT);
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
void SprdCamera3StereoPreview::ReProcessThread::dumpFps() {
    mVFrameCount++;
    int64_t now = systemTime();
    int64_t diff = now - mVLastFpsTime;
    if (diff > ms2ns(1000)) {
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
 * FUNCTION   :MuxerThread
 *
 * DESCRIPTION: constructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3StereoPreview::MuxerThread::MuxerThread() {
    mPreviewMuxerMsgList.clear();
    mGpuApi = (GPUAPI_t *)malloc(sizeof(GPUAPI_t));
    if (mGpuApi == NULL) {
        HAL_LOGE("mGpuApi malloc failed.");
    }
    memset(mGpuApi, 0, sizeof(GPUAPI_t));

    if (loadGpuApi() < 0) {
        HAL_LOGE("load gpu api failed.");
    }
    mReProcessThread = new ReProcessThread();
    HAL_LOGD("X");
}
/*===========================================================================
 * FUNCTION   :~~MuxerThread
 *
 * DESCRIPTION: deconstructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3StereoPreview::MuxerThread::~MuxerThread() {
    HAL_LOGD(" E");
    mPreviewMuxerMsgList.clear();
    if (mGpuApi != NULL) {
        unLoadGpuApi();
        free(mGpuApi);
        mGpuApi = NULL;
    }
    HAL_LOGD("X");
}
/*===========================================================================
 * FUNCTION   :loadGpuApi
 *
 * DESCRIPTION: loadGpuApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3StereoPreview::MuxerThread::loadGpuApi() {
    HAL_LOGD(" E");
    const char *error = NULL;
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
void SprdCamera3StereoPreview::MuxerThread::unLoadGpuApi() {
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
void SprdCamera3StereoPreview::MuxerThread::initGpuData() {
    pt_stream_info.dst_height = mPreviewMuxer->mPreviewSize.srcHeight;
    pt_stream_info.dst_width = mPreviewMuxer->mPreviewSize.srcWidth;
    pt_stream_info.src_height = mPreviewMuxer->mPreviewSize.stereoPreviewHeight;
    pt_stream_info.src_width = mPreviewMuxer->mPreviewSize.stereoPreviewWidth;
    pt_stream_info.rot_angle = ROT_270;
    HAL_LOGV("src_width = %d dst_height=%d,dst_width=%d,dst_height=%d",
             pt_stream_info.src_width, pt_stream_info.src_height,
             pt_stream_info.dst_width, pt_stream_info.dst_height);
    float buff[768];
    float H_left[9], H_right[9];
    FILE *fid =
        fopen("/productinfo/sprd_3d_calibration/calibration.data", "rb");
    if (fid != NULL) {
        HAL_LOGE("open calibration file success");
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
    if (!isInitRenderContest) {
        if (CONTEXT_FAIL ==
            mGpuApi->initRenderContext(&pt_stream_info,
                                       pt_line_buf.homography_matrix, 18)) {
            HAL_LOGE("initRenderContext fail");
        } else {
            isInitRenderContest = true;
        }
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
/*===========================================================================
 * FUNCTION   :threadLoop
 *
 * DESCRIPTION: threadLoop
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
bool SprdCamera3StereoPreview::MuxerThread::threadLoop() {

    buffer_handle_t *output_buffer = NULL;
    muxer_queue_msg_t muxer_msg;

    if (!isInitRenderContest)
        initGpuData();

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
            // flush queue
            for (List<muxer_queue_msg_t>::iterator i =
                     mPreviewMuxerMsgList.begin();
                 i != mPreviewMuxerMsgList.end();) {
                if (i != mPreviewMuxerMsgList.end()) {
                    if (muxerTwoFrame(output_buffer, &muxer_msg.combo_frame) !=
                        NO_ERROR) {
                        HAL_LOGE("Error happen when merging for frame idx:%d",
                                 muxer_msg.combo_frame.frame_number);
                        errorCallback(muxer_msg.combo_frame.frame_number);
                    } else {
                        Mutex::Autolock l(mReProcessThread->mMutex);
                        muxer_queue_msg_t reProcessMsg = *i;
                        reProcessMsg.combo_frame.buffer1 = output_buffer;
                        HAL_LOGD("Enqueue combo frame:%d for frame reprocess!",
                                 reProcessMsg.combo_frame.frame_number);
                        mReProcessThread->mReprocessMsgList.push_back(
                            reProcessMsg);
                        mReProcessThread->mSignal.signal();
                    }
                    {
                        Mutex::Autolock l(mMergequeueMutex);
                        i = mPreviewMuxerMsgList.erase(i);
                    }
                }
            }

            for (List<old_request>::iterator i =
                     mPreviewMuxer->mOldPreviewRequestList.begin();
                 i != mPreviewMuxer->mOldPreviewRequestList.end();) {
                if (!i->invalid) {
                    HAL_LOGD("flush frame_number=%d", i->frame_number);
                    errorCallback(i->frame_number);
                }
                i++;
            };
            mGpuApi->destroyRenderContext();
            isInitRenderContest = false;
            if (mReProcessThread != NULL) {
                if (mReProcessThread->isRunning()) {
                    mReProcessThread->requestExit();
                }
            }
            return false;
        } break;
        case MUXER_MSG_DATA_PROC: {
            if (muxerTwoFrame(output_buffer, &muxer_msg.combo_frame) !=
                NO_ERROR) {
                errorCallback(muxer_msg.combo_frame.frame_number);
            } else {
                Mutex::Autolock l(mReProcessThread->mMutex);
                muxer_queue_msg_t reProcessMsg = muxer_msg;
                reProcessMsg.combo_frame.buffer1 = output_buffer;
                HAL_LOGD("Enqueue combo frame:%d for frame reprocess!",
                         reProcessMsg.combo_frame.frame_number);
                mReProcessThread->mReprocessMsgList.push_back(reProcessMsg);
                mReProcessThread->mSignal.signal();
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
void SprdCamera3StereoPreview::MuxerThread::requestExit() {

    Mutex::Autolock l(mMergequeueMutex);
    muxer_queue_msg_t muxer_msg;
    muxer_msg.msg_type = MUXER_MSG_EXIT;
    mPreviewMuxerMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
}
/*===========================================================================
 * FUNCTION   :muxerTwoFrame
 *
 * DESCRIPTION: muxerTwoFrame
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3StereoPreview::MuxerThread::muxerTwoFrame(
    buffer_handle_t *&output_buf, frame_matched_info_t *combPreviewResult) {
    HAL_LOGV("E");

    int rc = NO_ERROR;
    int32_t rotation = ROT_270;
    static int32_t s_rotation = ROT_270;
    buffer_handle_t *input_buf1 = combPreviewResult->buffer1;
    buffer_handle_t *input_buf2 = combPreviewResult->buffer2;
    List<old_request>::iterator itor;
    void *addr = NULL;
    int size = 0;

    if (input_buf1 == NULL || input_buf2 == NULL || !isInitRenderContest) {
        HAL_LOGE("error,input_buf1:%p input_buf2:%p, isInitRenderContest:%d",
                 input_buf1, input_buf2, isInitRenderContest);
        return BAD_VALUE;
    }
    {
        Mutex::Autolock l(mPreviewMuxer->mRequest);

        itor = mPreviewMuxer->mOldPreviewRequestList.begin();
        while (itor != mPreviewMuxer->mOldPreviewRequestList.end()) {
            if ((itor)->frame_number == combPreviewResult->frame_number) {
                break;
            }
            itor++;
        }
    }
    // if no matching request found, return buffers
    if (itor == mPreviewMuxer->mOldPreviewRequestList.end()) {
        mPreviewMuxer->mLocalBufferList.push_back(input_buf1);
        mPreviewMuxer->mLocalBufferList.push_back(input_buf2);
        return UNKNOWN_ERROR;
    } else {

        if (itor->stream && itor->buffer) {
            output_buf = itor->buffer;
        } else {
            output_buf = itor->callback_buffer;
        }
        rotation = itor->rotation;
        if (output_buf == NULL)
            return BAD_VALUE;
    }

    dcam_info_t dcam;

    dcam.left_buf = (struct private_handle_t *)*input_buf1;
    dcam.right_buf = (struct private_handle_t *)*input_buf2;
    dcam.dst_buf = (struct private_handle_t *)*output_buf;
    if (rotation >= 0) {
        switch (rotation) {
        case 0:
            rotation = 3;
            break;
        case 90:
            rotation = 2;
            break;
        case 180:
            rotation = 1;
            break;
        case 270:
            rotation = 0;
            break;
        default:
            rotation = 3;
            break;
        }
        s_rotation = rotation;
    }

    dcam.rot_angle = s_rotation;
    mGpuApi->imageStitchingWithGPU(&dcam);
    {
        char prop[PROPERTY_VALUE_MAX] = {
            0,
        };
        property_get("debug.camera.3dpreview.saveyuv", prop, "0");
        if (1 == atoi(prop)) {
            addr = dcam.left_buf->base;
            size = dcam.left_buf->size;
            mPreviewMuxer->dumpImg(addr, size, dcam.left_buf->width,
                                   dcam.left_buf->height,
                                   combPreviewResult->frame_number, 1);
            addr = dcam.right_buf->base;
            size = dcam.right_buf->size;
            mPreviewMuxer->dumpImg(addr, size, dcam.right_buf->width,
                                   dcam.right_buf->height,
                                   combPreviewResult->frame_number, 2);
            addr = dcam.dst_buf->base;
            size = dcam.dst_buf->size;
            mPreviewMuxer->dumpImg(addr, size, dcam.dst_buf->width,
                                   dcam.dst_buf->height,
                                   combPreviewResult->frame_number, 3);
        }
    }
    {
        Mutex::Autolock l(mPreviewMuxer->mLocalBufferListLock);
        if (combPreviewResult->buffer1 != NULL) {
            mPreviewMuxer->mLocalBufferList.push_back(
                combPreviewResult->buffer1);
        }
        if (combPreviewResult->buffer2 != NULL) {
            mPreviewMuxer->mLocalBufferList.push_back(
                combPreviewResult->buffer2);
        }
    }
    HAL_LOGV("X");

    return rc;
}

/*===========================================================================
 * function   : errorCallback
 *
 * DESCRIPTION: callback to fw with an ERROR result
 *
 * PARAMETERS : camera3_capture_result_t
 *                        frame_number
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoPreview::MuxerThread::errorCallback(
    uint32_t frame_number) {
    camera3_capture_result_t result;
    List<old_request>::iterator i;
    camera3_stream_buffer_t result_buffers;
    i = mPreviewMuxer->mOldPreviewRequestList.begin();
    {
        Mutex::Autolock l(mPreviewMuxer->mRequest);
        for (; i != mPreviewMuxer->mOldPreviewRequestList.end();) {
            if (i->frame_number == frame_number) {
                if (i->invalid == true) {
                    return;
                }
                i->invalid = true;
                break;
            }
            i++;
        }
        if (i == mPreviewMuxer->mOldPreviewRequestList.end()) {
            return;
        }
    }
    if (i->stream && i->buffer) {
        result_buffers.stream = i->stream;
        result_buffers.buffer = i->buffer;
        result_buffers.status = CAMERA3_BUFFER_STATUS_ERROR;
        result_buffers.acquire_fence = -1;
        result_buffers.release_fence = -1;
        result.output_buffers = &result_buffers;
        result.result = NULL;
        result.frame_number = frame_number;
        result.num_output_buffers = 1;
        result.input_buffer = i->input_buffer;
        result.partial_result = 0;

        HAL_LOGD(":%d preview.stream=%p,buffer=%p", result.frame_number,
                 result_buffers.stream, result_buffers.buffer);
        mPreviewMuxer->mCallbackOps->process_capture_result(
            mPreviewMuxer->mCallbackOps, &result);
    }
    if (i->callback_stream && i->callback_buffer) {
        camera3_capture_result_t callback_result;
        camera3_stream_buffer_t callback_result_buffers;
        callback_result_buffers.stream = i->callback_stream;
        callback_result_buffers.buffer = i->callback_buffer;
        callback_result_buffers.status = CAMERA3_BUFFER_STATUS_ERROR;
        callback_result_buffers.acquire_fence = -1;
        callback_result_buffers.release_fence = -1;
        callback_result.result = NULL;
        callback_result.frame_number = frame_number;
        callback_result.num_output_buffers = 1;
        callback_result.output_buffers = &callback_result_buffers;
        callback_result.input_buffer = i->input_buffer;
        callback_result.partial_result = 0;
        mPreviewMuxer->mCallbackOps->process_capture_result(
            mPreviewMuxer->mCallbackOps, &callback_result);
        HAL_LOGD(":%d callback.stream=%p,buffer=%p", result.frame_number,
                 callback_result_buffers.stream,
                 callback_result_buffers.buffer);
    }
    {
        Mutex::Autolock l(mPreviewMuxer->mRequestLock);
        mPreviewMuxer->mPendingRequest--;
        if (mPreviewMuxer->mPendingRequest < mPreviewMuxer->mMaxPendingCount) {
            HAL_LOGV("signal request m_PendingRequest = %d",
                     mPreviewMuxer->mPendingRequest);
            mPreviewMuxer->mRequestSignal.signal();
        }
    }
    return;
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
void SprdCamera3StereoPreview::MuxerThread::waitMsgAvailable() {
    while (mPreviewMuxerMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, THREAD_TIMEOUT);
    }
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
int SprdCamera3StereoPreview::getStreamType(camera3_stream_t *new_stream) {
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
    HAL_LOGD("xxstream_type=%d", stream_type);
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
bool SprdCamera3StereoPreview::matchTwoFrame(
    hwi_frame_buffer_info_t result1, List<hwi_frame_buffer_info_t> &list,
    hwi_frame_buffer_info_t *result2) {
    List<hwi_frame_buffer_info_t>::iterator itor2;

    if (list.empty()) {
        HAL_LOGV("match failed for idx:%d, unmatched queue is empty",
                 result1.frame_number);
        return MATCH_FAILED;
    } else {
        itor2 = list.begin();
        while (itor2 != list.end()) {
            int64_t diff = result1.timestamp - itor2->timestamp;
            if (abs((int)diff) < TIME_DIFF) {
                *result2 = *itor2;
                list.erase(itor2);
                // HAL_LOGV("[%d:match:%d],diff=%llu T1:%llu,T2:%llu",
                //  result1.frame_number,itor2->frame_number,diff,result1.timestamp,itor2->timestamp);
                return MATCH_SUCCESS;
            }
            itor2++;
        }
        // HAL_LOGV("match failed for idx:%d, could not find matched frame",
        // result1.frame_number);
        return MATCH_FAILED;
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
void SprdCamera3StereoPreview::dump(const struct camera3_device *device,
                                    int fd) {
    HAL_LOGD("E");
    CHECK_MUXER();

    mPreviewMuxer->_dump(device, fd);

    HAL_LOGD("X");
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
int SprdCamera3StereoPreview::flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGD(" E");
    CHECK_MUXER_ERROR();

    rc = mPreviewMuxer->_flush(device);

    HAL_LOGD(" X");

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
int SprdCamera3StereoPreview::initialize(
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;

    HAL_LOGD("E");
    CHECK_HWI_ERROR(hwiMain);

    mIommuEnabled = 0;
    mNotifyListAux.clear();
    mNotifyListMain.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mOldPreviewRequestList.clear();
    mCallbackOps = callback_ops;
    mPerfectskinlevel = 0;
    mPreviewSize.srcWidth = 0;
    mPreviewSize.srcHeight = 0;
    mPreviewSize.stereoPreviewWidth = 0;
    mPreviewSize.stereoPreviewHeight = 0;
    mhasCallbackStream = false;
    mrotation = 270;
    mPendingRequest = 0;
    mMaxPendingCount = 4;
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
    mMaxLocalBufferNum = MAX_PREVIEW_QEQUEST_BUF;
    mLocalBuffer = (new_mem_t *)malloc(sizeof(new_mem_t) * mMaxLocalBufferNum);
    if (NULL == mLocalBuffer) {
        HAL_LOGE("fatal error! mLocalBuffer pointer is null.");
        return NO_MEMORY;
    }
    memset(mLocalBuffer, 0, sizeof(new_mem_t) * mMaxLocalBufferNum);
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
int SprdCamera3StereoPreview::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;
    bool is_recording = false;
    camera3_stream_t *newStream = NULL;
    camera3_stream_t *callBackStream = NULL;
    camera3_stream_t *previewStream = NULL;
    camera3_stream_t *pAuxStream = NULL;
    camera3_stream_t *pMainStream = NULL;
    size_t i = 0;
    size_t j = 0;
    int w = 0;
    int h = 0;

    Mutex::Autolock l(mLock);

    for (size_t i = 0; i < stream_list->num_streams; i++) {
        newStream = stream_list->streams[i];
        newStream->max_buffers = 1;
        if (getStreamType(newStream) == CALLBACK_STREAM) {
            callBackStream = newStream;
            mhasCallbackStream = true;
        }
    }
    HAL_LOGE("stream_list->num_streams=%d", stream_list->num_streams);

    for (size_t i = 0; i < stream_list->num_streams; i++) {
        newStream = stream_list->streams[i];
        if (getStreamType(newStream) == PREVIEW_STREAM) {
            previewStream = stream_list->streams[i];
            w = previewStream->width;
            h = previewStream->height;
            mMaxLocalBufferNum = MAX_PREVIEW_QEQUEST_BUF;

            if (mPreviewSize.srcWidth != w && mPreviewSize.srcHeight != h) {
                freeLocalBuffer(mLocalBuffer, mLocalBufferList,
                                mMaxLocalBufferNum);
                get3DPreviewSize(&mPreviewSize.stereoPreviewWidth,
                                 &mPreviewSize.stereoPreviewHeight);
                for (size_t j = 0; j < mMaxLocalBufferNum;) {
                    int tmp = allocateOne(mPreviewSize.stereoPreviewWidth,
                                          mPreviewSize.stereoPreviewHeight, 1,
                                          &(mLocalBuffer[j]));
                    if (tmp < 0) {
                        HAL_LOGE("request one buf failed.");
                        continue;
                    }
                    mLocalBufferList.push_back(
                        &(mLocalBuffer[j].native_handle));
                    j++;
                }
            }
            mPreviewSize.srcWidth = w;
            mPreviewSize.srcHeight = h;
            previewStream->width = mPreviewSize.stereoPreviewWidth;
            previewStream->height = mPreviewSize.stereoPreviewHeight;
            mAuxStreams = *previewStream;
            mMainStreams = *previewStream;
            pAuxStream = &mAuxStreams;
            pMainStream = &mMainStreams;
            break;
        }
    }

    mNotifyListAux.clear();
    mNotifyListMain.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();

    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;

    camera3_stream_configuration configMain, configAux;
    configMain = *stream_list;
    configMain.num_streams = 1;
    configMain.streams = &pMainStream;
    configAux = *stream_list;
    configAux.num_streams = 1;
    configAux.streams = &pAuxStream;

    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                    &configMain);
    if (rc < 0) {
        HAL_LOGE("failed. configure main streams!!");
        return rc;
    }
    rc = hwiAux->configure_streams(m_pPhyCamera[CAM_TYPE_AUX].dev, &configAux);
    if (rc < 0) {
        HAL_LOGE("failed. configure aux streams!!");
        return rc;
    }
    if (mhasCallbackStream) {
        callBackStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
    }
    memcpy(previewStream, &mMainStreams, sizeof(camera3_stream_t));
    previewStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
    previewStream->width = mPreviewSize.srcWidth;
    previewStream->height = mPreviewSize.srcHeight;
    mPreviewMuxerThread->mPreviewMuxerMsgList.clear();
    mPreviewMuxerThread->run(NULL);
    mPreviewMuxerThread->mReProcessThread->mReprocessMsgList.clear();
    mPreviewMuxerThread->mReProcessThread->run(NULL);

    HAL_LOGE("X.rc=%d", rc);
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
SprdCamera3StereoPreview::constructDefaultRequestSettings(
    const struct camera3_device *device, int type) {
    HAL_LOGD("E");
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
    HAL_LOGD("X");
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
void SprdCamera3StereoPreview::saveRequest(camera3_capture_request_t *request,
                                           int32_t perfectskinlevel) {

    size_t i = 0;
    camera3_stream_t *newStream = NULL;
    CameraMetadata metadata;
    old_request oldPreviewRequest;
    bzero(&oldPreviewRequest, sizeof(old_request));
    metadata = request->settings;
    if (metadata.exists(ANDROID_SPRD_SENSOR_ROTATION)) {
        mrotation = metadata.find(ANDROID_SPRD_SENSOR_ROTATION).data.i32[0];
    }
    oldPreviewRequest.frame_number = request->frame_number;
    oldPreviewRequest.input_buffer = request->input_buffer;
    oldPreviewRequest.rotation = mrotation;
    oldPreviewRequest.perfectskinlevel = perfectskinlevel;
    oldPreviewRequest.invalid = false;
    Mutex::Autolock l(mRequest);

    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        if (getStreamType(newStream) == CALLBACK_STREAM ||
            getStreamType(newStream) == PREVIEW_STREAM) {
            oldPreviewRequest.buffer = ((request->output_buffers)[i]).buffer;
            oldPreviewRequest.stream = ((request->output_buffers)[i]).stream;
        } else if (getStreamType(newStream) == DEFAULT_STREAM) {
            oldPreviewRequest.callback_stream = newStream;
            oldPreviewRequest.callback_buffer =
                ((request->output_buffers)[i]).buffer;
        }
    }
    mOldPreviewRequestList.push_back(oldPreviewRequest);
    if (mOldPreviewRequestList.size() == MAX_SAVE_REQUEST_QUEUE_SIZE) {
        List<old_request>::iterator itor = mOldPreviewRequestList.begin();
        mOldPreviewRequestList.erase(itor);
    }

    HAL_LOGD(
        "frame idx:%d,preview:stream:%p,buffer:%p,callback:stream%p,buffer%p",
        request->frame_number, oldPreviewRequest.stream,
        oldPreviewRequest.buffer, oldPreviewRequest.callback_stream,
        oldPreviewRequest.callback_buffer);
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
void SprdCamera3StereoPreview::clearFrameNeverMatched(int whichCamera) {
    List<hwi_frame_buffer_info_t>::iterator itor;

    if (whichCamera == CAM_TYPE_MAIN) {
        Mutex::Autolock l(mPreviewMuxer->mLocalBufferListLock);
        itor = mUnmatchedFrameListMain.begin();
        while (itor != mUnmatchedFrameListMain.end()) {
            mLocalBufferList.push_back(itor->buffer);
            HAL_LOGD("clear frame main idx:%d", itor->frame_number);
            mPreviewMuxerThread->errorCallback(itor->frame_number);
            itor = mUnmatchedFrameListMain.erase(itor);
        }
    } else {
        Mutex::Autolock l(mPreviewMuxer->mLocalBufferListLock);
        itor = mUnmatchedFrameListAux.begin();
        while (itor != mUnmatchedFrameListAux.end()) {
            mLocalBufferList.push_back(itor->buffer);
            HAL_LOGD("clear frame aux idx:%d", itor->frame_number);
            itor = mUnmatchedFrameListAux.erase(itor);
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
int SprdCamera3StereoPreview::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;
    uint32_t i = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    camera3_capture_request_t *req = request;
    camera3_capture_request_t req_main;
    camera3_capture_request_t req_aux;
    camera3_stream_t *new_stream = NULL;
    CameraMetadata metadata;

    rc = validateCaptureRequest(req);
    if (rc != NO_ERROR) {
        return rc;
    }
    metadata = request->settings;

    if (metadata.exists(ANDROID_SPRD_UCAM_SKIN_LEVEL)) {
        mPerfectskinlevel =
            metadata.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[0];
        HAL_LOGV("perfectskinlevel=%d", mPerfectskinlevel);
    }

    saveRequest(req, mPerfectskinlevel);
    HAL_LOGD("id:%d,num_output_buffers=%d", request->frame_number,
             request->num_output_buffers);

    camera3_stream_buffer_t out_streams_main;
    camera3_stream_buffer_t out_streams_aux;
    req_main = *req;
    req_aux = *req;
    // config request;

    for (size_t i = 0; i < request->num_output_buffers; i++) {
        new_stream = (request->output_buffers[i]).stream;
        if (getStreamType(new_stream) == CALLBACK_STREAM ||
            getStreamType(new_stream) == DEFAULT_STREAM ||
            getStreamType(new_stream) == PREVIEW_STREAM) {
            out_streams_main = request->output_buffers[i];
            Mutex::Autolock l(mLocalBufferListLock);
            out_streams_main.stream = &mMainStreams;
            out_streams_main.buffer = popRequestList(mLocalBufferList);
            if (NULL == out_streams_main.buffer) {
                HAL_LOGE("failed,LocalBufferList is empty!");
                return NO_MEMORY;
            }
            out_streams_aux = request->output_buffers[i];
            out_streams_aux.acquire_fence = -1;
            out_streams_aux.release_fence = -1;
            out_streams_aux.stream = &mAuxStreams;
            out_streams_aux.buffer = popRequestList(mLocalBufferList);
            if (NULL == out_streams_aux.buffer) {
                HAL_LOGE("failed, LocalBufferList is empty!");
                return NO_MEMORY;
            }
            break;
        }
    }
    req_main.output_buffers = &out_streams_main;
    req_aux.output_buffers = &out_streams_aux;
    req_main.num_output_buffers = 1;
    req_aux.num_output_buffers = 1;
    // send request

    rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                          &req_main);
    if (rc < 0) {
        HAL_LOGD("main request failed. rc= %d", rc);
        mLocalBufferList.push_back(req_main.output_buffers->buffer);
        return rc;
    }
    rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_AUX].dev,
                                          &req_aux);
    if (rc < 0) {
        mLocalBufferList.push_back(req_aux.output_buffers->buffer);
        HAL_LOGD("aux request failed. rc= %d", rc);
        return rc;
    }
    HAL_LOGD("max_buffers %d", mMainStreams.max_buffers);
    mMaxPendingCount = mAuxStreams.max_buffers + 1;
    {
        Mutex::Autolock l(mRequestLock);
        size_t pendingCount = 0;
        mPendingRequest++;
        while (mPendingRequest >= mMaxPendingCount) {
            mRequestSignal.waitRelative(mRequestLock, kPendingTime);
            if (pendingCount > kPendingTimeOut / kPendingTime) {
                HAL_LOGD("m_PendingRequest=%d", mPendingRequest);
                rc = -ENODEV;
                break;
            }
            pendingCount++;
        }
    }
    HAL_LOGD("request id%d ok.", request->frame_number);

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
void SprdCamera3StereoPreview::notifyMain(const camera3_notify_msg_t *msg) {
    camera3_notify_msg_t newMsg = *msg;

    mCallbackOps->notify(mCallbackOps, msg);
    {
        Mutex::Autolock l(mNotifyLockMain);
        mNotifyListMain.push_back(*msg);
        if (mNotifyListMain.size() == MAX_NOTIFY_QUEUE_SIZE) {
            List<camera3_notify_msg_t>::iterator itor = mNotifyListMain.begin();
            mNotifyListMain.erase(itor);
        }
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
void SprdCamera3StereoPreview::processCaptureResultMain(
    const camera3_capture_result_t *result) {

    uint64_t result_timestamp;
    uint32_t cur_frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    bool isRecording = false;
    cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;

    if (result->result) {
        Mutex::Autolock l(mRequest);
        CameraMetadata metadata;
        metadata = result->result;
        if (metadata.exists(ANDROID_STATISTICS_FACE_RECTANGLES)) {
            List<old_request>::iterator i = mOldPreviewRequestList.begin();
            while (i != mOldPreviewRequestList.end()) {
                if (i->frame_number == result->frame_number) {
                    i->g_face_info[0] =
                        metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[0];
                    i->g_face_info[1] =
                        metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[1];
                    i->g_face_info[2] =
                        metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[2];
                    i->g_face_info[3] =
                        metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[3];
                    HAL_LOGD("sx %d sy %d ex %d ey %d", i->g_face_info[0],
                             i->g_face_info[1], i->g_face_info[2],
                             i->g_face_info[3]);
                    break;
                }
                i++;
            }
        }
    }

    HAL_LOGD("id= %d output_buffers=%p", result->frame_number,
             result->output_buffers);

    if (result->output_buffers == NULL) {
        mCallbackOps->process_capture_result(mCallbackOps, result);
        return;
    }

    // process preview stream
    {
        Mutex::Autolock l(mNotifyLockMain);
        for (List<camera3_notify_msg_t>::iterator i = mNotifyListMain.begin();
             i != mNotifyListMain.end(); i++) {
            if (i->message.shutter.frame_number == cur_frame_number) {
                if (i->type == CAMERA3_MSG_SHUTTER) {
                    searchnotifyresult = NOTIFY_SUCCESS;
                    result_timestamp = i->message.shutter.timestamp;
                } else if (i->type == CAMERA3_MSG_ERROR) {
                    HAL_LOGE("Return error preview buffer:%d caused by error "
                             "Notify status",
                             result->frame_number);
                    mPreviewMuxerThread->errorCallback(result->frame_number);
                    {
                        Mutex::Autolock l(mLocalBufferListLock);
                        mLocalBufferList.push_back(
                            result->output_buffers->buffer);
                    }
                    searchnotifyresult = NOTIFY_ERROR;
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
        Mutex::Autolock l(mLocalBufferListLock);
        HAL_LOGE("Return error preview buffer:%d caused by error Buffer status",
                 result->frame_number);
        mPreviewMuxerThread->errorCallback(result->frame_number);
        mLocalBufferList.push_back(result->output_buffers->buffer);
        return;
    }

    hwi_frame_buffer_info_t matched_frame;
    hwi_frame_buffer_info_t cur_frame;
    cur_frame.frame_number = cur_frame_number;
    cur_frame.timestamp = result_timestamp;
    cur_frame.buffer = result->output_buffers->buffer;

    {
        Mutex::Autolock l(mUnmatchedQueueLock);
        if (MATCH_SUCCESS ==
            matchTwoFrame(cur_frame, mUnmatchedFrameListAux, &matched_frame)) {
            muxer_queue_msg_t muxer_msg;
            muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
            muxer_msg.combo_frame.frame_number = cur_frame.frame_number;
            muxer_msg.combo_frame.buffer1 = cur_frame.buffer;
            muxer_msg.combo_frame.buffer2 = matched_frame.buffer;
            muxer_msg.combo_frame.input_buffer = result->input_buffer;
            muxer_msg.combo_frame.stream = result->output_buffers->stream;
            {
                Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
                clearFrameNeverMatched(CAM_TYPE_MAIN);
                HAL_LOGD("Enqueue combo frame:%d for frame merge!",
                         muxer_msg.combo_frame.frame_number);
                mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(muxer_msg);
                mPreviewMuxerThread->mMergequeueSignal.signal();
            }
        } else {
            HAL_LOGD("Enqueue newest unmatched frame:%d for Main camera",
                     cur_frame.frame_number);
            hwi_frame_buffer_info_t *discard_frame =
                pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListMain);
            if (discard_frame != NULL) {
                Mutex::Autolock l(mLocalBufferListLock);
                HAL_LOGD("Discard oldest unmatched frame:%d for Main camera",
                         discard_frame->frame_number);
                mLocalBufferList.push_back(discard_frame->buffer);
                mPreviewMuxerThread->errorCallback(discard_frame->frame_number);
                delete discard_frame;
            }
        }
    }

    return;
}
/*===========================================================================
 * FUNCTION   :notifyAux
 *
 * DESCRIPTION: notify aux camera
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoPreview::notifyAux(const camera3_notify_msg_t *msg) {
    camera3_notify_msg_t newMsg = *msg;

    {
        Mutex::Autolock l(mNotifyLockAux);
        mNotifyListAux.push_back(*msg);
        if (mNotifyListAux.size() == MAX_NOTIFY_QUEUE_SIZE) {
            List<camera3_notify_msg_t>::iterator itor = mNotifyListAux.begin();
            mNotifyListAux.erase(itor);
        }
    }
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
void SprdCamera3StereoPreview::processCaptureResultAux(
    const camera3_capture_result_t *result) {

    uint64_t result_timestamp;
    uint32_t cur_frame_number = result->frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    camera3_stream_t *newStream = NULL;

    HAL_LOGD("id= %d output_buffers=%p", result->frame_number,
             result->output_buffers);

    if (result->output_buffers == NULL) {
        return;
    }

    // process preview buffer
    {
        Mutex::Autolock l(mNotifyLockAux);
        for (List<camera3_notify_msg_t>::iterator i = mNotifyListAux.begin();
             i != mNotifyListAux.end(); i++) {
            if (i->message.shutter.frame_number == cur_frame_number) {
                if (i->type == CAMERA3_MSG_SHUTTER) {
                    searchnotifyresult = NOTIFY_SUCCESS;
                    result_timestamp = i->message.shutter.timestamp;
                } else if (i->type == CAMERA3_MSG_ERROR) {
                    HAL_LOGE(
                        "Return local buffer:%d caused by error Notify status",
                        result->frame_number);
                    Mutex::Autolock l(mLocalBufferListLock);
                    searchnotifyresult = NOTIFY_ERROR;
                    mLocalBufferList.push_back(result->output_buffers->buffer);
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
        camera3_capture_result_t result_copy;
        result_copy = *result;
        HAL_LOGE("Return local buffer:%d caused by error Buffer status",
                 result->frame_number);
        mLocalBufferList.push_back(result->output_buffers->buffer);
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
                clearFrameNeverMatched(CAM_TYPE_AUX);
                mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(muxer_msg);
                mPreviewMuxerThread->mMergequeueSignal.signal();
            }
        } else {
            HAL_LOGD("Enqueue newest unmatched frame:%d for Aux camera",
                     cur_frame.frame_number);
            hwi_frame_buffer_info_t *discard_frame =
                pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListAux);
            if (discard_frame != NULL) {
                Mutex::Autolock l(mLocalBufferListLock);
                mLocalBufferList.push_back(discard_frame->buffer);
                delete discard_frame;
            }
        }
    }
    HAL_LOGE("X");

    return;
}
/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: deconstructor of SprdCamera3StereoPreview
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoPreview::_dump(const struct camera3_device *device,
                                     int fd) {

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
void SprdCamera3StereoPreview::dumpImg(void *addr, int size, int w, int h,
                                       int frameId, int flag) {

    HAL_LOGD(" E");

    char name[128];
    snprintf(name, sizeof(name), "/data/misc/media/%d_%d__%d_%d.yuv", w, h,
             frameId, flag);

    FILE *file_fd = fopen(name, "w");

    if (file_fd == NULL) {
        HAL_LOGE("open yuv file failed!\n");
    }
    int count = fwrite(addr, 1, size, file_fd);
    if (count != size) {
        HAL_LOGE("write dst.yuv failed\n");
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
int SprdCamera3StereoPreview::_flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGD("E");
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_MAIN].dev);
    HAL_LOGD("flush aux camera");

    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    rc = hwiAux->flush(m_pPhyCamera[CAM_TYPE_AUX].dev);

    if (mPreviewMuxerThread != NULL) {
        if (mPreviewMuxerThread->isRunning()) {
            mPreviewMuxerThread->requestExit();
        }
    }

    {
        // in case the other threads need to complete process data
        mPreviewMuxerThread->join();
        mPreviewMuxerThread->mReProcessThread->join();
    }
    clearFrameNeverMatched(CAM_TYPE_AUX);
    HAL_LOGD("X");
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
hwi_frame_buffer_info_t *SprdCamera3StereoPreview::pushToUnmatchedQueue(
    hwi_frame_buffer_info_t new_buffer_info,
    List<hwi_frame_buffer_info_t> &queue) {
    hwi_frame_buffer_info_t *pushout = NULL;
    if (queue.size() == MAX_UNMATCHED_QUEUE_SIZE) {
        pushout = new hwi_frame_buffer_info_t;
        List<hwi_frame_buffer_info_t>::iterator i = queue.begin();
        *pushout = *i;
        queue.erase(i);
    }
    queue.push_back(new_buffer_info);

    return pushout;
}
};
