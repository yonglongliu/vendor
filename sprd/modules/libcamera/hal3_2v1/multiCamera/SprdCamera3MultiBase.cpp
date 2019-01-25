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

#include "SprdCamera3MultiBase.h"
#include <linux/ion.h>
#include <cutils/ashmem.h>
#include <sys/mman.h>
#include "gralloc_priv.h"
#include "gralloc_buffer_priv.h"
#if defined(CONFIG_SPRD_ANDROID_8)
#include <ui/GraphicBuffer.h>
#endif

#include <ui/GraphicBufferMapper.h>

using namespace android;
namespace sprdcamera {
#define MAX_UNMATCHED_QUEUE_BASE_SIZE (3)

#ifdef CONFIG_CAMERA_SHARKLE_BRINGUP
#define MATCH_FRAME_TIME_DIFF (40)
#else
#define MATCH_FRAME_TIME_DIFF (30)
#endif

#define LUMA_SOOMTH_COEFF (5)
#define DARK_LIGHT_TH (3000)
#define LOW_LIGHT_TH (1500)
#define BRIGHT_LIGHT_TH (800)
#define DARK_LIGHT_LUAM_TH (150)
#ifdef CONFIG_COVERED_SENSOR
#define CAMERA3MAXFACE 11
#else
#define CAMERA3MAXFACE 10
#endif

#define IS_CASHE true

SprdCamera3MultiBase::SprdCamera3MultiBase()
    : mIommuEnabled(false), mVFrameCount(0), mVLastFrameCount(0),
      mVLastFpsTime(0), mLowLumaConut(0), mconut(0), mCurScene(DARK_LIGHT),
      mBrightConut(0), mLowConut(0), mDarkConut(0) {
    mLumaList.clear();
    mCameraMode = MODE_SINGLE_CAMERA;
    mReqState = PREVIEW_REQUEST_STATE;
    /* forcibly set to enabled */
    mIommuEnabled = true;
}

SprdCamera3MultiBase::~SprdCamera3MultiBase() {}
int SprdCamera3MultiBase::initialize(multiCameraMode mode) {
    int rc = 0;

    mLumaList.clear();
    mCameraMode = mode;
    mLowLumaConut = 0;
    mconut = 0;
    mCurScene = DARK_LIGHT;
    mBrightConut = 0;
    mLowConut = 0;
    mDarkConut = 0;

    return rc;
}

int SprdCamera3MultiBase::flushIonBuffer(int buffer_fd, void *v_addr,
                                         size_t size) {
    int ret = 0;
    ret = MemIon::Flush_ion_buffer(buffer_fd, v_addr, NULL, size);
    if (ret) {
        HAL_LOGW("abnormal ret=%d", ret);
        HAL_LOGW("fd=%d,vaddr=%p", buffer_fd, v_addr);
    }

    return ret;
}

int SprdCamera3MultiBase::allocateOne(int w, int h, new_mem_t *new_mem,
                                      int type) {

    sp<GraphicBuffer> graphicBuffer = NULL;
    native_handle_t *native_handle = NULL;
    void *vir_addr = 0;
    uint32_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                            GraphicBuffer::USAGE_SW_READ_OFTEN |
                            GraphicBuffer::USAGE_SW_WRITE_OFTEN;

#if defined(CONFIG_SPRD_ANDROID_8)
    graphicBuffer = new GraphicBuffer(w, h, HAL_PIXEL_FORMAT_YCrCb_420_SP, 1,
                                      yuvTextUsage, "allocateOne");
#else
    graphicBuffer =
        new GraphicBuffer(w, h, HAL_PIXEL_FORMAT_YCrCb_420_SP, yuvTextUsage);
#endif
    native_handle = (native_handle_t *)graphicBuffer->handle;
    new_mem->native_handle = native_handle;
    new_mem->graphicBuffer = graphicBuffer;
    new_mem->width = w;
    new_mem->height = h;
    new_mem->type = (camera_buffer_type_t)type;

    return 0;
}

int SprdCamera3MultiBase::findGraphicBuf(List<new_mem_t *> &list,
                                         native_handle_t *native_handle,
                                         sp<GraphicBuffer> &pbuffer) {
    int ret = NO_ERROR;
    if (list.empty()) {
        HAL_LOGE("list is NULL");
        return -EINVAL;
    }
    Mutex::Autolock l(mBufferListLock);
    List<new_mem_t *>::iterator j = list.begin();
    for (; j != list.end(); j++) {
        if ((*j)->native_handle == native_handle) {
            pbuffer = (*j)->graphicBuffer;
            list.erase(j);
            return ret;
        }
    }
    list.erase(j);
    return UNKNOWN_ERROR;
}

int SprdCamera3MultiBase::findWidthHeigth(List<new_mem_t *> &list,
                                          native_handle_t *native_handle,
                                          int *width, int *height) {
    int ret = NO_ERROR;
    if (list.empty()) {
        HAL_LOGE("list is NULL");
        return -EINVAL;
    }
    Mutex::Autolock l(mBufferListLock);
    List<new_mem_t *>::iterator j = list.begin();
    for (; j != list.end(); j++) {
        if ((*j)->native_handle == native_handle) {
            *width = (*j)->width;
            *height = (*j)->height;
            list.erase(j);
            return ret;
        }
    }
    list.erase(j);
    return UNKNOWN_ERROR;
}

void SprdCamera3MultiBase::freeOneBuffer(new_mem_t *buffer) {

    if (buffer != NULL) {
        if (buffer->graphicBuffer != NULL) {
            buffer->graphicBuffer.clear();
            buffer->graphicBuffer = NULL;
        }
        buffer->native_handle = NULL;
        buffer->vir_addr = NULL;
    } else {
        HAL_LOGD("Not allocated, No need to free");
    }
    HAL_LOGI("X");
    return;
}

int SprdCamera3MultiBase::validateCaptureRequest(
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

void SprdCamera3MultiBase::convertToRegions(int32_t *rect, int32_t *region,
                                            int weight) {
    region[0] = rect[0];
    region[1] = rect[1];
    region[2] = rect[2];
    region[3] = rect[3];
    if (weight > -1) {
        region[4] = weight;
    }
}

uint8_t SprdCamera3MultiBase::getCoveredValue(CameraMetadata &frame_settings,
                                              SprdCamera3HWI *hwiSub,
                                              SprdCamera3HWI *hwiMin,
                                              int convered_camera_id) {
    int rc = 0;
    uint32_t couvered_value = 0;
    int average_value = 0;
    uint32_t value = 0, main_value = 0;
    int max_convered_value = 8;
    int luma_soomth_coeff = LUMA_SOOMTH_COEFF;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    int i = 0;
    if (hwiMin) {
        rc = hwiMin->camera_ioctrl(CAMERA_IOCTRL_GET_SENSOR_LUMA, &main_value,
                                   NULL);
        if (rc < 0) {
            HAL_LOGD("read main sensor failed");
            main_value = BRIGHT_LIGHT_TH;
        }
    } else {
        main_value = BRIGHT_LIGHT_TH;
    }

    if (main_value <= BRIGHT_LIGHT_TH) {
        mCurScene = BRIGHT_LIGHT;
    } else if (mCameraMode == MODE_SELF_SHOT) {
        if (main_value <= LOW_LIGHT_TH) {
            mCurScene = LOW_LIGHT;
        } else {
            mCurScene = DARK_LIGHT;
        }
    }
    // 10 frame to see init scene
    if (mconut < 10) {
        if (main_value <= BRIGHT_LIGHT_TH) {
            mCurScene = BRIGHT_LIGHT;
            mBrightConut++;
        } else if (main_value <= LOW_LIGHT_TH) {
            mCurScene = LOW_LIGHT;
            mLowConut++;
        } else {
            mCurScene = DARK_LIGHT;
            mDarkConut++;
        }
        mCurScene = mBrightConut >= mLowConut ? BRIGHT_LIGHT : LOW_LIGHT;
        if (mCurScene == BRIGHT_LIGHT) {
            mCurScene = mBrightConut >= mDarkConut ? BRIGHT_LIGHT : DARK_LIGHT;
        } else {
            mCurScene = mLowConut >= mDarkConut ? LOW_LIGHT : DARK_LIGHT;
        }
        mconut++;
    }
    HAL_LOGD("avCurScene=%u,main_value=%u", mCurScene, main_value);

    if (mCurScene == BRIGHT_LIGHT) {
        if (main_value > DARK_LIGHT_TH) {
            // convered main camera
            luma_soomth_coeff = 1;
            max_convered_value = MAX_CONVERED_VALURE + 5;
        } else if (main_value > LOW_LIGHT_TH) {
            // convered main camera or low light or move
            luma_soomth_coeff = LUMA_SOOMTH_COEFF;
            max_convered_value = MAX_CONVERED_VALURE - 2;
        } else if (main_value < 200) {
            // high bright light, sensitivity high
            max_convered_value = MAX_CONVERED_VALURE + 5;
            luma_soomth_coeff = 1;
        } else if (main_value < BRIGHT_LIGHT_TH - 400) {
            // bright light, sensitivity high
            max_convered_value = MAX_CONVERED_VALURE;
            luma_soomth_coeff = 2;
        } else {
            luma_soomth_coeff = LUMA_SOOMTH_COEFF;
            max_convered_value = MAX_CONVERED_VALURE + 5;
        }
    } else if (mCurScene == LOW_LIGHT) {
        max_convered_value = 3;
    } else {
        max_convered_value = -1;
    }
    property_get("debug.camera.covered_max_th", prop, "0");
    if (0 != atoi(prop)) {
        max_convered_value = atoi(prop);
    }

    property_get("debug.camera.covered", prop, "0");

    rc = hwiSub->camera_ioctrl(CAMERA_IOCTRL_GET_SENSOR_LUMA, &value, NULL);
    if (rc < 0) {
        HAL_LOGD("read sub sensor failed");
    }

    if (0 != atoi(prop)) {
        value = atoi(prop);
    }
    property_get("debug.camera.covered_s_th", prop, "0");
    if (0 != atoi(prop)) {
        luma_soomth_coeff = atoi(prop);
    }
    if ((int)mLumaList.size() >= luma_soomth_coeff) {
        List<int>::iterator itor = mLumaList.begin();
        mLumaList.erase(itor);
    }
    mLumaList.push_back((int)value);
    for (List<int>::iterator i = mLumaList.begin(); i != mLumaList.end(); i++) {
        average_value += *i;
    }
    if (average_value)
        average_value /= mLumaList.size();

    if (average_value < max_convered_value) {
        mLowLumaConut++;
    } else {
        mLowLumaConut = 0;
    }
    if (mLowLumaConut >= luma_soomth_coeff) {
        couvered_value = BLUR_SELFSHOT_CONVERED;
    } else {
        couvered_value = BLUR_SELFSHOT_NO_CONVERED;
    }
    HAL_LOGD("update_value %u ,ori value=%u "
             ",average_value=%u,mLowLumaConut=%u,th:%d",
             couvered_value, value, average_value, mLowLumaConut,
             max_convered_value);
    if (mLowLumaConut >= luma_soomth_coeff) {
        mLowLumaConut--;
    }
    // update face[10].score info to mean convered value when api1 is used
    {
        FACE_Tag *faceDetectionInfo = (FACE_Tag *)&(
            hwiSub->mSetting->s_setting[convered_camera_id].faceInfo);
        uint8_t numFaces = faceDetectionInfo->face_num;
        uint8_t faceScores[CAMERA3MAXFACE];
        uint8_t dataSize = CAMERA3MAXFACE;
        int32_t faceRectangles[CAMERA3MAXFACE * 4];
        int j = 0;

        memset((void *)faceScores, 0, sizeof(uint8_t) * CAMERA3MAXFACE);
        memset((void *)faceRectangles, 0, sizeof(int32_t) * CAMERA3MAXFACE * 4);

        numFaces = CAMERA3MAXFACE - 1;

        for (int i = 0; i < numFaces; i++) {
            faceScores[i] = faceDetectionInfo->face[i].score;
            if (faceScores[i] == 0) {

                faceScores[i] = 1;
            }
            convertToRegions(faceDetectionInfo->face[i].rect,
                             faceRectangles + j, -1);
            j += 4;
        }
        faceScores[CAMERA3MAXFACE - 1] = couvered_value;
        faceRectangles[(CAMERA3MAXFACE * 4) - 1] = -1;

        frame_settings.update(ANDROID_STATISTICS_FACE_SCORES, faceScores,
                              dataSize);
        frame_settings.update(ANDROID_STATISTICS_FACE_RECTANGLES,
                              faceRectangles, dataSize * 4);
    }
    return couvered_value;
}

buffer_handle_t *
SprdCamera3MultiBase::popRequestList(List<buffer_handle_t *> &list) {
    buffer_handle_t *ret = NULL;
    if (list.empty()) {
        return NULL;
    }
    List<buffer_handle_t *>::iterator j = list.begin();
    ret = *j;
    list.erase(j);
    return ret;
}

buffer_handle_t *
SprdCamera3MultiBase::popBufferList(List<new_mem_t *> &list,
                                    camera_buffer_type_t type) {
    buffer_handle_t *ret = NULL;
    if (list.empty()) {
        HAL_LOGE("list is NULL");
        return NULL;
    }
    Mutex::Autolock l(mBufferListLock);
    List<new_mem_t *>::iterator j = list.begin();
    for (; j != list.end(); j++) {
        if ((*j)->type == type) {
            ret = &((*j)->native_handle);
            break;
        }
    }
    if (ret == NULL) {
        HAL_LOGE("popBufferList failed!");
        return ret;
    }
    list.erase(j);
    return ret;
}
void SprdCamera3MultiBase::pushBufferList(new_mem_t *localbuffer,
                                          buffer_handle_t *backbuf,
                                          int localbuffer_num,
                                          List<new_mem_t *> &list) {
    int i;

    if (backbuf == NULL) {
        HAL_LOGE("backbuf is NULL");
        return;
    }
    Mutex::Autolock l(mBufferListLock);
    for (i = 0; i < localbuffer_num; i++) {
        if (localbuffer[i].native_handle == NULL)
            continue;
        if ((&(localbuffer[i].native_handle)) == backbuf) {
            list.push_back(&(localbuffer[i]));
            break;
        }
    }
    if (i >= localbuffer_num) {
        HAL_LOGE("find backbuf failed");
    }
    return;
}
int SprdCamera3MultiBase::map(buffer_handle_t *buffer, void **vaddr) {
    int ret = NO_ERROR;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    int width = ADP_WIDTH(*buffer);
    int height = ADP_HEIGHT(*buffer);
    Rect bounds(width, height);
    int usage;

    usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
    ret = mapper.lock((const native_handle_t *)*buffer, usage, bounds, vaddr);
    if (ret != NO_ERROR) {
        HAL_LOGE("onQueueFilled, mapper.lock fail %p, ret %d", *buffer, ret);
    }
    return ret;
}

int SprdCamera3MultiBase::unmap(buffer_handle_t *buffer) {
    int ret = NO_ERROR;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    ret = mapper.unlock((const native_handle_t *)*buffer);
    if (ret != NO_ERROR) {
        ALOGE("onQueueFilled, mapper.unlock fail %p", *buffer);
    }
    return ret;
}

int SprdCamera3MultiBase::getStreamType(camera3_stream_t *new_stream) {
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

void SprdCamera3MultiBase::dumpFps() {
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    int64_t now = systemTime();
    int64_t diff = now - mVLastFpsTime;
    double mVFps;

    property_get("dump.mCamera.fps", prop, "0");
    if (atoi(prop) == 1) {
        mVFrameCount++;
        if (diff > ms2ns(10000)) {
            mVFps = (((double)(mVFrameCount - mVLastFrameCount)) *
                     (double)(s2ns(1))) /
                    (double)diff;
            HAL_LOGD("[KPI Perf]:Fps: %.4f ", mVFps);
            mVLastFpsTime = now;
            mVLastFrameCount = mVFrameCount;
        }
    }
}

void SprdCamera3MultiBase::dumpData(unsigned char *addr, int type, int size,
                                    int param1, int param2, int param3,
                                    const char param4[20]) {
    FILE *fp = NULL;
    char tmp_str[64] = {0};
    time_t timep;
    struct tm *p;
    time(&timep);
    char file_name[256] = {0};
    p = localtime(&timep);
    strcpy(file_name, "/data/misc/cameraserver/");
    sprintf(tmp_str, "%04d%02d%02d%02d%02d%02d", (1900 + p->tm_year),
            (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    strcat(file_name, tmp_str);
    switch (type) {
    case 1: {
        memset(tmp_str, 0, sizeof(tmp_str));
        sprintf(tmp_str, "_%dx%d_%d_%s.yuv", param1, param2, param3, param4);
        strcat(file_name, tmp_str);

        fp = fopen(file_name, "w");
        if (fp == NULL) {
            HAL_LOGE("open yuv file fail!\n");
            return;
        }
        fwrite((void *)addr, 1, size, fp);
        fclose(fp);
    } break;
    case 2: {
        memset(tmp_str, 0, sizeof(tmp_str));
        sprintf(tmp_str, "_%dx%d_%d_%s.jpg", param1, param2, param3, param4);
        strcat(file_name, tmp_str);
        fp = fopen(file_name, "wb");
        if (fp == NULL) {
            HAL_LOGE("can not open file: %s \n", file_name);
            return;
        }
        fwrite((void *)addr, 1, size, fp);
        fclose(fp);
    } break;
    case 3: {
        int i = 0;
        int j = 0;
        memset(tmp_str, 0, sizeof(tmp_str));
        snprintf(tmp_str, sizeof(tmp_str), "_%d_params_%s.txt", size, param4);
        strcat(file_name, tmp_str);
        fp = fopen(file_name, "w+");
        if (fp == NULL) {
            HAL_LOGE("open txt file fail!\n");
            return;
        }
        for (i = 0; i < size; i++) {
            int result = 0;
            if (i == size - 1) {
                for (j = 0; j < param1; j++) {
                    fprintf(fp, "%c", addr[i * param1 + j]);
                }
            } else {
                for (j = 0; j < param1; j++) {
                    result += addr[i * param1 + j] << j * 8;
                }
                fprintf(fp, "%d\n", result);
            }
        }
        fclose(fp);
    } break;
    default:
        break;
    }
}

bool SprdCamera3MultiBase::matchTwoFrame(hwi_frame_buffer_info_t result1,
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
            int64_t diff =
                (int64_t)result1.timestamp - (int64_t)itor2->timestamp;
            if (ns2ms(abs((cmr_s32)diff)) < MATCH_FRAME_TIME_DIFF) {
                *result2 = *itor2;
                list.erase(itor2);
                HAL_LOGV("[%d:match:%d],diff=%llu T1:%llu,T2:%llu",
                         result1.frame_number, itor2->frame_number, diff,
                         result1.timestamp, itor2->timestamp);
                return MATCH_SUCCESS;
            }
            itor2++;
        }
        HAL_LOGV("no match for idx:%d, could not find matched frame",
                 result1.frame_number);
        return MATCH_FAILED;
    }
}
hwi_frame_buffer_info_t *SprdCamera3MultiBase::pushToUnmatchedQueue(
    hwi_frame_buffer_info_t new_buffer_info,
    List<hwi_frame_buffer_info_t> &queue) {
    hwi_frame_buffer_info_t *pushout = NULL;
    if (queue.size() == MAX_UNMATCHED_QUEUE_BASE_SIZE) {
        pushout = new hwi_frame_buffer_info_t;
        List<hwi_frame_buffer_info_t>::iterator i = queue.begin();
        *pushout = *i;
        queue.erase(i);
    }
    queue.push_back(new_buffer_info);

    return pushout;
}
bool SprdCamera3MultiBase::alignTransform(void *src, int w_old, int h_old,
                                          int w_new, int h_new, void *dest) {
    if (w_new <= w_old && h_new <= h_old) {
        HAL_LOGE("check size failed");
        return false;
    }
    if (src == NULL || dest == NULL) {
        HAL_LOGE("ops, src data or dest is empty, please check it");
        return false;
    }
    int diff_num = 0;
    unsigned char *srcTmp = (unsigned char *)src;
    unsigned char *destTmp = (unsigned char *)dest;

    if (h_new > h_old && w_new == w_old) {
        // int heightOfData = height;
        diff_num = h_new - h_old;

        // copy y data
        memcpy(destTmp, srcTmp, w_old * h_old);

        // fill last y data
        destTmp += w_old * h_old;
        srcTmp += w_old * (h_old - 1);
        for (int i = 0; i < diff_num; i++) {
            memcpy(destTmp, srcTmp, w_old);
            destTmp += w_old;
        }

        // copy uv data
        srcTmp = (unsigned char *)src;
        destTmp = (unsigned char *)dest;
        destTmp += w_new * h_new;
        srcTmp += w_old * h_old;
        memcpy(destTmp, srcTmp, w_old * h_old / 2);

        ////fill last uv data
        srcTmp = (unsigned char *)src;
        destTmp = (unsigned char *)dest;
        destTmp += w_new * (h_new + h_old / 2);
        srcTmp += w_old * h_old * 3 / 2 - w_old;
        for (int i = 0; i < diff_num / 2; i++) {
            memcpy(destTmp, srcTmp, w_old);
            destTmp += w_old;
        }
    } else if (w_new > w_old && h_new == h_old) {
        int heightOfData = h_new * 3 / 2;
        diff_num = w_new - w_old;

        for (int i = 0; i < heightOfData; i++) {
            memcpy(destTmp, srcTmp, w_old);
            for (int j = 0; j < diff_num; j++) {
                *(destTmp + w_old + j) = *(srcTmp + w_old - 1);
            }
            srcTmp += w_old;
            destTmp += w_new;
        }
    }
    return true;
}

int SprdCamera3MultiBase::convertToImg_frm(buffer_handle_t *in, img_frm *out,
                                           cmr_u32 format, void *vir_addr) {
    HAL_LOGD("in:%p, out:%p format:%u", in, out, format);
    int ret = 0;
    out->addr_phy.addr_y = 0;
    out->addr_phy.addr_u =
        out->addr_phy.addr_y + ADP_WIDTH(*in) * ADP_HEIGHT(*in);
    out->addr_phy.addr_v = out->addr_phy.addr_u;
    HAL_LOGD("in->width:%d, in->height:%d", ADP_WIDTH(*in), ADP_HEIGHT(*in));
    out->addr_vir.addr_y = (cmr_uint)vir_addr;
    out->addr_vir.addr_u =
        out->addr_vir.addr_y + ADP_WIDTH(*in) * ADP_HEIGHT(*in);
    HAL_LOGD("out->addr_vir.addr_y:%lx", out->addr_vir.addr_y);
    out->buf_size = ADP_BUFSIZE(*in);
    HAL_LOGD("out->buf_size:%d", out->buf_size);
    out->fd = ADP_BUFFD(*in);
    out->fmt = format;
    out->rect.start_x = 0;
    out->rect.start_y = 0;
    out->rect.width = ADP_WIDTH(*in);
    out->rect.height = ADP_HEIGHT(*in);
    out->size.width = ADP_WIDTH(*in);
    out->size.height = ADP_HEIGHT(*in);
    return ret;
}

int SprdCamera3MultiBase::convertToImg_frm(void *phy_addr, void *vir_addr,
                                           int width, int height, int fd,
                                           cmr_u32 format, img_frm *out) {

    int ret = 0;
    out->addr_phy.addr_y = (cmr_uint)phy_addr;
    out->addr_phy.addr_u = out->addr_phy.addr_y + width * height;
    out->addr_phy.addr_v = out->addr_phy.addr_u;

    out->addr_vir.addr_y = (cmr_uint)vir_addr;
    out->addr_vir.addr_u = out->addr_vir.addr_y + width * height;
    out->buf_size = width * height * 3 >> 1;
    out->fd = fd;
    out->fmt = format;
    out->rect.start_x = 0;
    out->rect.start_y = 0;
    out->rect.width = width;
    out->rect.height = height;
    out->size.width = width;
    out->size.height = height;

    return ret;
}

/*
#ifdef CONFIG_FACE_BEAUTY
void SprdCamera3MultiBase::convert_face_info(int *ptr_cam_face_inf, int
width,
                                             int height) {}

void SprdCamera3MultiBase::doFaceMakeup(struct camera_frame_type *frame,
                                        int perfect_level, int *face_info) {
    // init the parameters table. save the value until the process is
restart or
    // the device is restart.
    int tab_skinWhitenLevel[10] = {0, 15, 25, 35, 45, 55, 65, 75, 85, 95};
    int tab_skinCleanLevel[10] = {0, 25, 45, 50, 55, 60, 70, 80, 85, 95};

    TSRect Tsface;
    YuvFormat yuvFormat = TSFB_FMT_NV21;
    if (face_info[0] != 0 || face_info[1] != 0 || face_info[2] != 0 ||
        face_info[3] != 0) {
        convert_face_info(face_info, frame->width, frame->height);
        Tsface.left = face_info[0];
        Tsface.top = face_info[1];
        Tsface.right = face_info[2];
        Tsface.bottom = face_info[3];
        HAL_LOGD("FACE_BEAUTY rect:%ld-%ld-%ld-%ld", Tsface.left,
Tsface.top,
                 Tsface.right, Tsface.bottom);

        int level = perfect_level;
        int skinWhitenLevel = 0;
        int skinCleanLevel = 0;
        int level_num = 0;
        // convert the skin_level set by APP to skinWhitenLevel &
skinCleanLevel
        // according to the table saved.
        level = (level < 0) ? 0 : ((level > 90) ? 90 : level);
        level_num = level / 10;
        skinWhitenLevel = tab_skinWhitenLevel[level_num];
        skinCleanLevel = tab_skinCleanLevel[level_num];
        HAL_LOGD("UCAM skinWhitenLevel is %d, skinCleanLevel is %d "
                 "frame->height %d frame->width %d",
                 skinWhitenLevel, skinCleanLevel, frame->height,
frame->width);

        TSMakeupData inMakeupData;
        unsigned char *yBuf = (unsigned char *)(frame->y_vir_addr);
        unsigned char *uvBuf =
            (unsigned char *)(frame->y_vir_addr) + frame->width *
frame->height;

        inMakeupData.frameWidth = frame->width;
        inMakeupData.frameHeight = frame->height;
        inMakeupData.yBuf = yBuf;
        inMakeupData.uvBuf = uvBuf;

        if (frame->width > 0 && frame->height > 0) {
            int ret_val =
                ts_face_beautify(&inMakeupData, &inMakeupData,
skinCleanLevel,
                                 skinWhitenLevel, &Tsface, 0, yuvFormat);
            if (ret_val != TS_OK) {
                HAL_LOGE("UCAM ts_face_beautify ret is %d", ret_val);
            } else {
                HAL_LOGD("UCAM ts_face_beautify return OK");
            }
        } else {
            HAL_LOGE("No face beauty! frame size If size is not zero, then "
                     "outMakeupData.yBuf is null!");
        }
    } else {
        HAL_LOGD("Not detect face!");
    }
}
#endif
*/
bool SprdCamera3MultiBase::ScaleNV21(uint8_t *a_ucDstBuf, uint16_t a_uwDstWidth,
                                     uint16_t a_uwDstHeight,
                                     uint8_t *a_ucSrcBuf, uint16_t a_uwSrcWidth,
                                     uint16_t a_uwSrcHeight,
                                     uint32_t a_udFileSize) {
    int i, j, x;
    uint16_t *uwHPos, *uwVPos;
    float fStep;
    if (!a_ucSrcBuf || !a_ucDstBuf)
        return false;
    if ((a_uwSrcWidth * a_uwSrcHeight * 1.5) > a_udFileSize)
        return false;

    uwHPos = new uint16_t[a_uwDstWidth];
    uwVPos = new uint16_t[a_uwDstHeight];
    if (!uwHPos || !uwVPos) {
        delete uwHPos;
        delete uwVPos;
        return false;
    }
    // build sampling array
    fStep = (float)a_uwSrcWidth / a_uwDstWidth;
    for (i = 0; i < a_uwDstWidth; i++) {
        uwHPos[i] = i * fStep;
    }
    fStep = (float)a_uwSrcHeight / a_uwDstHeight;
    for (i = 0; i < a_uwDstHeight; i++) {
        uwVPos[i] = i * fStep;
    }
    // Y resize
    for (j = 0; j < a_uwDstHeight; j++) {
        for (i = 0; i < a_uwDstWidth; i++) {
            x = (uwHPos[i]) + uwVPos[j] * a_uwSrcWidth;
            if (x >= 0 && x < (a_uwSrcWidth * a_uwSrcHeight)) {
                // y
                a_ucDstBuf[j * a_uwDstWidth + i] = a_ucSrcBuf[x];
            }
        }
    }
    // Cb/Cr Resize
    a_ucDstBuf = a_ucDstBuf + a_uwDstWidth * a_uwDstHeight;
    a_ucSrcBuf = a_ucSrcBuf + a_uwSrcWidth * a_uwSrcHeight;
    for (j = 0; j < a_uwDstHeight / 2; j++) {
        for (i = 0; i < a_uwDstWidth / 2; i++) {
            x = (uwHPos[i]) + (uwVPos[j] / 2) * a_uwSrcWidth;
            if (x >= 0 && x < (a_uwSrcWidth * a_uwSrcHeight / 2 - 1)) { // cbcr
                a_ucDstBuf[j * a_uwDstWidth + (i * 2) + 0] =
                    a_ucSrcBuf[x * 2 + 0];
                a_ucDstBuf[j * a_uwDstWidth + (i * 2) + 1] =
                    a_ucSrcBuf[x * 2 + 1];
            }
        }
    }
    delete[] uwHPos;
    delete[] uwVPos;
    return true;
}

bool SprdCamera3MultiBase::DepthRotateCCW90(uint16_t *a_uwDstBuf,
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

bool SprdCamera3MultiBase::DepthRotateCCW180(uint16_t *a_uwDstBuf,
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

bool SprdCamera3MultiBase::NV21Rotate90(uint8_t *a_ucDstBuf,
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

bool SprdCamera3MultiBase::NV21Rotate180(uint8_t *a_ucDstBuf,
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

void SprdCamera3MultiBase::setJpegSize(char *jpeg_base, uint32_t max_jpeg_size,
                                       uint32_t jpeg_size) {
    if (jpeg_base == NULL) {
        HAL_LOGE("jpeg_base is NULL");
        return;
    }
    camera3_jpeg_blob *jpegBlob = NULL;
    jpegBlob = (camera3_jpeg_blob *)(jpeg_base + (max_jpeg_size -
                                                  sizeof(camera3_jpeg_blob)));
    jpegBlob->jpeg_size = jpeg_size;
    jpegBlob->jpeg_blob_id = CAMERA3_JPEG_BLOB_ID;
    HAL_LOGI("max_jpeg_size %d, jpeg_size %d", max_jpeg_size, jpeg_size);
}

uint32_t SprdCamera3MultiBase::getJpegSize(uint8_t *jpegBuffer,
                                           uint32_t maxSize) {
    uint32_t size = 0;
    uint8_t *header = jpegBuffer + (maxSize - sizeof(camera3_jpeg_blob));
    camera3_jpeg_blob *blob = (camera3_jpeg_blob *)(header);

    if (blob->jpeg_blob_id == CAMERA3_JPEG_BLOB_ID) {
        size = blob->jpeg_size;
    }
    HAL_LOGI("Jpeg size %d, maxSize %d", size, maxSize);
    return size;
}

int SprdCamera3MultiBase::jpeg_encode_exif_simplify(img_frm *src_img,
                                                    img_frm *pic_enc_img,
                                                    struct img_frm *dst_img,
                                                    SprdCamera3HWI *hwi) {
    HAL_LOGI("E");

    int ret = NO_ERROR;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    struct enc_exif_param encode_exif_param;

    if (hwi == NULL || src_img == NULL || pic_enc_img == NULL) {
        HAL_LOGE("para is NULL");
        return BAD_VALUE;
    }

    memset(&encode_exif_param, 0, sizeof(struct enc_exif_param));

    memcpy(&encode_exif_param.src, src_img, sizeof(struct img_frm));
    memcpy(&encode_exif_param.pic_enc, pic_enc_img, sizeof(struct img_frm));
    if (dst_img != NULL)
        memcpy(&encode_exif_param.last_dst, dst_img, sizeof(struct img_frm));

    ret = hwi->camera_ioctrl(CAMERA_IOCTRL_JPEG_ENCODE_EXIF_PROC,
                             &encode_exif_param, NULL);

    if (ret == NO_ERROR)
        ret = encode_exif_param.stream_real_size;
    else
        ret = UNKNOWN_ERROR;
    property_get("bokeh.dump.encode_exif", prop, "0");
    if (atoi(prop) == 1) {
        unsigned char *vir_jpeg =
            (unsigned char *)(pic_enc_img->addr_vir.addr_y);
        dumpData(vir_jpeg, 2, encode_exif_param.stream_real_size,
                 src_img->size.width, src_img->size.height, 0, "jpegEncode");
    }
    HAL_LOGI("out,ret=%d", ret);
    return ret;
}

int SprdCamera3MultiBase::jpeg_encode_exif_simplify(
    buffer_handle_t *src_private_handle, void *src_vir_addr,
    buffer_handle_t *pic_enc_private_handle, void *pic_vir_addr,
    buffer_handle_t *dst_private_handle, void *dst_vir_addr,
    SprdCamera3HWI *hwi) {
    void *vir_addr = NULL;
    int ret = NO_ERROR;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    struct enc_exif_param encode_exif_param;
    struct img_frm src_img;
    struct img_frm pic_enc_img;
    struct img_frm dst_img;

    HAL_LOGI("src_private_handle :%p, pic_enc_private_handle:%p",
             src_private_handle, pic_enc_private_handle);
    if (hwi == NULL) {
        HAL_LOGE("hwi is NULL");
        return BAD_VALUE;
    }

    memset(&encode_exif_param, 0, sizeof(struct enc_exif_param));
    memset(&src_img, 0, sizeof(struct img_frm));
    memset(&pic_enc_img, 0, sizeof(struct img_frm));
    memset(&dst_img, 0, sizeof(struct img_frm));
    convertToImg_frm(src_private_handle, &src_img, IMG_DATA_TYPE_YUV420,
                     src_vir_addr);

    convertToImg_frm(pic_enc_private_handle, &pic_enc_img, IMG_DATA_TYPE_JPEG,
                     pic_vir_addr);
    if (dst_private_handle != NULL) {
        convertToImg_frm(dst_private_handle, &dst_img, IMG_DATA_TYPE_JPEG,
                         dst_vir_addr);
    }

    memcpy(&encode_exif_param.src, &src_img, sizeof(struct img_frm));
    memcpy(&encode_exif_param.pic_enc, &pic_enc_img, sizeof(struct img_frm));
    memcpy(&encode_exif_param.last_dst, &dst_img, sizeof(struct img_frm));

    ret = hwi->camera_ioctrl(CAMERA_IOCTRL_JPEG_ENCODE_EXIF_PROC,
                             &encode_exif_param, NULL);

    if (ret == NO_ERROR)
        ret = encode_exif_param.stream_real_size;
    else
        ret = UNKNOWN_ERROR;
    property_get("bokeh.dump.encode_exif", prop, "0");
    if (atoi(prop) == 1) {
        unsigned char *vir_jpeg = (unsigned char *)(pic_vir_addr);
        dumpData(vir_jpeg, 2, encode_exif_param.stream_real_size,
                 src_img.size.width, src_img.size.height, 0, "jpegEncode");
    }
    HAL_LOGI("out,ret=%d", ret);
    return ret;
}
};
