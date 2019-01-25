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

#include "SprdCamera3MultiBase.h"

using namespace android;
namespace sprdcamera {
#define MAX_UNMATCHED_QUEUE_BASE_SIZE 3

SprdCamera3MultiBase::SprdCamera3MultiBase() {
    mVFps = 0;
    mVFrameCount = 0;
    mVLastFpsTime = 0;
    mVLastFrameCount = 0;
    mIommuEnabled = 0;
}

SprdCamera3MultiBase::~SprdCamera3MultiBase() {}

int SprdCamera3MultiBase::initialize(
    const camera3_callback_ops_t *callback_ops) {
    int rc = 0;

    mVFps = 0;
    mVFrameCount = 0;
    mVLastFpsTime = 0;
    mVLastFrameCount = 0;
    mIommuEnabled = 0;

    return rc;
}

int SprdCamera3MultiBase::allocateOne(int w, int h, uint32_t is_cache,
                                      new_mem_t *new_mem) {
#ifdef CONFIG_GPU_PLATFORM_ROGUE
    int rc = 0;
    uint32_t stride = 0;
    uint32_t usage = 0;
    int format = 0;
#ifdef CONFIG_CAMERA_DCAM_SUPPORT_FORMAT_NV12
    format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
#else
    format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
#endif
    if (!mIommuEnabled) {
        usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_CAMERA_BUFFER;
    } else {
        usage = GRALLOC_USAGE_SW_READ_OFTEN;
    }
    GraphicBufferAllocator::get().alloc(
        w, h, format, usage, (buffer_handle_t *)(&new_mem->native_handle),
        &stride);
    if (new_mem->native_handle == NULL) {
        ALOGE("cannot alloc buffer");
        return -1;
    }
    HAL_LOGD("fd:%d", ADP_BUFFD((buffer_handle_t)(new_mem->native_handle)));
    HAL_LOGD("width:%d", ADP_WIDTH((buffer_handle_t)(new_mem->native_handle)));
    HAL_LOGD("height:%d",
             ADP_HEIGHT((buffer_handle_t)(new_mem->native_handle)));
    HAL_LOGD("addr:%p",
             getBufferAddr((buffer_handle_t *)&(new_mem->native_handle)));
    return rc;
#else
    int result = 0;
    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;
    private_handle_t *buffer;

    HAL_LOGI("E");
    mem_size = w * h * 3 / 2;
    // to make it page size aligned
    //	mem_size = (mem_size + 4095U) & (~4095U);

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

    HAL_LOGI("X");

    return result;

getpmem_fail:
    delete pHeapIon;

    return -1;
#endif
}

void SprdCamera3MultiBase::freeOneBuffer(new_mem_t *LocalBuffer) {
    HAL_LOGD("free local buffer");

    if (LocalBuffer != NULL) {
        if (LocalBuffer->native_handle != NULL) {
#ifdef CONFIG_GPU_PLATFORM_ROGUE
            GraphicBufferAllocator::get().free(
                (buffer_handle_t)(LocalBuffer->native_handle));

#else
            delete ((private_handle_t *)*(&(LocalBuffer->native_handle)));
#endif
            LocalBuffer->native_handle = NULL;
        }
        if (LocalBuffer->pHeapIon != NULL) {
            delete LocalBuffer->pHeapIon;
            LocalBuffer->pHeapIon = NULL;
        }
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

int SprdCamera3MultiBase::getStreamType(camera3_stream_t *new_stream) {
    int stream_type = 0;
    int format = new_stream->format;
    if (new_stream->stream_type == CAMERA3_STREAM_OUTPUT) {
        if (format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
            format = HAL_PIXEL_FORMAT_YCrCb_420_SP;

        switch (format) {
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
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

void *SprdCamera3MultiBase::getBufferAddr(buffer_handle_t *buffer) {
    void *vaddr = NULL;
    int ret = NO_ERROR;
#ifdef CONFIG_GPU_PLATFORM_ROGUE
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    int width = ADP_WIDTH(*buffer);
    int height = ADP_HEIGHT(*buffer);
    Rect bounds(width, height);
    int usage;

    usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
    ret = mapper.lock((const native_handle_t *)*buffer, usage, bounds, &vaddr);
    if (ret != NO_ERROR) {
        HAL_LOGE("onQueueFilled, mapper.lock fail %p, ret %d", *buffer, ret);
    }
    mapper.unlock((const native_handle_t *)*buffer);
#else
    struct private_handle_t *private_handle = NULL;
    private_handle = (struct private_handle_t *)(*buffer);
    if (NULL == private_handle) {
        HAL_LOGE("NULL buffer handle!");
    }
    vaddr = (void *)private_handle->base;
#endif
    return vaddr;
}

int SprdCamera3MultiBase::getBufferSize(buffer_handle_t *buffer) {
    int size;
#ifdef CONFIG_GPU_PLATFORM_ROGUE
    size = ADP_BUFSIZE(*buffer);
#else
    size = ((struct private_handle_t *)*(buffer))->size;
#endif
    return size;
}

void SprdCamera3MultiBase::dumpImg(void *addr, int size, int frameId,
                                   int flag) {

    HAL_LOGI(" E");
    char name[128];
    int count;
    snprintf(name, sizeof(name), "/data/misc/cameraserver/%d_%d_%d.yuv", size,
             frameId, flag);

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

void SprdCamera3MultiBase::dumpFps() {
    mVFrameCount++;
    int64_t now = systemTime();
    int64_t diff = now - mVLastFpsTime;
    if (diff > ms2ns(250)) {
        mVFps =
            (((double)(mVFrameCount - mVLastFrameCount)) * (double)(s2ns(1))) /
            (double)diff;
        HAL_LOGD("[KPI Perf]:Fps: %.4f ", mVFps);
        mVLastFpsTime = now;
        mVLastFrameCount = mVFrameCount;
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
            if (abs((cmr_s32)diff) < (15e6)) {
                *result2 = *itor2;
                list.erase(itor2);
                /*HAL_LOGV("[%d:match:%d],diff=%llu T1:%llu,T2:%llu",
                         result1.frame_number, itor2->frame_number, diff,
                         result1.timestamp, itor2->timestamp);*/
                return MATCH_SUCCESS;
            }
            itor2++;
        }
        HAL_LOGV("match failed for idx:%d, could not find matched frame",
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
/*
#ifdef CONFIG_FACE_BEAUTY
void SprdCamera3MultiBase::convert_face_info(int *ptr_cam_face_inf, int width,
                                             int height) {}

void SprdCamera3MultiBase::doFaceMakeup(struct camera_frame_type *frame,
                                        int perfect_level, int *face_info) {

    // init the parameters table. save the value until the process is restart or
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
            HAL_LOGE("No face beauty! frame size If size is not zero, then "
                     "outMakeupData.yBuf is null!");
        }
    } else {
        HAL_LOGD("Not detect face!");
    }
}
#endif
*/
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
};
