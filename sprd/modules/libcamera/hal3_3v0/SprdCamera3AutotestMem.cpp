
#define LOG_TAG "SprdCamera3AutotestMem"

#include "SprdCamera3AutotestMem.h"

using namespace android;

namespace sprdcamera {

SprdCamera3AutotestMem::SprdCamera3AutotestMem() {

    // return 0;
}

SprdCamera3AutotestMem::SprdCamera3AutotestMem(
    unsigned char camera_id, int target_buffer_id, int s_mem_method,
    sprd_camera_memory_t **const previewHeapArray) {

    m_mem_method = s_mem_method;
    m_target_buffer_id = target_buffer_id;
    mPreviewHeapArray = previewHeapArray;

    // return 0;
}

SprdCamera3AutotestMem::~SprdCamera3AutotestMem() {
    // ALOGD("mVideoHeapNum %d sum %d", mVideoHeapNum, sum);

    // return 0;
}

int SprdCamera3AutotestMem::Callback_VideoFree(cmr_uint *phy_addr,
                                               cmr_uint *vir_addr, cmr_s32 *fd,
                                               cmr_u32 sum) {
    cmr_u32 i;
    // Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGD("mVideoHeapNum %d sum %d", mVideoHeapNum, sum);
    for (i = 0; i < mVideoHeapNum; i++) {
        if (NULL != mVideoHeapArray[i]) {
            freeCameraMem(mVideoHeapArray[i]);
        }
        mVideoHeapArray[i] = NULL;
    }

    mVideoHeapNum = 0;

    return 0;
}

int SprdCamera3AutotestMem::Callback_VideoMalloc(cmr_u32 size, cmr_u32 sum,
                                                 cmr_uint *phy_addr,
                                                 cmr_uint *vir_addr,
                                                 cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i = 0;
    //  SPRD_DEF_Tag sprddefInfo;
    cmr_u32 BufferCount = (cmr_u32)kVideoBufferCount;

    // mSetting->getSPRDDEFTag(&sprddefInfo);
    // if (sprddefInfo.slowmotion <= 1)
    //    BufferCount = 8;

    HAL_LOGD("size %d sum %d mVideoHeapNum %d", size, sum, mVideoHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    for (i = BufferCount; i < (cmr_u32)sum; i++) {
        memory = allocCameraMem(size, 1, true);
        if (NULL == memory) {
            HAL_LOGE("error memory is null.");
            goto mem_fail;
        }

        mVideoHeapArray[mVideoHeapNum] = memory;
        mVideoHeapNum++;
        *phy_addr++ = (cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
    }

    return 0;

mem_fail:
    Callback_VideoFree(0, 0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3AutotestMem::Callback_ZslFree(cmr_uint *phy_addr,
                                             cmr_uint *vir_addr, cmr_s32 *fd,
                                             cmr_u32 sum) {
    cmr_u32 i;
    // Mutex::Autolock l(&mPrevBufLock);
    Mutex::Autolock zsllock(&mZslBufLock);

    HAL_LOGD("mZslHeapNum %d sum %d", mZslHeapNum, sum);
    for (i = 0; i < mZslHeapNum; i++) {
        if (NULL != mZslHeapArray[i]) {
            freeCameraMem(mZslHeapArray[i]);
        }
        mZslHeapArray[i] = NULL;
    }

    mZslHeapNum = 0;
    //    releaseZSLQueue();

    return 0;
}

int SprdCamera3AutotestMem::Callback_ZslMalloc(cmr_u32 size, cmr_u32 sum,
                                               cmr_uint *phy_addr,
                                               cmr_uint *vir_addr,
                                               cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i = 0;

    //    SPRD_DEF_Tag sprddefInfo;
    cmr_u32 BufferCount = (cmr_u32)kZslBufferCount;

    // mSetting->getSPRDDEFTag(&sprddefInfo);
    // if (sprddefInfo.slowmotion <= 1)
    //    BufferCount = 8;

    ALOGD("size %d sum %d mZslHeapNum %d, BufferCount %d", size, sum,
          mZslHeapNum, BufferCount);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    for (i = 0; i < (cmr_u32)mZslNum; i++) {
        if (mZslHeapArray[i] == NULL) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("error memory is null.");
                goto mem_fail;
            }
            mZslHeapArray[i] = memory;
            mZslHeapNum++;
        }
        *phy_addr++ = (cmr_uint)mZslHeapArray[i]->phys_addr;
        *vir_addr++ = (cmr_uint)mZslHeapArray[i]->data;
        *fd++ = mZslHeapArray[i]->fd;
    }

    if (sum > BufferCount) {
        mZslHeapNum = BufferCount;
        phy_addr += BufferCount;
        vir_addr += BufferCount;
        fd += BufferCount;
        for (i = BufferCount; i < (cmr_u32)sum; i++) {
            memory = allocCameraMem(size, 1, true);

            if (NULL == memory) {
                HAL_LOGE("error memory is null.");
                goto mem_fail;
            }

            mZslHeapArray[mZslHeapNum] = memory;
            mZslHeapNum++;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else {
        HAL_LOGD("Do not need malloc, malloced num %d,request num %d, request "
                 "size 0x%x",
                 mZslHeapNum, sum, size);
        goto mem_fail;
    }

    return 0;

mem_fail:
    Callback_ZslFree(0, 0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3AutotestMem::Callback_RefocusFree(cmr_uint *phy_addr,
                                                 cmr_uint *vir_addr,
                                                 cmr_u32 sum) {
    cmr_u32 i;
    // Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGD("mRefocusHeapNum %d sum %d", mRefocusHeapNum, sum);

    for (i = 0; i < mRefocusHeapNum; i++) {
        if (NULL != mRefocusHeapArray[i]) {
            freeCameraMem(mRefocusHeapArray[i]);
        }
        mRefocusHeapArray[i] = NULL;
    }
    mRefocusHeapNum = 0;

    return 0;
}

int SprdCamera3AutotestMem::Callback_RefocusMalloc(cmr_u32 size, cmr_u32 sum,
                                                   cmr_uint *phy_addr,
                                                   cmr_uint *vir_addr,
                                                   cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    HAL_LOGD("size %d sum %d mRefocusHeapNum %d", size, sum, mRefocusHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;

    for (i = mRefocusHeapNum; i < (cmr_int)sum; i++) {
        memory = allocCameraMem(size, 1, true);

        if (NULL == memory) {
            HAL_LOGE("error memory is null.");
            goto mem_fail;
        }

        mRefocusHeapArray[mRefocusHeapNum] = memory;
        mRefocusHeapNum++;
        *phy_addr++ = 0; //(cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        if (NULL != fd)
            *fd++ = (cmr_s32)memory->fd;
    }

    return 0;

mem_fail:
    Callback_RefocusFree(0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3AutotestMem::Callback_PdafRawFree(cmr_uint *phy_addr,
                                                 cmr_uint *vir_addr,
                                                 cmr_u32 sum) {
    cmr_u32 i;
    // Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGD("mPdafRawHeapNum %d sum %d", mPdafRawHeapNum, sum);

    for (i = 0; i < mPdafRawHeapNum; i++) {
        if (NULL != mPdafRawHeapArray[i]) {
            freeCameraMem(mPdafRawHeapArray[i]);
        }
        mPdafRawHeapArray[i] = NULL;
    }
    mPdafRawHeapNum = 0;

    return 0;
}

int SprdCamera3AutotestMem::Callback_PdafRawMalloc(cmr_u32 size, cmr_u32 sum,
                                                   cmr_uint *phy_addr,
                                                   cmr_uint *vir_addr,
                                                   cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    ALOGD("size %d sum %d mPdafRawHeapNum %d", size, sum, mPdafRawHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;

    for (i = mPdafRawHeapNum; i < (cmr_int)sum; i++) {
        memory = allocCameraMem(size, 1, true);

        if (NULL == memory) {
            HAL_LOGE("error memory is null.");
            goto mem_fail;
        }

        mPdafRawHeapArray[mPdafRawHeapNum] = memory;
        mPdafRawHeapNum++;
        *phy_addr++ = 0; //(cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        if (NULL != fd)
            *fd++ = (cmr_s32)memory->fd;
    }

    return 0;

mem_fail:
    Callback_PdafRawFree(0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3AutotestMem::Callback_PreviewFree(cmr_uint *phy_addr,
                                                 cmr_uint *vir_addr,
                                                 cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;
    // Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGI("mPreviewHeapNum %d sum %d", mPreviewHeapNum, sum);

    for (i = 0; i < mPreviewHeapNum; i++) {
        if (NULL != mPreviewHeapArray[i]) {
            freeCameraMem(mPreviewHeapArray[i]);
        }
        mPreviewHeapArray[i] = NULL;
    }
    mPreviewHeapNum = 0;

    return 0;
}

int SprdCamera3AutotestMem::Callback_PreviewMalloc(cmr_u32 size, cmr_u32 sum,
                                                   cmr_uint *phy_addr,
                                                   cmr_uint *vir_addr,
                                                   cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i = 0;
    //    SPRD_DEF_Tag sprddefInfo;
    cmr_u32 BufferCount = (cmr_u32)kPreviewBufferCount;

    ALOGD("size %d sum %d mPreviewHeapNum %d, BufferCount %d", size, sum,
          mPreviewHeapNum, BufferCount);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        memory = allocCameraMem(size, 1, true);

        if (NULL == memory) {
            HAL_LOGE("error memory is null.");
            goto mem_fail;
        }

        // mPreviewHeapArray[mPreviewHeapNum] = memory;
        mPreviewHeapArray[mPreviewHeapNum] = memory;

        mPreviewHeapNum++;
        *phy_addr++ = (cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
    }

    return 0;

mem_fail:
    Callback_PreviewFree(0, 0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3AutotestMem::Callback_CaptureFree(cmr_uint *phy_addr,
                                                 cmr_uint *vir_addr,
                                                 cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;
    HAL_LOGD("mSubRawHeapNum %d sum %d", mSubRawHeapNum, sum);
    Mutex::Autolock l(&mCapBufLock);

    for (i = 0; i < mSubRawHeapNum; i++) {
        if (NULL != mSubRawHeapArray[i]) {
            freeCameraMem(mSubRawHeapArray[i]);
        }
        mSubRawHeapArray[i] = NULL;
    }
    mSubRawHeapNum = 0;
    mSubRawHeapSize = 0;
    mCapBufIsAvail = 0;

    return 0;
}

int SprdCamera3AutotestMem::Callback_CaptureMalloc(cmr_u32 size, cmr_u32 sum,
                                                   cmr_uint *phy_addr,
                                                   cmr_uint *vir_addr,
                                                   cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    HAL_LOGD("size %d sum %d mSubRawHeapNum %d", size, sum, mSubRawHeapNum);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    mCapBufLock.lock();

    if (0 == mSubRawHeapNum) {
        for (i = 0; i < (cmr_int)sum; i++) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("error memory is null.");
                goto mem_fail;
            }

            mSubRawHeapArray[mSubRawHeapNum] = memory;
            mSubRawHeapNum++;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
        mSubRawHeapSize = size;
    } else {
        if ((mSubRawHeapNum >= sum) && (mSubRawHeapSize >= size)) {
            HAL_LOGD("use pre-alloc cap mem");
            for (i = 0; i < (cmr_int)sum; i++) {
                *phy_addr++ = (cmr_uint)mSubRawHeapArray[i]->phys_addr;
                *vir_addr++ = (cmr_uint)mSubRawHeapArray[i]->data;
                *fd++ = mSubRawHeapArray[i]->fd;
            }
        } else {
            mCapBufLock.unlock();
            Callback_CaptureFree(0, 0, 0, 0);
        }
    }

    mCapBufIsAvail = 1;
    mCapBufLock.unlock();
    return 0;

mem_fail:
    mCapBufLock.unlock();
    Callback_CaptureFree(0, 0, 0, 0);
    return -1;
}

int SprdCamera3AutotestMem::Callback_CapturePathMalloc(cmr_u32 size,
                                                       cmr_u32 sum,
                                                       cmr_uint *phy_addr,
                                                       cmr_uint *vir_addr,
                                                       cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    HAL_LOGI("Callback_CapturePathMalloc: size %d sum %d mPathRawHeapNum %d "
             "mPathRawHeapSize %d",
             size, sum, mPathRawHeapNum, mPathRawHeapSize);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (0 == mPathRawHeapNum) {
        mPathRawHeapSize = size;
        for (i = 0; i < (cmr_int)sum; i++) {
            memory = allocCameraMem(size, 1, true);

            if (NULL == memory) {
                HAL_LOGE("Callback_CapturePathMalloc: error memory is null.");
                goto mem_fail;
            }

            mPathRawHeapArray[mPathRawHeapNum] = memory;
            mPathRawHeapNum++;

            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else {
        if ((mPathRawHeapNum >= sum) && (mPathRawHeapSize >= size)) {
            HAL_LOGD("Callback_CapturePathMalloc :test");
            for (i = 0; i < (cmr_int)sum; i++) {
                *phy_addr++ = (cmr_uint)mPathRawHeapArray[i]->phys_addr;
                *vir_addr++ = (cmr_uint)mPathRawHeapArray[i]->data;
                *fd++ = mPathRawHeapArray[i]->fd;
            }
        } else {
            HAL_LOGE("failed to malloc memory, malloced num %d,request num %d, "
                     "size 0x%x, request size 0x%x",
                     mPathRawHeapNum, sum, mPathRawHeapSize, size);
            goto mem_fail;
        }
    }
    return 0;

mem_fail:
    Callback_CapturePathFree(0, 0, 0, 0);
    return -1;
}

int SprdCamera3AutotestMem::Callback_CapturePathFree(cmr_uint *phy_addr,
                                                     cmr_uint *vir_addr,
                                                     cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;

    ALOGI("Callback_CapturePathFree: mPathRawHeapNum %d sum %d",
          mPathRawHeapNum, sum);

    for (i = 0; i < mPathRawHeapNum; i++) {
        if (NULL != mPathRawHeapArray[i]) {
            freeCameraMem(mPathRawHeapArray[i]);
        }
        mPathRawHeapArray[i] = NULL;
    }
    mPathRawHeapNum = 0;
    mPathRawHeapSize = 0;

    return 0;
}

int SprdCamera3AutotestMem::Callback_OtherFree(enum camera_mem_cb_type type,
                                               cmr_uint *phy_addr,
                                               cmr_uint *vir_addr, cmr_s32 *fd,
                                               cmr_u32 sum) {
    cmr_u32 i;
    // Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGD("sum %d", sum);

    if (type == CAMERA_PREVIEW_RESERVED) {
        for (i = 0; i < PREV_RESERVED_FRM_CNT; i++) {
            if (NULL != mPreviewHeapReserved[i]) {
                freeCameraMem(mPreviewHeapReserved[i]);
                mPreviewHeapReserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_VIDEO_RESERVED) {
        for (i = 0; i < VIDEO_RESERVED_FRM_CNT; i++) {
            if (NULL != mVideoHeapReserved[i]) {
                freeCameraMem(mVideoHeapReserved[i]);
                mVideoHeapReserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_SNAPSHOT_ZSL_RESERVED) {
        for (i = 0; i < CAP_ZSL_RESERVED_FRM_CNT; i++) {
            if (NULL != mZslHeapReserved[i]) {
                freeCameraMem(mZslHeapReserved[i]);
                mZslHeapReserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_ISP_LSC) {
        if (NULL != mIspLscHeapReserved) {
            freeCameraMem(mIspLscHeapReserved);
        }
        mIspLscHeapReserved = NULL;
    }

    if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (NULL != mIspAntiFlickerHeapReserved) {
            freeCameraMem(mIspAntiFlickerHeapReserved);
        }
        mIspAntiFlickerHeapReserved = NULL;
    }

    if (type == CAMERA_PDAF_RAW_RESERVED) {
        if (NULL != mPdafRawHeapReserved) {
            freeCameraMem(mPdafRawHeapReserved);
        }
        mPdafRawHeapReserved = NULL;
    }

    if (type == CAMERA_ISP_BINGING4AWB) {
        for (i = 0; i < kISPB4awbCount; i++) {
            if (NULL != mIspB4awbHeapReserved[i]) {
                freeCameraMem(mIspB4awbHeapReserved[i]);
            }
            mIspB4awbHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_FIRMWARE) {
        if (NULL != mIspFirmwareReserved && !(--mIspFirmwareReserved_cnt)) {
            freeCameraMem(mIspFirmwareReserved);
            mIspFirmwareReserved = NULL;
        }
    }

    if (type == CAMERA_SNAPSHOT_HIGHISO) {
        if (NULL != mHighIsoSnapshotHeapReserved) {
            freeCameraMem(mHighIsoSnapshotHeapReserved);
        }
        mHighIsoSnapshotHeapReserved = NULL;
    }

    if (type == CAMERA_ISP_PREVIEW_Y) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspPreviewYReserved[i]) {
                freeCameraMem(mIspPreviewYReserved[i]);
            }
            mIspPreviewYReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_PREVIEW_YUV) {
        if (NULL != mIspYUVReserved) {
            freeCameraMem(mIspYUVReserved);
        }
        mIspYUVReserved = NULL;
    }

    if (type == CAMERA_ISP_RAW_DATA) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspRawDataReserved[i]) {
                freeCameraMem(mIspRawDataReserved[i]);
            }
            mIspRawDataReserved[i] = NULL;
        }
    }
    /* disable preview flag */
    m_previewvalid = 0;

    return 0;
}

int SprdCamera3AutotestMem::Callback_OtherMalloc(enum camera_mem_cb_type type,
                                                 cmr_u32 size, cmr_u32 *sum_ptr,
                                                 cmr_uint *phy_addr,
                                                 cmr_uint *vir_addr,
                                                 cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i;
    cmr_u32 mem_size;
    cmr_u32 mem_sum;
    int buffer_id;

    cmr_u32 sum = *sum_ptr;

    HAL_LOGD("size=%d, sum=%d, mem_type=%d", size, sum, type);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;
    if (type == CAMERA_PREVIEW_RESERVED) {
        for (i = 0; i < PREV_RESERVED_FRM_CNT; i++) {
            if (mPreviewHeapReserved[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("memory is null.");
                    goto mem_fail;
                }
                mPreviewHeapReserved[i] = memory;
            }
            *phy_addr++ = (cmr_uint)mPreviewHeapReserved[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mPreviewHeapReserved[i]->data;
            *fd++ = mPreviewHeapReserved[i]->fd;
        }
    } else if (type == CAMERA_VIDEO_RESERVED) {
        for (i = 0; i < VIDEO_RESERVED_FRM_CNT; i++) {
            if (mVideoHeapReserved[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("memory is null.");
                    goto mem_fail;
                }
                mVideoHeapReserved[i] = memory;
            }
            *phy_addr++ = (cmr_uint)mVideoHeapReserved[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mVideoHeapReserved[i]->data;
            *fd++ = mVideoHeapReserved[i]->fd;
        }
    } else if (type == CAMERA_SNAPSHOT_ZSL_RESERVED) {
        for (i = 0; i < CAP_ZSL_RESERVED_FRM_CNT; i++) {
            if (mZslHeapReserved[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("memory is null.");
                    goto mem_fail;
                }
                mZslHeapReserved[i] = memory;
            }
            *phy_addr++ = (cmr_uint)mZslHeapReserved[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mZslHeapReserved[i]->data;
            *fd++ = mZslHeapReserved[i]->fd;
        }
    } else if (type == CAMERA_SENSOR_DATATYPE_MAP_RESERVED) {
        if (mDepthHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mDepthHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mDepthHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mDepthHeapReserved->data;
        *fd++ = mDepthHeapReserved->fd;
    } else if (type == CAMERA_PDAF_RAW_RESERVED) {
        if (mPdafRawHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mPdafRawHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mPdafRawHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mPdafRawHeapReserved->data;
        *fd++ = mPdafRawHeapReserved->fd;
    } else if (type == CAMERA_ISP_LSC) {
        if (mIspLscHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("memory is null");
                goto mem_fail;
            }
            mIspLscHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mIspLscHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mIspLscHeapReserved->data;
        *fd++ = mIspLscHeapReserved->fd;
    } else if (type == CAMERA_ISP_BINGING4AWB) {
        cmr_u64 *phy_addr_64 = (cmr_u64 *)phy_addr;
        cmr_u64 *vir_addr_64 = (cmr_u64 *)vir_addr;
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        for (i = 0; i < sum; i++) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspB4awbHeapReserved[i] = memory;
            *phy_addr_64++ = (cmr_u64)memory->phys_addr;
            *vir_addr_64++ = (cmr_u64)memory->data;
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
            *phy_addr++ = kaddr;
            *phy_addr = kaddr >> 32;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (mIspAntiFlickerHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspAntiFlickerHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mIspAntiFlickerHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mIspAntiFlickerHeapReserved->data;
        *fd++ = mIspAntiFlickerHeapReserved->fd;
    } else if (type == CAMERA_ISP_FIRMWARE) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        if (++mIspFirmwareReserved_cnt == 1) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspFirmwareReserved = memory;
        } else {
            memory = mIspFirmwareReserved;
        }
        if (memory->ion_heap)
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
        *phy_addr++ = kaddr;
        *phy_addr++ = kaddr >> 32;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
        *fd++ = memory->dev_fd;
    } else if (type == CAMERA_SNAPSHOT_HIGHISO) {
        if (mHighIsoSnapshotHeapReserved == NULL) {
            memory = allocReservedMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mHighIsoSnapshotHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mHighIsoSnapshotHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mHighIsoSnapshotHeapReserved->data;
        *fd++ = mHighIsoSnapshotHeapReserved->fd;
    } else if (type == CAMERA_ISP_PREVIEW_Y) {
        for (i = 0; i < sum; i++) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspPreviewYReserved[i] = memory;
            *phy_addr++ = 0;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_PREVIEW_YUV) {
        if (mIspYUVReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mIspYUVReserved = memory;
        }
        *phy_addr++ = 0;
        *vir_addr++ = (cmr_uint)mIspYUVReserved->data;
        *fd++ = mIspYUVReserved->fd;
    } else if (type == CAMERA_ISP_RAW_DATA) {
        cmr_u64 *kaddr = (cmr_u64 *)vir_addr;
        size_t ksize = 0;

        for (i = 0; i < sum; i++) {
            if (mIspRawDataReserved[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("memory is null.");
                    break;
                }
                mIspRawDataReserved[i] = memory;
            }
            //			*phy_addr++ = 0;
            *phy_addr++ = (cmr_uint)mIspRawDataReserved[i]->data;
            mIspRawDataReserved[i]->ion_heap->get_kaddr(kaddr, &ksize);
            kaddr++;
            *fd++ = mIspRawDataReserved[i]->fd;
            HAL_LOGD("isp raw data fd=0x%0x, vir_addr=%p",
                     mIspRawDataReserved[i]->fd, mIspRawDataReserved[i]->data);
        }
        *sum_ptr = i;
    }

    return 0;

mem_fail:
    Callback_OtherFree(type, 0, 0, 0, 0);
    return BAD_VALUE;
}

sprd_camera_memory_t *
SprdCamera3AutotestMem::allocReservedMem(int buf_size, int num_bufs,
                                         uint32_t is_cache) {
    ATRACE_CALL();

    unsigned long paddr = 0;
    size_t psize = 0;
    int result = 0;
    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;

    HAL_LOGD("buf_size %d, num_bufs %d", buf_size, num_bufs);

    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        HAL_LOGE("fatal error! memory pointer is null.");
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        goto getpmem_fail;
    }

    if (is_cache) {
        pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                              (1 << 31) | ION_HEAP_ID_MASK_CAM);
    } else {
        pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                              ION_HEAP_ID_MASK_CAM);
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        HAL_LOGE("pHeapIon is null or getHeapID failed");
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
        HAL_LOGE("error getBase is null.");
        goto getpmem_fail;
    }

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    // memory->phys_addr is offset from memory->fd, always set 0 for yaddr
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    HAL_LOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
             memory->fd, memory->phys_addr, memory->data, memory->phys_size,
             pHeapIon);

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }
    return NULL;
}

sprd_camera_memory_t *
SprdCamera3AutotestMem::allocCameraMem(int buf_size, int num_bufs,
                                       uint32_t is_cache) {
    ATRACE_CALL();

    unsigned long paddr = 0;
    size_t psize = 0;
    int result = 0;
    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;

    ALOGD("buf_size %d, num_bufs %d", buf_size, num_bufs);
    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        ALOGE("fatal error! memory pointer is null.");
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        goto getpmem_fail;
    }

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
        ALOGE("pHeapIon is null or getHeapID failed");
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
        ALOGE("error getBase is null.");
        goto getpmem_fail;
    }

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    memory->dev_fd = pHeapIon->getIonDeviceFd();
    // memory->phys_addr is offset from memory->fd, always set 0 for yaddr
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    ALOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
          memory->fd, memory->phys_addr, memory->data, memory->phys_size,
          pHeapIon);

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }
    return NULL;
}

void SprdCamera3AutotestMem::freeCameraMem(sprd_camera_memory_t *memory) {
    if (memory) {
        if (memory->ion_heap) {
            ALOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
                  memory->fd, memory->phys_addr, memory->data,
                  memory->phys_size, memory->ion_heap);
            delete memory->ion_heap;
            memory->ion_heap = NULL;
        } else {
            ALOGD("memory->ion_heap is null:fd=0x%x, phys_addr=0x%lx, "
                  "virt_addr=%p, size=0x%lx",
                  memory->fd, memory->phys_addr, memory->data,
                  memory->phys_size);
        }
        free(memory);
        memory = NULL;
    }
}
}
