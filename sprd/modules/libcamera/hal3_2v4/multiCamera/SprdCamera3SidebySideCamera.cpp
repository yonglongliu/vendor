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
#define LOG_TAG "Cam3SideBySide"
#include "SprdCamera3SidebySideCamera.h"

using namespace android;
namespace sprdcamera {

SprdCamera3SideBySideCamera *mSidebyside = NULL;

// Error Check Macros
#define CHECK_SIDEBYSIDE()                                                     \
    if (!mSidebyside) {                                                        \
        HAL_LOGE("Error getting switch");                                      \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_SIDEBYSIDE_ERROR()                                               \
    if (!mSidebyside) {                                                        \
        HAL_LOGE("Error getting switch");                                      \
        return -ENODEV;                                                        \
    }

#define CHECK_HWI_ERROR(hwi)                                                   \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return -ENODEV;                                                        \
    }

camera3_device_ops_t SprdCamera3SideBySideCamera::mCameraCaptureOps = {
    .initialize = SprdCamera3SideBySideCamera::initialize,
    .configure_streams = SprdCamera3SideBySideCamera::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3SideBySideCamera::construct_default_request_settings,
    .process_capture_request =
        SprdCamera3SideBySideCamera::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3SideBySideCamera::dump,
    .flush = SprdCamera3SideBySideCamera::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3SideBySideCamera::callback_ops_main = {
    .process_capture_result =
        SprdCamera3SideBySideCamera::process_capture_result_main,
    .notify = SprdCamera3SideBySideCamera::notifyMain};

camera3_callback_ops SprdCamera3SideBySideCamera::callback_ops_aux = {
    .process_capture_result =
        SprdCamera3SideBySideCamera::process_capture_result_aux,
    .notify = SprdCamera3SideBySideCamera::notifyAux};
static int save_onlinecalbinfo(char *filename, uint8_t *input, uint32_t size) {
    char file_name[256];
    FILE *fp;
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);
    sprintf(file_name, "/data/misc/cameraserver/%04d%02d%02d%02d%02d%02d_%s",
            (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour,
            p->tm_min, p->tm_sec, filename);
    fp = fopen(file_name, "wb");
    if (fp) {
        fwrite(input, size, 1, fp);
        fclose(fp);
        return 0;
    }
    return -1;
}
static int savefile(char *filename, uint8_t *ptr, int width, int height) {
    int i;
    // static int inum = 0;
    char file_name[256];
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);
    sprintf(file_name, "/data/misc/cameraserver/%04d%02d%02d%02d%02d%02d_%s",
            (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour,
            p->tm_min, p->tm_sec, filename);
    // if(inum < 5)
    FILE *fp = fopen(file_name, "wb");
    if (fp) {
        fwrite(ptr, 1, width * height * 3 / 2, fp);
        fclose(fp);
        return 0;
    }
    return -1;
}
/*===========================================================================
 * FUNCTION   : SprdCamera3SideBySideCamera
 *
 * DESCRIPTION: SprdCamera3SideBySideCamera Constructor
 *
 * PARAMETERS:
 *
 *
 *==========================================================================*/
SprdCamera3SideBySideCamera::SprdCamera3SideBySideCamera() {
    HAL_LOGI("E");

    memset(&m_VirtualCamera, 0, sizeof(sprd_virtual_camera_t));
    memset(&mLocalCapBuffer, 0, sizeof(new_mem_t) * SBS_LOCAL_CAPBUFF_NUM);
    memset(&mSavedReqStreams, 0,
           sizeof(camera3_stream_t *) * SBS_MAX_NUM_STREAMS);

    m_VirtualCamera.id = CAM_SIDEBYSIDE_MAIN_ID;
    mStaticMetadata = NULL;
    mCaptureThread = new CaptureThread();

    memset(&mThumbReq, 0, sizeof(request_saved_sidebyside_t));
    mhasCallbackStream = false;
    mSavedRequestList.clear();
    memset(&mPrevOutReq, 0, sizeof(request_saved_sidebyside_t));
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mIsCapturing = false;
    mPreviewStreamsNum = 0;
    mjpegSize = 0;
    mCameraId = CAM_SIDEBYSIDE_MAIN_ID;
    mFlushing = false;
    mIsWaitSnapYuv = false;
    mPerfectskinlevel = 0;
    mRotation = 0;
    mJpegOrientation = 0;
    m_pPhyCamera = NULL;
    m_nPhyCameras = 0;
    mSubCaptureWidth = 1600;
    mSubCaptureHeight = 1200;
    mCapDepthWidth = 800;
    mCapDepthHeight = 600;
    // mCapDepthWidth = 324;
    // mCapDepthHeight = 243;
    mMainCaptureWidth = 1600;
    mMainCaptureHeight = 1200;
#ifdef CONFIG_SUPERRESOLUTION_ENABLED
    mScalingUpWidth = 1600;
    mScalingUpHeight = 1200;
#else
    mScalingUpWidth = 1600;
    mScalingUpHeight = 1200;
#endif
    mDepInputWidth = 800;
    mDepInputHeight = 600;
    mDepthBufferSize = 0;
    mFnumber = 5;
    mCoveredValue = BLUR_TIPS_DEFAULT;
    mBokehLevel = 128;
    mIsSrProcessed = 1;
    mRealtimeBokeh = 0;
    mBokehPointX = 800;
    mBokehPointY = 600;
    mDepthHandle = NULL;
    mDepthState = 0;
    mIspdebuginfosize = 300000;
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   : ~SprdCamera3SideBySideCamera
 *
 * DESCRIPTION: SprdCamera3SideBySideCamera Desctructor
 *
 *==========================================================================*/
SprdCamera3SideBySideCamera::~SprdCamera3SideBySideCamera() {
    HAL_LOGI("E");
    mCaptureThread = NULL;
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   : getCameraSidebyside
 *
 * DESCRIPTION: Creates Camera Blur if not created
 *
 * PARAMETERS:
 *   @pCapture               : Pointer to retrieve Camera Switch
 *
 *
 * RETURN    :  NONE
 *==========================================================================*/
void SprdCamera3SideBySideCamera::getCameraSidebyside(
    SprdCamera3SideBySideCamera **pSidebyside) {
    if (!mSidebyside) {
        mSidebyside = new SprdCamera3SideBySideCamera();
    }
    CHECK_SIDEBYSIDE();
    *pSidebyside = mSidebyside;
    HAL_LOGV("mSidebyside=%p", mSidebyside);

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
int SprdCamera3SideBySideCamera::get_camera_info(__unused int camera_id,
                                                 struct camera_info *info) {
    int rc = NO_ERROR;

    HAL_LOGV("E");
    if (info) {
        rc = mSidebyside->getCameraInfo(camera_id, info);
    }
    HAL_LOGV("X, rc=%d", rc);

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
int SprdCamera3SideBySideCamera::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    int rc = NO_ERROR;

    HAL_LOGV("id=%d", atoi(id));
    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    rc = mSidebyside->cameraDeviceOpen(atoi(id), hw_device);
    HAL_LOGV("id=%d, rc: %d", atoi(id), rc);

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
int SprdCamera3SideBySideCamera::close_camera_device(
    __unused hw_device_t *hw_dev) {
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }

    return mSidebyside->closeCameraDevice();
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
int SprdCamera3SideBySideCamera::closeCameraDevice() {
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
        HAL_LOGI("camera id:%d", i);
        rc = SprdCamera3HWI::close_camera_device(dev);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error, camera id:%d", i);
        }
        sprdCam->hwi = NULL;
        sprdCam->dev = NULL;
    }

    freeLocalCapBuffer();
    mSavedRequestList.clear();
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
int SprdCamera3SideBySideCamera::initialize(
    __unused const struct camera3_device *device,
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;

    HAL_LOGV("E");
    CHECK_SIDEBYSIDE_ERROR();
    rc = mSidebyside->initialize(callback_ops);

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
const camera_metadata_t *
SprdCamera3SideBySideCamera::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *rc;

    HAL_LOGV("E");
    if (!mSidebyside) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }
    rc = mSidebyside->constructDefaultRequestSettings(device, type);

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
int SprdCamera3SideBySideCamera::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;

    HAL_LOGV("E");
    CHECK_SIDEBYSIDE_ERROR();

    rc = mSidebyside->configureStreams(device, stream_list);

    HAL_LOGV("X");

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
int SprdCamera3SideBySideCamera::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;

    HAL_LOGV("frame_number=%d", request->frame_number);
    CHECK_SIDEBYSIDE_ERROR();
    rc = mSidebyside->processCaptureRequest(device, request);

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3SideBySideCamera
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3SideBySideCamera::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("frame_number=%d", result->frame_number);
    CHECK_SIDEBYSIDE();
    mSidebyside->processCaptureResultMain((camera3_capture_result_t *)result);
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
void SprdCamera3SideBySideCamera::notifyMain(
    const struct camera3_callback_ops *ops, const camera3_notify_msg_t *msg) {
    HAL_LOGV("frame_number=%d", msg->message.shutter.frame_number);
    CHECK_SIDEBYSIDE();
    mSidebyside->notifyMain(msg);
}

/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3SideBySideCamera
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3SideBySideCamera::process_capture_result_aux(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("frame_number=%d", result->frame_number);
    CHECK_SIDEBYSIDE();
    mSidebyside->processCaptureResultAux((camera3_capture_result_t *)result);
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
void SprdCamera3SideBySideCamera::notifyAux(
    const struct camera3_callback_ops *ops, const camera3_notify_msg_t *msg) {
    HAL_LOGD("frame_number=%d", msg->message.shutter.frame_number);
    CHECK_SIDEBYSIDE();
}

/*===========================================================================
 * FUNCTION   :allocateBuffer
 *
 * DESCRIPTION: deconstructor of SprdCamera3SidebySideCamera
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3SideBySideCamera::allocateBuffer(int w, int h, uint32_t is_cache,
                                                int format,
                                                new_mem_t *new_mem) {
    int result = 0;
    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;
    private_handle_t *buffer;

    if (format == HAL_PIXEL_FORMAT_YCbCr_422_SP)
        mem_size = w * h * 2;
    else
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

#if defined(CONFIG_SPRD_ANDROID_8)
    if (NULL == buffer) {
        HAL_LOGE("alloc buffer failed");
        goto getpmem_fail;
    }

    if (buffer->share_attr_fd < 0) {
        buffer->share_attr_fd =
            ashmem_create_region("camera_gralloc_shared_attr", PAGE_SIZE);
        if (buffer->share_attr_fd < 0) {
            HAL_LOGE("Failed to allocate page for shared attribute region");
            goto getpmem_fail;
        }
    }

    buffer->attr_base = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                             MAP_SHARED, buffer->share_attr_fd, 0);
    if (buffer->attr_base != MAP_FAILED) {
        attr_region *region = (attr_region *)buffer->attr_base;
        memset(buffer->attr_base, 0xff, PAGE_SIZE);
        munmap(buffer->attr_base, PAGE_SIZE);
        buffer->attr_base = MAP_FAILED;
    } else {
        HAL_LOGE("Failed to mmap shared attribute region");
        goto getpmem_fail;
    }
#endif

    buffer->share_fd = pHeapIon->getHeapID();
    buffer->format = format;
    buffer->byte_stride = w;
    buffer->internal_format = format;
    buffer->width = w;
    buffer->height = h;
    buffer->stride = w;
    buffer->internalWidth = w;
    buffer->internalHeight = h;

    new_mem->native_handle = buffer;
    new_mem->pHeapIon = pHeapIon;

    HAL_LOGD("fd=0x%x, size=%d, heap=%p, share_attr_fd=0x%x", buffer->share_fd,
             mem_size, pHeapIon, buffer->share_attr_fd);

    return result;

getpmem_fail:
    delete pHeapIon;

    return -1;
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
void SprdCamera3SideBySideCamera::freeLocalCapBuffer() {
    for (size_t i = 0; i < SBS_LOCAL_CAPBUFF_NUM; i++) {
        new_mem_t *localBuffer = &mLocalCapBuffer[i];
        freeOneBuffer(localBuffer);
    }
}

void SprdCamera3SideBySideCamera::dumpFile(unsigned char *addr, int type,
                                           int size, int width, int height,
                                           int frame_num, int param,
                                           int ismaincam) {
    HAL_LOGD(" E addr:%p type:%d size:%d w:%d h:%d", addr, type, size, width,
             height);
    int rc = 0;
    char name[256] = {0};
    char name1[256] = {0};
    char name2[256] = {0};
    char file_name[256] = {0};
    char tmp_str[64] = {0};
    FILE *fp = NULL;
    FILE *fp1 = NULL;
    FILE *fp2 = NULL;
    int i = 0;
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);

    struct tuning_param_info tuning_info;

    if (CAM_TYPE_MAIN == ismaincam) {
        rc = mCaptureThread->mDevMain->hwi->getTuningParam(&tuning_info);
    } else {
        rc = mCaptureThread->mDevAux->hwi->getTuningParam(&tuning_info);
    }

    strcpy(file_name, "/data/misc/cameraserver/");
    sprintf(tmp_str, "%04d%02d%02d%02d%02d%02d", (1900 + p->tm_year),
            (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    strcat(file_name, tmp_str);

    memset(tmp_str, 0, sizeof(tmp_str));
    sprintf(tmp_str, "_gain_%d_shutter_%d", tuning_info.gain,
            tuning_info.shutter);
    strcat(file_name, tmp_str);

    memset(tmp_str, 0, sizeof(tmp_str));
    sprintf(tmp_str, "_awbgain_r_%d_g_%d_b_%d", tuning_info.awb_info.r_gain,
            tuning_info.awb_info.g_gain, tuning_info.awb_info.b_gain);
    strcat(file_name, tmp_str);

    memset(tmp_str, 0, sizeof(tmp_str));
    sprintf(tmp_str, "_vcm_%d", tuning_info.pos);
    strcat(file_name, tmp_str);

    memset(tmp_str, 0, sizeof(tmp_str));
    sprintf(tmp_str, "_bv_%ld", tuning_info.bv);
    strcat(file_name, tmp_str);

    switch (type) {
    case SBS_FILE_NV21: {
        sprintf(tmp_str, "_%dx%d", width, height);
        strcat(file_name, tmp_str);
        if (param == 0) {
            strcat(name, file_name);
            strcat(name, "_main.NV21");
        } else if (param == 1) {
            strcat(name, file_name);
            strcat(name, "_sub.NV21");
        } else if (param == 2) {
            strcat(name, file_name);
            strcat(name, "_out.NV21");
        } else if (param == 3) {
            strcat(name, file_name);
            strcat(name, ".NV21");
        } else if (param == 4) {
            strcpy(name, "/data/misc/cameraserver/");
            memset(tmp_str, 0, sizeof(tmp_str));
            sprintf(tmp_str, "%04d%02d%02d%02d%02d%02d", (1900 + p->tm_year),
                    (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min,
                    p->tm_sec);
            strcat(name, tmp_str);
            memset(tmp_str, 0, sizeof(tmp_str));
            sprintf(tmp_str, "_%d", frame_num);
            strcat(name, tmp_str);
            memset(tmp_str, 0, sizeof(tmp_str));
            sprintf(tmp_str, "_%dx%d", width, height);
            strcat(name, tmp_str);
            strcat(name, "_main.NV21");
        } else if (param == 5) {
            strcpy(name, "/data/misc/cameraserver/");
            memset(tmp_str, 0, sizeof(tmp_str));
            sprintf(tmp_str, "%04d%02d%02d%02d%02d%02d", (1900 + p->tm_year),
                    (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min,
                    p->tm_sec);
            strcat(name, tmp_str);
            memset(tmp_str, 0, sizeof(tmp_str));
            sprintf(tmp_str, "_%d", frame_num);
            strcat(name, tmp_str);
            memset(tmp_str, 0, sizeof(tmp_str));
            sprintf(tmp_str, "_%dx%d", width, height);
            strcat(name, tmp_str);
            strcat(name, "_sub.NV21");
        } else if (param == 6) {
            strcpy(name, "/data/misc/cameraserver/");
            memset(tmp_str, 0, sizeof(tmp_str));
            sprintf(tmp_str, "%dx%d", width, height);
            strcat(name, tmp_str);
            strcat(name, "_left.NV21");
        } else if (param == 7) {
            strcpy(name, "/data/misc/cameraserver/");
            memset(tmp_str, 0, sizeof(tmp_str));
            sprintf(tmp_str, "%dx%d", width, height);
            strcat(name, tmp_str);
            strcat(name, "_right.NV21");
        }
        fp = fopen(name, "wb");
        if (fp == NULL) {
            HAL_LOGE("open yuv file fail!\n");
            return;
        }
        fwrite((void *)addr, 1, size, fp);
        fclose(fp);
    } break;
    case SBS_FILE_YUV422: {
        sprintf(tmp_str, "_%dx%d", width, height);
        strcat(file_name, tmp_str);
        strcat(name, file_name);
        strcat(name, ".YUYV");
        fp = fopen(name, "wb");
        if (fp == NULL) {
            HAL_LOGE("can not open file: %s \n", name);
            return;
        }
        fwrite((void *)addr, 1, size, fp);
        fclose(fp);
    } break;
    case SBS_FILE_DEPTH: {
        strcpy(name, "/data/misc/cameraserver/");
        memset(tmp_str, 0, sizeof(tmp_str));
        sprintf(tmp_str, "%04d%02d%02d%02d%02d%02d", (1900 + p->tm_year),
                (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
        strcat(name, tmp_str);
        memset(tmp_str, 0, sizeof(tmp_str));
        sprintf(tmp_str, "_%dx%d", width, height);
        strcat(name, tmp_str);
        strcat(name, ".GRAY");
        fp = fopen(name, "wb");
        if (fp == NULL) {
            HAL_LOGE("can not open file: %s \n", name);
            return;
        }
        fwrite((void *)addr, 1, size, fp);
        fclose(fp);
    } break;
    case SBS_FILE_RAW: {
        strcat(name, file_name);
        memset(tmp_str, 0, sizeof(tmp_str));
        sprintf(tmp_str, "_%dx%d", width, height);
        strcat(name, tmp_str);
        strcat(name, ".raw");
        fp = fopen(name, "wb");
        if (fp == NULL) {
            HAL_LOGE("can not open file: %s \n", name);
            return;
        }
        strcat(name1, file_name);
        memset(tmp_str, 0, sizeof(tmp_str));
        sprintf(tmp_str, "_%dx%d", width / 2, height);
        strcat(name1, tmp_str);
        strcat(name1, "_main.raw");
        fp1 = fopen(name1, "ab");
        if (fp1 == NULL) {
            HAL_LOGE("can not open file: %s \n", name1);
            return;
        }
        // get sub raw
        rc = mCaptureThread->mDevAux->hwi->getTuningParam(&tuning_info);
        memset(file_name, 0, sizeof(file_name));
        memset(tmp_str, 0, sizeof(tmp_str));
        strcpy(file_name, "/data/misc/cameraserver/");
        sprintf(tmp_str, "%04d%02d%02d%02d%02d%02d", (1900 + p->tm_year),
                (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
        strcat(file_name, tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        sprintf(tmp_str, "_gain_%d_shutter_%d", tuning_info.gain,
                tuning_info.shutter);
        strcat(file_name, tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        sprintf(tmp_str, "_awbgain_r_%d_g_%d_b_%d", tuning_info.awb_info.r_gain,
                tuning_info.awb_info.g_gain, tuning_info.awb_info.b_gain);
        strcat(file_name, tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        sprintf(tmp_str, "_vcm_%d", tuning_info.pos);
        strcat(file_name, tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        sprintf(tmp_str, "_bv_%ld", tuning_info.bv);
        strcat(file_name, tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        sprintf(tmp_str, "_%dx%d", width / 2, height);
        strcat(file_name, tmp_str);
        strcat(name2, file_name);
        strcat(name2, "_sub.raw");
        fp2 = fopen(name2, "ab");
        if (fp2 == NULL) {
            HAL_LOGE("can not open file: %s \n", name2);
            return;
        }

        fwrite((void *)addr, 1, size, fp);
        for (i = 0; i < height; i++) {
            fwrite((void *)(addr + (i * width * 2)), 1, width, fp1);
            fwrite((void *)(addr + (width + i * width * 2)), 1, width, fp2);
        }
        fclose(fp);
        fclose(fp1);
        fclose(fp2);
    } break;
    case SBS_FILE_JPEG: {
        strcpy(name, "/data/misc/cameraserver/");
        memset(tmp_str, 0, sizeof(tmp_str));
        sprintf(tmp_str, "%04d%02d%02d%02d%02d%02d", (1900 + p->tm_year),
                (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
        strcat(name, tmp_str);
        strcat(name, ".jpg");
        fp = fopen(name, "ab");
        if (fp == NULL) {
            HAL_LOGE("can not open file: %s \n", name);
            return;
        }
        fwrite((void *)addr, 1, size, fp);
        fclose(fp);
    } break;
    case SBS_FILE_OTP: {
        strcpy(name, "/data/misc/cameraserver/");
        strcat(name, "original_otp.bin");
        fp = fopen(name, "wb");
        if (fp == NULL) {
            HAL_LOGE("can not open file: %s \n", name);
            return;
        }
        fwrite((void *)addr, 1, size, fp);
        fclose(fp);
    } break;
    case SBS_FILE_ISPINFO: {
        strcpy(name, "/data/misc/cameraserver/");
        memset(tmp_str, 0, sizeof(tmp_str));
        sprintf(tmp_str, "%04d%02d%02d%02d%02d%02d", (1900 + p->tm_year),
                (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
        strcat(name, tmp_str);
        strcat(name, ".isp_info");
        fp = fopen(name, "ab");
        if (fp == NULL) {
            HAL_LOGE("can not open file: %s \n", name);
            return;
        }
        fwrite((void *)addr, 1, size, fp);
        fclose(fp);
    } break;
    default:
        break;
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
int SprdCamera3SideBySideCamera::cameraDeviceOpen(
    __unused int camera_id, struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    uint32_t phyId = 0;

    HAL_LOGI("E");
    if (camera_id == MODE_BLUR_FRONT) {
        mCameraId = CAM_SIDEBYSIDE_MAIN_ID_2;
        m_VirtualCamera.id = CAM_SIDEBYSIDE_MAIN_ID_2;
        m_nPhyCameras = 2;
    } else {
        mCameraId = CAM_SIDEBYSIDE_MAIN_ID;
        m_VirtualCamera.id = CAM_SIDEBYSIDE_MAIN_ID;
        m_nPhyCameras = 2;
    }

    hw_device_t *hw_dev[m_nPhyCameras];
    setupPhysicalCameras();

    // Open all physical cameras
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        if (mCameraId == CAM_SIDEBYSIDE_MAIN_ID_2) {
            phyId = m_pPhyCamera[i].id + 1;
        } else {
            phyId = m_pPhyCamera[i].id;
        }
        SprdCamera3HWI *hw = new SprdCamera3HWI((uint32_t)phyId);
        if (!hw) {
            HAL_LOGE("Allocation of hardware interface failed");
            return NO_MEMORY;
        }
        hw_dev[i] = NULL;
        hw->setMultiCameraMode(MODE_SBS);

        // choose real time bokeh
        char real[PROPERTY_VALUE_MAX];
        property_get("persist.sys.cam.sbs.realtime", real, "1");
        if (!strcmp(real, "1")) {
            mRealtimeBokeh = 1;
        }

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
int SprdCamera3SideBySideCamera::getCameraInfo(int blur_camera_id,
                                               struct camera_info *info) {
    int rc = NO_ERROR;
    int camera_id = 0;
    int32_t img_size = 0;

    HAL_LOGI("E camera_id=%d", camera_id);

    if (blur_camera_id == MODE_BLUR_FRONT) {
        m_VirtualCamera.id = CAM_SIDEBYSIDE_MAIN_ID_2;
    } else {
        m_VirtualCamera.id = CAM_SIDEBYSIDE_MAIN_ID;
    }
    camera_id = m_VirtualCamera.id;

    SprdCamera3Setting::initDefaultParameters(camera_id);

    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        HAL_LOGE("getStaticMetadata failed");
        return rc;
    }
    CameraMetadata metadata = mStaticMetadata;
    img_size = SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size * 2 +
               (SBS_REFOCUS_PARAM_NUM * 4) + 1024;
    SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size = img_size;

    metadata.update(
        ANDROID_JPEG_MAX_SIZE,
        &(SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size), 1);
    mStaticMetadata = metadata.release();

    SprdCamera3Setting::getCameraInfo(camera_id, info);

    info->device_version = CAMERA_DEVICE_API_VERSION_3_2;
    info->static_camera_characteristics = mStaticMetadata;
    info->conflicting_devices_length = 0;

    HAL_LOGI("X img_size=%d", img_size);
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
int SprdCamera3SideBySideCamera::setupPhysicalCameras() {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[m_nPhyCameras];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }
    memset(m_pPhyCamera, 0x00,
           (m_nPhyCameras * sizeof(sprdcamera_physical_descriptor_t)));

    m_pPhyCamera[CAM_TYPE_MAIN].id = CAM_SIDEBYSIDE_MAIN_ID;
    if (2 == m_nPhyCameras) {
        m_pPhyCamera[CAM_TYPE_AUX].id = CAM_BLUR_AUX_ID;
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
SprdCamera3SideBySideCamera::CaptureThread::CaptureThread()
    : mSavedCapReqsettings(NULL), mCaptureStreamsNum(0), mReprocessing(false),
      mBokehConfigParamState(true) {
    HAL_LOGI("E");
    memset(&mSavedCapReqstreambuff, 0, sizeof(camera3_stream_buffer_t));
    memset(&mMainStreams, 0, sizeof(camera3_stream_t) * SBS_MAX_NUM_STREAMS);
    memset(mOtpData, 0, sizeof(int8_t) * OTP_DUALCAM_DATA_SIZE);
    memset(mFaceInfo, 0, sizeof(int32_t) * 4);
    mDevMain = NULL;
    mDevAux = NULL;
    mSavedResultBuff = NULL;
    m3dVerificationEnable = 0;
    mRealtimeBokehNum = 0;
    memset(&mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
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
SprdCamera3SideBySideCamera::CaptureThread::~CaptureThread() {
    HAL_LOGI("E");
    mCaptureMsgList.clear();
}

void SprdCamera3SideBySideCamera::CaptureThread::ProcessDepthImage(
    unsigned char *subbuffer, unsigned char *mainbuffer,
    unsigned char *Depthoutbuffer) {
    int ret = 0;
    char acVersion[256];
    void *otp_data = NULL;
    int otp_size = 0;
    int has_read_otp = 0;
    FILE *fid = NULL;
    int i = 0;
    char otp[PROPERTY_VALUE_MAX] = {0};
    char dump[PROPERTY_VALUE_MAX] = {0};
    char Sensor_Otp_Data[OTP_DUALCAM_DATA_SIZE] = {0};
    int OTP_Datanum = 57;
    int OTP_Data[57] = {
        209771904, 154924608, 319766528, 320768896, 44446,     -91512,
        0,         524252,    -5971,     1268,      5900,      523596,
        26255,     -1566,     -26239,    523628,    301141280, 0,
        209325744, 0,         301141280, 137935056, 0,         0,
        524288,    208521040, 158706112, 301141280, 298885760, 14572,
        22123,     0,         524216,    -8509,     -1626,     8578,
        523588,    25679,     1207,      -25703,    523656,    301141280,
        0,         209325744, 0,         301141280, 137935056, 0,
        0,         524288,    6353880,   10666704,  209325744, 137935056,
        301141280, 46927,     0};
    struct depth_init_inputparam *inparam = NULL;
    struct depth_init_outputparam *outputinfo = NULL;
    void *depth_handle = NULL;
    struct isp_depth_cali_info cali_info;
    depth_mode mode = MODE_CAPTURE;
    outFormat format = MODE_DISPARITY;
    weightmap_param *wParams = NULL;
    if (subbuffer == NULL || mainbuffer == NULL || Depthoutbuffer == NULL) {
        HAL_LOGE("buffer is null");
        goto exit;
    }
    memset(&cali_info, 0, sizeof(isp_depth_cali_info));
    property_get("persist.sys.camera.refocus.otp", otp, "0");
    property_get("persist.sys.cam.sbs.savefile", dump, "0");

    sprd_depth_VersionInfo_Get(acVersion, 256);
    HAL_LOGD("sprd_depthmap[%s]", acVersion);

    // read otp from sensor
    mDevMain->hwi->getDualOtpData(&otp_data, &otp_size, &has_read_otp);
    HAL_LOGD("OTP INFO:addr %p, size = %d, have_read = %d", otp_data, otp_size,
             has_read_otp);

    if (has_read_otp && strcmp(otp, "1")) {
        memcpy(Sensor_Otp_Data, (char *)otp_data, otp_size);
        if (!strcmp(dump, "1")) {
            HAL_LOGD("OTP INFO:dump otp");
            mSidebyside->dumpFile((unsigned char *)Sensor_Otp_Data,
                                  SBS_FILE_OTP, otp_size, 0, 0, 0, 0, 0);
        }
    } else {
        fid = fopen("data/misc/cameraserver/otp.txt", "r");
        if (fid == NULL) {
            HAL_LOGE("file open fail");
        } else {
            HAL_LOGI("fid %p", fid);
            OTP_Datanum = 0;
            while (!feof(fid)) {
                fscanf(fid, "%d\n", &OTP_Data[OTP_Datanum]);
                HAL_LOGI("OTP_Datanum %d", OTP_Data[OTP_Datanum]);
                OTP_Datanum++;
            }
            fclose(fid);
            fid == NULL;
        }
    }

    if (m3dVerificationEnable) {
        memcpy(mOtpData, (char *)otp_data, otp_size);
    }

    inparam = (struct depth_init_inputparam *)malloc(
        sizeof(struct depth_init_inputparam));
    outputinfo = (struct depth_init_outputparam *)malloc(
        sizeof(struct depth_init_outputparam));
    inparam->input_width_main = mSidebyside->mMainCaptureWidth;
    inparam->input_height_main = mSidebyside->mMainCaptureHeight;
    inparam->input_width_sub = mSidebyside->mSubCaptureWidth;
    inparam->input_height_sub = mSidebyside->mSubCaptureHeight;
    inparam->output_depthwidth = mSidebyside->mCapDepthWidth;
    inparam->output_depthheight = mSidebyside->mCapDepthHeight;
    inparam->online_depthwidth = 0;
    inparam->online_depthheight = 0;
    inparam->online_threadNum = 0;
    inparam->imageFormat_main = YUV420_NV12;
    inparam->imageFormat_sub = YUV420_NV12;
    inparam->depth_threadNum = 1;
    if (has_read_otp && strcmp(otp, "1"))
        inparam->potpbuf = Sensor_Otp_Data;
    else
        inparam->potpbuf = OTP_Data;
    inparam->otpsize = OTP_Datanum;
    inparam->config_param = (char *)(&sprd_depth_config_para);
    HAL_LOGI("inparam %p, outputinfo %p,", inparam, outputinfo);
    depth_handle = sprd_depth_Init(inparam, outputinfo, mode, format);
    mSidebyside->mDepthHandle = depth_handle;
    if (!depth_handle)
        HAL_LOGE("sprd_depth_Init fail ret %p", depth_handle);
    HAL_LOGD("outputinfo outputsize %d, w: %d, h: %d", outputinfo->outputsize,
             outputinfo->calibration_width, outputinfo->calibration_height);
    mSidebyside->mDepthBufferSize = outputinfo->outputsize;

    HAL_LOGD("Disparity %p, subbuffer %p, mainbuffer %p", Depthoutbuffer,
             subbuffer, mainbuffer);

    // get online out buffer
    mDevMain->hwi->getOnlineBuffer(&cali_info);
    HAL_LOGD("online buffer addr %p", &cali_info);
    if (!cali_info.calbinfo_valid) {
        cali_info.buffer = NULL;
    }

    mSidebyside->mDepthState = 1;

    static int index = 0;
    char prop[PROPERTY_VALUE_MAX];
    char filename[128];
    property_get("save_onlinecalbinfo", prop, "no");
    if (!strcmp(prop, "yes") && (NULL != cali_info.buffer)) {
        sprintf(filename, "hal_save_onlineclab_%d.dump", index);
        save_onlinecalbinfo(filename, (uint8_t *)cali_info.buffer,
                            cali_info.size);
        sprintf(filename, "%dx%d_onlinecalb_capture_sub_index%d.yuv",
                mSidebyside->mSubCaptureWidth, mSidebyside->mSubCaptureHeight,
                index);
        savefile(filename, (uint8_t *)subbuffer, mSidebyside->mSubCaptureWidth,
                 mSidebyside->mSubCaptureHeight);
        sprintf(filename, "%dx%d_onlinecalb_capture_main_index%d.yuv",
                mSidebyside->mMainCaptureWidth, mSidebyside->mMainCaptureHeight,
                index);
        savefile(filename, (uint8_t *)mainbuffer,
                 mSidebyside->mMainCaptureWidth,
                 mSidebyside->mMainCaptureHeight);
        index++;
    }

    ret = sprd_depth_Run(depth_handle, (void *)Depthoutbuffer,
                         (void *)cali_info.buffer, (void *)subbuffer,
                         (void *)mainbuffer, wParams);
    if (ret)
        HAL_LOGE("sprd_depth_Run fail, ret %d", ret);
    mSidebyside->mDepthState = 0;

    sprd_depth_Close(depth_handle);
    if (inparam != NULL) {
        free(inparam);
        inparam = NULL;
    }
    if (outputinfo != NULL) {
        free(outputinfo);
        outputinfo = NULL;
    }
exit:
    HAL_LOGI("X");
}

void SprdCamera3SideBySideCamera::CaptureThread::ProcessBokehImage(
    unsigned char *inputbuffer, unsigned char *outputbuffer,
    unsigned char *Depthoutbuffer) {
    int ret = 0;
    char acVersion[256];
    FILE *fid = NULL;
    char *param = NULL;
    int width = 0;
    int height = 0;
    void *bokeh_handle = NULL;

    if (inputbuffer == NULL || outputbuffer == NULL || Depthoutbuffer == NULL) {
        HAL_LOGE("buffer is null");
        goto exit;
    }
    /*fid = fopen("/data/misc/cameraserver/bokeh_param.bin", "r");
    if(fid == NULL) {
        HAL_LOGE("file open fail");
        goto exit;
    }
    param = NULL*/;

    sprd_bokeh_VersionInfo_Get(acVersion, 256);
    HAL_LOGD("sprd_depthmap[%s]", acVersion);

    width = mSidebyside->mScalingUpWidth;
    height = mSidebyside->mScalingUpHeight;

    HAL_LOGD("width = %d, height = %d", width, height);

    ret = sprd_bokeh_Init(&bokeh_handle, width, height, param);
    if (ret)
        HAL_LOGE("sprd_bokeh_Init fail ret %d", ret);

    ret = sprd_bokeh_ReFocusPreProcess(bokeh_handle, (void *)inputbuffer,
                                       (void *)Depthoutbuffer);
    if (ret)
        HAL_LOGE("sprd_depth_Run fail, ret %d", ret);

    HAL_LOGD("BokehLevel: %d, postion x: %d, y: %d", mSidebyside->mBokehLevel,
             width / 2, height / 2);
    ret =
        sprd_bokeh_ReFocusGen(bokeh_handle, (void *)outputbuffer,
                              mSidebyside->mBokehLevel, width / 2, height / 2);
    if (ret)
        HAL_LOGE("sprd_depth_Run fail, ret %d", ret);

    sprd_bokeh_Close(bokeh_handle);
// fclose(fid);
exit:
    HAL_LOGI("X");
}

void SprdCamera3SideBySideCamera::CaptureThread::Save3dVerificationParam(
    buffer_handle_t *mSavedResultBuff, unsigned char *main_yuv,
    unsigned char *sub_yuv) {
    int i = 0;
    char prop[PROPERTY_VALUE_MAX];
    property_get("persist.sys.cam.sbs.savefile", prop, "0");

    unsigned char *buffer_base =
        (unsigned char *)((struct private_handle_t *)*mSavedResultBuff)->base;
    uint32_t buffer_size = ((struct private_handle_t *)*mSavedResultBuff)->size;

    // refocus commom param
    uint32_t WidethData = mSidebyside->mMainCaptureWidth;
    uint32_t HeightData = mSidebyside->mMainCaptureHeight;
    uint32_t SizeData = WidethData * HeightData * 3 / 2;
    uint32_t OtpSize = SPRD_DUAL_OTP_SIZE;
    unsigned char VerificationFlag[] = {'V', 'E', 'R', 'I'};

    unsigned char *p[] = {(unsigned char *)&WidethData,
                          (unsigned char *)&HeightData,
                          (unsigned char *)&SizeData, (unsigned char *)&OtpSize,
                          (unsigned char *)&VerificationFlag};

    // cpoy common param to tail
    buffer_base += (buffer_size - 5 * 4);
    HAL_LOGD("common param base=%p", buffer_base);
    for (i = 0; i < 5; i++) {
        memcpy(buffer_base + i * 4, p[i], 4);
    }

    // cpoy otp data to before param
    buffer_base -= (uint32_t)SPRD_DUAL_OTP_SIZE;
    HAL_LOGD("otp base=%p, ", buffer_base);
    memcpy(buffer_base, mOtpData, SPRD_DUAL_OTP_SIZE);

    if (!strcmp(prop, "1")) {
        mSidebyside->dumpFile((unsigned char *)buffer_base, SBS_FILE_OTP,
                              SPRD_DUAL_OTP_SIZE, 0, 0, 0, 1, 0);
    }

    // cpoy sub yuv to before otp
    buffer_base -= SizeData;
    HAL_LOGD("sub yuv base=%p, ", buffer_base);
    memcpy(buffer_base, sub_yuv, SizeData);

    if (!strcmp(prop, "1")) {
        mSidebyside->dumpFile(buffer_base, SBS_FILE_NV21, (int)SizeData,
                              WidethData, HeightData, 0, 7, 0);
    }

    // cpoy main yuv to before sub yuv
    buffer_base -= SizeData;
    HAL_LOGD("main yuv base=%p, ", buffer_base);
    memcpy(buffer_base, main_yuv, SizeData);

    if (!strcmp(prop, "1")) {
        mSidebyside->dumpFile(buffer_base, SBS_FILE_NV21, (int)SizeData,
                              WidethData, HeightData, 0, 6, 0);
    }

    HAL_LOGD("buffer size:%d", buffer_size);
}

void SprdCamera3SideBySideCamera::CaptureThread::saveCaptureBokehParams(
    buffer_handle_t *mSavedResultBuff, unsigned char *buffer,
    unsigned char *disparity_bufer) {
    int i = 0;
    unsigned char *buffer_base =
        (unsigned char *)((struct private_handle_t *)*mSavedResultBuff)->base;
    uint32_t buffer_size = ((struct private_handle_t *)*mSavedResultBuff)->size;
    void *ispInfoAddr = NULL;
    int ispInfoSize = 0;
    char userdebug[PROPERTY_VALUE_MAX] = {0};

    // get preview focus point
    mDevMain->hwi->GetFocusPoint(&(mSidebyside->mBokehPointX),
                                 &(mSidebyside->mBokehPointY));
    if (0 == mSidebyside->mBokehPointX && 0 == mSidebyside->mBokehPointY) {
        mSidebyside->mBokehPointX = 800;
        mSidebyside->mBokehPointY = 600;
    }
    HAL_LOGD("x %d, y %d", mSidebyside->mBokehPointX,
             mSidebyside->mBokehPointY);

    // refocus commom param
    uint32_t MainWidthData = mSidebyside->mScalingUpWidth;
    uint32_t MainHeightData = mSidebyside->mScalingUpHeight;
    uint32_t MainSizeData = MainWidthData * MainHeightData * 3 / 2;
    uint32_t DepthWidethData = mSidebyside->mCapDepthWidth;
    uint32_t DepthHeightData = mSidebyside->mCapDepthHeight;
    uint32_t DepthSizeData = mSidebyside->mDepthBufferSize;
    uint32_t BokehLevel = mSidebyside->mBokehLevel;
    uint32_t InPositionX = mSidebyside->mBokehPointX;
    uint32_t InPositionY = mSidebyside->mBokehPointY;
    uint32_t param_state = mBokehConfigParamState;
    uint32_t rotation = SprdCamera3Setting::s_setting[mSidebyside->mCameraId]
                            .jpgInfo.orientation;
    uint32_t processedsr = mSidebyside->mIsSrProcessed;
    unsigned char BokehFlag[] = {'B', 'O', 'K', 'E'};
    HAL_LOGD("rotation %d", rotation);
    unsigned char *p[] = {
        (unsigned char *)&MainWidthData,   (unsigned char *)&MainHeightData,
        (unsigned char *)&MainSizeData,    (unsigned char *)&DepthWidethData,
        (unsigned char *)&DepthHeightData, (unsigned char *)&DepthSizeData,
        (unsigned char *)&BokehLevel,      (unsigned char *)&InPositionX,
        (unsigned char *)&InPositionY,     (unsigned char *)&param_state,
        (unsigned char *)&rotation,        (unsigned char *)&processedsr,
        (unsigned char *)&BokehFlag};

    // copy common param to tail
    buffer_base += (buffer_size - BOKEH_REFOCUS_COMMON_PARAM_NUM * 4);
    HAL_LOGD("common param base=%p", buffer_base);
    for (i = 0; i < BOKEH_REFOCUS_COMMON_PARAM_NUM; i++) {
        memcpy(buffer_base + i * 4, p[i], 4);
    }

    // copy depth yuv to before param
    buffer_base -= DepthSizeData;
    HAL_LOGD("yuv base=%p, ", buffer_base);
    memcpy(buffer_base, disparity_bufer, DepthSizeData);

    // copy main yuv to before depth
    buffer_base -= MainSizeData;
    HAL_LOGD("yuv base=%p, ", buffer_base);
    memcpy(buffer_base, buffer, MainSizeData);

    // copy sub isp debug info if userdebug version before main yuv
    property_get("ro.debuggable", userdebug, "0");
    if (!strcmp(userdebug, "1")) {
        // get sub camera debug info
        mDevAux->hwi->getIspDebugInfo(&ispInfoAddr, &ispInfoSize);
        HAL_LOGD("ISP INFO:addr 0x%p, size = %d", ispInfoAddr, ispInfoSize);
        buffer_base -= ispInfoSize;
        HAL_LOGD("sub isp debug info base=%p, ", buffer_base);
        memcpy(buffer_base, ispInfoAddr, ispInfoSize);
    }
    HAL_LOGD("buffer size:%d", buffer_size);
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
bool SprdCamera3SideBySideCamera::CaptureThread::threadLoop() {
    buffer_handle_t *output_buffer = NULL;
    sidebyside_queue_msg_t capture_msg;
    int mime_type = (int)MODE_BLUR;
    int ret = 0;
    uint32_t frame_number = 0;
    cmr_s64 timestamp = 0;
    HAL_LOGI("E");

    while (!mCaptureMsgList.empty()) {
        List<sidebyside_queue_msg_t>::iterator itor1 = mCaptureMsgList.begin();
        capture_msg = *itor1;
        mCaptureMsgList.erase(itor1);
        switch (capture_msg.msg_type) {
        case SBS_MSG_EXIT: {
            // flush queue
            memset(&mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
            memset(&mSavedCapReqstreambuff, 0, sizeof(camera3_stream_buffer_t));
            if (NULL != mSavedCapReqsettings) {
                free_camera_metadata(mSavedCapReqsettings);
                mSavedCapReqsettings = NULL;
            }
            HAL_LOGD("SBS_MSG_EXIT");
            return false;
        }

        case SBS_MSG_FLUSHING_PROC: {
            output_buffer = &mSidebyside->mLocalCapBuffer[2].native_handle;
            HAL_LOGD("FLUSHING_PROC: mFlushing=%d, frame_number=%d",
                     mSidebyside->mFlushing,
                     capture_msg.combo_buff.frame_number);
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
                if (output_buffers != NULL) {
                    free((void *)output_buffers);
                    output_buffers = NULL;
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
            input_buffer->stream->width = mSidebyside->mMainCaptureWidth;
            input_buffer->stream->height = mSidebyside->mMainCaptureHeight;
            input_buffer->buffer = output_buffer;

            memcpy((void *)&output_buffers[0], &mSavedCapReqstreambuff,
                   sizeof(camera3_stream_buffer_t));
            output_buffers[0].stream = &mMainStreams[mCaptureStreamsNum - 1];
            output_buffers[0].stream->width = mSidebyside->mMainCaptureWidth;
            output_buffers[0].stream->height = mSidebyside->mMainCaptureHeight;
            HAL_LOGD("output_buffers.buffer=%p,input_buffer.buffer=%p",
                     output_buffers->buffer, input_buffer->buffer);
            request.output_buffers = output_buffers;
            request.input_buffer = input_buffer;
            mReprocessing = true;
            ((struct private_handle_t *)(*request.output_buffers[0].buffer))
                ->size = ((struct private_handle_t *)*mSavedResultBuff)->size -
                         (mSidebyside->mMainCaptureWidth *
                              mSidebyside->mMainCaptureHeight * 3 / 2 +
                          mSidebyside->mDepthBufferSize +
                          BOKEH_REFOCUS_COMMON_PARAM_NUM * 4 +
                          mSidebyside->mIspdebuginfosize);
            HAL_LOGD(
                "output buffers w=%d, h=%d, mMainCaptureWidth=%d, "
                "mMainCaptureHeight=%d, output_buffers size=%d",
                ((struct private_handle_t *)(*request.output_buffers[0].buffer))
                    ->width,
                ((struct private_handle_t *)(*request.output_buffers[0].buffer))
                    ->height,
                mSidebyside->mMainCaptureWidth, mSidebyside->mMainCaptureHeight,
                ((struct private_handle_t *)(*request.output_buffers[0].buffer))
                    ->size);

            ret =
                mDevMain->hwi->process_capture_request(mDevMain->dev, &request);
            if (ret) {
                HAL_LOGE("process_capture_request failed");
            }

            if (NULL != mSavedCapReqsettings) {
                free_camera_metadata(mSavedCapReqsettings);
                mSavedCapReqsettings = NULL;
            }
            mSidebyside->mIsWaitSnapYuv = false;
            HAL_LOGD("jpeg request ok");
            if (input_buffer != NULL) {
                free((void *)input_buffer);
                input_buffer = NULL;
            }
            if (output_buffers != NULL) {
                free((void *)output_buffers);
                output_buffers = NULL;
            }
        } break;

        case SBS_MSG_MAIN_PROC: {
            unsigned char *mainRawbuffer =
                (unsigned char *)(((struct private_handle_t *)*(
                                       capture_msg.combo_buff.main_buffer))
                                      ->base);
            unsigned char *main_yuv =
                (unsigned char *)(((struct private_handle_t *)*(
                                       &mSidebyside->mLocalCapBuffer[5]
                                            .native_handle))
                                      ->base);
            unsigned char *sub_yuv =
                (unsigned char *)(((struct private_handle_t *)*(
                                       &mSidebyside->mLocalCapBuffer[6]
                                            .native_handle))
                                      ->base);
            unsigned char *Disparity =
                (unsigned char *)(((struct private_handle_t *)*(
                                       &mSidebyside->mLocalCapBuffer[4]
                                            .native_handle))
                                      ->base);
            unsigned char *mainbuffer =
                (unsigned char *)(((struct private_handle_t *)*(
                                       &mSidebyside->mLocalCapBuffer[2]
                                            .native_handle))
                                      ->base);
            unsigned char *subbuffer =
                (unsigned char *)(((struct private_handle_t *)*(
                                       &mSidebyside->mLocalCapBuffer[3]
                                            .native_handle))
                                      ->base);
            unsigned char *SuperResolution =
                (unsigned char *)(((struct private_handle_t *)*(
                                       &mSidebyside->mLocalCapBuffer[7]
                                            .native_handle))
                                      ->base);
            output_buffer = &mSidebyside->mLocalCapBuffer[1].native_handle;
            unsigned char *outputbuffer =
                (unsigned char *)(((struct private_handle_t *)*(output_buffer))
                                      ->base);

            char prop[PROPERTY_VALUE_MAX];
            property_get("persist.sys.cam.sbs.savefile", prop, "0");

            HAL_LOGD("mFlushing:%d, stop preview and raw proc",
                     mSidebyside->mFlushing);
            if (!mSidebyside->mFlushing) {
                mDevMain->hwi->stopPreview();
                mDevMain->hwi->setCameraClearQBuff();
                HAL_LOGD("process 3200*1200 raw for yuv");

                // STEP_1.1:main raw proc
                struct img_sbs_info main_sbs_info;
                main_sbs_info.sbs_mode = SPRD_SBS_MODE_LEFT;
                main_sbs_info.size.width = SBS_RAW_DATA_WIDTH;
                main_sbs_info.size.height = SBS_RAW_DATA_HEIGHT;
                ret = mDevMain->hwi->rawPostProc(
                    capture_msg.combo_buff.main_buffer,
                    &mSidebyside->mLocalCapBuffer[2].native_handle,
                    &main_sbs_info);
                if (ret) {
                    HAL_LOGE("1st raw proc fail, %d", ret);
                    return false;
                }
                // STEP_1.2:sub raw proc
                struct img_sbs_info sub_sbs_info;
                sub_sbs_info.sbs_mode = SPRD_SBS_MODE_RIGHT;
                sub_sbs_info.size.width = SBS_RAW_DATA_WIDTH;
                sub_sbs_info.size.height = SBS_RAW_DATA_HEIGHT;
                ret = mDevAux->hwi->rawPostProc(
                    capture_msg.combo_buff.main_buffer,
                    &mSidebyside->mLocalCapBuffer[3].native_handle,
                    &sub_sbs_info);
                if (ret) {
                    HAL_LOGE("2nd raw proc fail, %d", ret);
                    return false;
                }
                ret = mDevAux->hwi->rawPostProc(
                    capture_msg.combo_buff.main_buffer,
                    &mSidebyside->mLocalCapBuffer[3].native_handle,
                    &sub_sbs_info);
                if (ret) {
                    HAL_LOGE("3rd raw proc fail, %d", ret);
                    return false;
                }

                // dump raw & 2-yuv
                if (!strcmp(prop, "1")) {
                    mSidebyside->dumpFile(
                        mainRawbuffer, SBS_FILE_RAW,
                        (int)((struct private_handle_t *)*(
                                  capture_msg.combo_buff.main_buffer))
                            ->size,
                        mSidebyside->mCaptureWidth, mSidebyside->mCaptureHeight,
                        capture_msg.combo_buff.frame_number, 0, 0);
                    mSidebyside->dumpFile(
                        mainbuffer, SBS_FILE_NV21,
                        (int)((struct private_handle_t *)*(
                                  &mSidebyside->mLocalCapBuffer[2]
                                       .native_handle))
                            ->size,
                        mSidebyside->mMainCaptureWidth,
                        mSidebyside->mMainCaptureHeight,
                        capture_msg.combo_buff.frame_number, 0, 0);
                    mSidebyside->dumpFile(
                        subbuffer, SBS_FILE_NV21,
                        (int)((struct private_handle_t *)*(
                                  &mSidebyside->mLocalCapBuffer[3]
                                       .native_handle))
                            ->size,
                        mSidebyside->mSubCaptureWidth,
                        mSidebyside->mSubCaptureHeight,
                        capture_msg.combo_buff.frame_number, 1, 1);
                }
                mDevMain->hwi->startPreview();
            }

            HAL_LOGD("mFlushing=%d, thumb start process",
                     mSidebyside->mFlushing);
            if (!mSidebyside->mFlushing) {
                // STEP_2.1:process thumb
                HAL_LOGD("process thumb");
                if (mSidebyside->mhasCallbackStream &&
                    mSidebyside->mThumbReq.frame_number) {
                    mSidebyside->thumbYuvProc(
                        &mSidebyside->mLocalCapBuffer[2].native_handle);
                }
            }
            HAL_LOGD("mFlushing=%d, thumb callback start process",
                     mSidebyside->mFlushing);
            if (!mSidebyside->mFlushing) {
                // STEP_2.2:callback thumb
                if (mSidebyside->mhasCallbackStream &&
                    mSidebyside->mThumbReq.frame_number) {
                    mSidebyside->CallBackSnapResult();
                }
            }

            /* scaling down moved to depth.so
            HAL_LOGD("mFlushing=%d, get depth input buffer",
                     mSidebyside->mFlushing);
            if (!mSidebyside->mFlushing) {
                mDevMain->hwi->getDepthBuffer(
                    &mSidebyside->mLocalCapBuffer[2].native_handle,
                    &mSidebyside->mLocalCapBuffer[5].native_handle);
                mDevMain->hwi->getDepthBuffer(
                    &mSidebyside->mLocalCapBuffer[3].native_handle,
                    &mSidebyside->mLocalCapBuffer[6].native_handle);
                if (!strcmp(prop, "1") && strcmp(prop1, "1")) {
                    mSidebyside->dumpFile(
                        main_yuv, SBS_FILE_NV21,
                        (int)((struct private_handle_t *)*(
                                  &mSidebyside->mLocalCapBuffer[2]
                                       .native_handle))
                            ->size,
                        mSidebyside->mDepInputWidth,
                        mSidebyside->mDepInputHeight,
                        capture_msg.combo_buff.frame_number, 4, 0);
                    mSidebyside->dumpFile(
                        sub_yuv, SBS_FILE_NV21,
                        (int)((struct private_handle_t *)*(
                                  &mSidebyside->mLocalCapBuffer[3]
                                       .native_handle))
                            ->size,
                        mSidebyside->mDepInputWidth,
                        mSidebyside->mDepInputHeight,
                        capture_msg.combo_buff.frame_number, 5, 1);
                }
            }
            */

            // FOR TEST:get 3d verification tag
            char verifi[PROPERTY_VALUE_MAX];
            property_get("persist.sys.cam.tool.debug", verifi, "0");
            if (!strcmp(verifi, "1")) {
                m3dVerificationEnable = 1;
            } else {
                m3dVerificationEnable = 0;
            }
            HAL_LOGD("3d verification %d", m3dVerificationEnable);

            HAL_LOGD("mFlushing:%d, depth lib start process",
                     mSidebyside->mFlushing);
            if (!mSidebyside->mFlushing) {
                // STEP_3.1: process depth
                // if (BLUR_TIPS_OK == mSidebyside->mCoveredValue ||
                //    m3dVerificationEnable) {
                ProcessDepthImage(subbuffer, mainbuffer, Disparity);
                //}
                // dump depth buffer
                if (!strcmp(prop, "1")) {
                    mSidebyside->dumpFile(Disparity, SBS_FILE_DEPTH,
                                          mSidebyside->mDepthBufferSize,
                                          mSidebyside->mCapDepthWidth,
                                          mSidebyside->mCapDepthHeight,
                                          capture_msg.combo_buff.frame_number,
                                          0, 0);
                }
            }

            // scaling up to 5M
            if (!mSidebyside->mFlushing) {
                mDevMain->hwi->getDepthBuffer(
                    &mSidebyside->mLocalCapBuffer[2].native_handle,
                    &mSidebyside->mLocalCapBuffer[7].native_handle);
            }

            HAL_LOGD("mFlushing=%d, bokeh lib start process",
                     mSidebyside->mFlushing);
            if (!mSidebyside->mFlushing) {
#if 0
                //STEP_4.1:process bokeh
                ProcessBokehImage(SuperResolution, outputbuffer, Disparity);
#else
                cmr_u32 size = 0;
                size = mSidebyside->mScalingUpWidth *
                       mSidebyside->mScalingUpHeight * 3 / 2;
                memcpy(outputbuffer, SuperResolution, size);
#endif

                HAL_LOGD("mSavedResultBuff=%p,capture_msg.combo_buff.buffer=%p",
                         mSavedResultBuff, capture_msg.combo_buff.main_buffer);
                if (m3dVerificationEnable) {
                    Save3dVerificationParam(mSavedResultBuff, mainbuffer,
                                            subbuffer);
                } else {
                    // if (BLUR_TIPS_OK == mSidebyside->mCoveredValue) {
                    saveCaptureBokehParams(mSavedResultBuff, SuperResolution,
                                           Disparity);
                    // }
                }
            }

            // STEP_5.1: process input capture request
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
                if (output_buffers != NULL) {
                    free((void *)output_buffers);
                    output_buffers = NULL;
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
            input_buffer->stream->width = mSidebyside->mScalingUpWidth;
            input_buffer->stream->height = mSidebyside->mScalingUpHeight;
            input_buffer->buffer = output_buffer;

            memcpy((void *)&output_buffers[0], &mSavedCapReqstreambuff,
                   sizeof(camera3_stream_buffer_t));
            output_buffers[0].stream = &mMainStreams[mCaptureStreamsNum - 1];
            output_buffers[0].stream->width = mSidebyside->mScalingUpWidth;
            output_buffers[0].stream->height = mSidebyside->mScalingUpHeight;

            HAL_LOGD("output_buffers.buffer=%p,input_buffer.buffer=%p",
                     output_buffers->buffer, input_buffer->buffer);
            request.output_buffers = output_buffers;
            request.input_buffer = input_buffer;
            request.num_output_buffers = 1;
            mReprocessing = true;
            ((struct private_handle_t *)(*request.output_buffers[0].buffer))
                ->size = ((struct private_handle_t *)*mSavedResultBuff)->size -
                         (mSidebyside->mScalingUpWidth *
                              mSidebyside->mScalingUpHeight * 3 / 2 +
                          mSidebyside->mDepthBufferSize +
                          BOKEH_REFOCUS_COMMON_PARAM_NUM * 4 +
                          mSidebyside->mIspdebuginfosize);
            HAL_LOGD(
                "output buffers w=%d, h=%d, mMainCaptureWidth=%d, "
                "mMainCaptureHeight=%d, output_buffers size = %d",
                ((struct private_handle_t *)(*request.output_buffers[0].buffer))
                    ->width,
                ((struct private_handle_t *)(*request.output_buffers[0].buffer))
                    ->height,
                mSidebyside->mScalingUpWidth, mSidebyside->mScalingUpHeight,
                ((struct private_handle_t *)(*request.output_buffers[0].buffer))
                    ->size);

            /*if (BLUR_TIPS_OK == mSidebyside->mCoveredValue ||
                m3dVerificationEnable) {
                mime_type = (1 << 8) | (int)MODE_BLUR;
            } else {
                mime_type = 0;
            }*/
            mime_type = (1 << 8) | (int)MODE_BLUR;
            mDevMain->hwi->camera_ioctrl(CAMERA_IOCTRL_SET_MIME_TYPE,
                                         &mime_type, NULL);

            ret =
                mDevMain->hwi->process_capture_request(mDevMain->dev, &request);
            if (ret) {
                HAL_LOGE("process_capture_request failed");
            }

            if (NULL != mSavedCapReqsettings) {
                free_camera_metadata(mSavedCapReqsettings);
                mSavedCapReqsettings = NULL;
            }
            mSidebyside->mIsWaitSnapYuv = false;
            HAL_LOGD("jpeg request ok");
            if (input_buffer != NULL) {
                free((void *)input_buffer);
                input_buffer = NULL;
            }
            if (output_buffers != NULL) {
                free((void *)output_buffers);
                output_buffers = NULL;
            }
            mSidebyside->mRealtimeBokeh = 1;
            // reset mRealtimeBokehNum
            mRealtimeBokehNum = 0;
        } break;
        case SBS_MSG_PREV_PROC: {
            HAL_LOGD("%p -%p -%p -%p -%p -%p -%p -%p -%p -%p -%p -%p -%p -%p",
                     mSidebyside->mLocalCapBuffer[0].native_handle,
                     mSidebyside->mLocalCapBuffer[1].native_handle,
                     mSidebyside->mLocalCapBuffer[2].native_handle,
                     mSidebyside->mLocalCapBuffer[3].native_handle,
                     mSidebyside->mLocalCapBuffer[4].native_handle,
                     mSidebyside->mLocalCapBuffer[5].native_handle,
                     mSidebyside->mLocalCapBuffer[6].native_handle,
                     mSidebyside->mLocalCapBuffer[7].native_handle,
                     mSidebyside->mLocalCapBuffer[8].native_handle,
                     mSidebyside->mLocalCapBuffer[9].native_handle,
                     mSidebyside->mLocalCapBuffer[10].native_handle,
                     mSidebyside->mLocalCapBuffer[11].native_handle,
                     mSidebyside->mLocalCapBuffer[12].native_handle,
                     mSidebyside->mLocalCapBuffer[13].native_handle);
            unsigned char *Prev_buffer_1 =
                (unsigned char *)(((struct private_handle_t *)*(
                                       &mSidebyside->mLocalCapBuffer[8]
                                            .native_handle))
                                      ->base);
            unsigned char *Prev_buffer_2 =
                (unsigned char *)(((struct private_handle_t *)*(
                                       &mSidebyside->mLocalCapBuffer[9]
                                            .native_handle))
                                      ->base);
            unsigned char *prev_out_buffer =
                (unsigned char *)(((struct private_handle_t *)*(
                                       capture_msg.prev_combo_buff.prev_buffer))
                                      ->base);
            unsigned char *Depth_buffer =
                (unsigned char *)(((struct private_handle_t *)*(
                                       &mSidebyside->mLocalCapBuffer[10]
                                            .native_handle))
                                      ->base);
            unsigned char *Raw_buffer_1 =
                (unsigned char *)(((struct private_handle_t *)*(
                                       &mSidebyside->mLocalCapBuffer[11]
                                            .native_handle))
                                      ->base);
            unsigned char *Raw_buffer_2 =
                (unsigned char *)(((struct private_handle_t *)*(
                                       &mSidebyside->mLocalCapBuffer[12]
                                            .native_handle))
                                      ->base);

// GPU buffer
#if defined(CONFIG_SPRD_ANDROID_8)
            uint32_t inLayCount = 1;
            uint64_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                                    GraphicBuffer::USAGE_SW_READ_OFTEN |
                                    GraphicBuffer::USAGE_SW_WRITE_OFTEN;
#else
            uint32_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                                    GraphicBuffer::USAGE_SW_READ_OFTEN |
                                    GraphicBuffer::USAGE_SW_WRITE_OFTEN;
#endif
            int32_t yuvTextFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
            uint32_t inWidth = 0, inHeight = 0, inStride = 0;

            inWidth = mSidebyside->mPreviewWidth;
            inHeight = mSidebyside->mPreviewHeight;
            inStride = mSidebyside->mPreviewWidth;
            sp<GraphicBuffer> srcBuffer = NULL;
            sp<GraphicBuffer> dstBuffer = NULL;

            // check buff
            cmr_s32 is_using = 0;
            is_using = mDevMain->hwi->ispSwCheckBuf((cmr_uint *)Prev_buffer_1);
            HAL_LOGV("buffer is using %d", is_using);
            cmr_u8 *prev_buffer = NULL;
            cmr_u8 *raw_buffer = NULL;
            if (is_using) {
                prev_buffer = Prev_buffer_2;
                raw_buffer = Raw_buffer_2;
#if defined(CONFIG_SPRD_ANDROID_8)
                srcBuffer = new GraphicBuffer(
                    (native_handle_t *)(*(
                        &mSidebyside->mLocalCapBuffer[9].native_handle)),
                    GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, inWidth,
                    inHeight, yuvTextFormat, inLayCount, yuvTextUsage,
                    inStride);
#else
                srcBuffer = new GraphicBuffer(
                    inWidth, inHeight, yuvTextFormat, yuvTextUsage, inStride,
                    (native_handle_t *)(*(
                        &mSidebyside->mLocalCapBuffer[9].native_handle)),
                    0);
#endif
            } else {
                prev_buffer = Prev_buffer_1;
                raw_buffer = Raw_buffer_1;
#if defined(CONFIG_SPRD_ANDROID_8)
                srcBuffer = new GraphicBuffer(
                    (native_handle_t *)(*(
                        &mSidebyside->mLocalCapBuffer[8].native_handle)),
                    GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, inWidth,
                    inHeight, yuvTextFormat, inLayCount, yuvTextUsage,
                    inStride);
#else
                srcBuffer = new GraphicBuffer(
                    inWidth, inHeight, yuvTextFormat, yuvTextUsage, inStride,
                    (native_handle_t *)(*(
                        &mSidebyside->mLocalCapBuffer[8].native_handle)),
                    0);
#endif
            }
#if defined(CONFIG_SPRD_ANDROID_8)
            dstBuffer = new GraphicBuffer(
                (native_handle_t *)(*capture_msg.prev_combo_buff.prev_buffer),
                GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, inWidth,
                inHeight, yuvTextFormat, inLayCount, yuvTextUsage, inStride);
#else
            dstBuffer = new GraphicBuffer(
                inWidth, inHeight, yuvTextFormat, yuvTextUsage, inStride,
                (native_handle_t *)(*capture_msg.prev_combo_buff.prev_buffer),
                0);
#endif
            HAL_LOGD("buffer is %p & %p & %p", prev_buffer, Prev_buffer_1,
                     Prev_buffer_2);

            // copy preview data to preview buffer
            cmr_s32 prev_size = mSidebyside->mPreviewWidth *
                                mSidebyside->mPreviewHeight * 3 / 2;
            memcpy(prev_buffer, prev_out_buffer, prev_size);

            frame_number = capture_msg.prev_combo_buff.frame_number;
            timestamp = capture_msg.timestamp;
            HAL_LOGV("timestamp %lld", timestamp);

            // get raw frame
            cmr_u8 *raw_frame = NULL;
            mDevMain->hwi->getRawFrame(timestamp, &raw_frame);
            cmr_s32 size = SBS_RAW_DATA_WIDTH * SBS_RAW_DATA_HEIGHT * 2;
            if (NULL != raw_frame) {
                memcpy(raw_buffer, raw_frame, size);
            } else {
                HAL_LOGD("cannot get matched zsl frame");
                mSidebyside->CallBackResult(frame_number,
                                            CAMERA3_BUFFER_STATUS_OK);
                break;
            }

#if 0
            mSidebyside->dumpFile(Raw_buffer, SBS_FILE_NV21,
                    size,
                     SBS_RAW_DATA_WIDTH, SBS_RAW_DATA_HEIGHT, frame_number, 3, 0);
#endif

            if (!mSidebyside->mFlushing) {
                // param combination
                struct soft_isp_frm_param *param_ptr = NULL;
                struct isp_img_frm raw_zsl;
                struct soft_isp_misc_img_frm preview;
                struct soft_isp_misc_img_frm prev_out;
                WeightParams_t depth_param;

                param_ptr = (struct soft_isp_frm_param *)malloc(
                    sizeof(struct soft_isp_frm_param));
                HAL_LOGD("param_ptr %p", param_ptr);

                // constract raw_zsl
                raw_zsl.img_fmt = ISP_DATA_NORMAL_RAW10;
                raw_zsl.img_size.w = SBS_RAW_DATA_WIDTH;
                raw_zsl.img_size.h = SBS_RAW_DATA_HEIGHT;
                raw_zsl.img_addr_vir.chn0 = (cmr_uint)raw_buffer;

                // constract preview
                preview.cpu_frminfo.img_fmt = ISP_DATA_YUV420_2FRAME;
                preview.cpu_frminfo.img_size.w = mSidebyside->mPreviewWidth;
                preview.cpu_frminfo.img_size.h = mSidebyside->mPreviewHeight;
                preview.cpu_frminfo.img_addr_vir.chn0 = (cmr_uint)prev_buffer;
                preview.graphicbuffer = &(*srcBuffer);

                // constract prev_out
                prev_out.cpu_frminfo.img_fmt = ISP_DATA_YUV420_2FRAME;
                prev_out.cpu_frminfo.img_size.w = mSidebyside->mPreviewWidth;
                prev_out.cpu_frminfo.img_size.h = mSidebyside->mPreviewHeight;
                prev_out.cpu_frminfo.img_addr_vir.chn0 =
                    (cmr_uint)prev_out_buffer;
                prev_out.graphicbuffer = &(*dstBuffer);

                // constract depth_param
                depth_param.F_number = mSidebyside->mFnumber;
                depth_param.sel_x = mSidebyside->mPreviewWidth / 2;
                depth_param.sel_y = mSidebyside->mPreviewHeight / 2;
                depth_param.DisparityImage = Depth_buffer;

                param_ptr->raw = raw_zsl;
                param_ptr->m_yuv_pre = preview;
                param_ptr->m_yuv_bokeh = prev_out;
                param_ptr->weightparam = depth_param;
                param_ptr->af_status = 0;

                // forced focus after take picture
                if (!mRealtimeBokehNum) {
                    param_ptr->af_status = 1;
                }
                HAL_LOGD("mRealtimeBokehNum %d, af_status %d",
                         mRealtimeBokehNum, param_ptr->af_status);
                mRealtimeBokehNum++;

                // isp sw proc
                if (BLUR_TIPS_NEED_LIGHT != (mSidebyside->mCoveredValue)) {
                    mDevMain->hwi->ispSwProc(param_ptr);
                }

#if 0
                    mSidebyside->dumpFile(
                        prev_out_buffer, SBS_FILE_NV21,
                        (int)((struct private_handle_t *)*(
                                  capture_msg.prev_combo_buff.prev_buffer))
                            ->size,
                        mSidebyside->mPreviewWidth,
                        mSidebyside->mPreviewHeight,
                        capture_msg.prev_combo_buff.frame_number, 5, 0);
#endif

                // free param_ptr
                if (NULL != param_ptr) {
                    free(param_ptr);
                    param_ptr = NULL;
                }
            }

            // callback
            mSidebyside->CallBackResult(frame_number, CAMERA3_BUFFER_STATUS_OK);
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
void SprdCamera3SideBySideCamera::CaptureThread::requestExit() {
    sidebyside_queue_msg_t sbs_msg;
    sbs_msg.msg_type = SBS_MSG_EXIT;
    mCaptureMsgList.push_back(sbs_msg);
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
void SprdCamera3SideBySideCamera::CaptureThread::waitMsgAvailable() {
    // TODO:what to do for timeout
    while (mCaptureMsgList.empty()) {
        if (mSidebyside->mFlushing) {
            Mutex::Autolock l(mSidebyside->mMergequeueFinishMutex);
            mSidebyside->mMergequeueFinishSignal.signal();
            HAL_LOGD("send mMergequeueFinishSignal.signal");
        }
        {
            Mutex::Autolock l(mMergequeueMutex);
            mMergequeueSignal.waitRelative(mMergequeueMutex,
                                           SBS_THREAD_TIMEOUT);
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
void SprdCamera3SideBySideCamera::dump(const struct camera3_device *device,
                                       int fd) {
    HAL_LOGV("E");
    CHECK_SIDEBYSIDE();

    mSidebyside->_dump(device, fd);

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
int SprdCamera3SideBySideCamera::flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGV("E");
    CHECK_SIDEBYSIDE_ERROR();

    rc = mSidebyside->_flush(device);

    HAL_LOGV("X");

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
int SprdCamera3SideBySideCamera::initialize(
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;

    HAL_LOGI("E");
    CHECK_HWI_ERROR(hwiMain);

    mIommuEnabled = true;
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mPreviewStreamsNum = 0;
    mCaptureThread->mCaptureStreamsNum = 0;
    mCaptureThread->mReprocessing = false;
    mCaptureThread->mSavedResultBuff = NULL;
    mCaptureThread->mSavedCapReqsettings = NULL;
    mjpegSize = 0;
    mFlushing = false;
    mIsWaitSnapYuv = false;
    mIsCapturing = false;
    mJpegOrientation = 0;
    mNotifyListMain.clear();

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
    }
    memset(mLocalCapBuffer, 0, sizeof(new_mem_t) * SBS_LOCAL_CAPBUFF_NUM);
    mCaptureThread->mCallbackOps = callback_ops;
    mCaptureThread->mDevMain = &m_pPhyCamera[CAM_TYPE_MAIN];
    mCaptureThread->mDevAux = &m_pPhyCamera[CAM_TYPE_AUX];
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
int SprdCamera3SideBySideCamera::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGV("E");

    int rc = 0;
    camera3_stream_t *pMainStreams[SBS_MAX_NUM_STREAMS];
    camera3_stream_t *pAuxStreams[SBS_MAX_NUM_STREAMS];
    size_t i = 0;
    size_t j = 0;
    int w = 0;
    int h = 0;
    struct stream_info_s stream_info;
    camera3_stream_t *thumbStream = NULL;
    mhasCallbackStream = false;

    Mutex::Autolock l(mLock);

    HAL_LOGD("num_streams=%d", stream_list->num_streams);
    for (size_t i = 0; i < stream_list->num_streams; i++) {
        HAL_LOGD("stream %d: stream_type=%d, width=%d, height=%d, format=%d", i,
                 stream_list->streams[i]->stream_type,
                 stream_list->streams[i]->width,
                 stream_list->streams[i]->height,
                 stream_list->streams[i]->format);

        int requestStreamType = getStreamType(stream_list->streams[i]);
        if (requestStreamType == PREVIEW_STREAM) {
            mPreviewStreamsNum = i;
            mPreviewWidth = stream_list->streams[i]->width;
            mPreviewHeight = stream_list->streams[i]->height;
        } else if (requestStreamType == SNAPSHOT_STREAM) {
            w = SBS_RAW_DATA_WIDTH;
            h = SBS_RAW_DATA_HEIGHT;

            if (mCaptureWidth != w && mCaptureHeight != h) {
                freeLocalCapBuffer();
                // for raw buffer
                if (0 > allocateBuffer(w, h, 1, HAL_PIXEL_FORMAT_YCbCr_422_SP,
                                       &(mLocalCapBuffer[0]))) {
                    HAL_LOGE("request one buf failed.");
                }
                // for bokeh out buffer
                if (0 > allocateBuffer(mScalingUpWidth, mScalingUpHeight, 1,
                                       HAL_PIXEL_FORMAT_YCrCb_420_SP,
                                       &(mLocalCapBuffer[1]))) {
                    HAL_LOGE("request one buf failed.");
                }
                // for main yuv buffer
                if (0 > allocateBuffer(mMainCaptureWidth, mMainCaptureHeight, 1,
                                       HAL_PIXEL_FORMAT_YCrCb_420_SP,
                                       &(mLocalCapBuffer[2]))) {
                    HAL_LOGE("request one buf failed.");
                }
                // for sub yuv buffer
                if (0 > allocateBuffer(mSubCaptureWidth, mSubCaptureHeight, 1,
                                       HAL_PIXEL_FORMAT_YCrCb_420_SP,
                                       &(mLocalCapBuffer[3]))) {
                    HAL_LOGE("request one buf failed.");
                }
                // for depth buffer
                if (0 > allocateBuffer(mCapDepthWidth, mCapDepthHeight, 1,
                                       HAL_PIXEL_FORMAT_YCrCb_420_SP,
                                       &(mLocalCapBuffer[4]))) {
                    HAL_LOGE("request one buf failed.");
                }
                // for depth lib input two yuv buffer
                if (0 > allocateBuffer(mDepInputWidth, mDepInputHeight, 1,
                                       HAL_PIXEL_FORMAT_YCrCb_420_SP,
                                       &(mLocalCapBuffer[5]))) {
                    HAL_LOGE("request one buf failed.");
                }
                if (0 > allocateBuffer(mDepInputWidth, mDepInputHeight, 1,
                                       HAL_PIXEL_FORMAT_YCrCb_420_SP,
                                       &(mLocalCapBuffer[6]))) {
                    HAL_LOGE("request one buf failed.");
                }
                // for scaled buff
                if (0 > allocateBuffer(mScalingUpWidth, mScalingUpHeight, 1,
                                       HAL_PIXEL_FORMAT_YCrCb_420_SP,
                                       &(mLocalCapBuffer[7]))) {
                    HAL_LOGE("request one buf failed.");
                }
                // for preview buffer
                if (0 > allocateBuffer(mPreviewWidth, mPreviewHeight, 1,
                                       HAL_PIXEL_FORMAT_YCrCb_420_SP,
                                       &(mLocalCapBuffer[8]))) {
                    HAL_LOGE("request one buf failed.");
                }
                if (0 > allocateBuffer(mPreviewWidth, mPreviewHeight, 1,
                                       HAL_PIXEL_FORMAT_YCrCb_420_SP,
                                       &(mLocalCapBuffer[9]))) {
                    HAL_LOGE("request one buf failed.");
                }
                // for preview depth buffer
                if (0 > allocateBuffer(mCapDepthWidth, mCapDepthHeight, 1,
                                       HAL_PIXEL_FORMAT_YCrCb_420_SP,
                                       &(mLocalCapBuffer[10]))) {
                    HAL_LOGE("request one buf failed.");
                }
                // for preview raw buffer
                if (0 > allocateBuffer(SBS_RAW_DATA_WIDTH, SBS_RAW_DATA_HEIGHT,
                                       1, HAL_PIXEL_FORMAT_YCbCr_422_SP,
                                       &(mLocalCapBuffer[11]))) {
                    HAL_LOGE("request one buf failed.");
                }
                if (0 > allocateBuffer(SBS_RAW_DATA_WIDTH, SBS_RAW_DATA_HEIGHT,
                                       1, HAL_PIXEL_FORMAT_YCbCr_422_SP,
                                       &(mLocalCapBuffer[12]))) {
                    HAL_LOGE("request one buf failed.");
                }
                if (0 > allocateBuffer(mIspdebuginfosize, 1, 1,
                                       HAL_PIXEL_FORMAT_YCrCb_420_SP,
                                       &(mLocalCapBuffer[13]))) {
                    HAL_LOGE("request one buf failed.");
                }
            }

            mCaptureWidth = w;
            mCaptureHeight = h;
            mCaptureThread->mCaptureStreamsNum = 2;
            mCaptureThread->mMainStreams[mCaptureThread->mCaptureStreamsNum]
                .max_buffers = 1;
            mCaptureThread->mMainStreams[mCaptureThread->mCaptureStreamsNum]
                .width = w;
            mCaptureThread->mMainStreams[mCaptureThread->mCaptureStreamsNum]
                .height = h;
            mCaptureThread->mMainStreams[mCaptureThread->mCaptureStreamsNum]
                .format = HAL_PIXEL_FORMAT_YCbCr_420_888;
            mCaptureThread->mMainStreams[mCaptureThread->mCaptureStreamsNum]
                .usage = stream_list->streams[i]->usage;
            mCaptureThread->mMainStreams[mCaptureThread->mCaptureStreamsNum]
                .stream_type = CAMERA3_STREAM_OUTPUT;
            mCaptureThread->mMainStreams[mCaptureThread->mCaptureStreamsNum]
                .data_space = stream_list->streams[i]->data_space;
            mCaptureThread->mMainStreams[mCaptureThread->mCaptureStreamsNum]
                .rotation = stream_list->streams[i]->rotation;

            mCaptureThread->mAuxStreams[mCaptureThread->mCaptureStreamsNum]
                .max_buffers = 1;
            mCaptureThread->mAuxStreams[mCaptureThread->mCaptureStreamsNum]
                .width = mSubCaptureWidth;
            mCaptureThread->mAuxStreams[mCaptureThread->mCaptureStreamsNum]
                .height = mSubCaptureHeight;
            mCaptureThread->mAuxStreams[mCaptureThread->mCaptureStreamsNum]
                .format = HAL_PIXEL_FORMAT_YCbCr_420_888;
            mCaptureThread->mAuxStreams[mCaptureThread->mCaptureStreamsNum]
                .usage = stream_list->streams[i]->usage;
            mCaptureThread->mAuxStreams[mCaptureThread->mCaptureStreamsNum]
                .stream_type = CAMERA3_STREAM_OUTPUT;
            mCaptureThread->mAuxStreams[mCaptureThread->mCaptureStreamsNum]
                .data_space = stream_list->streams[i]->data_space;
            mCaptureThread->mAuxStreams[mCaptureThread->mCaptureStreamsNum]
                .rotation = stream_list->streams[i]->rotation;

            pMainStreams[mCaptureThread->mCaptureStreamsNum] =
                &mCaptureThread
                     ->mMainStreams[mCaptureThread->mCaptureStreamsNum];
            pAuxStreams[mCaptureThread->mCaptureStreamsNum] =
                &mCaptureThread
                     ->mAuxStreams[mCaptureThread->mCaptureStreamsNum];
        } else if (requestStreamType == DEFAULT_STREAM) {
            mhasCallbackStream = true;
            (stream_list->streams[i])->max_buffers = 1;
        }

        if (!mhasCallbackStream) {
            mCaptureThread->mMainStreams[i] = *stream_list->streams[i];
            pMainStreams[i] = &mCaptureThread->mMainStreams[i];
        }
    }

    camera3_stream_configuration mainconfig;
    mainconfig = *stream_list;
    mainconfig.num_streams = SBS_MAX_NUM_STREAMS;
    mainconfig.streams = pMainStreams;

    HAL_LOGD("main configurestreams:");
    for (i = 0; i < mainconfig.num_streams; i++) {
        HAL_LOGD("stream_type=%d, width=%d, height=%d, format=%d",
                 pMainStreams[i]->stream_type, pMainStreams[i]->width,
                 pMainStreams[i]->height, pMainStreams[i]->format);
    }

    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                    &mainconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure main streams!!");
        return rc;
    }
    /*
        pAuxStreams[1]->width = mSubCaptureWidth;
        pAuxStreams[1]->height = mSubCaptureHeight;
        camera3_stream_configuration subconfig;
        subconfig = *stream_list;
        subconfig.num_streams = 1;
        subconfig.streams = pAuxStreams;
        SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
        rc = hwiAux->configure_streams(m_pPhyCamera[CAM_TYPE_AUX].dev,
       &subconfig);
        if (rc < 0) {
            HAL_LOGE("failed. configure aux streams!!");
            return rc;
        }

        for (i = 0; i < subconfig.num_streams; i++) {
            HAL_LOGD("aux configurestreams, streamtype:%d, format:%d, width:%d,
       "
                     "height:%d",
                     pAuxStreams[i]->stream_type, pAuxStreams[i]->format,
                     pAuxStreams[i]->width, pAuxStreams[i]->height);
        }
    */
    if (mainconfig.num_streams == 3) {
        memcpy(stream_list->streams[0], &mCaptureThread->mMainStreams[0],
               sizeof(camera3_stream_t));
        memcpy(stream_list->streams[1], &mCaptureThread->mMainStreams[1],
               sizeof(camera3_stream_t));
        stream_list->streams[1]->width = mCaptureWidth;
        stream_list->streams[1]->height = mCaptureHeight;
    }

    mCaptureThread->run(String8::format("SBS").string());

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
const camera_metadata_t *
SprdCamera3SideBySideCamera::constructDefaultRequestSettings(
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
void SprdCamera3SideBySideCamera::saveRequest(
    camera3_capture_request_t *request) {
    size_t i = 0;
    camera3_stream_t *newStream = NULL;
    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        if (getStreamType(newStream) == CALLBACK_STREAM) {
            request_saved_sidebyside_t currRequest;
            HAL_LOGV("save request %d", request->frame_number);
            Mutex::Autolock l(mRequestLock);
            currRequest.frame_number = request->frame_number;
            currRequest.buffer = request->output_buffers[i].buffer;
            currRequest.stream = request->output_buffers[i].stream;
            currRequest.input_buffer = request->input_buffer;
            mSavedRequestList.push_back(currRequest);
        } else if (getStreamType(newStream) == DEFAULT_STREAM) {
            mThumbReq.buffer = request->output_buffers[i].buffer;
            mThumbReq.stream = request->output_buffers[i].stream;
            mThumbReq.input_buffer = request->input_buffer;
            mThumbReq.frame_number = request->frame_number;
            HAL_LOGD("save thumb request:id %d, w= %d,h=%d",
                     request->frame_number, (mThumbReq.stream)->width,
                     (mThumbReq.stream)->height);
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
int SprdCamera3SideBySideCamera::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;
    uint32_t i = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    CameraMetadata metaSettings;
    camera3_capture_request_t *req = request;
    camera3_capture_request_t req_main;
    camera3_stream_buffer_t *out_streams_main = NULL;
    uint32_t tagCnt = 0;
    int snap_stream_num = 2;
    int has_snap_buffer = 0;

    rc = validateCaptureRequest(req);
    if (rc != NO_ERROR) {
        return rc;
    }

    memset(&req_main, 0x00, sizeof(camera3_capture_request_t));
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
        mPerfectskinlevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[0];
        HAL_LOGV("perfectskinlevel=%d", mPerfectskinlevel);
    }

    /* save bokehlevel */
    if (metaSettings.exists(ANDROID_SPRD_BLUR_F_NUMBER)) {
        int fnum = metaSettings.find(ANDROID_SPRD_BLUR_F_NUMBER).data.i32[0];
        if (fnum < MIN_F_FUMBER) {
            fnum = MIN_F_FUMBER;
        } else if (fnum > MAX_F_FUMBER) {
            fnum = MAX_F_FUMBER;
        }
        mSidebyside->mFnumber = (fnum * MAX_BLUR_F_FUMBER / MAX_F_FUMBER) - 1;
        mSidebyside->mBokehLevel =
            (MAX_F_FUMBER + 1 - fnum) * 255 / MAX_F_FUMBER;
        HAL_LOGD("fnum %d, f_number %d, bokeh_level %d", fnum,
                 mSidebyside->mFnumber, mSidebyside->mBokehLevel);
    }

    /*config main camera*/
    req_main = *req;
    out_streams_main =
        (camera3_stream_buffer_t *)malloc(sizeof(camera3_stream_buffer_t));
    if (!out_streams_main) {
        HAL_LOGE("failed");
        return NO_MEMORY;
    }
    memset(out_streams_main, 0x00, (sizeof(camera3_stream_buffer_t)));

    for (size_t i = 0; i < req->num_output_buffers; i++) {
        int requestStreamType =
            getStreamType(request->output_buffers[i].stream);
        HAL_LOGD("requestStreamType=%d", requestStreamType);

        if (requestStreamType == SNAPSHOT_STREAM) {
            has_snap_buffer = 1;
            *out_streams_main = req->output_buffers[i];
            HAL_LOGD("w=%d, h=%d, stream_format=%d",
                     req->output_buffers[i].stream->width,
                     req->output_buffers[i].stream->height,
                     req->output_buffers[i].stream->format);

            mCaptureThread->mSavedResultBuff =
                request->output_buffers[i].buffer;
            mjpegSize = ((struct private_handle_t *)(*request->output_buffers[i]
                                                          .buffer))
                            ->size;
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
            mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1] =
                req->output_buffers[i].stream;

            if (!mFlushing) {
                snap_stream_num = 2;
                out_streams_main->buffer = &mLocalCapBuffer[0].native_handle;
                mIsWaitSnapYuv = true;
            } else {
                snap_stream_num = 1;
                out_streams_main->buffer = (req->output_buffers[i]).buffer;
            }
            out_streams_main->stream =
                &mCaptureThread->mMainStreams[snap_stream_num];
            mIsCapturing = true;
            /* save Rotation angle*/
            if (metaSettings.exists(
                    ANDROID_SPRD_SENSOR_ROTATION_FOR_FRONT_BLUR)) {
                mRotation =
                    metaSettings.find(
                                    ANDROID_SPRD_SENSOR_ROTATION_FOR_FRONT_BLUR)
                        .data.i32[0];
                HAL_LOGD("rotation %d", mRotation);
            }
            if (metaSettings.exists(ANDROID_JPEG_ORIENTATION)) {
                mJpegOrientation =
                    metaSettings.find(ANDROID_JPEG_ORIENTATION).data.i32[0];
            }
        }
    }

    if (!has_snap_buffer) {
        *out_streams_main = req->output_buffers[0];
        mSavedReqStreams[mPreviewStreamsNum] = req->output_buffers[0].stream;
        out_streams_main->stream =
            &mCaptureThread->mMainStreams[mPreviewStreamsNum];
    }

    req_main.num_output_buffers = 1;
    req_main.output_buffers = out_streams_main;
    req_main.settings = metaSettings.release();

    HAL_LOGD("num_output_buffers=%d", req_main.num_output_buffers);
    for (i = 0; i < req_main.num_output_buffers; i++) {
        HAL_LOGD("width=%d, height=%d, format=%d",
                 req_main.output_buffers[i].stream->width,
                 req_main.output_buffers[i].stream->height,
                 req_main.output_buffers[i].stream->format);
    }

    rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                          &req_main);
    if (rc < 0) {
        HAL_LOGE("failed, mIsCapturing:%d  idx:%d", mIsCapturing,
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

int SprdCamera3SideBySideCamera::processAuxCaptureRequest(
    camera3_capture_request_t *request) {
    int rc = 0;
    uint32_t i = 0;

    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    CameraMetadata metaSettings;
    camera3_capture_request_t *req = request;
    camera3_capture_request_t req_aux;
    camera3_stream_buffer_t *out_streams_aux = NULL;
    uint32_t tagCnt = 0;
    int snap_stream_num = 0;

    rc = validateCaptureRequest(req);
    if (rc != NO_ERROR) {
        return rc;
    }
    metaSettings = request->settings;

    tagCnt = metaSettings.entryCount();
    if (tagCnt != 0) {
        uint8_t sprdBurstModeEnabled = 0;
        metaSettings.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                            &sprdBurstModeEnabled, 1);
        uint8_t sprdZslEnabled = 0;
        metaSettings.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);

        uint8_t captureIntent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
        metaSettings.update(ANDROID_CONTROL_CAPTURE_INTENT, &captureIntent, 1);
    }
    /*config aux camera * recreate a config without preview stream */
    req_aux = *req;
    out_streams_aux = (camera3_stream_buffer_t *)malloc(
        sizeof(camera3_stream_buffer_t) * (req_aux.num_output_buffers));
    if (!out_streams_aux) {
        HAL_LOGE("failed");
        goto req_fail;
    }
    memset(out_streams_aux, 0x00,
           (sizeof(camera3_stream_buffer_t)) * (req_aux.num_output_buffers));
    HAL_LOGD("num_output_buffers:%d", req->num_output_buffers);
    for (size_t i = 0; i < req->num_output_buffers; i++) {
        out_streams_aux[i] = mCaptureThread->mSavedCapReqstreambuff;
        out_streams_aux[i].buffer = &mLocalCapBuffer[3].native_handle;
        out_streams_aux[i].stream =
            &mCaptureThread->mAuxStreams[snap_stream_num];
        out_streams_aux[i].stream->width = mSubCaptureWidth;
        out_streams_aux[i].stream->height = mSubCaptureHeight;
        HAL_LOGD("stream:%p", out_streams_aux[i].stream);
        if (NULL == out_streams_aux[i].buffer) {
            HAL_LOGE("failed, LocalBufferList is empty!");
            goto req_fail;
        }
        // out_streams_aux[i].acquire_fence = -1;
    }
    req_aux.output_buffers = out_streams_aux;
    req_aux.settings = metaSettings.release();
    HAL_LOGD("mIsCapturing:%d, framenumber=%d", mIsCapturing,
             request->frame_number);
    rc = hwiAux->process_capture_request(m_pPhyCamera[CAM_TYPE_AUX].dev,
                                         &req_aux);
    if (rc < 0) {
        HAL_LOGE("failed, idx:%d", req_aux.frame_number);
        goto req_fail;
    }
req_fail:
    HAL_LOGD("rc, d%d", rc);
    if (req_aux.output_buffers != NULL) {
        free((void *)req_aux.output_buffers);
        req_aux.output_buffers = NULL;
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
void SprdCamera3SideBySideCamera::notifyMain(const camera3_notify_msg_t *msg) {
    if (msg->type == CAMERA3_MSG_SHUTTER &&
        msg->message.shutter.frame_number ==
            mCaptureThread->mSavedCapRequest.frame_number) {
        if (mCaptureThread->mReprocessing) {
            HAL_LOGD("hold cap notify");
            return;
        }
        HAL_LOGD("cap notify");
    }
    Mutex::Autolock l(mNotifyLockMain);
    mNotifyListMain.push_back(*msg);

    HAL_LOGD("preview timestamp %lld", msg->message.shutter.timestamp);
    mCaptureThread->mCallbackOps->notify(mCaptureThread->mCallbackOps, msg);
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

int SprdCamera3SideBySideCamera::thumbYuvProc(buffer_handle_t *src_buffer) {
    int ret = 0;
    int angle = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    struct img_frm src_img;
    struct img_frm dst_img;
    struct snp_thumb_yuv_param thumb_param;
    struct private_handle_t *src_private_handle =
        (struct private_handle_t *)(*src_buffer);
    struct private_handle_t *dst_private_handle =
        (struct private_handle_t *)*(mThumbReq.buffer);
    char prop[PROPERTY_VALUE_MAX];
    HAL_LOGI(" E");

    src_img.addr_phy.addr_y = 0;
    src_img.addr_phy.addr_u =
        src_img.addr_phy.addr_y +
        src_private_handle->width * src_private_handle->height;
    src_img.addr_phy.addr_v = src_img.addr_phy.addr_u;
    src_img.addr_vir.addr_y = (cmr_uint)src_private_handle->base;
    src_img.addr_vir.addr_u =
        (cmr_uint)src_private_handle->base +
        src_private_handle->width * src_private_handle->height;
    src_img.buf_size = src_private_handle->size;
    src_img.fd = src_private_handle->share_fd;
    src_img.fmt = IMG_DATA_TYPE_YUV420;
    src_img.rect.start_x = 0;
    src_img.rect.start_y = 0;
    src_img.rect.width = src_private_handle->width;
    src_img.rect.height = src_private_handle->height;
    src_img.size.width = src_private_handle->width;
    src_img.size.height = src_private_handle->height;

    dst_img.addr_phy.addr_y = 0;
    dst_img.addr_phy.addr_u =
        dst_img.addr_phy.addr_y +
        dst_private_handle->width * dst_private_handle->height;
    dst_img.addr_phy.addr_v = dst_img.addr_phy.addr_u;

    dst_img.addr_vir.addr_y = (cmr_uint)dst_private_handle->base;
    dst_img.addr_vir.addr_u =
        (cmr_uint)dst_private_handle->base +
        dst_private_handle->width * dst_private_handle->height;
    dst_img.buf_size = dst_private_handle->size;
    dst_img.fd = dst_private_handle->share_fd;
    dst_img.fmt = IMG_DATA_TYPE_YUV420;
    dst_img.rect.start_x = 0;
    dst_img.rect.start_y = 0;
    dst_img.rect.width = dst_private_handle->width;
    dst_img.rect.height = dst_private_handle->height;
    dst_img.size.width = dst_private_handle->width;
    dst_img.size.height = dst_private_handle->height;

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

    property_get("persist.sys.sbs.dump.thumb", prop, "0");
    if (atoi(prop) == 1) {
        dumpData((unsigned char *)thumb_param.src_img.addr_vir.addr_y, 1,
                 thumb_param.src_img.buf_size, thumb_param.src_img.size.width,
                 thumb_param.src_img.size.height, 5, "src");
        dumpData((unsigned char *)thumb_param.dst_img.addr_vir.addr_y, 1,
                 thumb_param.dst_img.buf_size, thumb_param.dst_img.size.width,
                 thumb_param.dst_img.size.height, 5, "dst");
    }

    HAL_LOGI("x,angle=%d JpegOrientation=%d", thumb_param.angle,
             mJpegOrientation);

    return ret;
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
void SprdCamera3SideBySideCamera::processCaptureResultMain(
    camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    CameraMetadata metadata;
    metadata = result->result;
    int rc = 0;
    Mutex::Autolock l(mResultLock);

    // meta process
    if (result_buffer == NULL && result->result != NULL) {
        if (result->frame_number ==
                mCaptureThread->mSavedCapRequest.frame_number &&
            0 != result->frame_number) {
            if (mCaptureThread->mReprocessing) {
                HAL_LOGD("hold yuv picture call back, framenumber:%d",
                         result->frame_number);
                return;
            } else {
                mCaptureThread->mCallbackOps->process_capture_result(
                    mCaptureThread->mCallbackOps, result);
                return;
            }
        }
        if (0) { //(mCoveredValue) {
            // update Blur Covered param
            int value = mCoveredValue;
            rc = hwiMain->camera_ioctrl(CAMERA_IOCTRL_GET_BLUR_COVERED, &value,
                                        NULL);
            HAL_LOGV("Covered Value is %d", value);
            metadata.update(ANDROID_SPRD_BLUR_COVERED, &value, 1);
            if (mRealtimeBokeh) {
                // mCoveredValue should not update when capture
                mCoveredValue = value;
            }
            camera3_capture_result_t new_result = *result;
            new_result.result = metadata.release();
            mCaptureThread->mCallbackOps->process_capture_result(
                mCaptureThread->mCallbackOps, &new_result);
            free_camera_metadata(
                const_cast<camera_metadata_t *>(new_result.result));
        } else {
            mCaptureThread->mCallbackOps->process_capture_result(
                mCaptureThread->mCallbackOps, result);
        }
        return;
    }

    // process preview frame
    if (mRealtimeBokeh) {
        int StreamType = getStreamType(result_buffer->stream);
        HAL_LOGV("StreamType:%d", StreamType);

        if (StreamType == CALLBACK_STREAM) {
            sidebyside_queue_msg_t capture_msg;
            Mutex::Autolock l(mNotifyLockMain);
            for (List<camera3_notify_msg_t>::iterator i =
                     mNotifyListMain.begin();
                 i != mNotifyListMain.end(); i++) {
                if (i->message.shutter.frame_number == cur_frame_number) {
                    if (i->type == CAMERA3_MSG_SHUTTER) {
                        capture_msg.timestamp = i->message.shutter.timestamp;
                        mNotifyListMain.erase(i);
                    } else if (i->type == CAMERA3_MSG_ERROR) {
                        HAL_LOGE("Return local buffer:%d caused by error "
                                 "Notify status",
                                 result->frame_number);
                        CallBackResult(cur_frame_number,
                                       CAMERA3_BUFFER_STATUS_ERROR);
                        mNotifyListMain.erase(i);
                        return;
                    }
                }
            }
            if (!mSidebyside->mFlushing) {
                HAL_LOGV("REAL TIME: process preview frame");
                capture_msg.msg_type = SBS_MSG_PREV_PROC;
                capture_msg.prev_combo_buff.frame_number = cur_frame_number;
                capture_msg.prev_combo_buff.prev_buffer = result_buffer->buffer;
                hwiMain->setMultiCallBackYuvMode(false);
                {
                    Mutex::Autolock l(mCaptureThread->mMergequeueMutex);
                    mCaptureThread->mCaptureMsgList.push_back(capture_msg);
                    mCaptureThread->mMergequeueSignal.signal();
                }
            } else {
                camera3_capture_result_t newResult;
                camera3_stream_buffer_t newOutput_buffers;
                memset(&newResult, 0, sizeof(camera3_capture_result_t));
                memset(&newOutput_buffers, 0, sizeof(camera3_stream_buffer_t));
                {
                    Mutex::Autolock l(mRequestLock);
                    List<request_saved_sidebyside_t>::iterator i =
                        mSavedRequestList.begin();
                    while (i != mSavedRequestList.end()) {
                        if (i->frame_number == result->frame_number) {
                            memcpy(&newResult, result,
                                   sizeof(camera3_capture_result_t));
                            memcpy(&newOutput_buffers,
                                   &result->output_buffers[0],
                                   sizeof(camera3_stream_buffer_t));
                            newOutput_buffers.stream = i->stream;
                            memcpy(newOutput_buffers.stream,
                                   result->output_buffers[0].stream,
                                   sizeof(camera3_stream_t));
                            newResult.output_buffers = &newOutput_buffers;
                            mCaptureThread->mCallbackOps
                                ->process_capture_result(
                                    mCaptureThread->mCallbackOps, &newResult);
                            mSavedRequestList.erase(i);
                            dumpFps();
                            HAL_LOGV("find preview frame %d",
                                     result->frame_number);
                            break;
                        }
                        i++;
                    }
                }
            }
#if 0
        unsigned char *prev_buffer =
            (unsigned char *)(((struct private_handle_t *)*(
                                   result->output_buffers->buffer))
                                  ->base);
        dumpFile(
            prev_buffer, SBS_FILE_NV21,
            (int)((struct private_handle_t *)*(result->output_buffers->buffer))
                ->size,
            mPreviewWidth, mPreviewHeight, result->frame_number, 4, 0);
#endif
            return;
        }
    }

    int currStreamType = getStreamType(result_buffer->stream);
    if (mIsCapturing && currStreamType == DEFAULT_STREAM) {
        HAL_LOGD("main yuv picture: frame_number=%d, currStreamType=%d, "
                 "format=%d, width=%d, height=%d, buff=%p",
                 result->frame_number, currStreamType,
                 result_buffer->stream->format, result_buffer->stream->width,
                 result_buffer->stream->height, result_buffer->buffer);

        if (!mSidebyside->mFlushing) {
            sidebyside_queue_msg_t capture_msg;
            capture_msg.msg_type = SBS_MSG_MAIN_PROC;
            capture_msg.combo_buff.frame_number = result->frame_number;
            capture_msg.combo_buff.main_buffer = result->output_buffers->buffer;
            capture_msg.combo_buff.sub_buffer = NULL;
            capture_msg.combo_buff.input_buffer = result->input_buffer;
            hwiMain->setMultiCallBackYuvMode(false);
            mRealtimeBokeh = 0;
            {
                Mutex::Autolock l(mCaptureThread->mMergequeueMutex);
                mCaptureThread->mCaptureMsgList.push_back(capture_msg);
                mCaptureThread->mMergequeueSignal.signal();
            }
        } else {
            sidebyside_queue_msg_t capture_msg;
            capture_msg.msg_type = SBS_MSG_FLUSHING_PROC;
            capture_msg.combo_buff.frame_number = result->frame_number;
            capture_msg.combo_buff.main_buffer = result->output_buffers->buffer;
            capture_msg.combo_buff.sub_buffer = NULL;
            capture_msg.combo_buff.input_buffer = result->input_buffer;
            hwiMain->setMultiCallBackYuvMode(false);
            {
                Mutex::Autolock l(mCaptureThread->mMergequeueMutex);
                mCaptureThread->mCaptureMsgList.push_back(capture_msg);
                mCaptureThread->mMergequeueSignal.signal();
            }
        }
    } else if (mIsCapturing && currStreamType == SNAPSHOT_STREAM) {
        camera3_capture_result_t newResult = *result;
        camera3_stream_buffer_t newOutput_buffers = *(result->output_buffers);
        HAL_LOGD("jpeg result stream:%p, result buffer:%p, savedreqstream:%p",
                 result->output_buffers[0].stream,
                 result->output_buffers[0].buffer,
                 mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1]);
        memcpy(mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1],
               result->output_buffers[0].stream, sizeof(camera3_stream_t));
        newOutput_buffers.stream =
            mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1];
        newOutput_buffers.buffer = result->output_buffers->buffer;

        newResult.output_buffers = &newOutput_buffers;
        newResult.input_buffer = NULL;
        newResult.result = NULL;
        newResult.partial_result = 0;
        ((struct private_handle_t *)*result->output_buffers->buffer)->size =
            mjpegSize;
        // dump jpeg
        char prop[PROPERTY_VALUE_MAX] = {0};
        property_get("persist.sys.cam.sbs.savefile", prop, "0");
        if (!strcmp(prop, "1")) {
            unsigned char *buffer_base =
                (unsigned char *)(((struct private_handle_t *)*(
                                       result->output_buffers->buffer))
                                      ->base);
            dumpFile(buffer_base, SBS_FILE_JPEG,
                     (int)((struct private_handle_t *)*(
                               result->output_buffers->buffer))
                         ->size,
                     mMainCaptureWidth, mMainCaptureHeight,
                     result->frame_number, 0, 0);
        }

        mCaptureThread->mCallbackOps->process_capture_result(
            mCaptureThread->mCallbackOps, &newResult);
        mCaptureThread->mReprocessing = false;
        mIsCapturing = false;
    } else {
        camera3_capture_result_t newResult;
        camera3_stream_buffer_t newOutput_buffers;
        memset(&newResult, 0, sizeof(camera3_capture_result_t));
        memset(&newOutput_buffers, 0, sizeof(camera3_stream_buffer_t));
        {
            Mutex::Autolock l(mRequestLock);
            List<request_saved_sidebyside_t>::iterator i =
                mSavedRequestList.begin();
            while (i != mSavedRequestList.end()) {
                if (i->frame_number == result->frame_number) {
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
 * FUNCTION   :CallBackSnapResult
 *
 * DESCRIPTION: CallBackSnapResult
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3SideBySideCamera::CallBackSnapResult() {

    camera3_capture_result_t result;
    camera3_stream_buffer_t result_buffers;
    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&result_buffers, sizeof(camera3_stream_buffer_t));

    result_buffers.stream = mThumbReq.stream;
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

    mCaptureThread->mCallbackOps->process_capture_result(
        mCaptureThread->mCallbackOps, &result);
    memset(&mThumbReq, 0, sizeof(request_saved_sidebyside_t));
    HAL_LOGD("snap id:%d ", result.frame_number);
}

void SprdCamera3SideBySideCamera::processCaptureResultAux(
    camera3_capture_result_t *result) {
    uint64_t result_timestamp;
    uint32_t cur_frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    /* Direclty pass meta result for Aux camera */
    if (result->num_output_buffers == 0) {
        return;
    }

    if (mIsCapturing) {
        const camera3_stream_buffer_t *result_buffer = result->output_buffers;
        HAL_LOGD("aux yuv picture: stream=%p, buffer=%p, format=%d, width=%d, "
                 "height=%d",
                 result->output_buffers->stream, result->output_buffers->buffer,
                 result->output_buffers->stream->format,
                 result->output_buffers->stream->width,
                 result->output_buffers->stream->height);
        if (NULL != result->output_buffers->buffer) {
            sidebyside_queue_msg_t capture_msg;
            capture_msg.msg_type = SBS_MSG_BOKEH_PROC;
            capture_msg.combo_buff.frame_number = result->frame_number;
            capture_msg.combo_buff.main_buffer =
                &mLocalCapBuffer[0].native_handle;
            capture_msg.combo_buff.sub_buffer = result->output_buffers->buffer;
            capture_msg.combo_buff.input_buffer = result->input_buffer;
            HAL_LOGD(
                "capture begin: frame_number=%d, main_buffer=%p, sub_buffer=%p",
                capture_msg.combo_buff.frame_number,
                capture_msg.combo_buff.main_buffer,
                capture_msg.combo_buff.sub_buffer);
            {
                Mutex::Autolock l(mCaptureThread->mMergequeueMutex);
                mCaptureThread->mCaptureMsgList.push_back(capture_msg);
                mCaptureThread->mMergequeueSignal.signal();
            }
        }
    }

    return;
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
void SprdCamera3SideBySideCamera::CallBackResult(
    uint32_t frame_number, camera3_buffer_status_t buffer_status) {

    camera3_capture_result_t result;
    List<request_saved_sidebyside_t>::iterator itor;
    camera3_stream_buffer_t result_buffers;
    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&result_buffers, sizeof(camera3_stream_buffer_t));
    {
        Mutex::Autolock l(mSidebyside->mRequestLock);
        itor = mSidebyside->mSavedRequestList.begin();
        while (itor != mSidebyside->mSavedRequestList.end()) {
            if (itor->frame_number == frame_number) {
                HAL_LOGD("erase frame_number %u", frame_number);
                result_buffers.stream = itor->stream;
                result_buffers.buffer = itor->buffer;
                mSidebyside->mSavedRequestList.erase(itor);
                break;
            }
            itor++;
        }
        if (itor == mSidebyside->mSavedRequestList.end()) {
            HAL_LOGE("can't find frame in mSavedRequestList %u:", frame_number);
            return;
        }
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

    mCaptureThread->mCallbackOps->process_capture_result(
        mCaptureThread->mCallbackOps, &result);

    HAL_LOGD("id:%d buffer_status %u", result.frame_number, buffer_status);
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
void SprdCamera3SideBySideCamera::_dump(const struct camera3_device *device,
                                        int fd) {
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
int SprdCamera3SideBySideCamera::_flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGI("E flush, mCaptureMsgList.size=%zu, mSavedRequestList.size:%zu",
             mCaptureThread->mCaptureMsgList.size(), mSavedRequestList.size());
    mFlushing = true;

    // stop depth
    if (mDepthState) {
        HAL_LOGI("stop processing depth when flush");
        sprd_depth_Set_Stopflag(mDepthHandle, DEPTH_STOP);
    }

    while (mIsCapturing && mIsWaitSnapYuv) {
        Mutex::Autolock l(mMergequeueFinishMutex);
        mMergequeueFinishSignal.waitRelative(mMergequeueFinishMutex,
                                             SBS_THREAD_TIMEOUT);
    }
    HAL_LOGD("wait until mCaptureMsgList.empty");

    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_MAIN].dev);
    if (2 == m_nPhyCameras) {
        HAL_LOGD("flush sub camera");
        SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
        rc = hwiAux->flush(m_pPhyCamera[CAM_TYPE_AUX].dev);
        rc = hwiAux->setSensorStream(STREAM_OFF);
        // int on_off = STREAM_OFF;
        // rc = hwiAux->camera_ioctrl(CAMERA_IOCTRL_COVERED_SENSOR_STREAM_CTRL,
        //                            &on_off, NULL);
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
