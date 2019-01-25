/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
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

#define LOG_TAG "SprdCamera3HWI"
//#define LOG_NDEBUG 0
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include <cutils/properties.h>
#include <hardware/camera3.h>
#include <camera/CameraMetadata.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <utils/Trace.h>
#if (MINICAMERA != 1)
#include <ui/Fence.h>
#endif
#ifdef CONFIG_GPU_PLATFORM_ROGUE
#include <gralloc_public.h>
#else
#include <gralloc_priv.h>
#endif
#include "SprdCamera3HWI.h"

#include "SprdCamera3Channel.h"

#include "SprdCamera3OEMIf.h"
#include "SprdCamera3Setting.h"
#include <sprd_ion.h>

#ifdef CONFIG_CAMERA_ISP
extern "C" {
#include "isp_video.h"
}
#endif

using namespace android;

namespace sprdcamera {

unsigned int SprdCamera3HWI::mCameraSessionActive = 0;
multiCameraMode SprdCamera3HWI::mMultiCameraMode = MODE_SINGLE_CAMERA;

// gHALLogLevel(default is 4):
//   1 - only show ALOGE, err log is always show
//   2 - show ALOGE and ALOGW
//   3 - show ALOGE, ALOGW and ALOGI
//   4 - show ALOGE, ALOGW, ALOGI and ALOGD
//   5 - show ALOGE, ALOGW, ALOGI and ALOGD, ALOGV
// use the following command to change gHALLogLevel:
//   adb shell setprop persist.sys.camera.hal.log 1
volatile uint32_t gHALLogLevel = 4;

camera3_device_ops_t SprdCamera3HWI::mCameraOps = {
    .initialize = SprdCamera3HWI::initialize,
    .configure_streams = SprdCamera3HWI::configure_streams,
    .register_stream_buffers = NULL, // SprdCamera3HWI::register_stream_buffers,
    .construct_default_request_settings =
        SprdCamera3HWI::construct_default_request_settings,
    .process_capture_request = SprdCamera3HWI::process_capture_request,
    .get_metadata_vendor_tag_ops =
        NULL, // SprdCamera3HWI::get_metadata_vendor_tag_ops,
    .dump = SprdCamera3HWI::dump,
    .flush = SprdCamera3HWI::flush,
    .reserved = {0},
};

static camera3_device_t *g_cam_device[4] = {0, 0, 0, 0};

// SprdCamera3Setting *SprdCamera3HWI::mSetting = NULL;

#define SPRD_CONTROL_CAPTURE_INTENT_FLUSH 0xFE
#define SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE 0xFF

SprdCamera3HWI::SprdCamera3HWI(int cameraId)
    : mCameraId(cameraId), mOEMIf(NULL), mCameraOpened(false),
      mCameraInitialized(false), mLastFrmNum(0), mCallbackOps(NULL),
      mInputStream(NULL), mMetadataChannel(NULL), mPictureChannel(NULL),
      mDeqBufNum(0), mRecSkipNum(0), mIsSkipFrm(false), mFlush(false) {
    ATRACE_CALL();

    // for camera id 2&3 debug
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.sys.camera.id", value, "0");
    if (mCameraId == 0) {
        if (!strcmp(value, "2"))
            mCameraId = 2;
    } else if (mCameraId == 1) {
        if (!strcmp(value, "3"))
            mCameraId = 3;
    }
    getLogLevel();
    HAL_LOGD("mCameraId %d,mCameraDevice %p", mCameraId, &mCameraDevice);
    mCameraDevice.common.tag = HARDWARE_DEVICE_TAG;
    mCameraDevice.common.version =
        CAMERA_DEVICE_API_VERSION_3_2; // CAMERA_DEVICE_API_VERSION_3_0;
    mCameraDevice.common.close = close_camera_device;
    mCameraDevice.ops = &mCameraOps;
    mCameraDevice.priv = this;
    if (SENSOR_MAIN == cameraId) {
        g_cam_device[0] = &mCameraDevice;
    } else if (SENSOR_SUB == cameraId) {
        g_cam_device[1] = &mCameraDevice;
    } else if (SENSOR_DEVICE2 == cameraId) {
        g_cam_device[2] = &mCameraDevice;
    } else if (SENSOR_DEVICE3 == cameraId) {
        g_cam_device[3] = &mCameraDevice;
    }
    mPendingRequest = 0;
    mCurrentRequestId = -1;
    mCurrentCapIntent = 0;

    mMetadataChannel = NULL;

    mRegularChan = NULL;
    mFirstRegularRequest = false;

    mPicChan = NULL;
    mPictureRequest = false;

    mCallbackChan = NULL;

    mBurstCapCnt = 1;

    mVideoSnapshotHint = false;
    mOldCapIntent = 0;
    mOldRequesId = 0;
    mPrvTimerID = NULL;
    mFrameNum = 0;
    mSetting = NULL;
    mSprdCameraLowpower = 0;

    HAL_LOGD("X");
}

int SprdCamera3HWI::getNumberOfCameras() {
    int num = SprdCamera3Setting::getNumberOfCameras();

    return num;
}

SprdCamera3HWI::~SprdCamera3HWI() {
    ATRACE_CALL();

    HAL_LOGD("E");

    SprdCamera3RegularChannel *regularChannel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    SprdCamera3PicChannel *picChannel =
        reinterpret_cast<SprdCamera3PicChannel *>(mPicChan);
    int64_t timestamp = systemTime();

    if (mHDRProcessFlag == true) {
        Mutex::Autolock l(mLock);
    }

    if (mOEMIf)
        mOEMIf->enablePowerHint();

    if (mMetadataChannel) {
        mMetadataChannel->stop(mFrameNum);
        delete mMetadataChannel;
        mMetadataChannel = NULL;
    }

    if (mRegularChan) {
        mRegularChan->stop(mFrameNum);
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_PREVIEW);
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_VIDEO);
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_CALLBACK);
        delete mRegularChan;
        mRegularChan = NULL;
    }

    if (mPicChan) {
        mPicChan->stop(mFrameNum);
        picChannel->channelClearAllQBuff(timestamp,
                                         CAMERA_STREAM_TYPE_PICTURE_CALLBACK);
        picChannel->channelClearAllQBuff(timestamp,
                                         CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);
        delete mPicChan;
        mPicChan = NULL;
    }

    if (mCallbackChan) {
        mCallbackChan->stop(mFrameNum);
        delete mCallbackChan;
        mCallbackChan = NULL;
    }

    if (mCameraOpened)
        closeCamera();

    timer_stop();

    HAL_LOGD("X");
}

SprdCamera3RegularChannel *SprdCamera3HWI::getRegularChan() {
    return mRegularChan;
}

SprdCamera3PicChannel *SprdCamera3HWI::getPicChan() { return mPicChan; }

SprdCamera3OEMIf *SprdCamera3HWI::getOEMif() { return mOEMIf; }

static int ispVideoStartPreview(uint32_t param1, uint32_t param2) {
    SprdCamera3HWI *dev =
        reinterpret_cast<SprdCamera3HWI *>(g_cam_device[0]->priv);
    SprdCamera3RegularChannel *regularChannel = dev->getRegularChan();
    int rtn = 0x00;

    LOGE("ISP_TOOL: ispVideoStartPreview, regularChannel %p", regularChannel);
    if (regularChannel != NULL) {
        regularChannel->setCapturePara(CAMERA_CAPTURE_MODE_PREVIEW);
        rtn = regularChannel->start(dev->mFrameNum);
    }

    return rtn;
}

static int ispVideoStopPreview(uint32_t param1, uint32_t param2) {

    SprdCamera3HWI *dev =
        reinterpret_cast<SprdCamera3HWI *>(g_cam_device[0]->priv);
    SprdCamera3RegularChannel *regularChannel = dev->getRegularChan();
    int rtn = 0x00;

    LOGE("ISP_TOOL: ispVideoStopPreview, regularChannel %p", regularChannel);
    if (regularChannel != NULL) {
        rtn = regularChannel->stop(dev->mFrameNum);
    }

    return rtn;
}

static int ispVideoTakePicture(uint32_t param1, uint32_t param2) {
    int rtn = 0x00;
    SprdCamera3HWI *dev =
        reinterpret_cast<SprdCamera3HWI *>(g_cam_device[0]->priv);
    SprdCamera3PicChannel *picChannel = dev->getPicChan();

    LOGE("ISP_TOOL: ispVideoTakePicture, regularChannel %p", picChannel);
    if (NULL != picChannel) {
        // param1 = 1 for simulation case
        if (1 == param1) {
            picChannel->setCapturePara(CAMERA_CAPTURE_MODE_ISP_SIMULATION_TOOL);
        } else {
            picChannel->setCapturePara(CAMERA_CAPTURE_MODE_ISP_TUNING_TOOL);
        }
        rtn = picChannel->start(dev->mFrameNum);
    }

    return rtn;
}

static int ispVideoSetParam(uint32_t width, uint32_t height) {
    int rtn = 0x00;
    SprdCamera3HWI *dev =
        reinterpret_cast<SprdCamera3HWI *>(g_cam_device[0]->priv);
    SprdCamera3PicChannel *picChannel = dev->getPicChan();
    SprdCamera3OEMIf *oemIf = dev->getOEMif();
    cam_dimension_t capture_size;

    LOGI("ISP_TOOL: ispVideoSetParam width:%d, height:%d, picChannel %p", width,
         height, picChannel);
    if (NULL != picChannel) {
        // picChannel->setCaptureSize(width,height);
        capture_size.width = width;
        capture_size.height = height;
        oemIf->SetDimensionCapture(capture_size);
        oemIf->setCaptureRawMode(1);
    }

    return rtn;
}

static int ispCtrlFlash(uint32_t param, uint32_t status) {
    SprdCamera3HWI *dev =
        reinterpret_cast<SprdCamera3HWI *>(g_cam_device[0]->priv);
    SprdCamera3OEMIf *oemIf = dev->getOEMif();
    oemIf->setIspFlashMode(status);

    return 0;
}

int SprdCamera3HWI::openCamera(struct hw_device_t **hw_device) {
    ATRACE_CALL();

    HAL_LOGD("E");

    int ret = 0;
    Mutex::Autolock l(mLock);

    // single camera mode can only open one camera .multicamera mode can only
    // open two cameras.
    if ((mCameraSessionActive == 1 && !isMultiCameraMode(mMultiCameraMode)) ||
        mCameraSessionActive > 2) {
        HAL_LOGE("multiple simultaneous camera instance not supported");
        return -EUSERS;
    }

    if (mCameraOpened) {
        *hw_device = NULL;
        return PERMISSION_DENIED;
    }

    ret = openCamera();
    if (ret == 0) {
        *hw_device = &mCameraDevice.common;
        mCameraSessionActive++;
#ifdef CONFIG_CAMERA_ISP
        startispserver();
        ispvideo_RegCameraFunc(1, ispVideoStartPreview);
        ispvideo_RegCameraFunc(2, ispVideoStopPreview);
        ispvideo_RegCameraFunc(3, ispVideoTakePicture);
        ispvideo_RegCameraFunc(4, ispVideoSetParam);
//		ispvideo_RegCameraFunc(REG_CTRL_FLASH, ispCtrlFlash);
#endif
    } else
        *hw_device = NULL;
    HAL_LOGD("X mCameraSessionActive %d", mCameraSessionActive);
    return ret;
}

int SprdCamera3HWI::openCamera() {
    ATRACE_CALL();

    int ret = NO_ERROR;

    if (mOEMIf) {
        HAL_LOGE("Failure: Camera already opened");
        return ALREADY_EXISTS;
    }

    mSetting = new SprdCamera3Setting(mCameraId);
    if (mSetting == NULL) {
        HAL_LOGE("alloc setting failed.");
        return NO_MEMORY;
    }

    mOEMIf = new SprdCamera3OEMIf(mCameraId, mSetting);
    if (!mOEMIf) {
        HAL_LOGE("alloc oemif failed.");
        if (mSetting) {
            delete mSetting;
            mSetting = NULL;
        }
        return NO_MEMORY;
    }
    mOEMIf->setMultiCameraMode(mMultiCameraMode);
    ret = mOEMIf->openCamera();
    if (NO_ERROR != ret) {
        HAL_LOGE("camera_open failed.");
        if (mOEMIf) {
            delete mOEMIf;
            mOEMIf = NULL;
        }
        if (mSetting) {
            delete mSetting;
            mSetting = NULL;
        }
        return ret;
    }

    mCameraOpened = true;

    return NO_ERROR;
}

int SprdCamera3HWI::closeCamera() {
    ATRACE_CALL();

    HAL_LOGD("E");
    int ret = NO_ERROR;

    if (mOEMIf) {
        if (mCameraSessionActive == 0) {
            mMultiCameraMode = MODE_SINGLE_CAMERA;
            mOEMIf->setMultiCameraMode(mMultiCameraMode);
        }
        mOEMIf->closeCamera();
        delete mOEMIf;
        mOEMIf = NULL;
    }

    if (mSetting) {
        delete mSetting;
        mSetting = NULL;
    }

    mCameraOpened = false;

    HAL_LOGD("X");
    return ret;
}

int SprdCamera3HWI::initialize(
    const struct camera3_callback_ops *callback_ops) {
    ATRACE_CALL();

    int ret = 0;
    Mutex::Autolock l(mLock);

    mCallbackOps = callback_ops;
    mCameraInitialized = true;
    mFrameNum = 0;
    mCurFrameTimeStamp = 0;
    if (mOEMIf)
        mOEMIf->initialize();

    return ret;
}

camera_metadata_t *SprdCamera3HWI::constructDefaultMetadata(int type) {
    ATRACE_CALL();

    camera_metadata_t *metadata = NULL;

    HAL_LOGI("S, type = %d", type);

    if (!mSetting) {
        HAL_LOGE("NULL camera device");
        return NULL;
    }
    mSetting->constructDefaultMetadata(type, &metadata);

    if (mOldCapIntent == SPRD_CONTROL_CAPTURE_INTENT_FLUSH) {
        mOldCapIntent = SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE;
    }

    HAL_LOGI("X");
    return metadata;
}

int SprdCamera3HWI::checkStreamList(
    camera3_stream_configuration_t *streamList) {
    int ret = NO_ERROR;

    // Sanity check stream_list
    if (streamList == NULL) {
        HAL_LOGE("NULL stream configuration");
        return BAD_VALUE;
    }
    if (streamList->streams == NULL) {
        HAL_LOGE("NULL stream list");
        return BAD_VALUE;
    }

    if (streamList->num_streams < 1) {
        HAL_LOGE("Bad number of streams requested: %d",
                 streamList->num_streams);
        return BAD_VALUE;
    }

    return ret;
}

int32_t
SprdCamera3HWI::tranStreamAndChannelType(camera3_stream_t *new_stream,
                                         camera_stream_type_t *stream_type,
                                         camera_channel_type_t *channel_type) {

    if (new_stream->stream_type == CAMERA3_STREAM_OUTPUT) {
        if (new_stream->format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
#ifdef CONFIG_CAMERA_DCAM_SUPPORT_FORMAT_NV12
            new_stream->format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
#else
            new_stream->format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
#endif
        switch (new_stream->format) {
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            if (new_stream->usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
                *stream_type = CAMERA_STREAM_TYPE_VIDEO;
                *channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
                new_stream->usage |= GRALLOC_USAGE_SW_READ_OFTEN;
            } else if (new_stream->usage & GRALLOC_USAGE_SW_READ_OFTEN) {
                *stream_type = CAMERA_STREAM_TYPE_CALLBACK;
                *channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
            } else {
                *stream_type = CAMERA_STREAM_TYPE_PREVIEW;
                *channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
                new_stream->usage |= GRALLOC_USAGE_SW_READ_OFTEN;
                new_stream->usage |= GRALLOC_USAGE_PRIVATE_1;
            }
            break;
        case HAL_PIXEL_FORMAT_YV12:
        case HAL_PIXEL_FORMAT_YCbCr_420_888:
            if (new_stream->usage & GRALLOC_USAGE_SW_READ_OFTEN) {
                *stream_type = CAMERA_STREAM_TYPE_DEFAULT;
                *channel_type = CAMERA_CHANNEL_TYPE_RAW_CALLBACK;
            }
            break;
        case HAL_PIXEL_FORMAT_BLOB:
            *stream_type = CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT;
            *channel_type = CAMERA_CHANNEL_TYPE_PICTURE;
            break;
        default:
            *stream_type = CAMERA_STREAM_TYPE_DEFAULT;
            break;
        }
    } else if (new_stream->stream_type == CAMERA3_STREAM_BIDIRECTIONAL) {
        *stream_type = CAMERA_STREAM_TYPE_CALLBACK;
        *channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
        new_stream->format = HAL_PIXEL_FORMAT_YCbCr_420_888;
        mInputStream = new_stream;
    }

    if (!mOEMIf->IommuIsEnabled()) {
        new_stream->usage |= GRALLOC_USAGE_CAMERA_BUFFER;
    }

    HAL_LOGD("stream_type = %d, channel_type = %d w=%d h=%d", *stream_type,
             *channel_type, new_stream->width, new_stream->height);

    return NO_ERROR;
}

int32_t SprdCamera3HWI::checkStreamSizeAndFormat(camera3_stream_t *new_stream) {
    SCALER_Tag scaleInfo;
    cmr_int i;
    mSetting->getSCALERTag(&scaleInfo);
    HAL_LOGD("newStream->width = %d, newStream->height = %d, "
             "new_stream->format = 0x%x, new_stream->usage = 0x%x",
             new_stream->width, new_stream->height, new_stream->format,
             new_stream->usage);
    HAL_LOGD("sizeof(scaleInfo.available_stream_configurations) = %zu, "
             "4*sizeof(int32_t) = %zu",
             sizeof(scaleInfo.available_stream_configurations),
             4 * sizeof(uint32_t));
    for (i = 0; i < (cmr_int)sizeof(scaleInfo.available_stream_configurations) /
                        (cmr_int)(4 * sizeof(int32_t));
         i++) {
        if (new_stream->format ==
                (int32_t)scaleInfo.available_stream_configurations[4 * i] &&
            new_stream->width ==
                (uint32_t)
                    scaleInfo.available_stream_configurations[4 * i + 1] &&
            new_stream->height ==
                (uint32_t)scaleInfo.available_stream_configurations[4 * i + 2])
            return NO_ERROR;
    }

    return BAD_VALUE;
}

int SprdCamera3HWI::configureStreams(
    camera3_stream_configuration_t *streamList) {
    ATRACE_CALL();

    HAL_LOGD("E");
    Mutex::Autolock l(mLock);

    int ret = NO_ERROR;
    bool preview_stream_flag = false;
    bool callback_stream_flag = false;
    cam_dimension_t preview_size = {0, 0};
    cam_dimension_t video_size = {0, 0};
    cam_dimension_t raw_size = {0, 0};
    cam_dimension_t capture_size = {0, 0};
    SPRD_DEF_Tag sprddefInfo;
    memset(&sprddefInfo, 0x00, sizeof(SPRD_DEF_Tag));

    ret = checkStreamList(streamList);
    if (ret) {
        HAL_LOGE("check failed ret=%d", ret);
        return ret;
    }

    // Create metadata channel and initialize it
    if (mMetadataChannel == NULL) {
        mMetadataChannel = new SprdCamera3MetadataChannel(
            mOEMIf, captureResultCb, mSetting, this);
        if (mMetadataChannel == NULL) {
            HAL_LOGE("failed to allocate metadata channel");
        }
    } else {
        mMetadataChannel->stop(mFrameNum);
    }

    // regular channel
    if (mRegularChan == NULL) {
        mRegularChan = new SprdCamera3RegularChannel(
            mOEMIf, captureResultCb, mSetting, mMetadataChannel,
            CAMERA_CHANNEL_TYPE_REGULAR, this);
        if (mRegularChan == NULL) {
            HAL_LOGE("channel created failed");
            return INVALID_OPERATION;
        }
    }

    // picture channel
    if (mPicChan == NULL) {
        mPicChan = new SprdCamera3PicChannel(mOEMIf, captureResultCb, mSetting,
                                             mMetadataChannel,
                                             CAMERA_CHANNEL_TYPE_PICTURE, this);
        if (mPicChan == NULL) {
            HAL_LOGE("channel created failed");
            return INVALID_OPERATION;
        }
    }

    // callback channel
    if (mCallbackChan == NULL) {
        mCallbackChan = new SprdCamera3RegularChannel(
            mOEMIf, captureResultCb, mSetting, mMetadataChannel,
            CAMERA_CHANNEL_TYPE_RAW_CALLBACK, this);
        if (mCallbackChan == NULL) {
            HAL_LOGE("channel created failed");
            return INVALID_OPERATION;
        }
    }

    /* Allocate channel objects for the requested streams */
    for (size_t i = 0; i < streamList->num_streams; i++) {
        camera3_stream_t *newStream = streamList->streams[i];
        camera_stream_type_t stream_type;
        camera_channel_type_t channel_type;
        cam_dimension_t channel_size;
        bool chan_created = false;

        tranStreamAndChannelType(newStream, &stream_type, &channel_type);

/*temp code; for debug refocus depth map, del it if depth map quality is OK*/
#ifdef CONFIG_CAMERA_RT_REFOCUS
        char value[PROPERTY_VALUE_MAX];
        char value1[PROPERTY_VALUE_MAX];
        property_get("debug.camera.save.refocus", value, "0");
        property_get("persist.camera.save.refocus", value1, "0");
        if ((atoi(value) == 2 || atoi(value1) == 2) &&
            stream_type == CAMERA_STREAM_TYPE_PREVIEW) {
            newStream->width = 480;
            newStream->height = 360;
        }
#endif

        char value[PROPERTY_VALUE_MAX];
        property_get("volte.incall.camera.enable", value, "false");
        if (!strcmp(value, "true") &&
            stream_type == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT &&
            channel_type == CAMERA_CHANNEL_TYPE_PICTURE) {
            newStream->width = 0;
            newStream->height = 0;
        }

        // for CTS MultiViewTest
        if (stream_type == CAMERA_STREAM_TYPE_PREVIEW) {
            if (preview_stream_flag == false)
                preview_stream_flag = true;
            else {
                stream_type = CAMERA_STREAM_TYPE_DEFAULT;
                channel_type = CAMERA_CHANNEL_TYPE_RAW_CALLBACK;
            }
        }

        // for CTS
        if ((stream_type == CAMERA_STREAM_TYPE_DEFAULT) &&
            (channel_type == CAMERA_CHANNEL_TYPE_RAW_CALLBACK)) {
            if (callback_stream_flag == false)
                callback_stream_flag = true;
            else {
                stream_type = CAMERA_STREAM_TYPE_PREVIEW;
                channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
            }
        }

        switch (channel_type) {
        case CAMERA_CHANNEL_TYPE_REGULAR: {
            mRegularChan->stop(mFrameNum);
            ret = mRegularChan->addStream(stream_type, newStream);
            if (ret) {
                HAL_LOGE("addStream failed");
            }
            if (stream_type == CAMERA_STREAM_TYPE_PREVIEW) {
                preview_size.width = newStream->width;
                preview_size.height = newStream->height;
            } else if (stream_type == CAMERA_STREAM_TYPE_VIDEO) {
                video_size.width = newStream->width;
                video_size.height = newStream->height;
            } else if (stream_type == CAMERA_STREAM_TYPE_CALLBACK) {
                raw_size.width = newStream->width;
                raw_size.height = newStream->height;
            }

            mSetting->getSPRDDEFTag(&sprddefInfo);
            if (preview_size.width > 3264 && preview_size.height > 2448)
                SprdCamera3RegularChannel::kMaxBuffers = 2;
            else if (sprddefInfo.slowmotion > 1)
                SprdCamera3RegularChannel::kMaxBuffers = 8;
            else if (sprddefInfo.sprd_eis_enabled)
                SprdCamera3RegularChannel::kMaxBuffers = 16;
            else
                SprdCamera3RegularChannel::kMaxBuffers = 4;

            HAL_LOGD("slowmotion=%d, kMaxBuffers=%d", sprddefInfo.slowmotion,
                     SprdCamera3RegularChannel::kMaxBuffers);

            newStream->max_buffers = SprdCamera3RegularChannel::kMaxBuffers;
            newStream->priv = mRegularChan;
            break;
        }
        case CAMERA_CHANNEL_TYPE_PICTURE: {
            mPicChan->stop(mFrameNum);
            ret = mPicChan->addStream(stream_type, newStream);
            if (ret) {
                HAL_LOGE("addStream failed");
            }
            if (stream_type == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
                capture_size.width = newStream->width;
                capture_size.height = newStream->height;
            } else if (stream_type == CAMERA_STREAM_TYPE_PICTURE_CALLBACK) {
                raw_size.width = newStream->width;
                raw_size.height = newStream->height;
            }

            newStream->priv = mPicChan;
            newStream->max_buffers = SprdCamera3PicChannel::kMaxBuffers;
            mPictureRequest = false;
            break;
        }
        case CAMERA_CHANNEL_TYPE_RAW_CALLBACK: {
            mRegularChan->stop(mFrameNum);
            mPicChan->stop(mFrameNum);
            ret =
                mRegularChan->addStream(CAMERA_STREAM_TYPE_CALLBACK, newStream);
            if (ret) {
                HAL_LOGE("addStream failed");
            }
            ret = mPicChan->addStream(CAMERA_STREAM_TYPE_PICTURE_CALLBACK,
                                      newStream);
            if (ret) {
                HAL_LOGE("addStream failed");
            }
            raw_size.width = newStream->width;
            raw_size.height = newStream->height;

            newStream->priv = mCallbackChan;
            newStream->max_buffers = SprdCamera3RegularChannel::kMaxBuffers;
            mPictureRequest = false;

            break;
        }
        default:
            HAL_LOGE("channel type is invalid channel");
            break;
        }
    }

    mOldCapIntent = SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE;
    mOEMIf->SetChannelHandle(mRegularChan, mPicChan);
#ifdef CONFIG_CAMERA_EIS
    if (sprddefInfo.sprd_eis_enabled) {
        // leave two*height*1.5 bytes space for eis parameters
        video_size.width = (video_size.width >> 4) << 4;
    }
#endif
    HAL_LOGI("preview: width=%d, height=%d, video: width=%d, height=%d",
             preview_size.width, preview_size.height, video_size.width,
             video_size.height);
    HAL_LOGI("raw: width=%d, height=%d, capture: width=%d, height=%d",
             raw_size.width, raw_size.height, capture_size.width,
             capture_size.height);

    mOEMIf->SetDimensionPreview(preview_size);
    mOEMIf->SetDimensionVideo(video_size);
    mOEMIf->SetDimensionRaw(raw_size);
    mOEMIf->SetDimensionCapture(capture_size);

    // need to update crop region each time when ConfigureStreams
    mOEMIf->setCameraConvertCropRegion();

    mSetting->setPreviewSize(preview_size);
    mSetting->setVideoSize(video_size);
    mSetting->setPictureSize(capture_size);
    // for cts
    if (preview_size.height * preview_size.width > 3264 * 2448 ||
        raw_size.height * raw_size.width > 3264 * 2448) {
        mReciveQeqMax = SprdCamera3PicChannel::kMaxBuffers;
    } else {
        mReciveQeqMax = SprdCamera3RegularChannel::kMaxBuffers;
    }

    /* Initialize mPendingRequestInfo and mPendnigBuffersMap */
    mPendingRequestsList.clear();

    return ret;
}

int SprdCamera3HWI::registerStreamBuffers(
    const camera3_stream_buffer_set_t *buffer_set) {
    int ret = 0;
    Mutex::Autolock l(mLock);

    if (buffer_set == NULL) {
        HAL_LOGE("Invalid buffer_set parameter.");
        return -EINVAL;
    }
    if (buffer_set->stream == NULL) {
        HAL_LOGE("Invalid stream parameter.");
        return -EINVAL;
    }
    if (buffer_set->num_buffers < 1) {
        HAL_LOGE("Invalid num_buffers %d.", buffer_set->num_buffers);
        return -EINVAL;
    }
    if (buffer_set->buffers == NULL) {
        HAL_LOGE("Invalid buffers parameter.");
        return -EINVAL;
    }

    SprdCamera3Channel *channel = reinterpret_cast<SprdCamera3Channel *>(
        buffer_set->stream->priv); //(SprdCamera3Channel *) stream->priv;
    if (channel) {
        ret = channel->registerBuffers(buffer_set);
        if (ret < 0) {
            HAL_LOGE("registerBUffers for stream %p failed",
                     buffer_set->stream);
            return -ENODEV;
        }
    }
    return NO_ERROR;
}

int SprdCamera3HWI::validateCaptureRequest(camera3_capture_request_t *request) {
    size_t idx = 0;
    const camera3_stream_buffer_t *b = NULL;

    /* Sanity check the request */
    if (request == NULL) {
        HAL_LOGE("NULL capture request");
        return BAD_VALUE;
    }

    uint32_t frameNumber = request->frame_number;
    if (request->input_buffer != NULL &&
        request->input_buffer->stream ==
            NULL) { /**modified for 3d capture, enable reprocessing*/
        HAL_LOGE("Request %d: Input buffer not from input stream!",
                 frameNumber);
        return BAD_VALUE;
    }
    if (request->num_output_buffers < 1 || request->output_buffers == NULL) {
        HAL_LOGE("Request %d: No output buffers provided!", frameNumber);
        return BAD_VALUE;
    }
    if (request->input_buffer != NULL) {
        b = request->input_buffer;
        if (b == NULL || b->stream == NULL || b->buffer == NULL) {
            HAL_LOGE("Request %d: Buffer %ld: input_buffer parameter is NULL!",
                     frameNumber, (cmr_int)idx);
            return BAD_VALUE;
        }
        SprdCamera3Channel *channel =
            static_cast<SprdCamera3Channel *>(b->stream->priv);
        if (channel == NULL) {
            HAL_LOGE("Request %d: Buffer %ld: Unconfigured stream!",
                     frameNumber, (cmr_int)idx);
            return BAD_VALUE;
        }
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            HAL_LOGE("Request %d: Buffer %ld: Status not OK!", frameNumber,
                     (cmr_int)idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            HAL_LOGE("Request %d: Buffer %ld: Has a release fence!",
                     frameNumber, (cmr_int)idx);
            return BAD_VALUE;
        }
    }
    // Validate all buffers
    b = request->output_buffers;
    do {
        if (b == NULL || b->stream == NULL || b->buffer == NULL) {
            HAL_LOGE(
                "Request %d: Buffer %ld: output_buffers parameter is NULL!",
                frameNumber, (cmr_int)idx);
            return BAD_VALUE;
        }
        HAL_LOGV("strm=0x%lx hdl=0x%lx b=0x%lx", (cmr_uint)b->stream,
                 (cmr_uint)b->buffer, (cmr_uint)b);
        SprdCamera3Channel *channel =
            static_cast<SprdCamera3Channel *>(b->stream->priv);
        if (channel == NULL) {
            HAL_LOGE("Request %d: Buffer %ld: Unconfigured stream!",
                     frameNumber, (cmr_int)idx);
            return BAD_VALUE;
        }
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            HAL_LOGE("Request %d: Buffer %ld: Status not OK!", frameNumber,
                     (cmr_uint)idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            HAL_LOGE("Request %d: Buffer %ld: Has a release fence!",
                     frameNumber, (cmr_uint)idx);
            return BAD_VALUE;
        }
        idx++;
        b = request->output_buffers + idx;
    } while (idx < (size_t)request->num_output_buffers);

    return NO_ERROR;
}

void SprdCamera3HWI::flushRequest(uint32_t frame_num) {
    ATRACE_CALL();

    SprdCamera3RegularChannel *regularChannel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    SprdCamera3PicChannel *picChannel =
        reinterpret_cast<SprdCamera3PicChannel *>(mPicChan);
    int64_t timestamp = 0;
    HAL_LOGD("ent");
    if (mMetadataChannel)
        mMetadataChannel->stop(mFrameNum);
    if (mRegularChan)
        mRegularChan->stop(mFrameNum);
    if (mPicChan)
        mPicChan->stop(mFrameNum);

    timestamp = systemTime();
    HAL_LOGD("clear stream");
    if (regularChannel) {
        regularChannel->channelUnmapCurrentQBuff(frame_num, timestamp,
                                                 CAMERA_STREAM_TYPE_PREVIEW);
        regularChannel->channelUnmapCurrentQBuff(frame_num, timestamp,
                                                 CAMERA_STREAM_TYPE_VIDEO);
        regularChannel->channelUnmapCurrentQBuff(frame_num, timestamp,
                                                 CAMERA_STREAM_TYPE_CALLBACK);
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_PREVIEW);
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_VIDEO);
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_CALLBACK);
    }

    if (picChannel) {
        picChannel->channelClearAllQBuff(timestamp,
                                         CAMERA_STREAM_TYPE_PICTURE_CALLBACK);
        picChannel->channelClearAllQBuff(timestamp,
                                         CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);
    }
    HAL_LOGD("exit");
}

void SprdCamera3HWI::getLogLevel() {
    char prop[PROPERTY_VALUE_MAX];
    int val = 0;

    property_get("persist.sys.camera.hal.log", prop, "0");
    val = atoi(prop);
    if (val > 0) {
        gHALLogLevel = (uint32_t)val;
    }
}

int SprdCamera3HWI::processCaptureRequest(camera3_capture_request_t *request) {
    ATRACE_CALL();

    int ret = NO_ERROR;
    CapRequestPara capturePara;
    CameraMetadata meta;
    bool invaildRequest = false;
    SprdCamera3Stream *pre_stream = NULL;
    int receive_req_max = mReciveQeqMax;
    int32_t width = 0, height = 0;
    Mutex::Autolock l(mLock);

    ret = validateCaptureRequest(request);
    if (ret != NO_ERROR) {
        HAL_LOGE("incoming request is not valid");
        return ret;
    }
    mFrameNum = request->frame_number;
    meta = request->settings;
    mMetadataChannel->request(meta);
    mMetadataChannel->getCapRequestPara(meta, &capturePara);

    // for BUG459753 HDR capture
    if (capturePara.cap_intent ==
            ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE &&
        capturePara.scene_mode == ANDROID_CONTROL_SCENE_MODE_HDR) {
        mHDRProcessFlag = true;
    } else {
        mHDRProcessFlag = false;
    }

    // check if need to stop multi-layer
    mOEMIf->checkIfNeedToStopMultiLayer();

    switch (capturePara.cap_intent) {
    case ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW:
        if (mOldCapIntent != capturePara.cap_intent) {
            if (!capturePara.sprd_zsl_enabled) {
                mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_PREVIEW, mFrameNum);
                mFirstRegularRequest = true;
            } else {
                mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_SPRD_ZSL_PREVIEW,
                                       mFrameNum);
                if (mOldCapIntent ==
                    ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE) {
                    mFirstRegularRequest = false;
                } else {
                    mFirstRegularRequest = true;
                }
            }
        }
        break;
    case ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD:
        mVideoSnapshotHint = false;
        if (mOldCapIntent != capturePara.cap_intent) {
            mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_VIDEO, mFrameNum);
            if (mOldCapIntent != ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT)
                mFirstRegularRequest = true;
        }
        break;
    case ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE:
        if (mOldCapIntent != capturePara.cap_intent) {
            mOEMIf->setCapturePara(
                CAMERA_CAPTURE_MODE_CONTINUE_NON_ZSL_SNAPSHOT, mFrameNum);
            mPictureRequest = true;
        }

        if (capturePara.sprd_zsl_enabled == true) {
            mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_SPRD_ZSL_SNAPSHOT,
                                   mFrameNum);
            mPictureRequest = true;
        }

        break;
    case ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT:
        // for CTS
        if (mOldCapIntent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD) {
            mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_VIDEO_SNAPSHOT,
                                   mFrameNum);
            mPictureRequest = true;
            if (mVideoSnapshotHint == true) {
                mOEMIf->mBurstVideoSnapshot = true;
            }
        } else if (mOldCapIntent !=
                       ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE &&
                   mOldCapIntent !=
                       ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) {
            mOEMIf->setCapturePara(
                CAMERA_CAPTURE_MODE_CONTINUE_NON_ZSL_SNAPSHOT, mFrameNum);
            mPictureRequest = true;
        }
        break;
    case ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG:
        if (mOldCapIntent != capturePara.cap_intent) {
            mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_ZSL_PREVIEW, mFrameNum);
            mFirstRegularRequest = true;
        }
        break;
    default:
        break;
    }

    if (mOldCapIntent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD &&
        capturePara.cap_intent ==
            ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) {
        mOldCapIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD;
        mVideoSnapshotHint = true;
    } else if (mOldCapIntent != SPRD_CONTROL_CAPTURE_INTENT_FLUSH) {
        mOldCapIntent = capturePara.cap_intent;
    } else {
        mOldCapIntent = SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE;
        invaildRequest = true;
    }

    if (capturePara.cap_request_id == 0)
        capturePara.cap_request_id = mOldRequesId;
    else
        mOldRequesId = capturePara.cap_request_id;

    HAL_LOGI("num_output_buffers=%d, frame_num=%d, mPictureRequest=%d, "
             "captureIntent=%d, mFirstRegularRequest=%d mVideoSnapshotHint =%d",
             request->num_output_buffers, request->frame_number,
             mPictureRequest, capturePara.cap_intent, mFirstRegularRequest,
             mVideoSnapshotHint);

    // fix BUG618304, reset crop ratio when request have both jpeg stream and
    // callback stream
    if (request->num_output_buffers == 3) {
        const camera3_stream_buffer_t &output1 = request->output_buffers[1];
        const camera3_stream_buffer_t &output2 = request->output_buffers[2];
        if (output1.stream->data_space == HAL_DATASPACE_JFIF &&
            output2.stream->data_space == output1.stream->data_space) {
            HAL_LOGD("callback and capture stream dataspace %d",
                     output2.stream->data_space);
            mOEMIf->mSetCapRatioFlag = true;
            mOEMIf->setCameraConvertCropRegion();
            mOEMIf->mSetCapRatioFlag = false;
        }
    }
    if (request->num_output_buffers == 2 &&
        (capturePara.cap_intent ==
         ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE)) {
        if ((request->output_buffers[0].stream->data_space ==
             HAL_DATASPACE_JFIF) ||
            (request->output_buffers[1].stream->data_space ==
             HAL_DATASPACE_JFIF)) {
            mOEMIf->mSetCapRatioFlag = true;
            mOEMIf->setCameraConvertCropRegion();
            mOEMIf->mSetCapRatioFlag = false;
        }
    }
#ifndef MINICAMERA
    for (size_t i = 0; i < request->num_output_buffers; i++) {
        const camera3_stream_buffer_t &output = request->output_buffers[i];
        sp<Fence> acquireFence = new Fence(output.acquire_fence);

        ret = acquireFence->wait(Fence::TIMEOUT_NEVER);
        if (ret != OK) {
            HAL_LOGE("%s: fence wait failed %d", __func__, ret);
            return ret;
        }

        acquireFence = NULL;
    }
#endif

    {
        Mutex::Autolock lr(mRequestLock);
        /* Update pending request list and pending buffers map */
        uint32_t frameNumber = request->frame_number;
        FLASH_Tag flashInfo;
        CONTROL_Tag controlInfo;
        PendingRequestInfo pendingRequest;
        int32_t buf_num = 0, buf_num1 = 0;

        mSetting->getFLASHTag(&flashInfo);
        mSetting->getCONTROLTag(&controlInfo);
        pendingRequest.meta_info.flash_mode = flashInfo.mode;
        memcpy(pendingRequest.meta_info.ae_regions, controlInfo.ae_regions,
               5 * sizeof(controlInfo.ae_regions[0]));
        memcpy(pendingRequest.meta_info.af_regions, controlInfo.af_regions,
               5 * sizeof(controlInfo.af_regions[0]));
        pendingRequest.frame_number = frameNumber;
        pendingRequest.num_buffers = request->num_output_buffers;
        pendingRequest.request_id = capturePara.cap_request_id;
        pendingRequest.bNotified = 0;
        pendingRequest.input_buffer = request->input_buffer;
        for (size_t i = 0; i < request->num_output_buffers; i++) {
            const camera3_stream_buffer_t &output = request->output_buffers[i];
            camera3_stream_t *stream = output.stream;
            SprdCamera3Channel *channel = (SprdCamera3Channel *)stream->priv;

            if (channel == NULL) {
                HAL_LOGE("invalid channel pointer for stream");
                continue;
            }

            if (channel != mCallbackChan) {
                ret = channel->request(stream, output.buffer, frameNumber);
                if (ret) {
                    HAL_LOGE("request buff %p (%d)failed", output.buffer,
                             frameNumber);
                    continue;
                }

                if (capturePara.cap_intent ==
                        ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD &&
                    channel == mPicChan) {
                    mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_VIDEO_SNAPSHOT,
                                           frameNumber);
                    mPictureRequest = true;
                } else if (capturePara.cap_intent ==
                               ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW &&
                           channel == mPicChan) {
                    if (request->num_output_buffers >= 2) {
                        mOEMIf->setCapturePara(
                            CAMERA_CAPTURE_MODE_PREVIEW_SNAPSHOT, frameNumber);
                        if (mOEMIf->GetCameraStatus(CAMERA_STATUS_PREVIEW) ==
                            CAMERA_PREVIEW_IDLE)
                            mFirstRegularRequest = true;
                        receive_req_max = SprdCamera3PicChannel::kMaxBuffers;
                    } else {
                        mOEMIf->setCapturePara(
                            CAMERA_CAPTURE_MODE_ONLY_SNAPSHOT, frameNumber);
                        mPictureRequest = true;
                        mFirstRegularRequest = false;
                    }
                }
            } else {
                if (capturePara.cap_intent ==
                        ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW ||
                    capturePara.cap_intent ==
                        ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD ||
                    capturePara.cap_intent ==
                        ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG) {
                    ret = mRegularChan->request(stream, output.buffer,
                                                frameNumber);
                    if (ret) {
                        HAL_LOGE("request buff %p (%d)failed", output.buffer,
                                 frameNumber);
                        continue;
                    }
                } else if (capturePara.cap_intent ==
                               ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE ||
                           capturePara.cap_intent ==
                               ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) {
                    /**add for 3d capture, set raw callback mode when stream
                     * format is 420_888 begin*/
                    if ((mMultiCameraMode == MODE_3D_CAPTURE ||
                         mMultiCameraMode == MODE_3D_CALIBRATION) &&
                        stream->format == HAL_PIXEL_FORMAT_YCbCr_420_888) {
                        HAL_LOGE("call back stream request!");
                        mOEMIf->setCallBackYuvMode(1);
                    }
                    /**add for 3d capture, set raw callback mode when stream
                     * format is 420_888 end*/
                    ret = mPicChan->request(stream, output.buffer, frameNumber);
                    if (ret) {
                        HAL_LOGE("request buff %p (%d)failed", output.buffer,
                                 frameNumber);
                        continue;
                    }
                }
            }

            RequestedBufferInfo requestedBuf;
            requestedBuf.stream = output.stream;
            requestedBuf.buffer = output.buffer;
            pendingRequest.buffers.push_back(requestedBuf);
        }
        pendingRequest.receive_req_max = receive_req_max;

        mPendingRequestsList.push_back(pendingRequest);
        mPendingRequest++;

        if (request->input_buffer != NULL) {
            const camera3_stream_buffer_t *input = request->input_buffer;
            camera3_stream_t *stream = input->stream;
            SprdCamera3Channel *channel = (SprdCamera3Channel *)stream->priv;

            if (channel == NULL) {
                HAL_LOGE("invalid channel pointer for stream");
                return INVALID_OPERATION;
            }

            HAL_LOGD("input buff %p (%d)", input->buffer, frameNumber);
            mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_ZSL_SNAPSHOT,
                                   frameNumber);
            mPictureRequest = true;
            mOEMIf->setCaptureReprocessMode(true, stream->width,
                                            stream->height);
            ret = mRegularChan->setZSLInputBuff(input->buffer);
            if (ret) {
                HAL_LOGE("zsl input buff %p (%d)failed", input->buffer,
                         frameNumber);
                return INVALID_OPERATION;
            }
        }
    }

    if (invaildRequest) {
        mFirstRegularRequest = false;
        mPictureRequest = false;
        timer_set(this, 1);
        usleep(100 * 1000);
        return NO_ERROR;
    }

    mMetadataChannel->start(mFrameNum);

    // For first regular channel request
    if (mFirstRegularRequest) {
        SprdCamera3Channel *channel;
        for (size_t i = 0; i < request->num_output_buffers; i++) {
            camera3_stream_t *stream = request->output_buffers[i].stream;
            channel = (SprdCamera3Channel *)stream->priv;
            if (channel == mRegularChan || channel == mCallbackChan) {
                ret = mRegularChan->start(mFrameNum);
                if (ret != NO_ERROR) {
                    return ret;
                }
                mFirstRegularRequest = false;
                break;
            }
        }
    }

    /* For take picture channel request*/
    if (mPictureRequest) {
        /*refocus mode, need sync timestamp*/
        if (MODE_REFOCUS == mMultiCameraMode) {
            uint64_t currentTimestamp = getZslBufferTimestamp();
            setZslBufferTimestamp(currentTimestamp);
        }
    }

    // For take picture channel request
    if (mPictureRequest) {
        SprdCamera3Channel *channel;
        for (size_t i = 0; i < request->num_output_buffers; i++) {
            camera3_stream_t *stream = request->output_buffers[i].stream;
            channel = (SprdCamera3Channel *)stream->priv;
            if (channel == mPicChan || channel == mCallbackChan) {
                ret = mPicChan->start(mFrameNum);
                if (ret != NO_ERROR) {
                    HAL_LOGE("fail to take picture");
                }
                mPictureRequest = false;
                break;
            }
        }
    }

    {
        Mutex::Autolock lr(mRequestLock);
        size_t pendingCount = 0;
        while (mPendingRequest >= receive_req_max) {
            mRequestSignal.waitRelative(mRequestLock, kPendingTime);
            if (mFlush || pendingCount > kPendingTimeOut / kPendingTime) {
                HAL_LOGD("mFlush=%d, pendingCount=%ld", mFlush,
                         (cmr_uint)pendingCount);
                if (mFlush)
                    ret = 0;
                else
                    ret = -ENODEV;
                break;
            }
            pendingCount++;
        }
    }
    if (ret == -ENODEV)
        flushRequest(request->frame_number);
    mHDRProcessFlag = false;

    return ret;
}

void SprdCamera3HWI::handleCbDataWithLock(cam_result_data_info_t *result_info) {
    ATRACE_CALL();

    Mutex::Autolock l(mRequestLock);

    uint32_t frame_number = result_info->frame_number;
    buffer_handle_t *buffer = result_info->buffer;
    int64_t capture_time = result_info->timestamp;
    bool is_urgent = result_info->is_urgent;
    camera3_stream_t *stream = result_info->stream;
    camera3_buffer_status_t buffer_status = result_info->buff_status;
    camera3_msg_type_t msg_type = result_info->msg_type;
    int oldrequest = mPendingRequest;
    int num_buffers = 0;
    SprdCamera3Stream *pre_stream = NULL;
    int receive_req_max = SprdCamera3RegularChannel::kMaxBuffers;
    int32_t width = 0, height = 0;

    for (List<PendingRequestInfo>::iterator i = mPendingRequestsList.begin();
         i != mPendingRequestsList.end();) {
        camera3_capture_result_t result;
        camera3_notify_msg_t notify_msg;
        HAL_LOGI("i->frame_number = %d, frame_number = %d, i->request_id = %d",
                 i->frame_number, frame_number, i->request_id);

        if (i->frame_number < frame_number) {
            /**add for 3d capture reprocessing begin   */
            HAL_LOGI("result stream format =%d", result_info->stream->format);
            if (NULL != i->input_buffer) {
                HAL_LOGI("reprocess capture request, continue search");
                i++;
                continue;
            }
            /**add for 3d capture reprocessing end   */
            if (!i->bNotified) {
                notify_msg.type = CAMERA3_MSG_SHUTTER;
                notify_msg.message.shutter.frame_number = i->frame_number;
                notify_msg.message.shutter.timestamp = capture_time;
                mCallbackOps->notify(mCallbackOps, &notify_msg);
                i->bNotified = true;
                HAL_LOGD("drop msg frame_num = %d, timestamp = %" PRId64,
                         i->frame_number, capture_time);

                SENSOR_Tag sensorInfo;
                REQUEST_Tag requestInfo;
                meta_info_t metaInfo;
                mSetting->getSENSORTag(&sensorInfo);
                sensorInfo.timestamp = capture_time;
                mSetting->setSENSORTag(sensorInfo);
                mSetting->getREQUESTTag(&requestInfo);
                requestInfo.id = i->request_id;
                requestInfo.frame_count = i->frame_number;
                mSetting->setREQUESTTag(&requestInfo);
                metaInfo.flash_mode = i->meta_info.flash_mode;
                memcpy(metaInfo.ae_regions, i->meta_info.ae_regions,
                       5 * sizeof(i->meta_info.ae_regions[0]));
                memcpy(metaInfo.af_regions, i->meta_info.af_regions,
                       5 * sizeof(i->meta_info.af_regions[0]));
                mSetting->setMETAInfo(metaInfo);

                result.result = mSetting->translateLocalToFwMetadata();
                result.frame_number = i->frame_number;
                result.num_output_buffers = 0;
                result.output_buffers = NULL;
                result.input_buffer = NULL;
                result.partial_result = 1;
                mCallbackOps->process_capture_result(mCallbackOps, &result);
                free_camera_metadata(
                    const_cast<camera_metadata_t *>(result.result));
            }
            i++;

        } else if (i->frame_number == frame_number) {
            if (!i->bNotified) {
                notify_msg.type = CAMERA3_MSG_SHUTTER;
                notify_msg.message.shutter.frame_number = i->frame_number;
                notify_msg.message.shutter.timestamp = capture_time;
                mCallbackOps->notify(mCallbackOps, &notify_msg);
                i->bNotified = true;
                HAL_LOGV("notified frame_num = %d, timestamp = %" PRId64,
                         i->frame_number, notify_msg.message.shutter.timestamp);

                SENSOR_Tag sensorInfo;
                REQUEST_Tag requestInfo;
                meta_info_t metaInfo;
                mSetting->getSENSORTag(&sensorInfo);
                sensorInfo.timestamp = capture_time;
                mSetting->setSENSORTag(sensorInfo);
                mSetting->getREQUESTTag(&requestInfo);
                requestInfo.id = i->request_id;
                requestInfo.frame_count = i->frame_number;
                mSetting->setREQUESTTag(&requestInfo);
                metaInfo.flash_mode = i->meta_info.flash_mode;
                memcpy(metaInfo.ae_regions, i->meta_info.ae_regions,
                       5 * sizeof(i->meta_info.ae_regions[0]));
                memcpy(metaInfo.af_regions, i->meta_info.af_regions,
                       5 * sizeof(i->meta_info.af_regions[0]));
                mSetting->setMETAInfo(metaInfo);

                result.result = mSetting->translateLocalToFwMetadata();
                result.frame_number = i->frame_number;
                result.num_output_buffers = 0;
                result.output_buffers = NULL;
                result.input_buffer = NULL;
                result.partial_result = 1;
                mCallbackOps->process_capture_result(mCallbackOps, &result);
                free_camera_metadata(
                    const_cast<camera_metadata_t *>(result.result));
            }

            for (List<RequestedBufferInfo>::iterator j = i->buffers.begin();
                 j != i->buffers.end();) {
                if (j->stream == stream && j->buffer == buffer) {
                    camera3_stream_buffer_t *result_buffers =
                        new camera3_stream_buffer_t[1];

                    result_buffers->stream = stream;
                    result_buffers->buffer = buffer;
                    result_buffers->status = buffer_status;
                    result_buffers->acquire_fence = -1;
                    result_buffers->release_fence = -1;

                    result.result = NULL;
                    result.frame_number = i->frame_number;
                    result.num_output_buffers = 1;
                    result.output_buffers = result_buffers;
                    result.input_buffer = i->input_buffer;
                    result.partial_result = 0;

                    if (mMultiCameraMode == MODE_3D_VIDEO) {
                        setVideoBufferTimestamp(capture_time);
                    }
                    mCallbackOps->process_capture_result(mCallbackOps, &result);
                    HAL_LOGV("data frame_number = %d, input_buffer = %p",
                             result.frame_number, i->input_buffer);

                    delete[] result_buffers;

                    i->num_buffers--;
                    j = i->buffers.erase(j);

                    break;
                } else {
                    ++j;
                }
            }

            HAL_LOGI("num_buffers =%d, mPendingRequest =%d", i->num_buffers,
                     mPendingRequest);

            if (0 == i->num_buffers) {
                receive_req_max = i->receive_req_max;
                i = mPendingRequestsList.erase(i);
                mPendingRequest--;
                break;
            } else {
                ++i;
            }
        } else if (i->frame_number > frame_number) {
            /**add for 3d capture reprocessing begin   */
            HAL_LOGI("result stream format =%d", result_info->stream->format);
            if (HAL_PIXEL_FORMAT_BLOB == result_info->stream->format) {
                HAL_LOGI("capture result, continue search");
                i++;
                continue;
            }
            /**add for 3d capture reprocessing end   */
            break;
        }
    }

    if (mPendingRequest != oldrequest && oldrequest >= receive_req_max) {
        HAL_LOGV("signal request=%d", oldrequest);
        mRequestSignal.signal();
    }
}

/**add for 3d capture, get/set needed zsl buffer's timestamp in zsl query
 * begin*/
uint64_t SprdCamera3HWI::getZslBufferTimestamp() {
    return mOEMIf->getZslBufferTimestamp();
}

void SprdCamera3HWI::setZslBufferTimestamp(uint64_t timestamp) {
    mOEMIf->setZslBufferTimestamp(timestamp);
}

/**add for 3d capture, get/set needed zsl buffer's timestamp in zsl query end*/
void SprdCamera3HWI::setMultiCallBackYuvMode(bool mode) {
    mOEMIf->setMultiCallBackYuvMode(mode);
}

void SprdCamera3HWI::setVideoBufferTimestamp(uint64_t timestamp) {
    mCurFrameTimeStamp = timestamp;
}

uint64_t SprdCamera3HWI::getVideoBufferTimestamp() {
    return mCurFrameTimeStamp;
}

void SprdCamera3HWI::getMetadataVendorTagOps(vendor_tag_query_ops_t *ops) {
    ops->get_camera_vendor_section_name = mSetting->getVendorSectionName;
    ops->get_camera_vendor_tag_type = mSetting->getVendorTagType;
    ops->get_camera_vendor_tag_name = mSetting->getVendorTagName;
    ops->get_camera_vendor_tag_count = mSetting->getVendorTagCnt;
    ops->get_camera_vendor_tags = mSetting->getVendorTags;
    return;
}

void SprdCamera3HWI::dump(int /*fd */) {
    /*Enable lock when we implement this function */
    /*
       pthread_mutex_lock(&mMutex);

       pthread_mutex_unlock(&mMutex);
     */
    HAL_LOGD("dump E");
    HAL_LOGD("dump X");
    return;
}

int SprdCamera3HWI::flush() {
    ATRACE_CALL();

    /*Enable lock when we implement this function */
    int ret = NO_ERROR;
    int64_t timestamp = 0;
    // Mutex::Autolock l(mLock);

    if (mOEMIf)
        mOEMIf->enablePowerHint();
    timestamp = systemTime();
    if (mHDRProcessFlag == true) {
        if (mPicChan) {
            mPicChan->stop(mFrameNum);
            mPicChan->channelClearAllQBuff(timestamp,
                                           CAMERA_STREAM_TYPE_PICTURE_CALLBACK);
            mPicChan->channelClearAllQBuff(timestamp,
                                           CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);
        }
    }

    mFlush = true;
    Mutex::Autolock l(mLock);

    // for performance tuning: close camera
    mOEMIf->setSensorCloseFlag();

    if (mMetadataChannel)
        mMetadataChannel->stop(mFrameNum);
    if (mRegularChan)
        mRegularChan->stop(mFrameNum);
    if (mPicChan)
        mPicChan->stop(mFrameNum);

    timer_set(this, 3);
    mOldCapIntent = SPRD_CONTROL_CAPTURE_INTENT_FLUSH;
    ret = mFlushSignal.waitRelative(mLock, 500000000); // 500ms
    if (ret == TIMED_OUT) {
        HAL_LOGE("Flush is time out");
        mFlush = false;
        return -ENODEV;
    }

    mFlush = false;
    if (mOEMIf)
        mOEMIf->disablePowerHint();

    return 0;
}

void SprdCamera3HWI::captureResultCb(cam_result_data_info_t *result_info) {
    // Mutex::Autolock l(mLock);

    if (NULL == result_info) {
        HAL_LOGE("param invalid");
        return;
    }

    handleCbDataWithLock(result_info);

    return;
}

void SprdCamera3HWI::captureResultCb(cam_result_data_info_t *result_info,
                                     void *userdata) {
    SprdCamera3HWI *hw = (SprdCamera3HWI *)userdata;
    if (hw == NULL) {
        HAL_LOGE("Invalid hw %p", hw);
        return;
    }

    hw->captureResultCb(result_info);
    return;
}

int SprdCamera3HWI::initialize(const struct camera3_device *device,
                               const camera3_callback_ops_t *callback_ops) {
    ATRACE_CALL();

    HAL_LOGD("mCameraOps E");
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return -ENODEV;
    }

    int ret = hw->initialize(callback_ops);
    HAL_LOGD("mCameraOps X");
    return ret;
}

int SprdCamera3HWI::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    ATRACE_CALL();

    HAL_LOGD("mCameraOps E");
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return -ENODEV;
    }
    int ret = hw->configureStreams(stream_list);
    HAL_LOGD("mCameraOps X");
    return ret;
}

int SprdCamera3HWI::register_stream_buffers(
    const struct camera3_device *device,
    const camera3_stream_buffer_set_t *buffer_set) {
    HAL_LOGD("mCameraOps E");
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return -ENODEV;
    }
    int ret = hw->registerStreamBuffers(buffer_set);
    HAL_LOGD("mCameraOps X");
    return ret;
}

const camera_metadata_t *SprdCamera3HWI::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    HAL_LOGD("mCameraOps E");
    camera_metadata_t *fwk_metadata = NULL;
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return NULL;
    }

    fwk_metadata = hw->constructDefaultMetadata(type);
    HAL_LOGD("mCameraOps X");
    return fwk_metadata;
}

int SprdCamera3HWI::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    ATRACE_CALL();

    HAL_LOGV("mCameraOps E");
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return -EINVAL;
    }

    int ret = hw->processCaptureRequest(request);
    HAL_LOGV("mCameraOps X");
    return ret;
}

void SprdCamera3HWI::get_metadata_vendor_tag_ops(
    const struct camera3_device *device, vendor_tag_query_ops_t *ops) {
    HAL_LOGD("mCameraOps E");
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return;
    }

    hw->getMetadataVendorTagOps(ops);
    HAL_LOGD("mCameraOps X");
    return;
}

void SprdCamera3HWI::dump(const struct camera3_device *device, int fd) {
    HAL_LOGV("S");
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return;
    }

    hw->dump(fd);
    HAL_LOGV("X");
    return;
}

int SprdCamera3HWI::flush(const struct camera3_device *device) {
    ATRACE_CALL();

    int ret;

    HAL_LOGD("mCameraOps E");
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return -EINVAL;
    }

    ret = hw->flush();
    HAL_LOGD("mCameraOps X");
    return ret;
}

int SprdCamera3HWI::close_camera_device(struct hw_device_t *device) {
    ATRACE_CALL();

    int ret = NO_ERROR;
    int id;
    HAL_LOGI("E");

    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(
        reinterpret_cast<camera3_device_t *>(device)->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return BAD_VALUE;
    }
    HAL_LOGD("camera id %d, device addr %p", hw->mCameraId, device);
    id = hw->mCameraId;
    delete hw;
    hw = NULL;
    device == NULL;

#ifdef CONFIG_CAMERA_ISP
    stopispserver();
    ispvideo_RegCameraFunc(1, NULL);
    ispvideo_RegCameraFunc(2, NULL);
    ispvideo_RegCameraFunc(3, NULL);
    ispvideo_RegCameraFunc(4, NULL);
//	ispvideo_RegCameraFunc(REG_CTRL_FLASH, NULL);
#endif
    if (id == SENSOR_MAIN) {
        g_cam_device[0] = NULL;
    } else if (id == SENSOR_SUB) {
        g_cam_device[1] = NULL;
    } else if (id == SENSOR_DEVICE2) {
        g_cam_device[2] = NULL;
    } else if (id == SENSOR_DEVICE3) {
        g_cam_device[3] = NULL;
    }

    if (mCameraSessionActive > 0)
        mCameraSessionActive--;
    mMultiCameraMode = MODE_SINGLE_CAMERA;
    HAL_LOGI("X mCameraSessionActive %d", mCameraSessionActive);
    return ret;
}

int SprdCamera3HWI::timer_stop() {
    HAL_LOGD("E");

    if (mPrvTimerID) {
        timer_delete(mPrvTimerID);
        mPrvTimerID = NULL;
    }

    return NO_ERROR;
}

int SprdCamera3HWI::timer_set(void *obj, int32_t delay_ms) {
    int status;
    struct itimerspec ts;
    struct sigevent se;
    SprdCamera3HWI *dev = reinterpret_cast<SprdCamera3HWI *>(obj);
    HAL_LOGD("E");

    if (mPrvTimerID == NULL) {
        memset(&se, 0, sizeof(struct sigevent));
        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = dev;
        se.sigev_notify_function = timer_handler;
        se.sigev_notify_attributes = NULL;

        status = timer_create(CLOCK_MONOTONIC, &se, &mPrvTimerID);
        if (status != 0) {
            HAL_LOGI("time create fail");
            return status;
        }
    }

    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = delay_ms * 1000000;
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    status = timer_settime(mPrvTimerID, 0, &ts, NULL);

    return status;
}

void SprdCamera3HWI::timer_handler(union sigval arg) {
    ATRACE_CALL();

    SprdCamera3HWI *dev = reinterpret_cast<SprdCamera3HWI *>(arg.sival_ptr);
    SprdCamera3RegularChannel *regularChannel =
        reinterpret_cast<SprdCamera3RegularChannel *>(dev->mRegularChan);
    SprdCamera3PicChannel *picChannel =
        reinterpret_cast<SprdCamera3PicChannel *>(dev->mPicChan);
    int64_t timestamp = 0;

    HAL_LOGD("E");

    timestamp = systemTime();

    if (regularChannel) {
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_PREVIEW);
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_VIDEO);
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_CALLBACK);
    }

    if (picChannel) {
        picChannel->channelClearAllQBuff(timestamp,
                                         CAMERA_STREAM_TYPE_PICTURE_CALLBACK);
        picChannel->channelClearAllQBuff(timestamp,
                                         CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);
    }
    // dev->mOldCapIntent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
    (dev->mFlushSignal).signal();
    HAL_LOGD("X");
}

void SprdCamera3HWI::setMultiCameraMode(multiCameraMode multiCameraModeId) {
    mMultiCameraMode = multiCameraModeId;
    HAL_LOGD("mMultiCameraMode=%d ", mMultiCameraMode);
}

bool SprdCamera3HWI::isMultiCameraMode(int cameraId) {
    HAL_LOGD("cameraId= %d ", cameraId);
    if ((MIN_MULTI_CAMERA_FAKE_ID <= cameraId) &&
        (cameraId <= MAX_MULTI_CAMERA_FAKE_ID))
        return true;
    else
        return false;
}

void SprdCamera3HWI::setSprdCameraLowpower(int flag) {
    mSprdCameraLowpower = flag;
    mOEMIf->setSprdCameraLowpower(flag);
}

}; // end namespace sprdcamera
