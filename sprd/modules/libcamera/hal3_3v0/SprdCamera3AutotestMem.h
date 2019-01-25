#include <MemIon.h>
#include "cmr_preview.h"
#include "cmr_common.h"

#include <camera/Camera.h>
//#include "SprdCamera3OEMIf.h"
#include "SprdCamera3HALHeader.h"
#include "SprdCamera3Setting.h"
#include <utils/Log.h>
#include <utils/String16.h>
#include <utils/Trace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <sprd_ion.h>
#include <media/hardware/MetadataBufferType.h>
#include "cmr_common.h"

using namespace android;

namespace sprdcamera {
#define PREVIEW_BUFF_NUM 8 /*preview buffer*/
#define MAX_SUB_RAWHEAP_NUM 10

typedef struct sprd_camera_memory {
    MemIon *ion_heap;
    cmr_uint phys_addr;
    cmr_uint phys_size;
    cmr_s32 fd;
    cmr_s32 dev_fd;
    void *handle;
    void *data;
    bool busy_flag;
} sprd_camera_memory_t;

static sprd_camera_memory_t **mPreviewHeapArray;

static const int kPreviewBufferCount = 8;
static const int kPreviewRotBufferCount = 8;
static const int kVideoBufferCount = 8;
static const int kVideoRotBufferCount = 8;
static const int kZslBufferCount = 8;
static const int kZslRotBufferCount = 8;
static const int kRefocusBufferCount = 8; // 24;
static const int kPdafRawBufferCount = 8;
static const int kRawBufferCount = 8;
static const int kJpegBufferCount = 8;
static const int kRawFrameHeaderSize = 8;
static const int kISPB4awbCount = 16;

static int32_t mZslNum;
static uint32_t mCapBufIsAvail;
static bool mIommuEnabled;
static uint32_t mPreviewHeapNum;
static uint32_t mVideoHeapNum;
static uint32_t mZslHeapNum;
static uint32_t mRefocusHeapNum;
static uint32_t mPdafRawHeapNum;
static uint32_t mSubRawHeapNum;
static uint32_t mSubRawHeapSize;
static uint32_t mPathRawHeapNum;
static uint32_t mPathRawHeapSize;
static uint32_t mPreviewDcamAllocBufferCnt;
static uint32_t mIspFirmwareReserved_cnt;

// static Mutex mPrevBufLock;
static Mutex mCapBufLock;
static Mutex mZslLock;
static Mutex mZslBufLock;

// static sprd_camera_memory_t *mPreviewHeapArray[PREVIEW_BUFF_NUM];
static sprd_camera_memory_t *mVideoHeapArray[PREVIEW_BUFF_NUM];
static sprd_camera_memory_t *mZslHeapArray[PREVIEW_BUFF_NUM];
static sprd_camera_memory_t *mRefocusHeapArray[PREVIEW_BUFF_NUM];
static sprd_camera_memory_t *mPdafRawHeapArray[PREVIEW_BUFF_NUM];
static sprd_camera_memory_t *mSubRawHeapArray[PREVIEW_BUFF_NUM];
static sprd_camera_memory_t *mPathRawHeapArray[PREVIEW_BUFF_NUM];
static sprd_camera_memory_t *mPreviewHeapReserved[PREVIEW_BUFF_NUM];
static sprd_camera_memory_t *mVideoHeapReserved[PREVIEW_BUFF_NUM];
static sprd_camera_memory_t *mZslHeapReserved[PREVIEW_BUFF_NUM];
static sprd_camera_memory_t *mIspB4awbHeapReserved[kISPB4awbCount];

static sprd_camera_memory_t *mIspAntiFlickerHeapReserved;
static sprd_camera_memory_t *mPdafRawHeapReserved;
static sprd_camera_memory_t *mHighIsoSnapshotHeapReserved;
static sprd_camera_memory_t *mIspPreviewYReserved[2];
static sprd_camera_memory_t *mIspYUVReserved;
static sprd_camera_memory_t *mIspRawDataReserved[ISP_RAWBUF_NUM];
static sprd_camera_memory_t *mDepthHeapReserved;
static sprd_camera_memory_t *mIspLscHeapReserved;
static sprd_camera_memory_t *mIspAFLHeapReserved;
static sprd_camera_memory_t *mIspFirmwareReserved;

class SprdCamera3AutotestMem {
  public:
    int m_previewvalid;
    unsigned char m_camera_id;
    int m_target_buffer_id;
    int m_mem_method;
    // static sprd_camera_memory_t **mPreviewHeapArray;
    Mutex m_previewLock; /*preview lock*/
    Mutex mPrevBufLock;  /*preview lock*/

  public:
    SprdCamera3AutotestMem();
    SprdCamera3AutotestMem(unsigned char camera_id, int target_buffer_id,
                           int s_mem_method,
                           sprd_camera_memory_t **const previewHeapArray);

    virtual ~SprdCamera3AutotestMem();
    int Callback_VideoFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_s32 *fd,
                           cmr_u32 sum);
    int Callback_VideoMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                             cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_PreviewFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                             cmr_s32 *fd, cmr_u32 sum);
    int Callback_PreviewMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_ZslFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_s32 *fd,
                         cmr_u32 sum);
    int Callback_ZslMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                           cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_RefocusFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                             cmr_u32 sum);
    int Callback_RefocusMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_PdafRawFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                             cmr_u32 sum);
    int Callback_PdafRawMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_CaptureFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                             cmr_s32 *fd, cmr_u32 sum);
    int Callback_CaptureMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_CapturePathMalloc(cmr_u32 size, cmr_u32 sum,
                                   cmr_uint *phy_addr, cmr_uint *vir_addr,
                                   cmr_s32 *fd);
    int Callback_CapturePathFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                 cmr_s32 *fd, cmr_u32 sum);
    int Callback_OtherFree(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                           cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum);
    int Callback_OtherMalloc(enum camera_mem_cb_type type, cmr_u32 size,
                             cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                             cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_Free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                      cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum,
                      void *private_data);
    int Callback_Malloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr,
                        cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                        cmr_uint *vir_addr, cmr_s32 *fd, void *private_data);
    void freeCameraMem(sprd_camera_memory_t *camera_memory);
    sprd_camera_memory_t *allocReservedMem(int buf_size, int num_bufs,
                                           uint32_t is_cache);

    sprd_camera_memory_t *allocCameraMem(int buf_size, int num_bufs,
                                         uint32_t is_cache);
};
}
