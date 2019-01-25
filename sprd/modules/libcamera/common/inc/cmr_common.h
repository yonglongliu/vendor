/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _CMR_COMMON_H_
#define _CMR_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <utils/Log.h>
#include <utils/Timers.h>
#include "cmr_type.h"
#include "sensor_raw.h"
#include "isp_app.h"
#include "jpeg_exif_header.h"
#include <sprd_sensor_k.h>
#include "sprd_img.h"
#include "cmr_log.h"

#define OEM_LIBRARY_PATH "libcamoem.so"
#ifdef CONFIG_USE_CAMERASERVER_PROC
#define CAMERA_DUMP_PATH "/data/misc/cameraserver/"
#else
#define CAMERA_DUMP_PATH "/data/misc/media/"
#endif

#define UNUSED(x) (void) x

//#define CAMERA_BRINGUP                   1

#define CMR_EVT_GRAB_BASE (1 << 16)
#define CMR_EVT_CVT_BASE (1 << 17)
#define CMR_EVT_ISP_BASE (1 << 18)
#define CMR_EVT_SENSOR_BASE (1 << 19)
#define CMR_EVT_JPEG_BASE (1 << 20)
#define CMR_EVT_OEM_BASE (1 << 21)
#define CMR_EVT_SETTING_BASE (1 << 22)
#define CMR_EVT_IPM_BASE (1 << 23)
#define CMR_EVT_FOCUS_BASE (1 << 24)
#define CMR_EVT_PREVIEW_BASE (1 << 25)
#define CMR_EVT_SNAPSHOT_BASE (1 << 26)

#define CMR_PREV_ID_BASE 0x1000
#define CMR_CAP0_ID_BASE 0x2000
#define CMR_CAP1_ID_BASE 0x4000
#define CMR_VIDEO_ID_BASE 0x8000
#if defined(CONFIG_CAMERA_ISP_DIR_2_1) || defined(CONFIG_CAMERA_ISP_DIR_2_4)
#define CMR_REFOCUS_ID_BASE 0xF000
#else
#define CMR_SENSOR_DATATYPE_ID_BASE 0xF000
#endif
#define CMR_PDAF_ID_BASE 0xA000
#define CMR_BASE_ID(x) ((x)&0xF000)
#define JPEG_EXIF_SIZE (64 * 1024)
#define RAWRGB_BIT_WIDTH 10
#define CMR_ZOOM_FACTOR 2
#define CMR_SLICE_HEIGHT 256
#define CMR_SHARK_SCALING_TH 2048
#define CMR_DOLPHIN_SCALING_TH 1280
#define GRAB_BUF_MAX IMG_PATH_BUFFER_COUNT
#define GRAB_CHANNEL_MAX 6
#define SESNOR_NAME_LEN 40
#define CMR_CAPTURE_MEM_SUM 1

#define CAMERA_PIXEL_ALIGNED 4
#define CAMERA_SENSOR_INFO_2_ISP_NUM 3
#define CMR_MAX_SKIP_NUM 10
#define CAMERA_DEPTH_META_SIZE (480 * 360 + 1280)
#define CAMERA_EMBEDDED_INFO_META_SIZE (480 * 360 + 1280)
#define CAMERA_PDAF_META_SIZE (144 * 864 * 5 / 4)
#define CAMERA_DEPTH_META_DATA_TYPE 0x35
#define CAMERA_CONFIG_BUFFER_TO_KERNAL_ARRAY_SIZE 4
#define CAMERA_EMBEDDED_INFO_TYPE 0x12

#define HDR_CAP_NUM 3
#define PRE_3DNR_NUM 2
#define CAP_3DNR_NUM 5
#define PRE_SW_3DNR_RESERVE_NUM 8
#define FACE_DETECT_NUM 10
#define FRAME_NUM_MAX 0xFFFFFFFF
#define FRAME_FLASH_MAX 0x0000FFFF
#define INVALID_FORMAT_PATTERN 255
#define FLASH_CAPTURE_SKIP_FRAME_NUM 0

#if defined(CONFIG_CAMERA_ISP_DIR_2_1) || defined(CONFIG_CAMERA_ISP_DIR_2_4)
// some vsp and jpeg need height 16 alignment
#define HEIGHT_2M 1080
#ifdef CONFIG_CAMERA_MEET_JPG_ALIGNMENT
#undef HEIGHT_2M
#define HEIGHT_2M 1088
#endif
#endif

#define PREV_OUT_DATA 0xF000    /*debug.camera.save.snpfile 16*/
#define ZSL_OUT_DATA 0xE000     /*debug.camera.save.snpfile 16*/
#define SNP_CHN_OUT_DATA 0x8000 /*debug.camera.save.snpfile 1*/
#define SNP_ROT_DATA 0x8001     /*debug.camera.save.snpfile 2*/
#define SNP_SCALE_DATA 0x8002
#define SNP_REDISPLAY_DATA 0x8003  /*debug.camera.save.snpfile 3*/
#define SNP_ENCODE_SRC_DATA 0x8004 /*debug.camera.save.snpfile 4*/
#define SNP_ENCODE_STREAM 0x9000   /*debug.camera.save.snpfile 5*/
#define SNP_THUMB_DATA 0x8006      /*debug.camera.save.snpfile 6*/
#define SNP_THUMB_STREAM 0x1000    /*debug.camera.save.snpfile 7*/
#define SNP_JPEG_STREAM 0x2000     /*debug.camera.save.snpfile 8*/
#define SNP_HDR_OUT_DATA 0x3000    /*debug.camera.save.snpfile 9*/

#define FORM_DUMPINDEX(flag, dumpindex, ext)                                   \
    (((dumpindex) << 24) + ((flag) << 8) + (ext))

#define CAMERA_SAFE_SCALE_DOWN(w) (cmr_u32)((w)*11 / 10)
#define CAMERA_START(w) ((w) & ~(2 - 1))
#define CAMERA_WIDTH(w) ((w) & ~(8 - 1))
#define CAMERA_HEIGHT(h) ((h) & ~(8 - 1))
#define CMR_ADDR_ALIGNED(x) ((((x) + 256 - 1) >> 8) << 8)

#define CMR_3DNR_4_3_SMALL_WIDTH 1280
#define CMR_3DNR_4_3_SMALL_HEIGHT 960
#define CMR_3DNR_16_9_SMALL_WIDTH 1280
#define CMR_3DNR_16_9_SMALL_HEIGHT 720

#define CMR_JPEG_SZIE(w, h) (cmr_u32)((w) * (h)*3 / 2)
#define CMR_EVT_MASK_BITS                                                      \
    (cmr_u32)(CMR_EVT_GRAB_BASE | CMR_EVT_CVT_BASE | CMR_EVT_ISP_BASE |        \
              CMR_EVT_SENSOR_BASE | CMR_EVT_JPEG_BASE | CMR_EVT_OEM_BASE |     \
              CMR_EVT_SETTING_BASE | CMR_EVT_IPM_BASE | CMR_EVT_FOCUS_BASE)
#define CMR_RTN_IF_ERR(n)                                                      \
    do {                                                                       \
        if (n) {                                                               \
            CMR_LOGW("ret %ld", n);                                            \
            goto exit;                                                         \
        }                                                                      \
    } while (0)

#ifdef __LP64__
#define CMR_PRINT_TIME                                                         \
    do {                                                                       \
        nsecs_t timestamp = systemTime(CLOCK_MONOTONIC);                       \
        CMR_LOGD("timestamp = %ld.", timestamp / 1000000);                     \
    } while (0)

#define CMR_PRINT_TIME_V                                                       \
    do {                                                                       \
        nsecs_t timestamp = systemTime(CLOCK_MONOTONIC);                       \
        CMR_LOGV("timestamp = %ld.", timestamp / 1000000);                     \
    } while (0)
#else
#define CMR_PRINT_TIME                                                         \
    do {                                                                       \
        nsecs_t timestamp = systemTime(CLOCK_MONOTONIC);                       \
        CMR_LOGD("timestamp = %lld.", timestamp / 1000000);                    \
    } while (0)

#define CMR_PRINT_TIME_V                                                       \
    do {                                                                       \
        nsecs_t timestamp = systemTime(CLOCK_MONOTONIC);                       \
        CMR_LOGV("timestamp = %lld.", timestamp / 1000000);                    \
    } while (0)
#endif

#define CMR_PERROR                                                             \
    do {                                                                       \
        CMR_LOGE("device.errno %d %s", errno, strerror(errno));                \
    } while (0)

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

#define PERFORMANCE_OPTIMIZATION 1

/*********************************** common* **********************************/
#define cmr_bzero(b, len) memset((b), '\0', (len))
#define cmr_copy(b, a, len) memcpy((b), (a), (len))
#define CAMERA_ALIGNED_16(w) ((((w) + 16 - 1) >> 4) << 4)
#define cmr_array_size(x) (sizeof(x) / sizeof((x)[0]))
#define CMR_YUV_BUF_GAP 4
/******************************************************************************/

/********************************** fun type **********************************/
struct after_set_cb_param {
    cmr_u32 re_mode;
    cmr_u32 skip_mode;
    cmr_u32 skip_number;
    cmr_u32 padding;
    nsecs_t timestamp;
};

enum preview_param_mode { PARAM_NORMAL = 0, PARAM_ZOOM, PARAM_MODE_MAX };

typedef void (*cmr_evt_cb)(cmr_int evt, void *data, void *privdata);
typedef cmr_int (*cmr_before_set_cb)(cmr_handle oem_handle,
                                     enum preview_param_mode mode);
typedef cmr_int (*cmr_after_set_cb)(cmr_handle oem_handle,
                                    struct after_set_cb_param *param);
/******************************************************************************/
enum camera_index {
    CAMERA_ID_0 = 0,
    CAMERA_ID_1,
    CAMERA_ID_2,
    CAMERA_ID_3,
    CAMERA_ID_MAX
};

enum img_angle {
    IMG_ANGLE_0 = 0,
    IMG_ANGLE_90,
    IMG_ANGLE_270,
    IMG_ANGLE_180,
    IMG_ANGLE_MIRROR,
    IMG_ANGLE_MAX
};

enum sensor_data_type {
    SENSOR_DATATYPE_DISABLED = 0,
    SENSOR_REAL_DEPTH_ENABLE,
    SENSOR_EMBEDDED_INFO_ENABLE,
    SENSOR_DATATYPE_PDAF_ENABLE,
    SENSOR_DATATYPE_MAX
};

enum img_data_type {
    IMG_DATA_TYPE_YUV422 = 0,
    IMG_DATA_TYPE_YUV420,
    IMG_DATA_TYPE_YVU420,
    IMG_DATA_TYPE_YUV420_3PLANE,
    IMG_DATA_TYPE_RAW,
    IMG_DATA_TYPE_RAW2,
    IMG_DATA_TYPE_PDAF_TYPE3,
    IMG_DATA_TYPE_RGB565,
    IMG_DATA_TYPE_RGB666,
    IMG_DATA_TYPE_RGB888,
    IMG_DATA_TYPE_JPEG,
    IMG_DATA_TYPE_YV12,
    IMG_DATA_TYPE_MAX
};

/*0: 3 plane;  1: 2 plane uvuv;  2: 2 plane vuvu*/
enum img_data_endian {
    IMG_DATA_ENDIAN_3PLANE = 0,
    IMG_DATA_ENDIAN_2PLANE_UVUV,
    IMG_DATA_ENDIAN_2PLANE_VUVU,
    IMG_DATA_ENDIAN_MAX
};

enum img_skip_mode {
    IMG_SKIP_HW = 0,
    IMG_SKIP_SW,
    /* skip frame in kernel driver */
    IMG_SKIP_SW_KER,
    IMG_SKIP_MAX
};

enum restart_mode {
    RESTART_LIGHTLY = 0,
    RESTART_MIDDLE,
    RESTART_HEAVY,
    RESTART_ZOOM,
    RESTART_ZOOM_RECT,
    RESTART_MAX
};

enum restart_trigger {
    RESTART_BEFORE = 0,
    RESTART_AFTER,
    RESTART_ERROR,
    RESTART_SEQUENCE_MAX
};

enum raw_video_mode { VIDEO_CONTINUE = 0, VIDEO_SINGLE, VIDEO_MODE_MAX };

enum camera_capture_mode {
    CAMERA_CAP_MODE_AUTO,
    CAMERA_CAP_MODE_ZSL,
    CAMERA_CAP_MODE_HDR,
    CAMERA_CAP_MODE_VIDEO,
    CAMERA_CAP_MODE_VIDEO_HDR,
    CAMERA_CAP_MODE_BRACKET,
    CAMERA_CAP_MODE_RAW_DATA,
    CAMERA_CAP_MODE_MAX
};

enum camera_mem_type {
    CAMERA_MEM_PREVIEW = 0,
    CAMERA_MEM_SNAPSHOT,
    CAMERA_MEM_TYPE_MAX
};

enum camera_mem_aligned {
    CAMERA_MEM_ALIGNED = 0,
    CAMERA_MEM_NO_ALIGNED,
    CAMERA_MEM_ALIGNED_MAX
};

enum camera_states {
    IDLE = 0x00,
    INITED,
    PREVIEWING,
    SNAPSHOTING,
    RECOVERING_IDLE,
    RECOVERING,
    WORKING,
    CODEC_WORKING,
    POST_PROCESSING,
    IPM_WORKING,
    WRITE_EXIF,
    ERROR,
    CAMERA_STATE_MAX
};

enum common_isp_cmd_type {
    COM_ISP_SET_AE_MODE,
    COM_ISP_SET_AE_MEASURE_LUM,
    COM_ISP_SET_AE_METERING_AREA,
    COM_ISP_SET_BRIGHTNESS,
    COM_ISP_SET_CONTRAST,
    COM_ISP_SET_SATURATION,
    COM_ISP_SET_SHARPNESS,
    COM_ISP_SET_SPECIAL_EFFECT,
    COM_ISP_SET_EV,
    COM_ISP_SET_AWB_MODE,
    COM_ISP_SET_ANTI_BANDING,
    COM_ISP_SET_ISO,
    COM_ISP_SET_VIDEO_MODE,
    COM_ISP_SET_FPS_LLS_MODE,
    COM_ISP_SET_FLASH_EG,
    COM_ISP_SET_AF,
    COM_ISP_SET_AF_MODE, /*15*/
    COM_ISP_SET_AF_STOP,
    COM_ISP_GET_EXIF_IMAGE_INFO,
    COM_ISP_GET_LOW_LUX_EB, /*capability*/
    COM_ISP_SET_FLASH_NOTICE,
    COM_ISP_SET_FLASH_LEVEL,
    COM_ISP_SET_FACE_AREA,
    COM_ISP_SET_SNAPSHOT_NOTICE,
    COM_ISP_SET_RANGE_FPS,
    COM_ISP_GET_AE_LUM,
    COM_ISP_SET_HDR,
    COM_ISP_SET_AE_LOCK_UNLOCK,
    COM_ISP_SET_ROI_CONVERGENCE_REQ,
    COM_ISP_SET_SNAPSHOT_FINISHED,
    COM_ISP_SET_EXIF_DEBUG_INFO,
    COM_ISP_GET_EXIF_DEBUG_INFO,
    COM_ISP_GET_CUR_ADGAIN_EXP,
    COM_ISP_SET_FLASH_MODE,
    COM_ISP_GET_YIMG_INFO,
    COM_ISP_SET_PREVIEW_YIMG,
    COM_ISP_SET_PREVIEW_YUV,
    COM_ISP_GET_VCM_INFO,
    COM_ISP_SET_PREVIEW_PDAF_RAW,
    COM_ISP_SET_PREVIEW_PDAF_OPEN,
    COM_ISP_SET_AWB_LOCK_UNLOCK,
    COM_ISP_GET_LEDS_CTRL,
    COM_ISP_SET_3DNR,
    COM_ISP_SET_AE_MODE_CONTROL,
    COM_ISP_SET_EXPOSURE_TIME,
    COM_ISP_SET_SENSITIVITY,
    COM_ISP_SET_AF_BYPASS,
    COM_ISP_SET_AF_POS,
    COM_ISP_TYPE_MAX
};

enum common_sn_cmd_type {
    COM_SN_GET_AUTO_FLASH_STATE,
    COM_SN_SET_BRIGHTNESS,
    COM_SN_SET_CONTRAST,
    COM_SN_SET_SATURATION,
    COM_SN_SET_SHARPNESS,
    COM_SN_SET_IMAGE_EFFECT,
    COM_SN_SET_EXPOSURE_COMPENSATION,
    COM_SN_SET_EXPOSURE_LEVEL,
    COM_SN_SET_WB_MODE,
    COM_SN_SET_PREVIEW_MODE,
    COM_SN_SET_ANTI_BANDING,
    COM_SN_SET_ISO,
    COM_SN_SET_VIDEO_MODE,
    COM_SN_SET_FPS_LLS_MODE,
    COM_SN_SET_BEFORE_SNAPSHOT,
    COM_SN_SET_AFTER_SNAPSHOT,
    COM_SN_SET_EXT_FUNC,
    COM_SN_SET_AE_ENABLE,
    COM_SN_SET_EXIF_FOCUS,
    COM_SN_SET_FOCUS,
    COM_SN_GET_RESOLUTION_IMAGE_FORMAT,
    COM_SN_GET_FRAME_SKIP_NUMBER,
    COM_SN_GET_PREVIEW_MODE,
    COM_SN_GET_CAPTURE_MODE,
    COM_SN_GET_SENSOR_ID,
    COM_SN_GET_VIDEO_MODE,
    COM_SN_GET_EXIF_IMAGE_INFO,
    COM_SN_SET_HDR_EV,
    COM_SN_GET_INFO,
    COM_SN_GET_FLASH_LEVEL,
    COM_SN_TYPE_MAX,
};

enum cmr_jpeg_buf_type {
    JPEG_YUV_SLICE_ONE_BUF = 0,
    JPEG_YUV_SLICE_MUTI_BUF,
};

enum take_picture_flag { TAKE_PICTURE_NO = 0, TAKE_PICTURE_NEEDED };

enum highiso_mode_type {
    HIGHISO_MODE_NONE = 0,
    HIGHISO_CAP_MODE,
    HIGHISO_RAWDATA_MODE,
    HIGHISO_MODE_MAX
};

enum frame_contrl_type {
    FRAME_STOP = 0,
    FRAME_CONTINUE,
    FRAME_HDR_PROC,
    FRAME_3DNR_PROC
};

enum blur_tips_type {
    BLUR_TIPS_OK = 50,
    BLUR_TIPS_FURTHER,
    BLUR_TIPS_CLOSE,
    BLUR_TIPS_NEED_LIGHT,
    BLUR_TIPS_UNABLED,
    BLUR_TIPS_DEFAULT
};

struct img_addr {
    cmr_uint addr_y;
    cmr_uint addr_u;
    cmr_uint addr_v;
};

struct img_mfd {
    cmr_u32 y;
    cmr_u32 u;
    cmr_u32 v;
};

struct img_frm {
    cmr_u32 fmt;
    cmr_u32 buf_size;
    struct img_rect rect;
    struct img_size size;
    struct img_addr addr_phy;
    struct img_addr addr_vir;
    cmr_s32 fd;
    struct img_data_end data_end;
    cmr_u32 format_pattern;
    void *reserved;
};

struct snp_thumb_yuv_param {
    int angle;
    struct img_frm src_img;
    struct img_frm dst_img;
};

struct enc_exif_param {
    struct img_frm src;      // yuv
    struct img_frm pic_enc;  // yuv -> encoded
    struct img_frm last_dst; // in encoded and exif, maybe null
    cmr_u32 stream_real_size;
};

struct ccir_if {
    cmr_u8 v_sync_pol;
    cmr_u8 h_sync_pol;
    cmr_u8 pclk_pol;
    cmr_u8 res1;
    cmr_u32 padding;
};

struct mipi_if {
    cmr_u8 use_href;
    cmr_u8 bits_per_pxl;
    cmr_u8 is_loose;
    cmr_u8 lane_num;
    cmr_u32 pclk;
};

struct cmr_op_mean {
    cmr_u32 slice_height;
    cmr_u32 slice_mode;
    cmr_u32 ready_line_num;
    cmr_u32 slice_num;
    cmr_u32 is_sync;
    cmr_u32 is_thumb;
    cmr_u32 rot;
    cmr_u32 quality_level;
    cmr_uint out_param;
    struct img_frm temp_buf;
};

struct video_start_param {
    struct img_size size;
#ifdef CONFIG_CAMERA_OFFLINE
    struct img_size dcam_size;
#endif
    cmr_u32 img_format;
    cmr_u32 video_mode;
    cmr_u32 work_mode;
    cmr_u32 is_need_flash;
    cmr_u32 capture_skip_num;
    cmr_u32 is_snapshot;
#ifdef CONFIG_CAMERA_OFFLINE
    cmr_u32 sprd_zsl_flag;
#endif
    struct img_size live_view_sz;
    struct img_size lv_size;
    struct img_size video_size;
};

struct memory_param {
    cmr_malloc alloc_mem;
    cmr_free free_mem;
    cmr_gpu_malloc gpu_alloc_mem;
};

struct isptool_scene_param {
    cmr_u32 width;
    cmr_u32 height;
    cmr_u32 gain;
    cmr_u32 awb_gain_r;
    cmr_u32 awb_gain_g;
    cmr_u32 awb_gain_b;
    cmr_u32 smart_ct;
    cmr_u32 smart_bv;
};

struct leds_ctrl {
    cmr_u32 led0_ctrl;
    cmr_u32 led1_ctrl;
};

typedef struct {
    int fd;
    size_t size;
    // offset from fd, always set to 0
    void *addr_phy;
    void *addr_vir;
} cam_ion_buffer_t;

struct cam_face {
    int32_t rect[4];
    int32_t score;
    int32_t id;
    int32_t left_eye[2];
    int32_t right_eye[2];
    int32_t mouth[2];
};

struct face_tag {
    struct cam_face face[10];
    int angle[10];
    int pose[10];
    uint8_t face_num;
};

struct img_debug {
    struct img_addr input;
    struct img_addr output;
    struct img_size size;
    cmr_u32 format;
    void *params;
};

/********************************* v4l2 start *********************************/

/********************************* v4l2 start *********************************/
enum cmr_v4l2_evt {
    CMR_GRAB_TX_DONE = CMR_EVT_GRAB_BASE,
    CMR_GRAB_TX_ERROR,
    CMR_GRAB_TX_NO_MEM,
    CMR_GRAB_CSI2_ERR,
    CMR_GRAB_TIME_OUT,
    CMR_GRAB_CANCELED_BUF,
    CMR_GRAB_MAX,
};
enum channel_num { CHN_0 = 0, CHN_1, CHN_2, CHN_3, CHN_MAX = GRAB_CHANNEL_MAX};

enum cmr_buf_flag { BUF_FLAG_INIT, BUF_FLAG_RUNNING, BUF_FLAG_MAX };

struct sensor_if {
    cmr_u8 if_type;
    cmr_u8 img_fmt;
    cmr_u8 img_ptn;
    cmr_u8 frm_deci;
    cmr_u16 sensor_width;
    cmr_u16 sensor_height;
    cmr_u8 res[4];
    union {
        struct ccir_if ccir;
        struct mipi_if mipi;
    } if_spec;
};

struct img_frm_cap {
    cmr_u32 src_img_change;
    struct img_size src_img_size;
    struct img_rect src_img_rect;
    struct img_size dst_img_size;
    cmr_u32 dst_img_fmt;
    cmr_u32 src_img_fmt;
    cmr_u32 notice_slice_height;
    cmr_u32 need_isp;
    cmr_u32 need_binning;
    cmr_u32 need_isp_tool;
    cmr_u32 need_3dnr;
    struct dcam_regular_desc regular_desc;
    cmr_u32 flip_on;
    cmr_u32 pdaf_type3;
    struct sprd_pdaf_control pdaf_ctrl;
    cmr_u32 sence_mode;
    cmr_u32 slowmotion;
    cmr_u32 chn_skip_num;
};

struct buffer_cfg {
    cmr_u32 channel_id;
    cmr_u32 base_id;
    cmr_u32 count;
    cmr_u32 length;
    cmr_u32 slice_height;
    cmr_u32 start_buf_id;
    cmr_u32 is_reserved_buf;
    cmr_u32 flag;
    cmr_u32 index[GRAB_BUF_MAX];
    struct img_addr addr[GRAB_BUF_MAX];
    struct img_addr addr_vir[GRAB_BUF_MAX];
    cmr_u32 fd[GRAB_BUF_MAX];
    cmr_uint zsl_private;
};

struct cap_cfg {
    cmr_u32 chn_deci_factor;
    cmr_u32 frm_num;
    cmr_u32 buffer_cfg_isp;
    cmr_u32 hdr_cap;
    cmr_u32 video_enabled;
    cmr_u32 sensor_id;
    struct img_frm_cap cfg;
};

struct channel_start_param {
    cmr_u32 is_lightly; /*0:normal, 1:lightly, only update cap_cfg*/
    cmr_u32 sensor_mode;
    cmr_u32 frm_num;
    cmr_u32 skip_num;
    struct sensor_if sn_if;
    struct cap_cfg cap_inf_cfg;
    struct buffer_cfg buffer;
};
struct cmr_path_info {
    uint32_t line_buf;
    uint32_t support_yuv;
    uint32_t support_raw;
    uint32_t support_jpeg;
    uint32_t support_scaling;
    uint32_t is_scaleing_path;
    ;
};
struct cmr_path_capability {
    uint32_t is_video_prev_diff;
    uint32_t hw_scale_available;
    uint32_t yuv_available_cnt;
    uint32_t capture_no_trim;
    uint32_t capture_pause;
    uint32_t zoom_post_proc;
    uint32_t support_3dnr_mode;
};

struct dual_sensor_luma_info {
    uint32_t sub_luma;
    uint32_t main_gain;
    uint32_t main_luma;
};

enum cmr_af_focus_type {
    CAM_AF_FOCUS_SAF,
    CAM_AF_FOCUS_CAF,
    CAM_AF_FOCUS_FAF,
    CAM_AF_FOCUS_MAX
};

struct cmr_focus_status {
    int is_in_focus;
    int af_focus_type;
};

/********************************** v4l2 end **********************************/

/******************************** memory start ********************************/
struct cmr_cap_mem {
    struct img_frm cap_raw;
    /* for whale2 raw capture*/
    struct img_frm cap_raw2;
    struct img_frm cap_yuv;
    struct img_frm target_yuv;
    struct img_frm target_jpeg;
    struct img_frm thum_yuv;
    struct img_frm thum_jpeg;
    struct img_frm jpeg_tmp;
    struct img_frm scale_tmp;
    struct img_frm cap_yuv_rot;
    struct img_frm isp_tmp;
};
/********************************* memory end *********************************/

/******************************** sensor start ********************************/
enum sensor_mode {
    SENSOR_MODE_COMMON_INIT = 0,
    SENSOR_MODE_PREVIEW_ONE,
    SENSOR_MODE_SNAPSHOT_ONE_FIRST,
    SENSOR_MODE_SNAPSHOT_ONE_SECOND,
    SENSOR_MODE_SNAPSHOT_ONE_THIRD,
    SENSOR_MODE_PREVIEW_TWO,
    SENSOR_MODE_SNAPSHOT_TWO_FIRST,
    SENSOR_MODE_SNAPSHOT_TWO_SECOND,
    SENSOR_MODE_SNAPSHOT_TWO_THIRD,
    SENSOR_MODE_MAX
};

#define SENSOR_VIDEO_MODE_MAX 4

struct sensor_interface {
    cmr_u32 type;
    cmr_u32 bus_width;
    cmr_u32 pixel_width;
    cmr_u32 is_loose;
};
struct sensor_mode_info {
    cmr_u16 mode;
    cmr_u16 width;
    cmr_u16 height;
    cmr_u16 trim_start_x;
    cmr_u16 trim_start_y;
    cmr_u16 trim_width;
    cmr_u16 trim_height;
    cmr_u16 image_format;
    cmr_u32 line_time;
    cmr_u32 bps_per_lane;
    cmr_u32 frame_line;
    cmr_u32 padding;
    struct img_rect scaler_trim;
    cmr_u16 out_width;  // sensor output width after binning and trimming
    cmr_u16 out_height; // sensor outpout width after binning and trimming
};

struct sensor_ae_info {
    cmr_u32 min_frate;
    cmr_u32 max_frate;
    cmr_u32 line_time;
    cmr_u32 gain;
};

struct sensor_video_info {
    struct sensor_ae_info ae_info[SENSOR_VIDEO_MODE_MAX];
    void *setting_pptr;
};

struct sensor_view_angle {
    cmr_u16 horizontal_val;
    cmr_u16 vertical_val;
};

struct exif_info {
    float aperture;
    float focus_distance;
};

struct sensor_exp_info {
    cmr_u32 image_format;
    cmr_u32 image_pattern;
    cmr_u32 change_setting_skip_num;
    cmr_u32 sensor_image_type;
    cmr_u8 pclk_polarity;
    cmr_u8 vsync_polarity;
    cmr_u8 hsync_polarity;
    cmr_u8 padding1;
    cmr_u16 source_width_max;
    cmr_u16 source_height_max;
    cmr_u32 image_effect;
    cmr_u32 step_count;
    cmr_u32 preview_skip_num;
    cmr_u32 capture_skip_num;
    cmr_u32 flash_capture_skip_num;
    cmr_u32 mipi_cap_skip_num;
    cmr_u32 preview_deci_num;
    cmr_u32 video_preview_deci_num;
    cmr_u16 threshold_eb;
    cmr_u16 threshold_mode;
    cmr_u16 threshold_start;
    cmr_u16 threshold_end;
    struct sensor_mode_info mode_info[SENSOR_MODE_MAX];
    struct sensor_raw_info *raw_info_ptr;
    struct sensor_interface sn_interface;
    struct sensor_video_info video_info[SENSOR_MODE_MAX];
    struct sensor_view_angle view_angle;
    struct sensor_raw_info *raw_info_list_ptr[CAMERA_SENSOR_INFO_2_ISP_NUM];
    cmr_s8 *name;
    cmr_s8 *sensor_version_info;
};

struct yuv_sn_af_param {
    uint8_t cmd;
    uint8_t param;
    uint16_t zone_cnt;
    struct img_rect zone[FOCUS_ZONE_CNT_MAX];
};

/********************************* sensor end *********************************/

/***************************** common ctrl start ******************************/
struct common_isp_cmd_param {
    cmr_uint camera_id;
    union {
        cmr_u32 cmd_value;
        void *cmd_ptr;
        cmr_u32 padding;
        cmr_u32 vcm_step;
        struct isp_af_win af_param;
        struct exif_spec_pic_taking_cond_tag exif_pic_info;
        struct cmr_win_area win_area;
        struct isp_flash_notice flash_notice;
        struct isp_face_area fd_param;
        struct isp_ae_fps fps_param;
        struct cmr_range_fps_param range_fps;
        struct isp_info isp_dbg_info;
        struct isp_adgain_exp_info isp_adgain;
        struct isp_yimg_info isp_yimg;
        struct img_size size_param;
        struct leds_ctrl leds_ctrl;
    };
};

struct common_sn_cmd_param {
    cmr_uint camera_id;
    struct sensor_exp_info sensor_static_info;
    struct sensor_flash_level flash_level;
    union {
        cmr_u32 cmd_value;
        cmr_u32 padding;
        struct yuv_sn_af_param yuv_sn_af_param;
        struct exif_spec_pic_taking_cond_tag exif_pic_info;
    };
};
/**************************** common ctrl end *********************************/

/*************************** snapshot post data type **************************/
enum cmr_zoom_mode {
    ZOOM_BY_CAP = 0,
    ZOOM_POST_PROCESS,
    ZOOM_POST_PROCESS_WITH_TRIM
};

struct snp_proc_param {
    cmr_uint rot_angle;
    cmr_uint channel_zoom_mode;
    struct img_size dealign_actual_snp_size;
    struct img_size actual_snp_size;
    struct img_size snp_size;
    struct img_size max_size;
    struct img_data_end data_endian;
    cmr_uint is_need_scaling;
    struct img_rect scaler_src_rect[CMR_CAPTURE_MEM_SUM];
    struct img_frm chn_out_frm[CMR_CAPTURE_MEM_SUM];
    struct cmr_cap_mem mem[CMR_CAPTURE_MEM_SUM];
    struct img_size thumb_size;
};
/*************************** snapshot post data type **************************/

/*************************** ipm data type **************************/

enum ipm_class_type {
    IPM_TYPE_NONE = 0x0,
    IPM_TYPE_HDR = 0x00000001,
    IPM_TYPE_FD = 0x00000002,
    IPM_TYPE_UVDE = 0x00000004,
    IPM_TYPE_YDE = 0x00000008,
    IPM_TYPE_REFOCUS = 0x00000010,
    IPM_TYPE_3DNR = 0x00000020,
    IPM_TYPE_3DNR_PRE = 0x00000040,
    IPM_TYPE_FILTER = 0x00000080,
};

enum img_fmt {
    IMG_FMT_RGB565 = 0x0010,
    IMG_FMT_RGB666,
    IMG_FMT_XRGB8888,
    IMG_FMT_YCBCR420 = 0x0020,
    IMG_FMT_YCRCB420,
    IMG_FMT_YCBCR422P,
    IMG_FMT_YCBYCR422 = 0x0040,
    IMG_FMT_YCRYCB422,
    IMG_FMT_CBYCRY422,
    IMG_FMT_CRYCBY422,
};

struct face_finder_data {
    int face_id;
    int sx;
    int sy;
    int srx;
    int sry;
    int ex;
    int ey;
    int elx;
    int ely;
    int brightness;
    int angle;
    int smile_level;
    int blink_level;
    int pose;
    int score;
    int smile_conf;
};

struct img_face_area {
    cmr_uint face_count;
    struct face_finder_data range[FACE_DETECT_NUM];
};

struct img_otp_data {
    cmr_uint otp_size;
    void *otp_ptr;
};

struct img_depth_map {
    cmr_uint width;
    cmr_uint height;
    void *depth_map_ptr;
};

/*************************** ipm data type **************************/

/******************************* jpeg data type *******************************/
struct jpeg_enc_cb_param {
    cmr_uint stream_buf_phy;
    cmr_uint stream_buf_vir;
    cmr_u32 stream_size;
    cmr_u32 slice_height;
    cmr_u32 total_height;
    cmr_u32 is_thumbnail;
};

struct jpeg_dec_cb_param {
    cmr_u32 slice_height;
    cmr_u32 total_height;
    struct img_frm *src_img;
    struct img_data_end data_endian;
};
/******************************* jpeg data type *******************************/

/*********************** camera_takepic_step timestamp  ***********************/
enum CAMERA_TAKEPIC_STEP {
    CMR_STEP_TAKE_PIC = 0,
    CMR_STEP_CAP_S,
    CMR_STEP_CAP_E,
    CMR_STEP_ROT_S,
    CMR_STEP_ROT_E,
    CMR_STEP_ISP_PP_S,
    CMR_STEP_ISP_PP_E,
    CMR_STEP_JPG_DEC_S,
    CMR_STEP_JPG_DEC_E,
    CMR_STEP_SC_S,
    CMR_STEP_SC_E,
    CMR_STEP_UVDENOISE_S,
    CMR_STEP_UVDENOISE_E,
    CMR_STEP_YDENOISE_S,
    CMR_STEP_YDENOISE_E,
    CMR_STEP_REFOCUS_S,
    CMR_STEP_REFOCUS_E,
    CMR_STEP_JPG_ENC_S,
    CMR_STEP_JPG_ENC_E,
    CMR_STEP_CVT_THUMB_S,
    CMR_STEP_CVT_THUMB_E,
    CMR_STEP_THUM_ENC_S,
    CMR_STEP_THUM_ENC_E,
    CMR_STEP_WR_EXIF_S,
    CMR_STEP_WR_EXIF_E,
    CMR_STEP_CALL_BACK,
    CMR_STEP_MAX
};

struct CAMERA_TAKEPIC_STAT {
    cmr_u8 step_name[24];
    nsecs_t timestamp;
    cmr_u32 valid;
    cmr_u32 padding;
};

enum video_snapshot_tpye {
    VIDEO_SNAPSHOT_NONE = 0,
    VIDEO_SNAPSHOT_VIDEO,
    VIDEO_SNAPSHOT_ZSL,
    VIDEO_SNAPSHOT_PREVIEW,
    VIDEO_SNAPSHOT_MAX
};

/**********************************************************8*******************/
cmr_int camera_get_trim_rect(struct img_rect *src_trim_rect,
                             cmr_uint zoom_level, struct img_size *dst_size);

cmr_int camera_get_trim_rect2(struct img_rect *src_trim_rect, float zoom_ratio,
                              float dst_aspect_ratio, cmr_u32 sensor_w,
                              cmr_u32 sensor_h, cmr_u8 rot);

cmr_int camera_scale_down_software(struct img_frm *src, struct img_frm *dst);

cmr_int camera_save_yuv_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width,
                                cmr_u32 height, struct img_addr *addr);

cmr_int camera_save_jpg_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width,
                                cmr_u32 height, cmr_u32 stream_size,
                                struct img_addr *addr);

cmr_int read_file(const char *file_name, void *data_buf, uint32_t buf_size);

cmr_int save_file(const char *file_name, void *data, uint32_t data_size);

cmr_int camera_save_y_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width,
                              cmr_u32 height, void *addr);

cmr_int camera_get_data_from_file(char *file_name, cmr_u32 img_fmt,
                                  cmr_u32 width, cmr_u32 height,
                                  struct img_addr *addr);

cmr_int camera_parse_raw_filename(char *file_name,
                                  struct isptool_scene_param *scene_param);

void camera_snapshot_step_statisic(struct img_size *img_size);

void camera_take_snapshot_step(enum CAMERA_TAKEPIC_STEP step);

/* White balancing type, used for CAMERA_PARM_WHITE_BALANCING */
enum {
    CAMERA_WB_AUTO = 0,
    CAMERA_WB_INCANDESCENT,
    CAMERA_WB_FLUORESCENT = 4, // id 2 and 3 not used
    CAMERA_WB_DAYLIGHT,
    CAMERA_WB_CLOUDY_DAYLIGHT,
    CAMERA_WB_MAX
};

enum {
    CAMERA_SCENE_MODE_AUTO = 0,
    CAMERA_SCENE_MODE_NIGHT,
    CAMERA_SCENE_MODE_ACTION,    // not support
    CAMERA_SCENE_MODE_PORTRAIT,  // not support
    CAMERA_SCENE_MODE_LANDSCAPE, // not support
    CAMERA_SCENE_MODE_NORMAL,
    CAMERA_SCENE_MODE_HDR,
    CAMERA_SCENE_MODE_MAX
};

enum {
    CAMERA_AE_FRAME_AVG = 0,
    CAMERA_AE_CENTER_WEIGHTED,
    CAMERA_AE_SPOT_METERING,
    CAMERA_AE_METERING_INTELLIWT,
    CAMERA_AE_MODE_MAX
};

enum camera_format_type {
    CAMERA_YCBCR_4_2_0,
    CAMERA_YCBCR_4_2_2,
    CAMERA_FORMAT_TYPE_MAX
};

enum cmr_flash_mode {
    CAMERA_FLASH_MODE_OFF = 0,
    CAMERA_FLASH_MODE_ON = 1,
    CAMERA_FLASH_MODE_TORCH = 2,
    CAMERA_FLASH_MODE_AUTO = 3,
    CAMERA_FLASH_MODE_MAX
};

enum cmr_focus_mode {
    CAMERA_FOCUS_MODE_AUTO = 0,
    CAMERA_FOCUS_MODE_AUTO_MULTI = 1,
    CAMERA_FOCUS_MODE_MACRO = 2,
    CAMERA_FOCUS_MODE_INFINITY = 3,
    CAMERA_FOCUS_MODE_CAF = 4,
    CAMERA_FOCUS_MODE_CAF_VIDEO = 5,
    CAMERA_FOCUS_MODE_MACRO_FIXED = 6,
    CAMERA_FOCUS_MODE_PICTURE = 7,
    CAMERA_FOCUS_MODE_FULLSCAN = 8,
    CAMERA_FOCUS_MODE_MAX
};

enum camera_cb_type {
    CAMERA_RSP_CB_SUCCESS,
    CAMERA_EXIT_CB_DONE,
    CAMERA_EXIT_CB_FAILED,
    CAMERA_EXIT_CB_ABORT,
    CAMERA_EVT_CB_FRAME,
    CAMERA_EVT_CB_CAPTURE_FRAME_DONE,
    CAMERA_EVT_CB_SNAPSHOT_DONE,
    CAMERA_EVT_CB_FD,
    CAMERA_EVT_CB_FOCUS_MOVE,
    CAMERA_EVT_CB_FLUSH,
    CAMERA_EVT_CB_ZSL_FRM,
    CAMERA_EXIT_CB_PREPARE, /* prepared to be executed      */
    CAMERA_EVT_CB_SNAPSHOT_JPEG_DONE,
    CAMERA_EVT_CB_RESUME,
    CAMERA_EVT_CB_AE_STAB_NOTIFY,
    CAMERA_EVT_CB_AE_LOCK_NOTIFY,
    CAMERA_EVT_CB_AE_UNLOCK_NOTIFY,
    CAMERA_EVT_CB_RETURN_ZSL_BUF,
    CAMERA_EVT_SENSOR_DATATYPE,
    CAMERA_EVT_HDR_PLUS,
    CAMERA_EVT_CB_AE_FLASH_FIRED,
    CAMERA_CB_TYPE_MAX
};

enum camera_func_type {
    CAMERA_FUNC_START,
    CAMERA_FUNC_STOP,
    CAMERA_FUNC_START_PREVIEW,
    CAMERA_FUNC_TAKE_PICTURE,
    CAMERA_FUNC_ENCODE_PICTURE,
    CAMERA_FUNC_START_FOCUS, /*5*/
    CAMERA_FUNC_STOP_PREVIEW,
    CAMERA_FUNC_RELEASE_PICTURE,
    CAMERA_FUNC_AE_STATE_CALLBACK,
    CAMERA_FUNC_SENSOR_DATATYPE,
    CAMERA_FUNC_TYPE_MAX
};

enum takepicture_mode {
    CAMERA_NORMAL_MODE = 0,
    CAMERA_ZSL_MODE,
    CAMERA_ISP_TUNING_MODE,
    CAMERA_LLS_MUTLI_FRAME_MODE,
    CAMERA_UTEST_MODE,
    CAMERA_AUTOTEST_MODE,
    CAMERA_ISP_SIMULATION_MODE,
    TAKEPICTURE_MODE_MAX
};

enum preview_buffer_usage_mode {
    PREVIEW_BUFFER_USAGE_DCAM = 0,
    PREVIEW_BUFFER_USAGE_GRAPHICS,
    PREVIEW_BUFFER_USAGE_MODE_MAX,
};

enum camera_param_type {
    CAMERA_PARAM_ZOOM = 0,
    CAMERA_PARAM_ENCODE_ROTATION,
    CAMERA_PARAM_CONTRAST,
    CAMERA_PARAM_BRIGHTNESS,
    CAMERA_PARAM_SHARPNESS,
    CAMERA_PARAM_WB,
    CAMERA_PARAM_EFFECT,
    CAMERA_PARAM_FLASH,
    CAMERA_PARAM_ANTIBANDING,
    CAMERA_PARAM_FOCUS_RECT,
    CAMERA_PARAM_AF_MODE, /*10*/
    CAMERA_PARAM_AUTO_EXPOSURE_MODE,
    CAMERA_PARAM_ISO,
    CAMERA_PARAM_EXPOSURE_COMPENSATION,
    CAMERA_PARAM_PREVIEW_FPS,
    CAMERA_PARAM_PREVIEW_LLS_FPS,
    CAMERA_PARAM_SATURATION,
    CAMERA_PARAM_SCENE_MODE,
    CAMERA_PARAM_JPEG_QUALITY,
    CAMERA_PARAM_THUMB_QUALITY,
    CAMERA_PARAM_SENSOR_ORIENTATION,
    CAMERA_PARAM_FOCAL_LENGTH,
    CAMERA_PARAM_SENSOR_ROTATION,
    CAMERA_PARAM_PERFECT_SKIN_LEVEL,
    CAMERA_PARAM_FLIP_ON,
    CAMERA_PARAM_SHOT_NUM,
    CAMERA_PARAM_ROTATION_CAPTURE,
    CAMERA_PARAM_POSITION,
    CAMERA_PARAM_PREVIEW_SIZE,
    CAMERA_PARAM_RAW_CAPTURE_SIZE,
    CAMERA_PARAM_PREVIEW_FORMAT,
    CAMERA_PARAM_CAPTURE_SIZE,
    CAMERA_PARAM_CAPTURE_FORMAT,
    CAMERA_PARAM_CAPTURE_MODE,
    CAMERA_PARAM_THUMB_SIZE,
    CAMERA_PARAM_ANDROID_ZSL,
    CAMERA_PARAM_VIDEO_SIZE,
    CAMERA_PARAM_RANGE_FPS,
    CAMERA_PARAM_ISP_FLASH,
    CAMERA_PARAM_SPRD_ZSL_ENABLED,
    CAMERA_PARAM_ISP_AE_LOCK_UNLOCK,
    CAMERA_PARAM_SLOW_MOTION_FLAG,
    CAMERA_PARAM_SPRD_PIPVIV_ENABLED,
    CAMERA_PARAM_SPRD_HIGHISO_ENABLED,
    CAMERA_PARAM_SPRD_EIS_ENABLED,
    CAMERA_PARAM_REFOCUS_ENABLE,
    CAMERA_PARAM_TOUCH_XY,
    CAMERA_PARAM_VIDEO_SNAPSHOT_TYPE,
    CAMERA_PARAM_SPRD_3DCAL_ENABLE,
    CAMERA_PARAM_SPRD_BURSTMODE_ENABLED,
    CAMERA_PARAM_SPRD_YUV_CALLBACK_ENABLE,
    CAMERA_PARAM_UHD_RECORDING_ENABLED,
    CAMERA_PARAM_ISP_AWB_LOCK_UNLOCK,
    CAMERA_PARAM_AE_REGION,
    CAMERA_PARAM_SPRD_HDR_PLUS_ENABLED,
    CAMERA_PARAM_EXIF_MIME_TYPE,
    CAMERA_PARAM_SPRD_3DNR_ENABLED,
    CAMERA_PARAM_FILTER_TYPE,
    CAMERA_PARAM_AE_MODE,
    CAMERA_PARAM_EXPOSURE_TIME,
    CAMERA_PARAM_SENSITIVITY,
    CAMERA_PARAM_AF_BYPASS,
    CAMERA_PARAM_LENS_FOCUS_DISTANCE,
    CAMERA_PARAM_SPRD_ADJUST_FLASH_LEVEL,
    CAMERA_PARAM_SPRD_GET_FLASH_LEVEL,
    CAMERA_PARAM_TYPE_MAX
};

enum camera_data {
    CAMERA_PREVIEW_DATA = 0,
    CAMERA_SNAPSHOT_DATA,
    CAMERA_VIDEO_DATA,
    CAMERA_DATA_MAX
};

enum camera_preview_mode_type {
    CAMERA_PREVIEW_MODE_SNAPSHOT = 0,
    CAMERA_PREVIEW_MODE_MOVIE,
    CAMERA_PREVIEW_MODE_SLOWMOTION,
    CAMERA_MAX_PREVIEW_MODE
};

enum fast_ctrl_mode { CAMERA_FAST_MODE_FD = 0, CAMERA_FAST_MODE_MAX };

struct camera_face_info {
    cmr_u32 face_id;
    cmr_u32 sx;
    cmr_u32 sy;
    cmr_u32 srx;
    cmr_u32 sry;
    cmr_u32 ex;
    cmr_u32 ey;
    cmr_u32 elx;
    cmr_u32 ely;
    cmr_u32 brightness;
    cmr_u32 angle;
    cmr_u32 pose;
    cmr_u32 smile_level;
    cmr_u32 blink_level;
    cmr_u32 padding;
};

struct camera_jpeg_param {
    void *outPtr;
    cmr_u32 size;
    cmr_u32 need_free;
    cmr_u32 index;
    cmr_u32 reserved;
};

struct camera_sensor_info {
    cmr_s32 exposure_time_numerator;
    cmr_s32 exposure_time_denominator;
};

struct camera_frame_type {
    cmr_u32 format;
    cmr_u32 width;
    cmr_u32 height;
    cmr_u32 buf_id;
    cmr_u32 order_buf_id;
    cmr_u32 face_num;
    cmr_uint y_phy_addr;
    cmr_uint y_vir_addr;
    cmr_uint uv_phy_addr;
    cmr_uint uv_vir_addr;
    cmr_u32 fd;
    cmr_s64 timestamp;
    cmr_int status;
    cmr_int type;
    cmr_int lls_info;
    cmr_u32 need_free;
    struct camera_face_info face_info[FACE_DETECT_NUM];
    struct camera_jpeg_param jpeg_param;
    struct camera_sensor_info sensor_info;
    cmr_uint zsl_private;
    float zoom_ratio;
    cmr_s64 ae_time;
    cmr_s64 monoboottime;
    cmr_u32 vcm_step;
    cmr_int isMatchFlag;
};
/*
struct camera_cap_frm_info {
        cmr_uint                 y_virt_addr;
        cmr_uint                 u_virt_addr;
        cmr_u32                  width;
        cmr_u32                  height;
        cmr_s64                  timestamp;
        struct frm_info          frame_info;
};*/

struct camera_position_type {
    long timestamp;
    double latitude;
    double longitude;
    double altitude;
    const char *process_method;
};

struct cmr_focus_param {
    cmr_u32 zone_cnt;
    struct img_rect zone[FOCUS_ZONE_CNT_MAX];
};

typedef enum {
    MODE_SINGLE_CAMERA = 0,
    MODE_3D_VIDEO = 5, // Camera2 apk open  camera id is MODE_3D_VIDEO,camera
                       // hal transform to open physics Camera id is 1 and 3
    MODE_RANGE_FINDER, // Camera2 apk open  camera id is
                       // MODE_RANGE_FINDER,camera hal transform to open physics
                       // Camera id is 1 and 3
    MODE_3D_CAPTURE,   // Camera2 apk open  camera id is MODE_3D_CAPTURE,camera
                       // hal transform to open physics Camera id is 1 and 3
    MODE_3D_CALIBRATION = 8, // ValidationTools apk open  camera id is
                             // MODE_3D_CALIBRATION and 3 ,camera hal transform
                             // to open physics Camera id is 1 and 3
    MODE_REFOCUS = 9, // Camera2 apk open  camera id is MODE_REFOCUS and 2
                      // ,camera hal transform to open physics Camera id is 0
                      // and 2
    MODE_3D_PREVIEW = 10, // Camera2 apk open  camera id is
                          // MODE_3D_PREVIEW,camera hal transform to open
                          // physics Camera id is 1 and 3
    MODE_SOFY_OPTICAL_ZOOM = 11,
    MODE_BLUR = 12,
    MODE_SELF_SHOT = 13, // Camera2 apk open  camera id is MODE_SELF_SHOT,camera
                         // hal transform to open physics Camera id is 1 and 2
    MODE_PAGE_TURN = 14, // Camera2 apk open  camera id is MODE_PAGE_TURN,camera
                         // hal transform to open physics Camera id is 2
    MODE_BLUR_FRONT = 15,
    MODE_BOKEH = 16,
    MODE_SBS = 17,
    MODE_TUNING = 50,
    MODE_CAMERA_MAX
} multiCameraMode;
typedef enum {
    MODE_3D_VIDEO_ID = 5, // Camera2 apk open  camera id is MODE_3D_VIDEO,camera
                          // hal transform to open physics Camera id is 1 and 3
    MODE_RANGE_FINDER_ID = 6, // Camera2 apk open  camera id is
    // MODE_RANGE_FINDER,camera hal transform to open physics
    // Camera id is 1 and 3
    MODE_3D_CAPTURE_ID =
        7, // Camera2 apk open  camera id is MODE_3D_CAPTURE,camera
           // hal transform to open physics Camera id is 1 and 3
    MODE_3D_CALIBRATION_ID = 8, // ValidationTools apk open  camera id is
    // MODE_3D_CALIBRATION and 3 ,camera hal transform
    // to open physics Camera id is 1/0 and 3
    MODE_REFOCUS_ID = 9, // Camera2 apk open  camera id is MODE_REFOCUS and 2
                         // ,camera hal transform to open physics Camera id is 0
                         // and 2
    MODE_3D_PREVIEW_ID = 10, // Camera2 apk open  camera id is
                             // MODE_3D_PREVIEW,camera hal transform to open
                             // physics Camera id is 1 and 3
    MODE_SOFY_OPTICAL_ZOOM_ID = 11,
    MODE_BLUR_ID = 12,
    MODE_SELF_SHOT_ID =
        13, // Camera2 apk open  camera id is MODE_SELF_SHOT,camera
            // hal transform to open physics Camera id is 1 and 2
    MODE_PAGE_TURN_ID =
        14, // Camera2 apk open  camera id is MODE_PAGE_TURN,camera
            // hal transform to open physics Camera id is 2
    MODE_BLUR_FRONT_ID = 15,

    MODE_CAMERA_ID_MAX
} multiCameraId; // hal and app interforce

struct img_sbs_info {
    cmr_u32 sbs_mode;
    struct img_size size;
};

struct isp_awb_info {
    cmr_u32 r_gain;
    cmr_u32 g_gain;
    cmr_u32 b_gain;
    cmr_u32 r_offset;
    cmr_u32 g_offset;
    cmr_u32 b_offset;
};

struct tuning_param_info {
    cmr_u32 pos; // VCM position
    cmr_u32 gain;
    cmr_u32 shutter;
    cmr_int bv;
    struct isp_awb_info awb_info;
};

struct isp_af_otp_info {
    cmr_u16 infinite_cali;
    cmr_u16 macro_cali;
};

typedef enum {
    CAMERA_IOCTRL_SET_MULTI_CAMERAMODE = 0,
    CAMERA_IOCTRL_GET_SENSOR_LUMA,
    CAMERA_IOCTRL_COVERED_SENSOR_STREAM_CTRL,
    CAMERA_IOCTRL_GET_FULLSCAN_INFO,
    CAMERA_IOCTRL_SET_AF_POS,
    CAMERA_IOCTRL_SET_3A_BYPASS,
    CAMERA_IOCTRL_GET_AE_FPS,
    CAMERA_IOCTRL_3DNR_VIDEOMODE,
    CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP,
    CAMERA_IOCTRL_GET_MICRODEPTH_PARAM,
    CAMERA_IOCTRL_SET_MICRODEPTH_DEBUG_INFO,
    CAMERA_IOCTRL_GET_SENSOR_FORMAT,
    CAMERA_IOCTRL_GET_CPP_CAPABILITY,
    CAMERA_IOCTRL_THUMB_YUV_PROC,
    CAMERA_IOCTRL_JPEG_ENCODE_EXIF_PROC,
    CAMERA_IOCTRL_SET_MIME_TYPE,
    CAMERA_IOCTRL_SET_CAPTURE_FACE_BEAUTIFY,
    CAMERA_IOCTRL_GET_BLUR_COVERED,
    CAMERA_IOCTRL_DEBUG_IMG,
    CAMERA_IOCTRL_GET_GRAB_CAPABILITY,
    CAMERA_IOCTRL_CMD_MAX
} cmr_ioctr_cmd;

typedef void (*camera_cb_of_type)(enum camera_cb_type cb,
                                  const void *client_data,
                                  enum camera_func_type func, void *parm4);

typedef cmr_int (*camera_cb_of_malloc)(enum camera_mem_cb_type type,
                                       cmr_u32 *size_ptr, cmr_u32 *sum_ptr,
                                       cmr_uint *phy_addr, cmr_uint *vir_addr,
                                       cmr_s32 *mfd, void *private_data);
typedef cmr_int (*camera_cb_of_free)(enum camera_mem_cb_type type,
                                     cmr_uint *phy_addr, cmr_uint *vir_addr,
                                     cmr_s32 *fd, cmr_u32 sum,
                                     void *private_data);
typedef cmr_int (*camera_cb_of_gpu_malloc)(
    enum camera_mem_cb_type type, cmr_u32 *size_ptr, cmr_u32 *sum_ptr,
    cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_s32 *mfd, void **handle,
    cmr_uint *width, cmr_uint *height, void *private_data);
typedef struct oem_ops {
    cmr_int (*camera_init)(cmr_u32 camera_id, camera_cb_of_type callback,
                           void *client_data, cmr_uint is_autotest,
                           cmr_handle *camera_handle, void *cb_of_malloc,
                           void *cb_of_free);

    cmr_int (*camera_deinit)(cmr_handle camera_handle);

    cmr_int (*camera_release_frame)(cmr_handle camera_handle,
                                    enum camera_data data, cmr_uint index);

    cmr_int (*camera_set_param)(cmr_handle camera_handle,
                                enum camera_param_type id, cmr_uint param);

    cmr_int (*camera_start_preview)(cmr_handle camera_handle,
                                    enum takepicture_mode mode);

    cmr_int (*camera_stop_preview)(cmr_handle camera_handle);

    cmr_int (*camera_start_autofocus)(cmr_handle camera_handle);

    cmr_int (*camera_cancel_autofocus)(cmr_handle camera_handle);

    cmr_int (*camera_cancel_takepicture)(cmr_handle camera_handle);

    uint32_t (*camera_safe_scale_th)(void);

    cmr_int (*camera_take_picture)(cmr_handle camera_handle,
                                   enum takepicture_mode cap_mode);

    cmr_int (*camera_get_sn_trim)(cmr_handle camera_handle, cmr_u32 mode,
                                  cmr_uint *trim_x, cmr_uint *trim_y,
                                  cmr_uint *trim_w, cmr_uint *trim_h,
                                  cmr_uint *width, cmr_uint *height);

    cmr_int (*camera_set_mem_func)(cmr_handle camera_handle, void *cb_of_malloc,
                                   void *cb_of_free, void *private_data);

    cmr_int (*camera_get_redisplay_data)(
        cmr_handle camera_handle, cmr_s32 output_fd, cmr_uint output_addr,
        cmr_uint output_vir_addr, cmr_uint output_width, cmr_uint output_height,
        cmr_s32 input_fd, cmr_uint input_addr_y, cmr_uint input_addr_uv,
        cmr_uint input_vir_addr, cmr_uint input_width, cmr_uint input_height);

    cmr_int (*camera_is_change_size)(cmr_handle camera_handle,
                                     cmr_u32 cap_width, cmr_u32 cap_height,
                                     cmr_u32 preview_width,
                                     cmr_u32 preview_height,
                                     cmr_u32 video_width, cmr_u32 video_height,
                                     cmr_uint *is_change);
#if defined(CONFIG_ISP_2_1) || defined(CONFIG_CAMERA_ISP_DIR_2_4) ||           \
    defined(CONFIG_ISP_2_3) || defined(CONFIG_ISP_2_2)
    int (*camera_get_postprocess_capture_size)(cmr_u32 camera_id,
                                               cmr_u32 *mem_size);
#else
    int (*camera_pre_capture_get_buffer_id)(cmr_u32 camera_id, cmr_u16 width,
                                            cmr_u16 height);

    int (*camera_get_reserve_buffer_size)(cmr_u32 camera_id,
                                          cmr_s32 mem_size_id,
                                          cmr_u32 *mem_size, cmr_u32 *mem_sum);

    int (*camera_pre_capture_get_buffer_size)(cmr_u32 camera_id,
                                              cmr_s32 mem_size_id,
                                              cmr_u32 *mem_size,
                                              cmr_u32 *mem_sum);
#endif
    cmr_int (*camera_get_preview_rect)(cmr_handle camera_handle,
                                       cmr_uint *rect_x, cmr_uint *rect_y,
                                       cmr_uint *rect_width,
                                       cmr_uint *rect_height);

    cmr_int (*camera_get_zsl_capability)(cmr_handle camera_handle,
                                         cmr_uint *is_support,
                                         cmr_uint *max_widht,
                                         cmr_uint *max_height);

    cmr_int (*camera_get_sensor_info_for_raw)(
        cmr_handle camera_handle, struct sensor_mode_info *mode_info);

    cmr_int (*camera_get_sensor_trim)(cmr_handle camera_handle,
                                      struct img_rect *sn_trim);

    cmr_int (*camera_get_sensor_trim2)(cmr_handle camera_handle,
                                       struct img_rect *sn_trim);

    cmr_uint (*camera_get_preview_rot_angle)(cmr_handle camera_handle);

    void (*camera_fd_enable)(cmr_handle camera_handle, cmr_u32 is_enable);
    void (*camera_flip_enable)(cmr_handle camera_handle, cmr_u32 param);

    void (*camera_fd_start)(cmr_handle camera_handle, cmr_u32 param);

    cmr_int (*camera_is_need_stop_preview)(cmr_handle camera_handle);

    cmr_int (*camera_takepicture_process)(cmr_handle camera_handle,
                                          cmr_uint src_phy_addr,
                                          cmr_uint src_vir_addr, cmr_u32 width,
                                          cmr_u32 height);

    uint32_t (*camera_get_size_align_page)(uint32_t size);

    cmr_int (*camera_fast_ctrl)(cmr_handle camera_handle,
                                enum fast_ctrl_mode mode, cmr_u32 param);

    cmr_int (*camera_start_preflash)(cmr_handle camera_handle);

    cmr_int (*camera_get_viewangle)(cmr_handle camera_handle,
                                    struct sensor_view_angle *view_angle);

    cmr_uint (*camera_get_sensor_exif_info)(cmr_handle camera_handle,
                                            struct exif_info *exif_info);
    cmr_uint (*camera_get_sensor_result_exif_info)(
        cmr_handle camera_handle,
        struct exif_spec_pic_taking_cond_tag *exif_pic_info);
    cmr_s32 (*camera_get_iommu_status)(cmr_handle camera_handle);
    cmr_int (*camera_set_preview_buffer)(cmr_handle camera_handle,
                                         cmr_uint src_phy_addr,
                                         cmr_uint src_vir_addr, cmr_s32 fd);
    cmr_int (*camera_set_video_buffer)(cmr_handle camera_handle,
                                       cmr_uint src_phy_addr,
                                       cmr_uint src_vir_addr, cmr_s32 fd);
    cmr_int (*camera_set_zsl_buffer)(cmr_handle camera_handle,
                                     cmr_uint src_phy_addr,
                                     cmr_uint src_vir_addr, cmr_s32 fd);
    cmr_int (*camera_set_video_snapshot_buffer)(cmr_handle camera_handle,
                                                cmr_uint src_phy_addr,
                                                cmr_uint src_vir_addr,
                                                cmr_s32 fd);
    cmr_int (*camera_set_zsl_snapshot_buffer)(cmr_handle camera_handle,
                                              cmr_uint src_phy_addr,
                                              cmr_uint src_vir_addr,
                                              cmr_s32 fd);
    cmr_int (*camera_zsl_snapshot_need_pause)(cmr_handle camera_handle,
                                              cmr_int *flag);
    cmr_int (*camera_get_isp_handle)(cmr_handle camera_handle,
                                     cmr_handle *isp_handle);
    void (*camera_lls_enable)(cmr_handle camera_handle, cmr_u32 is_enable);
    cmr_int (*camera_is_lls_enabled)(cmr_handle camera_handle);
    void (*camera_vendor_hdr_enable)(cmr_handle camera_handle,
                                     cmr_u32 is_enable);
    cmr_int (*camera_is_vendor_hdr)(cmr_handle camera_handle);

    void (*camera_set_lls_shot_mode)(cmr_handle camera_handle,
                                     cmr_u32 is_enable);
    cmr_int (*camera_get_lls_shot_mode)(cmr_handle camera_handle);
    cmr_int (*camera_get_isp_info)(cmr_handle camera_handle, void **addr,
                                   int *size);

    void (*camera_start_burst_notice)(cmr_handle camera_handle);
    void (*camera_end_burst_notice)(cmr_handle camera_handle);

    cmr_int (*camera_transfer_caf_to_af)(cmr_handle camera_handle);
    cmr_int (*camera_transfer_af_to_caf)(cmr_handle camera_handle);

    cmr_int (*dump_jpeg_file)(void *virt_addr, unsigned int size, int width,
                              int height);

    cmr_int (*camera_get_gain_thrs)(cmr_handle camera_handle,
                                    cmr_u32 *is_over_thrs);
    cmr_int (*camera_set_sensor_info_to_af)(
        cmr_handle camera_handle, struct cmr_af_aux_sensor_info *sensor_info);
    cmr_int (*camera_get_sensor_max_fps)(cmr_handle camera_handle,
                                         cmr_u32 camera_id, cmr_u32 *max_fps);
    cmr_int (*camera_snapshot_is_need_flash)(cmr_handle oem_handle,
                                             cmr_u32 camera_id,
                                             cmr_u32 *is_need_flash);
    cmr_uint (*camera_get_sensor_otp_info)(
        cmr_handle camera_handle, struct sensor_otp_cust_info *otp_info);
#if defined(CONFIG_ISP_2_1)
    cmr_uint (*camera_get_online_buffer)(cmr_handle camera_handle,
                                         void *cali_info);
#endif
    cmr_uint (*camera_get_sensor_vcm_step)(cmr_handle camera_handle,
                                           cmr_u32 camera_id,
                                           cmr_u32 *vcm_step);
#if defined(CONFIG_CAMERA_ISP_DIR_3)
    cmr_int (*camera_stop_multi_layer)(cmr_handle camera_handle);
#endif
    cmr_int (*camera_set_sensor_close_flag)(cmr_handle camera_handle);
    cmr_int (*camera_set_reprocess_picture_size)(
        cmr_handle camera_handle, cmr_uint is_reprocessing, cmr_u32 camera_id,
        cmr_u32 width,
        cmr_u32
            height); /**add for 3d capture to reset reprocessing capture size*/
#if defined(CONFIG_CAMERA_ISP_DIR_2_1) || defined(CONFIG_CAMERA_ISP_DIR_2_4)
    cmr_int (*camera_start_capture)(cmr_handle camera_handle);
    cmr_int (*camera_stop_capture)(cmr_handle camera_handle);
#endif

#if defined(CONFIG_ISP_2_1) || defined(CONFIG_CAMERA_ISP_DIR_2_4) ||           \
    defined(CONFIG_ISP_2_3) || defined(CONFIG_ISP_2_2)
    cmr_int (*camera_set_largest_picture_size)(cmr_u32 camera_id, cmr_u16 width,
                                               cmr_u16 height);
#else
    cmr_int (*camera_pre_capture_set_buffer_size)(cmr_u32 camera_id,
                                                  cmr_u16 width,
                                                  cmr_u16 height);
#endif
    cmr_int (*camera_ioctrl)(cmr_handle handle, int cmd, void *param);

    cmr_int (*camera_reprocess_yuv_for_jpeg)(cmr_handle camera_handle,
                                             enum takepicture_mode cap_mode,
                                             struct frm_info *frm_data);

#if defined(CONFIG_ISP_2_1) || defined(CONFIG_CAMERA_ISP_DIR_2_4)
    cmr_int (*camera_get_focus_point)(cmr_handle camera_handle,
                                      cmr_s32 *point_x, cmr_s32 *point_y);
    cmr_s32 (*camera_isp_sw_check_buf)(cmr_handle camera_handle,
                                       cmr_uint *param_ptr);
    cmr_int (*camera_isp_sw_proc)(cmr_handle camera_handle,
                                  struct soft_isp_frm_param *param_ptr);
    cmr_int (*camera_raw_post_proc)(cmr_handle camera_handle,
                                    struct img_frm *raw_buff,
                                    struct img_frm *yuv_buff,
                                    struct img_sbs_info *sbs_info);
    cmr_int (*camera_get_tuning_param)(cmr_handle camera_handle,
                                       struct tuning_param_info *tuning_info);
#endif
#if defined(CONFIG_ISP_2_3)
    cmr_int (*camera_set_gpu_mem_ops)(cmr_handle camera_handle,
                                      void *cb_of_malloc, void *cb_of_free);
#endif
#ifdef CAMERA_SUPPORT_ROLLING_SHUTTER_SKEW // Added only for HAL3_2v1 and oem2v1
    cmr_int (*camera_get_rolling_shutter)(cmr_handle camera_handle,
                                          cmr_s64 *rolling_shutter_skew);
#endif
} oem_ops_t;

typedef struct oem_module {
    uint32_t tag;

    /** Modules methods */
    oem_ops_t *ops;

    /** module's dso */
    void *dso;

} oem_module_t;

/*sw 3dnr */
struct threednr_pre_miscinfo {
    cmr_uint small_width;
    cmr_uint small_height;
    cmr_uint width;
    cmr_uint height;
    unsigned long *small_buf_phy;
    unsigned long *small_buf_vir;
    cmr_s32 *small_buf_fd;
};

/**
* Name of the hal_module_info
*/
#define OEM_MODULE_INFO_SYM OMI

/**
* Name of the hal_module_info as a string
*/
#define OEM_MODULE_INFO_SYM_AS_STR "OMI"

/**
* Name of the hal_module_info
*/
//#define OEM_MODULE_ISP_CALI_INFO_SYM    OMICI

/**
* Name of the hal_module_info as a string
*/
#define OEM_MODULE_ISP_CALI_INFO_SYM_AS_STR "OMICI"

#ifdef __cplusplus
}
#endif

#endif // for _CMR_COMMON_H_
