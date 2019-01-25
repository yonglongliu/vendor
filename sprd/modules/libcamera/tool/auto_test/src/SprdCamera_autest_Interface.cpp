#include <utils/Log.h>
#if defined (CONFIG_CAMERA_ISP_DIR_2_1)
#if defined (CONFIG_CAMERA_ISP_DIR_2_1_A)
#include "hal3_2v1a/SprdCamera3OEMIf.h"
#include "hal3_2v1a/SprdCamera3Setting.h"
#else
#include "hal3_2v1/SprdCamera3OEMIf.h"
#include "hal3_2v1/SprdCamera3Setting.h"
#endif
#endif
#if defined (CONFIG_CAMERA_ISP_DIR_2_4)
#include "hal3_2v4/SprdCamera3OEMIf.h"
#include "hal3_2v4/SprdCamera3Setting.h"
#endif
#if defined (CONFIG_CAMERA_ISP_DIR_3)
#include "hal3_3v0/SprdCamera3AutotestMem.h"
#include "hal3_3v0/SprdCamera3Setting.h"
#endif
#include <utils/String16.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <media/hardware/MetadataBufferType.h>
#include "cmr_common.h"
#include "sprd_ion.h"

//#include "graphics.h"
#include <linux/fb.h>
//#include "sprd_rot_k.h"
#include "sprd_cpp.h"
using namespace sprdcamera;

#include <dlfcn.h>

typedef enum enumYUVFormat {
    FMT_NV21 = 0,
    FMT_NV12,
} YUVFormat;

typedef struct {
    int width;
    int height;
    int row_bytes;
    int pixel_bytes;
    unsigned char *data;
} GRSurface;

typedef GRSurface *gr_surface;

typedef struct minui_backend {
    // Initializes the backend and returns a gr_surface to draw into.
    gr_surface (*init)(struct minui_backend *);

    // Causes the current drawing surface (returned by the most recent
    // call to flip() or init()) to be displayed, and returns a new
    // drawing surface.
    gr_surface (*flip)(struct minui_backend *);

    // Blank (or unblank) the screen.
    void (*blank)(struct minui_backend *, bool);

    // Device cleanup when drawing is done.
    void (*exit)(struct minui_backend *);
} minui_backend;

static minui_backend *gr_backend = NULL;
static GRSurface *gr_draw = NULL;

/*yuv->rgb*/
#define RGB565(r, g, b)                                                        \
    ((unsigned short)((((unsigned char)(r) >> 3) |                             \
                       ((unsigned short)(((unsigned char)(g) >> 2)) << 5)) |   \
                      (((unsigned short)((unsigned char)(b >> 3))) << 11)))

/*rotation device nod*/
static int rot_fd = -1;
#define ROT_DEV "/dev/sprd_rotation"

/*process control*/
static Mutex previewLock;           /*preview lock*/
static int previewvalid = 0;        /*preview flag*/
static int s_mem_method = 0;        /*0: physical address, 1: iommu  address*/
static unsigned char camera_id = 0; /*camera id: fore=1,back=0*/

/*data processing useful*/
#define PREVIEW_WIDTH 960  // 1600//1280//640
#define PREVIEW_HIGHT 720  // 1200//960//480
#define PREVIEW_BUFF_NUM 8 /*preview buffer*/
#define SPRD_MAX_PREVIEW_BUF PREVIEW_BUFF_NUM
struct frame_buffer_t {
    cmr_uint phys_addr;
    cmr_uint virt_addr;
    cmr_uint length; // buffer's length is different from cap_image_size
};
static struct frame_buffer_t fb_buf[SPRD_MAX_PREVIEW_BUF + 1];
static uint8_t *tmpbuf2, *tmpbuf3; //*tmpbuf1, *tmpbuf,
static uint32_t post_preview_buf[PREVIEW_WIDTH * PREVIEW_HIGHT];
static struct fb_var_screeninfo var;
static uint32_t frame_num = 0; /*record frame number*/

#if defined(CONFIG_CAMERA_ISP_DIR_3)
static SprdCamera3AutotestMem *AutotestMem;
#else

static unsigned int mPreviewHeapNum = 0; /*allocated preview buffer number*/
static sprd_camera_memory_t *mPreviewHeapReserved = NULL;
static sprd_camera_memory_t *mIspLscHeapReserved = NULL;
static sprd_camera_memory_t *mIspStatisHeapReserved = NULL;
static sprd_camera_memory_t *mIspAFLHeapReserved = NULL;
static sprd_camera_memory_t *mIspFirmwareReserved = NULL;
static const int kISPB4awbCount = 16;
static sprd_camera_memory_t *mIspB4awbHeapReserved[kISPB4awbCount];
static sprd_camera_memory_t *mIspRawAemHeapReserved[kISPB4awbCount];
#endif
static sprd_camera_memory_t
    *previewHeapArray[PREVIEW_BUFF_NUM]; /*preview heap arrary*/
static int target_buffer_id;

static oem_module_t *mHalOem;

struct client_t {
    int reserved;
};
static struct client_t client_data;
static cmr_handle oem_handle = 0;

static uint32_t lcd_w = 0, lcd_h = 0;

int autotest_IommuIsEnabled(void) {
    int ret;
    int iommuIsEnabled = 0;

    if (NULL == oem_handle || NULL == mHalOem || NULL == mHalOem->ops) {
        ALOGE("AutoTest: %s,%d, oem is null or oem ops is null.\n", __func__,
              __LINE__);
        return 0;
    }

    ret = mHalOem->ops->camera_get_iommu_status(oem_handle);
    if (ret) {
        iommuIsEnabled = 0;
        return iommuIsEnabled;
    }

    iommuIsEnabled = 1;
    return iommuIsEnabled;
}

static bool getLcdSize(uint32_t *width, uint32_t *height) {
    if (NULL != gr_draw) {
        *width = (uint32_t)gr_draw->width;
        *height = (uint32_t)gr_draw->height;
    }
    return true;
}
static void RGBRotate90_anticlockwise(uint8_t *des, uint8_t *src, int width,
                                      int height, int bits) {

    int i, j;
    int m = bits >> 3;
    int size;

    unsigned int *des32 = (unsigned int *)des;
    unsigned int *src32 = (unsigned int *)src;
    unsigned short *des16 = (unsigned short *)des;
    unsigned short *src16 = (unsigned short *)src;

    if ((!des) || (!src))
        return;
    if (m == 4) {
        for (j = 0; j < height; j++) {
            size = 0;
            for (i = 0; i < width; i++) {
                size += height;
                *(des32 + size - j - 1) = *src32++;
            }
        }
    } else {
        for (j = 0; j < height; j++) {
            size = 0;
            for (i = 0; i < width; i++) {
                size += height;
                *(des16 + size - j - 1) = *src16++;
            }
        }
    }
}
static void RGBRotate90_clockwise(uint8_t *des, uint8_t *src, int width,
                                  int height, int bits) {
    int i, j;
    int m = bits >> 3;
    int size;

    unsigned int *des32 = (unsigned int *)des;
    unsigned int *src32 = (unsigned int *)src;
    unsigned short *des16 = (unsigned short *)des;
    unsigned short *src16 = (unsigned short *)src;

    if ((!des) || (!src))
        return;
    if (m == 4) {
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                size = (i + 1) * height;
                *(des32 + size - j - 1) = *src32++;
            }
        }
    } else {
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                size = (i + 1) * height;
                *(des16 + size - j - 1) = *src16++;
            }
        }
    }
}

static void YUVRotate90(uint8_t *des, uint8_t *src, int width, int height) {
    int i = 0, j = 0, n = 0;
    int hw = width / 2, hh = height / 2;

    for (j = width; j > 0; j--) {
        for (i = 0; i < height; i++) {
            des[n++] = src[width * i + j];
        }
    }

    unsigned char *ptmp = src + width * height;
    for (j = hw; j > 0; j--) {
        for (i = 0; i < hh; i++) {
            des[n++] = ptmp[hw * i + j];
        }
    }

    ptmp = src + width * height * 5 / 4;
    for (j = hw; j > 0; j--) {
        for (i = 0; i < hh; i++) {
            des[n++] = ptmp[hw * i + j];
        }
    }
}

static void StretchColors(void *pDest, int nDestWidth, int nDestHeight,
                          int nDestBits, void *pSrc, int nSrcWidth,
                          int nSrcHeight, int nSrcBits) {
    double dfAmplificationX = ((double)nDestWidth) / nSrcWidth;
    double dfAmplificationY = ((double)nDestHeight) / nSrcHeight;
    double stretch =
        1 / (dfAmplificationY > dfAmplificationX ? dfAmplificationY
                                                 : dfAmplificationX);
    int offsetX = (dfAmplificationY > dfAmplificationX
                       ? (int)(nSrcWidth - nDestWidth / dfAmplificationY) >> 1
                       : 0);
    int offsetY = (dfAmplificationX > dfAmplificationY
                       ? (int)(nSrcHeight - nDestHeight / dfAmplificationX) >> 1
                       : 0);
    int i = 0;
    int j = 0;
    double tmpY = 0;
    unsigned int *pSrcPos = (unsigned int *)pSrc;
    unsigned int *pDestPos = (unsigned int *)pDest;
    int linesize;
    ALOGI("AutoTest: nDestWidth = %d, nDestHeight = %d, nSrcWidth = %d, "
          "nSrcHeight = %d, offsetX = %d, offsetY= %d \n",
          nDestWidth, nDestHeight, nSrcWidth, nSrcHeight, offsetX, offsetY);

    for (i = 0; i < nDestHeight; i++) {
        double tmpX = 0;

        int nLine = (int)(tmpY + 0.5) + offsetY;

        linesize = nLine * nSrcWidth;

        for (j = 0; j < nDestWidth; j++) {

            int nRow = (int)(tmpX + 0.5) + offsetX;

            *pDestPos++ = *(pSrcPos + linesize + nRow);

            tmpX += stretch;
        }
        tmpY += stretch;
    }
}

static void yuv420_to_rgb(int width, int height, unsigned char *src,
                          unsigned int *dst, unsigned int format) {
    int frameSize = width * height;
    int j = 0, yp = 0, i = 0;
    unsigned short *dst16 = (unsigned short *)dst;
    unsigned char *yuv420sp = src;

    for (j = 0, yp = 0; j < height; j++) {
        int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;

        for (i = 0; i < width; i++, yp++) {
            int y = (0xff & ((int)yuv420sp[yp])) - 16;

            if (y < 0)
                y = 0;
            if (format == FMT_NV21) {
                if ((i & 1) == 0) {
                    u = (0xff & yuv420sp[uvp++]) - 128;
                    v = (0xff & yuv420sp[uvp++]) - 128;
                }
            } else {
                if ((i & 1) == 0) {
                    v = (0xff & yuv420sp[uvp++]) - 128;
                    u = (0xff & yuv420sp[uvp++]) - 128;
                }
            }

            int y1192 = 1192 * y;
            int r = (y1192 + 1634 * v);
            int g = (y1192 - 833 * v - 400 * u);
            int b = (y1192 + 2066 * u);

            if (r < 0)
                r = 0;
            else if (r > 262143)
                r = 262143;
            if (g < 0)
                g = 0;
            else if (g > 262143)
                g = 262143;
            if (b < 0)
                b = 0;
            else if (b > 262143)
                b = 262143;

            if (var.bits_per_pixel == 32) {
                dst[yp] = ((((r << 6) & 0xff0000) >> 16) << 16) |
                          (((((g >> 2) & 0xff00) >> 8)) << 8) |
                          ((((b >> 10) & 0xff)) << 0);
            } else {
                dst16[yp] =
                    RGB565((((r << 6) & 0xff0000) >> 16),
                           (((g >> 2) & 0xff00) >> 8), (((b >> 10) & 0xff)));
            }
        }
    }
}

static void auto_dcamtest_switchTB(uint8_t *buffer, uint16_t width,
                                   uint16_t height, uint8_t pixel) {
    uint32_t j;
    uint32_t linesize;
    uint8_t *dst = NULL;
    uint8_t *src = NULL;
    uint8_t *tmpBuf = NULL;

    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);

    /*  */
    linesize = width * (pixel / 8);

    tmpBuf = (uint8_t *)malloc(linesize);
    if (!tmpBuf) {
        ALOGE("AutoTest: %s,%d Fail to alloc temp buffer\n", __func__,
              __LINE__);
        return;
    }

    /*  */
    for (j = 0; j < height / 2; j++) {
        src = buffer + j * linesize;
        dst = buffer + height * linesize - (j + 1) * linesize;
        memcpy(tmpBuf, src, linesize);

        for (j = 0; j < height / 2; j++) {
            src = buffer + j * linesize;
            dst = buffer + height * linesize - (j + 1) * linesize;
            memcpy(tmpBuf, src, linesize);
            memcpy(src, dst, linesize);
            memcpy(dst, tmpBuf, linesize);
        }
    }
    free(tmpBuf);
}

static int autotest_rotation(uint32_t agree, uint32_t width, uint32_t height,
                             uint32_t in_addr, uint32_t out_addr) {
    struct sprd_cpp_rot_cfg_parm rot_params;

    /* set rotation params */
    rot_params.format = ROT_YUV420;
    switch (agree) {
    case 90:
        rot_params.angle = ROT_90;
        break;
    case 180:
        rot_params.angle = ROT_180;
        break;
    case 270:
        rot_params.angle = ROT_270;
        break;
    default:
        rot_params.angle = ROT_ANGLE_MAX;
        break;
    }
    rot_params.size.w = width;
    rot_params.size.h = height;
    rot_params.src_addr.y = in_addr;
    rot_params.src_addr.u =
        rot_params.src_addr.y + rot_params.size.w * rot_params.size.h;
    rot_params.src_addr.v =
        rot_params.src_addr.u + rot_params.size.w * rot_params.size.h / 4;
    rot_params.dst_addr.y = out_addr;
    rot_params.dst_addr.u =
        rot_params.dst_addr.y + rot_params.size.w * rot_params.size.h;
    rot_params.dst_addr.v =
        rot_params.dst_addr.u + rot_params.size.w * rot_params.size.h / 4;

    /* open rotation device  */
    rot_fd = open(ROT_DEV, O_RDWR, 0);
    if (-1 == rot_fd) {
        ALOGE("AutoTest: %s,%d Fail to open rotation device\n", __func__,
              __LINE__);
        return -1;
    }

    /* call ioctl */
    if (-1 == ioctl(rot_fd, SPRD_CPP_IO_START_ROT, &rot_params)) {
        ALOGE("AutoTest: %s,%d Fail to SC8800G_ROTATION_DONE\n", __func__,
              __LINE__);
        return -1;
    }

    return 0;
}

static int autotest_fb_open(void) {
    int i;
    void *bits;
    int offset_page_align;

    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);

    var.yres = lcd_h;
    var.xres = lcd_w;
    var.bits_per_pixel = (8 * gr_draw->pixel_bytes);
    ALOGI("AutoTest: var.yres = %d, var.xres = %d, var.bits_per_pixel = %d\n",
          var.yres, var.xres, var.bits_per_pixel);
    /* set framebuffer address */
    memset(&fb_buf, 0, sizeof(fb_buf));

    fb_buf[0].virt_addr = (size_t)gr_draw->data;

    fb_buf[1].virt_addr =
        (size_t)var.yres * var.xres * (var.bits_per_pixel / 8);

    fb_buf[2].virt_addr = (size_t)tmpbuf2;

    fb_buf[3].virt_addr = (size_t)tmpbuf3;
    /*
            for(i = 0; i < 6; i++){
                    ALOGD("DCAM: buf[%d] virt_addr=0x%x, phys_addr=0x%x,
       length=%d", \
                            i,
       fb_buf[i].virt_addr,fb_buf[i].phys_addr,fb_buf[i].length);
            }
    */

    return 0;
}

static unsigned int getPreviewBufferIDForFd(cmr_s32 fd) {
    unsigned int i = 0;

    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        if (!previewHeapArray[i])
            continue;

        if (!(cmr_uint)previewHeapArray[i]->fd)
            continue;

        if (previewHeapArray[i]->fd == fd)
            return i;
    }

    return 0xFFFFFFFF;
}

static void autotest_fb_update(const camera_frame_type *frame) {
    // int width, height;
    int crtc = 0;
    unsigned int buffer_id;

    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);

    if (!frame)
        return;

    buffer_id = getPreviewBufferIDForFd(frame->fd);

    if (NULL != gr_draw) {
        gr_draw = gr_backend->flip(gr_backend);
        fb_buf[0].virt_addr = (size_t)gr_draw->data;
    }

    /*  */
    if (!previewHeapArray[buffer_id]) {
        ALOGI("AutoTest: %s,%d preview heap array empty, do nothing\n",
              __func__, __LINE__);
        return;
    }
    /*  */
    if (NULL == mHalOem || NULL == mHalOem->ops) {
        ALOGI("AutoTest: oem is null or oem ops is null, do nothing\n");
        return;
    }

    mHalOem->ops->camera_set_preview_buffer(
        oem_handle, (cmr_uint)previewHeapArray[buffer_id]->phys_addr,
        (cmr_uint)previewHeapArray[buffer_id]->data,
        (cmr_s32)previewHeapArray[buffer_id]->fd);
}

void autotest_camera_close(void) {
    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);
    return;
}

int autotest_flashlight_ctrl(uint32_t flash_status) {
    int ret = 0;

    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);

    return ret;
}

static void data_mirror(uint8_t *des, uint8_t *src, int width, int height,
                        int bits) {
    int size = 0;
    int i, j;
    int lineunm;
    int m = bits >> 3;
    unsigned int *des32 = (unsigned int *)des;
    unsigned int *src32 = (unsigned int *)src;
    unsigned short *des16 = (unsigned short *)des;
    unsigned short *src16 = (unsigned short *)src;

    /*  */
    if ((!des) || (!src))
        return;

    /*  */

    if (m == 4) {
        for (j = 0; j < height; j++) {
            size += width;
            for (i = 0; i < width; i++) {
                *(des32 + size - i - 1) = *src32++;
            }
        }
    } else {
        for (j = 0; j < height; j++) {
            size += width;
            for (i = 0; i < width; i++) {
                *(des16 + size - i - 1) = *src16++;
            }
        }
    }
    return;
}

void autotest_camera_cb(enum camera_cb_type cb, const void *client_data,
                        enum camera_func_type func, void *parm4) {
    struct camera_frame_type *frame = (struct camera_frame_type *)parm4;
    int num;

    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);

    if (!frame) {
        ALOGI(
            "AutoTest: %s,%d, camera call back parm4 error: NULL, do nothing\n",
            __func__, __LINE__);
        return;
    }

    if (CAMERA_FUNC_START_PREVIEW != func) {
        ALOGI("AutoTest: %s,%d, camera func type error: %d, do nothing\n",
              __func__, __LINE__, func);
        return;
    }

    if (CAMERA_EVT_CB_FRAME != cb) {
        ALOGI("AutoTest: %s,%d, camera cb type error: %d, do nothing\n",
              __func__, __LINE__, cb);
        return;
    }

    /*lock*/
    previewLock.lock();

    /*empty preview arry, do nothing*/
    if (!previewHeapArray[frame->buf_id]) {
        ALOGI("AutoTest: %s,%d, preview heap array empty, do nothine\n",
              __func__, __LINE__);
        previewLock.unlock();
        return;
    }

    /*get frame buffer id*/
    frame->buf_id = getPreviewBufferIDForFd(frame->fd);

    /*preview enable or disable?*/
    if (!previewvalid) {
        ALOGI("AutoTest: %s,%d, preview disabled, do nothing\n", __func__,
              __LINE__);
        previewLock.unlock();
        return;
    }

#ifdef CONFIG_CAMERA_DCAM_SUPPORT_FORMAT_NV12
    unsigned int format = FMT_NV12;
#else
    unsigned int format = FMT_NV21;
#endif
    // 1.yuv -> rgb
    yuv420_to_rgb(PREVIEW_WIDTH, PREVIEW_HIGHT,
                  (unsigned char *)previewHeapArray[frame->buf_id]->data,
                  post_preview_buf, format);

    /*unlock*/
    previewLock.unlock();

    /* fore && back camera: istrech,mirror,rotate*/
    if (0 == camera_id) {
        // 2. stretch
        StretchColors((void *)(fb_buf[2].virt_addr), var.yres, var.xres,
                      var.bits_per_pixel, post_preview_buf, PREVIEW_WIDTH,
                      PREVIEW_HIGHT, var.bits_per_pixel);

        /*FIXME: here need 2 or 3 framebuffer pingpang ??*/
        if (!(frame_num % 2)) {

            // 3. rotation
            RGBRotate90_anticlockwise((uint8_t *)(fb_buf[0].virt_addr),
                                      (uint8_t *)fb_buf[2].virt_addr, var.yres,
                                      var.xres, var.bits_per_pixel);
        } else {

            // 3. rotation
            RGBRotate90_anticlockwise((uint8_t *)(fb_buf[0].virt_addr),
                                      (uint8_t *)fb_buf[2].virt_addr, var.yres,
                                      var.xres, var.bits_per_pixel);
            // RGBRotate90_anticlockwise((uint8_t *)(fb_buf[1].virt_addr),
            // (uint8_t*)fb_buf[2].virt_addr, var.yres, var.xres,
            // var.bits_per_pixel);
        }
    } else if (1 == camera_id) {
        // 2. stretch
        StretchColors((void *)(fb_buf[3].virt_addr), var.yres, var.xres,
                      var.bits_per_pixel, post_preview_buf, PREVIEW_WIDTH,
                      PREVIEW_HIGHT, var.bits_per_pixel);
        // mirror fore camera
        data_mirror((uint8_t *)(fb_buf[2].virt_addr),
                    (uint8_t *)(fb_buf[3].virt_addr), var.yres, var.xres,
                    var.bits_per_pixel);
        if (!(frame_num % 2)) {
            RGBRotate90_anticlockwise((uint8_t *)(fb_buf[0].virt_addr),
                                      (uint8_t *)fb_buf[2].virt_addr, var.yres,
                                      var.xres, var.bits_per_pixel);
        } else {
            RGBRotate90_anticlockwise((uint8_t *)(fb_buf[0].virt_addr),
                                      (uint8_t *)fb_buf[2].virt_addr, var.yres,
                                      var.xres, var.bits_per_pixel);
            // RGBRotate90_anticlockwise((uint8_t *)(fb_buf[1].virt_addr),
            // (uint8_t*)fb_buf[2].virt_addr, var.yres, var.xres,
            // var.bits_per_pixel);
        }
    } else if (2 == camera_id) {
        // 2. stretch
        StretchColors((void *)(fb_buf[2].virt_addr), var.yres, var.xres,
                      var.bits_per_pixel, post_preview_buf, PREVIEW_WIDTH,
                      PREVIEW_HIGHT, var.bits_per_pixel);

        /*FIXME: here need 2 or 3 framebuffer pingpang ??*/
        if (!(frame_num % 2)) {

            // 3. rotation
            RGBRotate90_clockwise((uint8_t *)(fb_buf[0].virt_addr),
                                  (uint8_t *)fb_buf[2].virt_addr, var.yres,
                                  var.xres, var.bits_per_pixel);
        } else {

            // 3. rotation
            RGBRotate90_clockwise((uint8_t *)(fb_buf[0].virt_addr),
                                  (uint8_t *)fb_buf[2].virt_addr, var.yres,
                                  var.xres, var.bits_per_pixel);
            // RGBRotate90_anticlockwise((uint8_t *)(fb_buf[1].virt_addr),
            // (uint8_t*)fb_buf[2].virt_addr, var.yres, var.xres,
            // var.bits_per_pixel);
        }
    }

    /*lock*/
    previewLock.lock();
    ALOGI("AutoTest: "
          "fb_buf[0].virt_addr=0x%lx,var.xres=%d,var.yres=%d,var.bits_per_"
          "pixel=%d\n",
          fb_buf[0].virt_addr, var.xres, var.yres, var.bits_per_pixel);

    // 4. update
    autotest_fb_update(frame);

    /*unlock*/
    previewLock.unlock();

    frame_num++;
}

#if defined(CONFIG_CAMERA_ISP_DIR_3)

static int Callback_Free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                         cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum,
                         void *private_data) {

    int ret = 0;
    ALOGD("E");
    SprdCamera3AutotestMem *camera = (SprdCamera3AutotestMem *)private_data;
    /*lock*/
    previewLock.lock();

    if (!private_data || !vir_addr || !fd) {
        ALOGE("error param 0x%x 0x%lx 0x%lx", *fd, (cmr_uint)vir_addr,
              (cmr_uint)private_data);
        return BAD_VALUE;
    }

    if (type >= CAMERA_MEM_CB_TYPE_MAX) {
        ALOGE("mem type error %ld", (cmr_uint)type);
        return BAD_VALUE;
    }

    if (CAMERA_PREVIEW == type) {
        ret = camera->Callback_PreviewFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_SNAPSHOT == type) {
        // Performance optimization:move Callback_CaptureFree to closeCamera
        // function
        // ret = camera->Callback_CaptureFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_VIDEO == type) {
        ret = camera->Callback_VideoFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_SNAPSHOT_ZSL == type) {
        ret = camera->Callback_ZslFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_SENSOR_DATATYPE_MAP == type) {
        ret = camera->Callback_RefocusFree(phy_addr, vir_addr, sum);
    } else if (CAMERA_PDAF_RAW == type) {
        ret = camera->Callback_PdafRawFree(phy_addr, vir_addr, sum);
    } else if (CAMERA_SNAPSHOT_PATH == type) {
        ret = camera->Callback_CapturePathFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_PREVIEW_RESERVED == type ||
               CAMERA_VIDEO_RESERVED == type || CAMERA_ISP_FIRMWARE == type ||
               CAMERA_SNAPSHOT_ZSL_RESERVED == type ||
               CAMERA_SENSOR_DATATYPE_MAP_RESERVED == type ||
               CAMERA_PDAF_RAW_RESERVED == type || CAMERA_ISP_LSC == type ||
               CAMERA_ISP_BINGING4AWB == type ||
               CAMERA_SNAPSHOT_HIGHISO == type || CAMERA_ISP_RAW_DATA == type ||
               CAMERA_ISP_PREVIEW_Y == type || CAMERA_ISP_PREVIEW_YUV == type) {
        ret = camera->Callback_OtherFree(type, phy_addr, vir_addr, fd, sum);
    }

    /*unlock*/
    previewLock.unlock();

    /* disable preview flag */
    previewvalid = 0;

    ALOGD("X");
    return ret;
}

static int Callback_Malloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr,
                           cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                           cmr_uint *vir_addr, cmr_s32 *fd,
                           void *private_data) {

    int ret = 0, i = 0;
    uint32_t size, sum;
    SprdCamera3AutotestMem *camera = (SprdCamera3AutotestMem *)private_data;

    ALOGV("E");

    /*lock*/
    previewLock.lock();

    if (!private_data || !vir_addr || !fd || !size_ptr || !sum_ptr ||
        (0 == *size_ptr) || (0 == *sum_ptr)) {
        ALOGE("param error 0x%x 0x%lx 0x%lx 0x%lx 0x%lx", *fd,
              (cmr_uint)vir_addr, (cmr_uint)private_data, (cmr_uint)*size_ptr,
              (cmr_uint)*sum_ptr);
        /*unlock*/
        previewLock.unlock();
        return BAD_VALUE;
    }

    size = *size_ptr;
    sum = *sum_ptr;

    if (type >= CAMERA_MEM_CB_TYPE_MAX) {
        ALOGE("mem type error %ld", (cmr_uint)type);
        /*unlock*/
        previewLock.unlock();
        return BAD_VALUE;
    }

    if (CAMERA_PREVIEW == type) {
        ret = camera->Callback_PreviewMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SNAPSHOT == type) {
        ret = camera->Callback_CaptureMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_VIDEO == type) {
        ret = camera->Callback_VideoMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SNAPSHOT_ZSL == type) {
        ret = camera->Callback_ZslMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SENSOR_DATATYPE_MAP == type) {
        ret = camera->Callback_RefocusMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_PDAF_RAW == type) {
        ret = camera->Callback_PdafRawMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SNAPSHOT_PATH == type) {
        ret = camera->Callback_CapturePathMalloc(size, sum, phy_addr, vir_addr,
                                                 fd);
    } else if (CAMERA_PREVIEW_RESERVED == type ||
               CAMERA_VIDEO_RESERVED == type || CAMERA_ISP_FIRMWARE == type ||
               CAMERA_SNAPSHOT_ZSL_RESERVED == type ||
               CAMERA_SENSOR_DATATYPE_MAP_RESERVED == type ||
               CAMERA_PDAF_RAW_RESERVED == type || CAMERA_ISP_LSC == type ||
               CAMERA_ISP_BINGING4AWB == type ||
               CAMERA_SNAPSHOT_HIGHISO == type || CAMERA_ISP_RAW_DATA == type ||
               CAMERA_ISP_PREVIEW_Y == type || CAMERA_ISP_PREVIEW_YUV == type) {
        ret = camera->Callback_OtherMalloc(type, size, sum_ptr, phy_addr,
                                           vir_addr, fd);
    }

    /*unlock*/
    previewLock.unlock();

    /* enable preview flag */
    previewvalid = 1;

    ALOGV("X");
    return ret;
}

#else
static void freeCameraMem(sprd_camera_memory_t *memory) {
    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);

    if (!memory)
        return;

    if (memory->ion_heap) {
#if 0
        if (s_mem_method)
            ; // memory->ion_heap->free_iova(ION_MM,memory->phys_addr,
              // memory->phys_size);
#endif
        delete memory->ion_heap;
        memory->ion_heap = NULL;
    }

    free(memory);
}

static int Callback_OtherFree(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                              cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum) {
    int i;
    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL != mPreviewHeapReserved) {
            freeCameraMem(mPreviewHeapReserved);
            mPreviewHeapReserved = NULL;
        }
    }

    if (type == CAMERA_ISP_LSC) {
        if (NULL != mIspLscHeapReserved) {
            freeCameraMem(mIspLscHeapReserved);
            mIspLscHeapReserved = NULL;
        }
    }
#if defined(CONFIG_CAMERA_ISP_DIR_2_1) || defined(CONFIG_CAMERA_ISP_DIR_2_4)
    if (type == CAMERA_ISP_STATIS) {
        if (NULL != mIspStatisHeapReserved) {
            mIspStatisHeapReserved->ion_heap->free_kaddr();
            freeCameraMem(mIspStatisHeapReserved);
            mIspStatisHeapReserved = NULL;
        }
    }
#endif
    if (type == CAMERA_ISP_BINGING4AWB) {
        for (i = 0; i < kISPB4awbCount; i++) {
            if (NULL != mIspB4awbHeapReserved[i]) {
                freeCameraMem(mIspB4awbHeapReserved[i]);
                mIspB4awbHeapReserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_ISP_FIRMWARE) {
        if (NULL != mIspFirmwareReserved) {
            freeCameraMem(mIspFirmwareReserved);
            mIspFirmwareReserved = NULL;
        }
    }

    if (type == CAMERA_ISP_RAWAE) {
        for (i = 0; i < kISPB4awbCount; i++) {
            if (NULL != mIspRawAemHeapReserved[i]) {
                mIspRawAemHeapReserved[i]->ion_heap->free_kaddr();
                freeCameraMem(mIspRawAemHeapReserved[i]);
            }
            mIspRawAemHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (NULL != mIspAFLHeapReserved) {
            freeCameraMem(mIspAFLHeapReserved);
            mIspAFLHeapReserved = NULL;
        }
    }

    return 0;
}

static int Callback_PreviewFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;

    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);

    /*lock*/
    previewLock.lock();

    /*  */
    for (i = 0; i < mPreviewHeapNum; i++) {
        if (!previewHeapArray[i])
            continue;

        freeCameraMem(previewHeapArray[i]);
        previewHeapArray[i] = NULL;
    }

    mPreviewHeapNum = 0;

    /*unlock*/
    previewLock.unlock();

    return 0;
}

static cmr_int Callback_Free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                             cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum,
                             void *private_data) {
    cmr_int ret = 0;

    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);

    /*  */
    if (!private_data || !vir_addr || !fd) {
        ALOGE("AutoTest: %s,%d, error param 0x%lx 0x%lx 0x%lx\n", __func__,
              __LINE__, (cmr_uint)phy_addr, (cmr_uint)vir_addr,
              (cmr_uint)private_data);
        return -1;
    }

    if (CAMERA_MEM_CB_TYPE_MAX <= type) {
        ALOGE("AutoTest: %s,%d, mem type error %d\n", __func__, __LINE__, type);
        return -1;
    }

    if (CAMERA_PREVIEW == type) {
        ret = Callback_PreviewFree(phy_addr, vir_addr, fd, sum);
    } else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE
#if defined(CONFIG_CAMERA_ISP_DIR_2_1) || defined(CONFIG_CAMERA_ISP_DIR_2_4)
               || type == CAMERA_ISP_STATIS
#endif
               || type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE ||
               type == CAMERA_ISP_ANTI_FLICKER) {
        ret = Callback_OtherFree(type, phy_addr, vir_addr, fd, sum);
    } else {
        ALOGE("AutoTest: %s,%s,%d, type ignore: %d, do nothing.\n", __FILE__,
              __func__, __LINE__, type);
    }

    /* disable preview flag */
    previewvalid = 0;

    return ret;
}

static sprd_camera_memory_t *allocCameraMem(int buf_size, int num_bufs,
                                            uint32_t is_cache) {
    size_t psize = 0;
    int result = 0;
    size_t mem_size = 0;
    unsigned long paddr = 0;
    MemIon *pHeapIon = NULL;

    ALOGI("AutoTest: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
    ALOGI("buf_size %d, num_bufs %d", buf_size, num_bufs);

    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        ALOGE("AutoTest: %s,%d, failed: fatal error! memory pointer is null.\n",
              __func__, __LINE__);
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        ALOGE("AutoTest: %s,%d, failed: mem size err.\n", __func__, __LINE__);
        goto getpmem_fail;
    }

    if (0 == s_mem_method) {
        ALOGI("AutoTest: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_MM);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_MM);
        }
    } else {
        ALOGI("AutoTest: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_SYSTEM);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_SYSTEM);
        }
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        ALOGE("AutoTest: %s,%s,%d, failed: pHeapIon is null or getHeapID "
              "failed.\n",
              __FILE__, __func__, __LINE__);
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
        ALOGE("AutoTest: error getBase is null. %s,%s,%d, failed: ion get base "
              "err.\n",
              __FILE__, __func__, __LINE__);
        goto getpmem_fail;
    }
    ALOGI("AutoTest: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    memory->dev_fd = pHeapIon->getIonDeviceFd();
    // memory->phys_addr is offset from memory->fd, always set 0 for yaddr
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    if (0 == s_mem_method) {
        ALOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
              memory->fd, memory->phys_addr, memory->data, memory->phys_size,
              pHeapIon);
    } else {
        ALOGD("iommu: fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, "
              "heap=%p",
              memory->fd, memory->phys_addr, memory->data, memory->phys_size,
              pHeapIon);
    }

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }
    return NULL;
}

static int Callback_PreviewMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                                  cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_uint i = 0;

    ALOGI("size %d sum %d mPreviewHeapNum %d", size, sum, mPreviewHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        memory = allocCameraMem(size, 1, true);
        if (!memory) {
            ALOGE("AutoTest: %s,%d, failed: alloc camera mem err.\n", __func__,
                  __LINE__);
            goto mem_fail;
        }

        previewHeapArray[i] = memory;
        mPreviewHeapNum++;

        *phy_addr++ = (cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
    }

    return 0;

mem_fail:
    Callback_PreviewFree(0, 0, 0, 0);
    return -1;
}

static int Callback_OtherMalloc(enum camera_mem_cb_type type, cmr_u32 size,
                                cmr_u32 sum, cmr_uint *phy_addr,
                                cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i;
    cmr_u32 mem_size;
    cmr_u32 mem_sum;
    int buffer_id;
    ALOGD("size %d sum %d mPreviewHeapNum %d, type %d", size, sum,
          mPreviewHeapNum, type);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;
    ALOGI("AutoTest: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL == mPreviewHeapReserved) {
            ALOGI("AutoTest: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                ALOGE("AutoTest: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
                LOGE("error memory is null.");
                goto mem_fail;
            }
            mPreviewHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            ALOGI("AutoTest: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
            ALOGI("malloc Common memory for preview, video, and zsl, malloced "
                  "type %d,request num %d, request size 0x%x",
                  type, sum, size);
            *phy_addr++ = (cmr_uint)mPreviewHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mPreviewHeapReserved->data;
            *fd++ = mPreviewHeapReserved->fd;
        }
    } else if (type == CAMERA_ISP_LSC) {
        if (mIspLscHeapReserved == NULL) {
            ALOGI("AutoTest: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                ALOGE("AutoTest: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
                ALOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspLscHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            ALOGI("malloc isp lsc memory, malloced type %d,request num %d, "
                  "request size 0x%x",
                  type, sum, size);
            *phy_addr++ = (cmr_uint)mIspLscHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mIspLscHeapReserved->data;
            *fd++ = mIspLscHeapReserved->fd;
        }
#if defined(CONFIG_CAMERA_ISP_DIR_2_1) || defined(CONFIG_CAMERA_ISP_DIR_2_4)
    } else if (type == CAMERA_ISP_STATIS) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;
        if (mIspStatisHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                ALOGE("memory is null");
                goto mem_fail;
            }
            mIspStatisHeapReserved = memory;
        }
        mIspStatisHeapReserved->ion_heap->get_kaddr(&kaddr, &ksize);
        *phy_addr++ = kaddr;
        *phy_addr = kaddr >> 32;
        *vir_addr++ = (cmr_uint)mIspStatisHeapReserved->data;
        *fd++ = mIspStatisHeapReserved->fd;
        *fd++ = mIspStatisHeapReserved->dev_fd;
#endif
    } else if (type == CAMERA_ISP_BINGING4AWB) {
        cmr_u64 *phy_addr_64 = (cmr_u64 *)phy_addr;
        cmr_u64 *vir_addr_64 = (cmr_u64 *)vir_addr;
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        for (i = 0; i < sum; i++) {
            ALOGI("AutoTest: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                ALOGE("AutoTest: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
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
    } else if (type == CAMERA_ISP_RAWAE) {
        cmr_u64 kaddr = 0;
        cmr_u64 *phy_addr_64 = (cmr_u64 *)phy_addr;
        cmr_u64 *vir_addr_64 = (cmr_u64 *)vir_addr;
        size_t ksize = 0;
        for (i = 0; i < sum; i++) {
            ALOGI("AutoTest: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                ALOGE("AutoTest: error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspRawAemHeapReserved[i] = memory;
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
            *phy_addr_64++ = kaddr;
            *vir_addr_64++ = (cmr_u64)(memory->data);
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (mIspAFLHeapReserved == NULL) {
            ALOGI("AutoTest: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                ALOGE("AutoTest: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
                goto mem_fail;
            }
            mIspAFLHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            ALOGI("malloc isp afl memory, malloced type %d,request num %d, "
                  "request size 0x%x",
                  type, sum, size);
            *phy_addr++ = (cmr_uint)mIspAFLHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mIspAFLHeapReserved->data;
            *fd++ = mIspAFLHeapReserved->fd;
        }
    } else if (type == CAMERA_ISP_FIRMWARE) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;
        if (mIspFirmwareReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                ALOGE("AutoTest: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
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
    } else {
        ALOGE("AutoTest: %s,%s,%d, type ignore: %d, do nothing.\n", __FILE__,
              __func__, __LINE__, type);
    }

    return 0;

mem_fail:
    Callback_OtherFree(type, 0, 0, 0, 0);
    return -1;
}

static cmr_int Callback_Malloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr,
                               cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd,
                               void *private_data) {
    cmr_int ret = 0;
    cmr_u32 size;
    cmr_u32 sum;

    ALOGI("AutoTest: %s,%s,%d type %d IN\n", __FILE__, __func__, __LINE__,
          type);

    /*lock*/
    previewLock.lock();

    if (!phy_addr || !vir_addr || !size_ptr || !sum_ptr || (0 == *size_ptr) ||
        (0 == *sum_ptr)) {
        ALOGE("AutoTest: %s,%d, alloc param error 0x%lx 0x%lx 0x%lx\n",
              __func__, __LINE__, (cmr_uint)phy_addr, (cmr_uint)vir_addr,
              (cmr_uint)size_ptr);
        /*unlock*/
        previewLock.unlock();
        return -1;
    }

    size = *size_ptr;
    sum = *sum_ptr;

    if (CAMERA_PREVIEW == type) {
        /* preview buffer */
        ALOGI("AutoTest: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
        ret = Callback_PreviewMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE
#if defined(CONFIG_CAMERA_ISP_DIR_2_1) || defined(CONFIG_CAMERA_ISP_DIR_2_4)
               || type == CAMERA_ISP_STATIS
#endif
               || type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE ||
               type == CAMERA_ISP_ANTI_FLICKER) {
        ALOGI("AutoTest: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
        ret = Callback_OtherMalloc(type, size, sum, phy_addr, vir_addr, fd);
    } else {
        ALOGE("AutoTest: %s,%s,%d, type ignore: %d, do nothing.\n", __FILE__,
              __func__, __LINE__, type);
    }

    /* enable preview flag */
    previewvalid = 1;

    /*unlock*/
    previewLock.unlock();

    return ret;
}
#endif

static void autotest_camera_startpreview(void) {
    cmr_int ret = 0;
    struct img_size preview_size;
    struct cmr_zoom_param zoom_param;
    struct img_size capture_size;
    struct cmr_range_fps_param fps_param;

    if (!oem_handle || NULL == mHalOem || NULL == mHalOem->ops)
        return;

    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);

    /*  */
    preview_size.width = PREVIEW_WIDTH;
    preview_size.height = PREVIEW_HIGHT;

    zoom_param.mode = 1;
    zoom_param.zoom_level = 1;
    zoom_param.zoom_info.zoom_ratio = 1.00000;
    zoom_param.zoom_info.prev_aspect_ratio =
        (float)PREVIEW_WIDTH / PREVIEW_HIGHT;

    fps_param.is_recording = 0;
    fps_param.min_fps = 5;
    fps_param.max_fps = 30;
    fps_param.video_mode = 0;

    /*  */
    mHalOem->ops->camera_fast_ctrl(oem_handle, CAMERA_FAST_MODE_FD, 0);

    /*  */
    SET_PARM(mHalOem, oem_handle, CAMERA_PARAM_PREVIEW_SIZE,
             (cmr_uint)&preview_size);
#if defined(CONFIG_CAMERA_ISP_DIR_3)
    SET_PARM(mHalOem, oem_handle, CAMERA_PARAM_AF_MODE, CAMERA_FOCUS_MODE_CAF);
#endif
    // SET_PARM(oem_handle , CAMERA_PARAM_VIDEO_SIZE     ,
    // (cmr_uint)&video_size);
    // SET_PARM(oem_handle , CAMERA_PARAM_CAPTURE_SIZE   ,
    // (cmr_uint)&capture_size);
    SET_PARM(mHalOem, oem_handle, CAMERA_PARAM_PREVIEW_FORMAT,
             CAMERA_DATA_FORMAT_YUV420);
    // SET_PARM(oem_handle , CAMERA_PARAM_CAPTURE_FORMAT ,
    // CAMERA_DATA_FORMAT_YUV420);
    SET_PARM(mHalOem, oem_handle, CAMERA_PARAM_SENSOR_ROTATION, 0);
    SET_PARM(mHalOem, oem_handle, CAMERA_PARAM_ZOOM, (cmr_uint)&zoom_param);
    SET_PARM(mHalOem, oem_handle, CAMERA_PARAM_RANGE_FPS, (cmr_uint)&fps_param);

    /* set malloc && free callback*/
    ret = mHalOem->ops->camera_set_mem_func(oem_handle, (void *)Callback_Malloc,
                                            (void *)Callback_Free, NULL);
    if (CMR_CAMERA_SUCCESS != ret) {
        ALOGE("AutoTest: %s,%d, failed: camera set mem func error.\n", __func__,
              __LINE__);
        return;
    }

    /*start preview*/
    ret = mHalOem->ops->camera_start_preview(oem_handle, CAMERA_NORMAL_MODE);
    if (CMR_CAMERA_SUCCESS != ret) {
        ALOGE("AutoTest: %s,%d, failed: camera start preview error.\n",
              __func__, __LINE__);
        return;
    }
}

static int autotest_camera_stoppreview(void) {
    int ret;

    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);

    if (!oem_handle || NULL == mHalOem || NULL == mHalOem->ops) {
        ALOGI("AutoTest: oem is null or oem ops is null, do nothing\n");
        return -1;
    }

    ret = mHalOem->ops->camera_stop_preview(oem_handle);

    return ret;
}

extern "C" {
int autotest_camera_deinit() {
    cmr_int ret;

    ALOGI("AutoTest: %s,%d IN\n", __func__, __LINE__);

    ret = autotest_camera_stoppreview();

    if (oem_handle != NULL && mHalOem != NULL && mHalOem->ops != NULL) {
        ret = mHalOem->ops->camera_deinit(oem_handle);
    }

    //	memcpy(gr_draw->data, tmpbuf1, lcd_w*lcd_h*4);
    //	gr_draw = gr_backend->flip(gr_backend);
    //	memcpy(gr_draw->data, tmpbuf1, lcd_w*lcd_h*4);

    if (NULL != mHalOem && NULL != mHalOem->dso) {
        dlclose(mHalOem->dso);
    }
    if (NULL != mHalOem) {
        free((void *)mHalOem);
        mHalOem = NULL;
    }
    //	free(tmpbuf);
    //	free(tmpbuf1);
    free(tmpbuf2);
    free(tmpbuf3);
    return ret;
}
static cmr_int autotest_load_hal_lib(void) {
    int ret = 0;
    if (!mHalOem) {
        oem_module_t *omi;

        mHalOem = (oem_module_t *)malloc(sizeof(oem_module_t));

        mHalOem->dso = dlopen(OEM_LIBRARY_PATH, RTLD_NOW);
        if (NULL == mHalOem->dso) {
            char const *err_str = dlerror();
            ALOGE("dlopen error%s", err_str ? err_str : "unknown");
            ret = -1;
            goto loaderror;
        }

        /* Get the address of the struct hal_module_info. */
        const char *sym = OEM_MODULE_INFO_SYM_AS_STR;
        omi = (oem_module_t *)dlsym(mHalOem->dso, sym);
        if (omi == NULL) {
            ALOGE("load: couldn't find symbol %s", sym);
            ret = -1;
            goto loaderror;
        }

        mHalOem->ops = omi->ops;

        ALOGV("loaded HAL libcamoem mHalOem->dso = %p", mHalOem->dso);
    }

    return ret;

loaderror:

    if (NULL != mHalOem->dso) {
        dlclose(mHalOem->dso);
    }
    free((void *)mHalOem);
    mHalOem = NULL;
    return ret;
}

int autotest_camera_init(int cameraId, minui_backend *backend,
                         GRSurface *draw) {
    int ret = 0;

    ALOGI("AutoTest:%s,%d IN, cameraId %d\n", __func__, __LINE__, cameraId);
    //	gr_init();
    gr_backend = backend;
    gr_draw = draw;
    if (NULL != gr_draw)
        ALOGI("AutoTest:%s,line:%d in skd mode\n", __func__, __LINE__);
    else
        ALOGI("AutoTest:%s,line:%d in BBAT mode\n", __func__, __LINE__);

    if (2 == cameraId)
        camera_id = 2; // back camera
    else if (1 == cameraId)
        camera_id = 1; // front camera
    else
        camera_id = 0; // back camera
    if (getLcdSize(&lcd_w, &lcd_h)) {
        /*update preivew size by lcd*/
        ALOGI("AutoTest:%s lcd_w=%d,lcd_h=%d\n", __func__, lcd_w, lcd_h);
    }

    //	tmpbuf = (uint8_t*)malloc(lcd_w*lcd_h*4);
    //	tmpbuf1 = (uint8_t*)malloc(lcd_w*lcd_h*4);
    tmpbuf2 = (uint8_t *)malloc(lcd_w * lcd_h * 4);
    tmpbuf3 = (uint8_t *)malloc(lcd_w * lcd_h * 4);
    //	memcpy(tmpbuf1, gr_draw->data, lcd_w*lcd_h*4);
    if (NULL != gr_draw)
        autotest_fb_open();
    if (autotest_load_hal_lib()) {
        return -1;
    }
#if defined(CONFIG_CAMERA_ISP_DIR_3)
    AutotestMem = new SprdCamera3AutotestMem(camera_id, target_buffer_id,
                                             s_mem_method, previewHeapArray);
    ret = mHalOem->ops->camera_init(cameraId, autotest_camera_cb, AutotestMem,
                                    0, &oem_handle, (void *)Callback_Malloc,
                                    (void *)Callback_Free);

#else
    ret = mHalOem->ops->camera_init(cameraId, autotest_camera_cb, &client_data,
                                    0, &oem_handle, (void *)Callback_Malloc,
                                    (void *)Callback_Free);
#endif
    if (ret) {
        ALOGE("Native MMI Test: camera_init failed, ret=%d", ret);
        goto exit;
    }
    s_mem_method = autotest_IommuIsEnabled();
    ALOGI("AutoTest: %s,%s,%d， s_mem_method %d IN\n", __FILE__, __func__,
          __LINE__, s_mem_method);

    autotest_camera_startpreview();

exit:
    return ret;
}
int autotest_read_cam_buf(int **pp_image_addr, int size, int *out_size) {
    ALOGI("AutoTest E:");
    cmr_uint vir_addr;

    if (!previewHeapArray[target_buffer_id]) {
        return -1;
    } else {
        vir_addr = (cmr_uint)previewHeapArray[target_buffer_id]
                       ->ion_heap->getBase(); // virtual address
        *out_size = previewHeapArray[target_buffer_id]->phys_size; // image size
        memcpy((void *)*pp_image_addr, (void *)vir_addr, size);
    }
    ALOGI("AutoTest X:");
    return 1;
}
}
