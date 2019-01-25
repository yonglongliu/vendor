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
#define LOG_TAG "cmr_mem"

#include "cmr_mem.h"
#include "cmr_oem.h"
#include <unistd.h>
#include "cutils/properties.h"

/*
          to add more.........
     8M-----------16M-------------8M
     5M-----------16M-------------0M
     3M-----------8M--------------2M
     2M-----------4M--------------2M
     1.3M---------4M--------------1M
*/

#define PIXEL_1P3_MEGA 0x180000   // actually 1.5 *1024*1024
#define PIXEL_2P0_MEGA 0x200000   // actually 2.0 *1024*1024
#define PIXEL_3P0_MEGA 0x300000   // actually 3.0 *1024*1024
#define PIXEL_4P0_MEGA 0x400000   // actually 3.0 *1024*1024
#define PIXEL_5P0_MEGA 0x500000   // 5.0 *1024*1024
#define PIXEL_6P0_MEGA 0x600000   // 6.0 *1024*1024
#define PIXEL_7P0_MEGA 0x700000   // 7.0 *1024*1024
#define PIXEL_8P0_MEGA 0x800000   // 8.0 *1024*1024
#define PIXEL_9P0_MEGA 0x900000   // 9.0 *1024*1024
#define PIXEL_AP0_MEGA 0xA00000   // 10.0 *1024*1024
#define PIXEL_BP0_MEGA 0xB00000   // 11.0 *1024*1024
#define PIXEL_CP0_MEGA 0xC00000   // 12.0 *1024*1024
#define PIXEL_DP0_MEGA 0xD00000   // 13.0 *1024*1024
#define PIXEL_10P0_MEGA 0x1000000 // 16.0 *1024*1024
#define PIXEL_15P0_MEGA 0x1500000 // 21.0 *1024*1024
#define MIN_GAP_LINES 0x80
#define ALIGN_PAGE_SIZE (1 << 12)

#define ISP_YUV_TO_RAW_GAP CMR_SLICE_HEIGHT
#define BACK_CAMERA_ID 0
#define FRONT_CAMERA_ID 1
#define DEV2_CAMERA_ID 2
#define DEV3_CAMERA_ID 3
#define JPEG_SMALL_SIZE (300 * 1024)
#define ADDR_BY_WORD(a) (((a) + 3) & (~3))
#define CMR_NO_MEM(a, b)                                                       \
    do {                                                                       \
        if ((a) > (b)) {                                                       \
            CMR_LOGE("No memory, 0x%x 0x%x", (a), (b));                        \
            return -1;                                                         \
        }                                                                      \
    } while (0)

enum {
    IMG_1P3_MEGA = 0,
    IMG_2P0_MEGA,
    IMG_3P0_MEGA,
    IMG_4P0_MEGA,
    IMG_5P0_MEGA,
    IMG_6P0_MEGA,
    IMG_7P0_MEGA,
    IMG_8P0_MEGA,
    IMG_9P0_MEGA,
    IMG_AP0_MEGA,
    IMG_BP0_MEGA,
    IMG_CP0_MEGA,
    IMG_DP0_MEGA,
    IMG_10P0_MEGA,
    IMG_15P0_MEGA,
    IMG_SIZE_NUM
};

enum {
    JPEG_TARGET = 0,
    THUM_YUV,
    THUM_JPEG,

    BUF_TYPE_NUM
};

typedef uint32_t (*cmr_get_size)(uint32_t width, uint32_t height,
                                 uint32_t thum_width, uint32_t thum_height);

struct cap_size_to_mem {
    uint32_t pixel_num;
    uint32_t mem_size;
};

static const struct cap_size_to_mem back_cam_mem_size_tab[IMG_SIZE_NUM] = {
    {PIXEL_1P3_MEGA, (20 << 20)}, {PIXEL_2P0_MEGA, (20 << 20)},
    {PIXEL_3P0_MEGA, (20 << 20)}, {PIXEL_4P0_MEGA, (20 << 20)},
    {PIXEL_5P0_MEGA, (20 << 20)}, {PIXEL_6P0_MEGA, (35 << 20)},
    {PIXEL_7P0_MEGA, (35 << 20)}, {PIXEL_8P0_MEGA, (35 << 20)},
    {PIXEL_9P0_MEGA, (45 << 20)}, {PIXEL_AP0_MEGA, (45 << 20)},
    {PIXEL_BP0_MEGA, (45 << 20)}, {PIXEL_CP0_MEGA, (45 << 20)},
    {PIXEL_DP0_MEGA, (45 << 20)}, {PIXEL_10P0_MEGA, (55 << 20)},
    {PIXEL_15P0_MEGA, (65 << 20)}};

/* for whale2, how to calculate cap size(raw capture):
* target_yuv = w * h * 3 / 2;
* cap_yuv = w * h * 3 / 2;
* cap_raw = w * h * 3 / 2, resue target_yuv or cap_yuv
* cap_raw2 = w * h * 3 / 2 * 2, altek raw, for tuning
* thum_yuv = thumW * thumH * 3 / 2; for thumbnail yuv
* thum_jpeg = thumW * thumH * 3 / 2; for thumbnail jpeg
* target_jpeg, resue
* so, the total cap size is:
* w * h * 3 / 2 + w * h * 3 / 2 + w * h * 3 / 2 * 2 + thumW * thumH * 3 = 6 * w
* * h + thumW * thumH * 3(bytes);
*/
static const struct cap_size_to_mem back_cam_raw_mem_size_tab[IMG_SIZE_NUM] = {
    {PIXEL_1P3_MEGA, (45 << 20)},  {PIXEL_2P0_MEGA, (45 << 20)},
    {PIXEL_3P0_MEGA, (45 << 20)},  {PIXEL_4P0_MEGA, (45 << 20)},
    {PIXEL_5P0_MEGA, (45 << 20)},  {PIXEL_6P0_MEGA, (50 << 20)},
    {PIXEL_7P0_MEGA, (50 << 20)},  {PIXEL_8P0_MEGA, (50 << 20)},
    {PIXEL_9P0_MEGA, (78 << 20)},  {PIXEL_AP0_MEGA, (78 << 20)},
    {PIXEL_BP0_MEGA, (78 << 20)},  {PIXEL_CP0_MEGA, (78 << 20)},
    {PIXEL_DP0_MEGA, (82 << 20)},  {PIXEL_10P0_MEGA, (95 << 20)},
    {PIXEL_15P0_MEGA, (125 << 20)}};

static const struct cap_size_to_mem front_cam_mem_size_tab[IMG_SIZE_NUM] = {
    {PIXEL_1P3_MEGA, (8 << 20)},  {PIXEL_2P0_MEGA, (10 << 20)},
    {PIXEL_3P0_MEGA, (15 << 20)}, {PIXEL_4P0_MEGA, (20 << 20)},
    {PIXEL_5P0_MEGA, (22 << 20)}, {PIXEL_6P0_MEGA, (22 << 20)},
    {PIXEL_7P0_MEGA, (25 << 20)}, {PIXEL_8P0_MEGA, (26 << 20)}};
static const struct cap_size_to_mem front_cam_raw_mem_size_tab[IMG_SIZE_NUM] = {
    {PIXEL_1P3_MEGA, (50 << 20)}, {PIXEL_2P0_MEGA, (50 << 20)},
    {PIXEL_3P0_MEGA, (50 << 20)}, {PIXEL_4P0_MEGA, (50 << 20)},
    {PIXEL_5P0_MEGA, (50 << 20)}, {PIXEL_6P0_MEGA, (50 << 20)},
    {PIXEL_7P0_MEGA, (50 << 20)}, {PIXEL_8P0_MEGA, (50 << 20)}};

static const struct cap_size_to_mem Stereo_video_mem_size_tab[IMG_SIZE_NUM] = {
    {PIXEL_1P3_MEGA, (25 << 20)}, {PIXEL_2P0_MEGA, (25 << 20)},
    {PIXEL_3P0_MEGA, (25 << 20)}, {PIXEL_4P0_MEGA, (25 << 20)},
    {PIXEL_5P0_MEGA, (25 << 20)}, {PIXEL_6P0_MEGA, (25 << 20)},
    {PIXEL_7P0_MEGA, (25 << 20)}, {PIXEL_8P0_MEGA, (25 << 20)}};

/*for ATV*/
static const struct cap_size_to_mem mem_size_tab[IMG_SIZE_NUM] = {
    {PIXEL_1P3_MEGA, (5 << 20)},  {PIXEL_2P0_MEGA, (9 << 20)},
    {PIXEL_3P0_MEGA, (18 << 20)}, {PIXEL_4P0_MEGA, (18 << 20)},
    {PIXEL_5P0_MEGA, (19 << 20)}, {PIXEL_6P0_MEGA, (19 << 20)},
    {PIXEL_7P0_MEGA, (20 << 20)}, {PIXEL_8P0_MEGA, (20 << 20)}};

static const struct cap_size_to_mem reserve_mem_size_tab[IMG_SIZE_NUM] = {
    {PIXEL_1P3_MEGA, (3 << 20)},  {PIXEL_2P0_MEGA, (3 << 20)},
    {PIXEL_3P0_MEGA, (5 << 20)},  {PIXEL_4P0_MEGA, (6 << 20)},
    {PIXEL_5P0_MEGA, (8 << 20)},  {PIXEL_6P0_MEGA, (9 << 20)},
    {PIXEL_7P0_MEGA, (11 << 20)}, {PIXEL_8P0_MEGA, (12 << 20)},
    {PIXEL_9P0_MEGA, (14 << 20)}, {PIXEL_AP0_MEGA, (15 << 20)},
    {PIXEL_BP0_MEGA, (17 << 20)}, {PIXEL_CP0_MEGA, (18 << 20)},
    {PIXEL_DP0_MEGA, (20 << 20)}};
static multiCameraMode is_multi_camera_mode_mem;
extern int camera_get_is_noscale(void);

static uint32_t get_jpeg_size(uint32_t width, uint32_t height,
                              uint32_t thum_width, uint32_t thum_height);
static uint32_t get_thum_yuv_size(uint32_t width, uint32_t height,
                                  uint32_t thum_width, uint32_t thum_height);
static uint32_t get_thum_jpeg_size(uint32_t width, uint32_t height,
                                   uint32_t thum_width, uint32_t thum_height);
static uint32_t get_jpg_tmp_size(uint32_t width, uint32_t height,
                                 uint32_t thum_width, uint32_t thum_height);
static uint32_t get_scaler_tmp_size(uint32_t width, uint32_t height,
                                    uint32_t thum_width, uint32_t thum_height);
static uint32_t get_isp_tmp_size(uint32_t width, uint32_t height,
                                 uint32_t thum_width, uint32_t thum_height);

static int arrange_raw_buf(struct cmr_cap_2_frm *cap_2_frm,
                           struct img_size *sn_size, struct img_rect *sn_trim,
                           struct img_size *image_size, uint32_t orig_fmt,
                           struct img_size *cap_size,
                           struct img_size *thum_size,
                           struct cmr_cap_mem *capture_mem, uint32_t need_rot,
                           uint32_t *io_mem_res, uint32_t *io_mem_end,
                           uint32_t *io_channel_size);

static int arrange_jpeg_buf(struct cmr_cap_2_frm *cap_2_frm,
                            struct img_size *sn_size, struct img_rect *sn_trim,
                            struct img_size *image_size, uint32_t orig_fmt,
                            struct img_size *cap_size,
                            struct img_size *thum_size,
                            struct cmr_cap_mem *capture_mem, uint32_t need_rot,
                            uint32_t *io_mem_res, uint32_t *io_mem_end,
                            uint32_t *io_channel_size);

static int arrange_yuv_buf(struct cmr_cap_2_frm *cap_2_frm,
                           struct img_size *sn_size, struct img_rect *sn_trim,
                           struct img_size *image_size, uint32_t orig_fmt,
                           struct img_size *cap_size,
                           struct img_size *thum_size,
                           struct cmr_cap_mem *capture_mem, uint32_t need_rot,
                           uint32_t *io_mem_res, uint32_t *io_mem_end,
                           uint32_t *io_channel_size);

static int arrange_misc_buf(struct cmr_cap_2_frm *cap_2_frm,
                            struct img_size *sn_size, struct img_rect *sn_trim,
                            struct img_size *image_size, uint32_t orig_fmt,
                            struct img_size *cap_size,
                            struct img_size *thum_size,
                            struct cmr_cap_mem *capture_mem, uint32_t need_rot,
                            uint32_t *io_mem_res, uint32_t *io_mem_end,
                            uint32_t *io_channel_size);

static int arrange_rot_buf(struct cmr_cap_2_frm *cap_2_frm,
                           struct img_size *sn_size, struct img_rect *sn_trim,
                           struct img_size *image_size, uint32_t orig_fmt,
                           struct img_size *cap_size,
                           struct img_size *thum_size,
                           struct cmr_cap_mem *capture_mem, uint32_t need_rot,
                           uint32_t need_scale, uint32_t *io_mem_res,
                           uint32_t *io_mem_end, uint32_t *io_channel_size);

static const cmr_get_size get_size[BUF_TYPE_NUM] = {
    get_jpeg_size, get_thum_yuv_size, get_thum_jpeg_size,

};

#define SENSOR_MAX_NUM 6

static cmr_u16 max_sensor_width[SENSOR_MAX_NUM];
static cmr_u16 max_sensor_height[SENSOR_MAX_NUM];

int camera_pre_capture_sensor_size_set(cmr_u32 camera_id, cmr_u16 width,
                                       cmr_u16 height) {
    max_sensor_width[camera_id] = width;
    max_sensor_height[camera_id] = height;

    return 0;
}

int camera_pre_capture_sensor_size_get(cmr_u32 camera_id, cmr_u16 *width,
                                       cmr_u16 *height) {
    *width = max_sensor_width[camera_id];
    *height = max_sensor_height[camera_id];

    return 0;
}
int camera_pre_capture_buf_id(cmr_u32 camera_id, cmr_u16 width,
                              cmr_u16 height) {

    UNUSED(camera_id);

    int buffer_id = 0;

    CMR_LOGI("camera_pre_capture_buf_id width = %d, height = %d", width,
             height);
    if ((width * height <= 5312 * 3984) &&
        (width * height > 4608 * 3456)) { // 21M
        buffer_id = IMG_15P0_MEGA;
    } else if ((width * height <= 4608 * 3456) &&
               (width * height > 4160 * 3120)) { // 16M
        buffer_id = IMG_10P0_MEGA;
    } else if ((width * height <= 4160 * 3120) &&
               (width * height > 3264 * 2448)) { // 13M
        buffer_id = IMG_DP0_MEGA;
    } else if ((width * height <= 3264 * 2448) &&
               (width * height > 2592 * 1944)) { // 8M
        buffer_id = IMG_8P0_MEGA;
    } else if ((width * height <= 2592 * 1944) &&
               (width * height > 1600 * 1200)) { // 5M
        buffer_id = IMG_5P0_MEGA;
    } else if (width * height <= 1600 * 1200) { // 2M
        buffer_id = IMG_2P0_MEGA;
    } else {
        CMR_LOGI("unsupport buffer id = %d", buffer_id);
    }

    CMR_LOGI("buffer id = %d", buffer_id);

    return buffer_id;
}

#if 0
int camera_pre_capture_buf_id(cmr_u32 camera_id)
{
	UNUSED(camera_id);

	int buffer_id = 0;

	if (FRONT_CAMERA_ID == camera_id || DEV3_CAMERA_ID == camera_id) {
#if defined(CONFIG_FRONT_CAMERA_SUPPORT_13M)
		buffer_id = IMG_DP0_MEGA;
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_8M)
		buffer_id = IMG_8P0_MEGA;
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_7M)
		buffer_id = IMG_7P0_MEGA;
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_6M)
		buffer_id = IMG_6P0_MEGA;
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_5M)
		buffer_id = IMG_5P0_MEGA;
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_4M)
		buffer_id = IMG_4P0_MEGA;
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_3M)
		buffer_id = IMG_3P0_MEGA;
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_2M)
		buffer_id = IMG_2P0_MEGA;
#else
		buffer_id = IMG_2P0_MEGA;
#endif
	} else if (BACK_CAMERA_ID == camera_id  || DEV2_CAMERA_ID == camera_id) {
#if defined(CONFIG_CAMERA_SUPPORT_21M)
		buffer_id = IMG_15P0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_16M)
		buffer_id = IMG_10P0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_13M)
		buffer_id = IMG_DP0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_8M)
		buffer_id = IMG_8P0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_7M)
		buffer_id = IMG_7P0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_6M)
		buffer_id = IMG_6P0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_5M)
		buffer_id = IMG_5P0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_4M)
		buffer_id = IMG_4P0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_3M)
		buffer_id = IMG_3P0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_2M)
		buffer_id = IMG_2P0_MEGA;
#else
		buffer_id = IMG_2P0_MEGA;
#endif

	} else {
#if defined(CONFIG_CAMERA_DEV_2_SUPPORT_13M)
		buffer_id = IMG_DP0_MEGA;
#elif defined(CONFIG_CAMERA_DEV_2_SUPPORT_8M)
		buffer_id = IMG_8P0_MEGA;
#elif defined(CONFIG_CAMERA_DEV_2_SUPPORT_7M)
		buffer_id = IMG_7P0_MEGA;
#elif defined(CONFIG_CAMERA_DEV_2_SUPPORT_6M)
		buffer_id = IMG_6P0_MEGA;
#elif defined(CONFIG_CAMERA_DEV_2_SUPPORT_5M)
		buffer_id = IMG_5P0_MEGA;
#elif defined(CONFIG_CAMERA_DEV_2_SUPPORT_4M)
		buffer_id = IMG_4P0_MEGA;
#elif defined(CONFIG_CAMERA_DEV_2_SUPPORT_3M)
		buffer_id = IMG_3P0_MEGA;
#elif defined(CONFIG_CAMERA_DEV_2_SUPPORT_2M)
		buffer_id = IMG_2P0_MEGA;
#else
		buffer_id = IMG_2P0_MEGA;
#endif
	}

	CMR_LOGI("buffer id = %d", buffer_id);

	return buffer_id;
}
#endif

int camera_reserve_buf_size(cmr_u32 camera_id, cmr_s32 mem_size_id,
                            cmr_u32 *mem_size, cmr_u32 *mem_sum) {
    struct cap_size_to_mem *mem_tab_ptr = NULL;

    if (mem_size_id < IMG_1P3_MEGA || mem_size_id >= IMG_SIZE_NUM ||
        NULL == mem_size) {
        CMR_LOGE("no matched size for this image: id=%d, %p", mem_size_id,
                 mem_size);
        return -1;
    }
    *mem_sum = 1;

    mem_tab_ptr = (struct cap_size_to_mem *)&reserve_mem_size_tab[0];
    *mem_size = mem_tab_ptr[mem_size_id].mem_size;

    CMR_LOGI("image size num, %d, mem size 0x%x", mem_size_id,
             mem_tab_ptr[mem_size_id].mem_size);

    return 0;
}

int camera_pre_capture_buf_size(cmr_u32 camera_id, cmr_s32 mem_size_id,
                                cmr_u32 *mem_size, cmr_u32 *mem_sum) {
    struct cap_size_to_mem *mem_tab_ptr = NULL;
    struct cap_size_to_mem *yuv_mem_tab_ptr = NULL;

    if (mem_size_id < IMG_1P3_MEGA || mem_size_id >= IMG_SIZE_NUM ||
        NULL == mem_size) {
        CMR_LOGE("no matched size for this image: id=%d, %p", mem_size_id,
                 mem_size);
        return -1;
    }

    cmr_u32 is_raw_capture = 0;
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.sys.camera.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }

    *mem_sum = 1;

    if (BACK_CAMERA_ID == camera_id || DEV2_CAMERA_ID == camera_id) {
        mem_tab_ptr = (struct cap_size_to_mem *)&back_cam_raw_mem_size_tab[0];
        yuv_mem_tab_ptr = (struct cap_size_to_mem *)&back_cam_mem_size_tab[0];
        if (is_raw_capture)
            *mem_size = MAX(mem_tab_ptr[mem_size_id].mem_size,
                            yuv_mem_tab_ptr[mem_size_id].mem_size);
        else
            *mem_size = yuv_mem_tab_ptr[mem_size_id].mem_size;
    } else if (FRONT_CAMERA_ID == camera_id || DEV3_CAMERA_ID == camera_id) {
        int cameraMode = 0;

        cameraMode = is_multi_camera_mode_mem;
        if (cameraMode == MODE_3D_VIDEO) {
            CMR_LOGI("current mode is 3D video");
            mem_tab_ptr =
                (struct cap_size_to_mem *)&Stereo_video_mem_size_tab[0];
            yuv_mem_tab_ptr =
                (struct cap_size_to_mem *)&Stereo_video_mem_size_tab[0];
        } else if (cameraMode == MODE_3D_CAPTURE) { /**add for reducing mem size
                                                       in 3D capture mode
                                                       begin*/
            CMR_LOGI("current mode is 3D capture");
            mem_tab_ptr = (struct cap_size_to_mem *)&front_cam_mem_size_tab[0];
            yuv_mem_tab_ptr =
                (struct cap_size_to_mem *)&front_cam_mem_size_tab[0];
        } else {
            mem_tab_ptr =
                (struct cap_size_to_mem *)&front_cam_raw_mem_size_tab[0];
            yuv_mem_tab_ptr =
                (struct cap_size_to_mem *)&front_cam_mem_size_tab[0];
        }

        *mem_size = MAX(mem_tab_ptr[mem_size_id].mem_size,
                        yuv_mem_tab_ptr[mem_size_id].mem_size);
    } else {
        mem_tab_ptr = (struct cap_size_to_mem *)&mem_size_tab[0];
        *mem_size = mem_tab_ptr[mem_size_id].mem_size;
    }

    CMR_LOGI("image size num, %d, mem size 0x%x", mem_size_id,
             mem_tab_ptr[mem_size_id].mem_size);

    return 0;
}

int camera_capture_buf_size(uint32_t camera_id, uint32_t sn_fmt,
                            struct img_size *image_size, uint32_t *mem_size) {
    uint32_t size_pixel;
    int i;
    struct cap_size_to_mem *mem_tab_ptr = NULL;

    if (NULL == image_size || NULL == mem_size) {
        CMR_LOGE("Parameter error 0x%p 0x%p ", image_size, mem_size);
        return -1;
    }

    size_pixel = (uint32_t)(image_size->width * image_size->height);

    if (SENSOR_IMAGE_FORMAT_RAW == sn_fmt) {
        if (BACK_CAMERA_ID == camera_id || DEV2_CAMERA_ID == camera_id) {
            mem_tab_ptr =
                (struct cap_size_to_mem *)&back_cam_raw_mem_size_tab[0];
        } else if (FRONT_CAMERA_ID == camera_id ||
                   DEV3_CAMERA_ID == camera_id) {
            mem_tab_ptr =
                (struct cap_size_to_mem *)&front_cam_raw_mem_size_tab[0];
        } else {
            mem_tab_ptr = (struct cap_size_to_mem *)&mem_size_tab[0];
        }
    } else {
        if (BACK_CAMERA_ID == camera_id || DEV2_CAMERA_ID == camera_id) {
            mem_tab_ptr = (struct cap_size_to_mem *)&back_cam_mem_size_tab[0];
        } else if (FRONT_CAMERA_ID == camera_id ||
                   DEV3_CAMERA_ID == camera_id) {
            mem_tab_ptr = (struct cap_size_to_mem *)&front_cam_mem_size_tab[0];
        } else {
            mem_tab_ptr = (struct cap_size_to_mem *)&mem_size_tab[0];
        }
    }

    for (i = IMG_1P3_MEGA; i < IMG_SIZE_NUM; i++) {
        if (size_pixel <= mem_tab_ptr[i].pixel_num)
            break;
    }

    if (i == IMG_SIZE_NUM) {
        CMR_LOGE("No matched size for this image, 0x%x", size_pixel);
        return -1;
    } else {
        CMR_LOGI("image size num, %d, mem size 0x%x", i,
                 mem_tab_ptr[i].mem_size);
    }

    *mem_size = mem_tab_ptr[i].mem_size;

    return 0;
}

int camera_arrange_capture_buf(
    struct cmr_cap_2_frm *cap_2_frm, struct img_size *sn_size,
    struct img_rect *sn_trim, struct img_size *image_size, uint32_t orig_fmt,
    struct img_size *cap_size, struct img_size *thum_size,
    struct cmr_cap_mem *capture_mem, uint32_t need_rot, uint32_t need_scale,
    uint32_t image_cnt) {
    if (NULL == cap_2_frm || NULL == image_size || NULL == thum_size ||
        NULL == capture_mem || NULL == sn_size || NULL == sn_trim) {
        CMR_LOGE("Parameter error 0x%p 0x%p 0x%p 0x%p 0x%p 0x%p", cap_2_frm,
                 image_size, thum_size, capture_mem, sn_size, sn_trim);
        return -1;
    }

    uint32_t channel_size;
    uint32_t mem_res = 0, mem_end = 0;
    uint32_t i = 0;
    int32_t ret = -1;
    struct cmr_cap_mem *cap_mem = &capture_mem[0];

    channel_size = (uint32_t)(image_size->width * image_size->height);
    mem_res = cap_2_frm->mem_frm.buf_size;

    CMR_LOGI("mem frame, 0x%lx 0x%lx, fd 0x%x, buf_size = %d bytes",
             cap_2_frm->mem_frm.addr_phy.addr_y,
             cap_2_frm->mem_frm.addr_vir.addr_y, cap_2_frm->mem_frm.fd,
             cap_2_frm->mem_frm.buf_size);

    CMR_LOGI("channel_size, 0x%x, image_cnt %d, rot %d, orig_fmt %d",
             channel_size, image_cnt, need_rot, orig_fmt);

    CMR_LOGI(
        "sn_size %d %d, sn_trim %d %d %d %d, image_size %d %d, cap_size %d %d",
        sn_size->width, sn_size->height, sn_trim->start_x, sn_trim->start_y,
        sn_trim->width, sn_trim->height, image_size->width, image_size->height,
        cap_size->width, cap_size->height);

    /* get target_jpeg buffer size first, will be used later */
    memset((void *)cap_mem, 0, sizeof(struct cmr_cap_mem));
    cap_mem->target_jpeg.buf_size =
        get_jpeg_size(image_size->width, image_size->height, thum_size->width,
                      thum_size->height);

    if (IMG_DATA_TYPE_RAW == orig_fmt) {
        ret = arrange_raw_buf(cap_2_frm, sn_size, sn_trim, image_size, orig_fmt,
                              cap_size, thum_size, cap_mem, need_rot, &mem_res,
                              &mem_end, &channel_size);
        if (ret) {
            CMR_LOGE("raw fmt arrange failed!");
            return -1;
        }
    } else if (IMG_DATA_TYPE_JPEG == orig_fmt) {
        ret = arrange_jpeg_buf(cap_2_frm, sn_size, sn_trim, image_size,
                               orig_fmt, cap_size, thum_size, cap_mem, need_rot,
                               &mem_res, &mem_end, &channel_size);
        if (ret) {
            CMR_LOGE("jpeg fmt arrange failed!");
            return -1;
        }
    } else {
        ret = arrange_yuv_buf(cap_2_frm, sn_size, sn_trim, image_size, orig_fmt,
                              cap_size, thum_size, cap_mem, need_rot, &mem_res,
                              &mem_end, &channel_size);
        if (ret) {
            CMR_LOGE("yuv fmt arrange failed!");
            return -1;
        }
    }

    /* arrange misc buffers */
    ret = arrange_misc_buf(cap_2_frm, sn_size, sn_trim, image_size, orig_fmt,
                           cap_size, thum_size, cap_mem, need_rot, &mem_res,
                           &mem_end, &channel_size);
    if (ret) {
        CMR_LOGE("misc buf arrange failed!");
        return -1;
    }

    /* arrange rot buf */
    if (need_rot) {
        ret = arrange_rot_buf(cap_2_frm, sn_size, sn_trim, image_size, orig_fmt,
                              cap_size, thum_size, cap_mem, need_rot,
                              need_scale, &mem_res, &mem_end, &channel_size);
        if (ret) {
            CMR_LOGE("rot buf arrange failed!");
            return -1;
        }
    }

    CMR_LOGI("mem_end, mem_res: 0x%x 0x%x ", mem_end, mem_res);

    /* resize target jpeg buffer */
    cap_mem->target_jpeg.addr_phy.addr_y =
        cap_mem->target_jpeg.addr_phy.addr_y + JPEG_EXIF_SIZE;
    cap_mem->target_jpeg.addr_vir.addr_y =
        cap_mem->target_jpeg.addr_vir.addr_y + JPEG_EXIF_SIZE;
    cap_mem->target_jpeg.buf_size =
        cap_mem->target_jpeg.buf_size - JPEG_EXIF_SIZE;

    CMR_LOGD("target_yuv, phy offset 0x%lx 0x%lx, vir 0x%lx 0x%lx, fd 0x%x, "
             "size 0x%x",
             cap_mem->target_yuv.addr_phy.addr_y,
             cap_mem->target_yuv.addr_phy.addr_u,
             cap_mem->target_yuv.addr_vir.addr_y,
             cap_mem->target_yuv.addr_vir.addr_u, cap_mem->target_yuv.fd,
             cap_mem->target_yuv.buf_size);

    CMR_LOGD(
        "cap_yuv, phy offset 0x%lx 0x%lx, vir 0x%lx 0x%lx, fd 0x%x, size 0x%x",
        cap_mem->cap_yuv.addr_phy.addr_y, cap_mem->cap_yuv.addr_phy.addr_u,
        cap_mem->cap_yuv.addr_vir.addr_y, cap_mem->cap_yuv.addr_vir.addr_u,
        cap_mem->cap_yuv.fd, cap_mem->cap_yuv.buf_size);

    CMR_LOGD("cap_yuv_rot, phy offset 0x%lx 0x%lx, vir 0x%lx 0x%lx, fd 0x%x, "
             "size 0x%x",
             cap_mem->cap_yuv_rot.addr_phy.addr_y,
             cap_mem->cap_yuv_rot.addr_phy.addr_u,
             cap_mem->cap_yuv_rot.addr_vir.addr_y,
             cap_mem->cap_yuv_rot.addr_vir.addr_u, cap_mem->cap_yuv_rot.fd,
             cap_mem->cap_yuv_rot.buf_size);

    CMR_LOGD(
        "cap_raw, phy offset 0x%lx 0x%lx, vir 0x%lx 0x%lx, fd 0x%x, size 0x%x",
        cap_mem->cap_raw.addr_phy.addr_y, cap_mem->cap_raw.addr_phy.addr_u,
        cap_mem->cap_raw.addr_vir.addr_y, cap_mem->cap_raw.addr_vir.addr_u,
        cap_mem->cap_raw.fd, cap_mem->cap_raw.buf_size);

    CMR_LOGD(
        "cap_raw2, phy offset 0x%lx 0x%lx, vir 0x%lx 0x%lx, fd 0x%x, size 0x%x",
        cap_mem->cap_raw2.addr_phy.addr_y, cap_mem->cap_raw2.addr_phy.addr_u,
        cap_mem->cap_raw2.addr_vir.addr_y, cap_mem->cap_raw2.addr_vir.addr_u,
        cap_mem->cap_raw2.fd, cap_mem->cap_raw2.buf_size);

    CMR_LOGD("target_jpeg, phy offset 0x%lx, vir 0x%lx, fd 0x%x, size 0x%x",
             cap_mem->target_jpeg.addr_phy.addr_y,
             cap_mem->target_jpeg.addr_vir.addr_y, cap_mem->target_jpeg.fd,
             cap_mem->target_jpeg.buf_size);

    CMR_LOGD(
        "thum_yuv, phy offset 0x%lx 0x%lx, vir 0x%lx 0x%lx, fd 0x%x, size 0x%x",
        cap_mem->thum_yuv.addr_phy.addr_y, cap_mem->thum_yuv.addr_phy.addr_u,
        cap_mem->thum_yuv.addr_vir.addr_y, cap_mem->thum_yuv.addr_vir.addr_u,
        cap_mem->thum_yuv.fd, cap_mem->thum_yuv.buf_size);

    CMR_LOGD("thum_jpeg, phy offset 0x%lx, vir 0x%lx, fd 0x%x, size 0x%x",
             cap_mem->thum_jpeg.addr_phy.addr_y,
             cap_mem->thum_jpeg.addr_vir.addr_y, cap_mem->thum_jpeg.fd,
             cap_mem->thum_jpeg.buf_size);

    CMR_LOGD("scale_tmp, phy offset 0x%lx, vir 0x%lx, fd 0x%x, size 0x%x",
             cap_mem->scale_tmp.addr_phy.addr_y,
             cap_mem->scale_tmp.addr_vir.addr_y, cap_mem->scale_tmp.fd,
             cap_mem->scale_tmp.buf_size);

    for (i = 1; i < image_cnt; i++) {
        memcpy((void *)&capture_mem[i].cap_raw, (void *)&capture_mem[0].cap_raw,
               sizeof(struct img_frm));

        memcpy((void *)&capture_mem[i].cap_raw2,
               (void *)&capture_mem[0].cap_raw2, sizeof(struct img_frm));

        memcpy((void *)&capture_mem[i].target_yuv,
               (void *)&capture_mem[0].target_yuv, sizeof(struct img_frm));

        memcpy((void *)&capture_mem[i].cap_yuv, (void *)&capture_mem[0].cap_yuv,
               sizeof(struct img_frm));

        memcpy((void *)&capture_mem[i].target_jpeg,
               (void *)&capture_mem[0].target_jpeg, sizeof(struct img_frm));

        memcpy((void *)&capture_mem[i].thum_yuv,
               (void *)&capture_mem[0].thum_yuv, sizeof(struct img_frm));

        memcpy((void *)&capture_mem[i].thum_jpeg,
               (void *)&capture_mem[0].thum_jpeg, sizeof(struct img_frm));

        memcpy((void *)&capture_mem[i].jpeg_tmp,
               (void *)&capture_mem[0].jpeg_tmp, sizeof(struct img_frm));

        memcpy((void *)&capture_mem[i].scale_tmp,
               (void *)&capture_mem[0].scale_tmp, sizeof(struct img_frm));

        memcpy((void *)&capture_mem[i].cap_yuv_rot,
               (void *)&capture_mem[0].cap_yuv_rot, sizeof(struct img_frm));

        memcpy((void *)&capture_mem[i].isp_tmp, (void *)&capture_mem[0].isp_tmp,
               sizeof(struct img_frm));
    }

    return 0;
}

int arrange_raw_buf(struct cmr_cap_2_frm *cap_2_frm, struct img_size *sn_size,
                    struct img_rect *sn_trim, struct img_size *image_size,
                    uint32_t orig_fmt, struct img_size *cap_size,
                    struct img_size *thum_size, struct cmr_cap_mem *capture_mem,
                    uint32_t need_rot, uint32_t *io_mem_res,
                    uint32_t *io_mem_end, uint32_t *io_channel_size) {
    UNUSED(thum_size);
    if (IMG_DATA_TYPE_RAW != orig_fmt || NULL == io_mem_res ||
        NULL == io_mem_end || NULL == io_channel_size) {
        return -1;
    }

    uint32_t mem_res = 0, mem_end = 0;
    uint32_t offset = 0;
    uint32_t raw_size = 0, raw2_size = 0;
    uint32_t tmp1, tmp2, tmp3, max_size;
    /*&capture_mem[0];*/
    struct cmr_cap_mem *cap_mem = capture_mem;

    mem_res = *io_mem_res;
    mem_end = *io_mem_end;
    CMR_LOGD("mem_end=%0x", mem_end);

    raw_size = sn_size->width * sn_size->height * RAWRGB_BIT_WIDTH / 8;
    raw2_size = ((sn_size->width * 4 / 3 + 7) & (~7)) * sn_size->height;

    tmp1 = image_size->width * image_size->height;
    tmp2 = cap_size->width * cap_size->height;
    tmp3 = sn_size->width * sn_size->height;
    max_size = tmp1 > tmp2 ? tmp1 : tmp2;
    max_size = max_size > tmp3 ? max_size : tmp3;

    cap_mem->target_yuv.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y;
    cap_mem->target_yuv.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y;
    cap_mem->target_yuv.addr_phy.addr_u =
        cap_mem->target_yuv.addr_phy.addr_y +
        image_size->width * image_size->height;
    cap_mem->target_yuv.addr_vir.addr_u =
        cap_mem->target_yuv.addr_vir.addr_y +
        image_size->width * image_size->height;
    cap_mem->target_yuv.fd = cap_2_frm->mem_frm.fd;
    cap_mem->target_yuv.buf_size =
        image_size->width * image_size->height * 3 / 2;
    cap_mem->target_yuv.size.width = image_size->width;
    cap_mem->target_yuv.size.height = image_size->height;
    cap_mem->target_yuv.fmt = IMG_DATA_TYPE_YUV420;

    cap_mem->cap_yuv.addr_phy.addr_y =
        cap_2_frm->mem_frm.addr_phy.addr_y + max_size * 3 / 2;
    cap_mem->cap_yuv.addr_vir.addr_y =
        cap_2_frm->mem_frm.addr_vir.addr_y + max_size * 3 / 2;
    cap_mem->cap_yuv.addr_phy.addr_u =
        cap_mem->cap_yuv.addr_phy.addr_y + cap_size->width * cap_size->height;
    cap_mem->cap_yuv.addr_vir.addr_u =
        cap_mem->cap_yuv.addr_vir.addr_y + cap_size->width * cap_size->height;
    cap_mem->cap_yuv.fd = cap_2_frm->mem_frm.fd;
    cap_mem->cap_yuv.buf_size = cap_size->width * cap_size->height * 3 / 2;
    cap_mem->cap_yuv.size.width = cap_size->width;
    cap_mem->cap_yuv.size.height = cap_size->height;
    cap_mem->cap_yuv.fmt = IMG_DATA_TYPE_YUV420;

    cap_mem->cap_raw.addr_phy.addr_y = cap_mem->cap_yuv.addr_phy.addr_y;
    cap_mem->cap_raw.addr_vir.addr_y = cap_mem->cap_yuv.addr_vir.addr_y;
    cap_mem->cap_raw.fd = cap_mem->cap_yuv.fd;
    cap_mem->cap_raw.buf_size = raw_size;
    cap_mem->cap_raw.size.width = sn_size->width;
    cap_mem->cap_raw.size.height = sn_size->height;
    cap_mem->cap_raw.fmt = IMG_DATA_TYPE_RAW;

    cap_mem->cap_raw2.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y +
                                        max_size * 3 / 2 + max_size * 3 / 2;
    cap_mem->cap_raw2.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y +
                                        max_size * 3 / 2 + max_size * 3 / 2;
    cap_mem->cap_raw2.fd = cap_2_frm->mem_frm.fd;
    cap_mem->cap_raw2.buf_size = raw2_size;
    cap_mem->cap_raw2.size.width = sn_size->width;
    cap_mem->cap_raw2.size.height = sn_size->height;
    cap_mem->cap_raw2.fmt = IMG_DATA_TYPE_RAW2;

    cap_mem->target_jpeg.addr_phy.addr_y = cap_mem->cap_yuv.addr_phy.addr_y;
    cap_mem->target_jpeg.addr_vir.addr_y = cap_mem->cap_yuv.addr_vir.addr_y;
    cap_mem->target_jpeg.fd = cap_mem->cap_yuv.fd;

    /* update io param */
    CMR_NO_MEM(max_size * 3 / 2 + max_size * 3 / 2 + raw2_size, mem_res);
    *io_mem_res = mem_res - (max_size * 3 / 2 + max_size * 3 / 2 + raw2_size);
    *io_mem_end = mem_end + max_size * 3 / 2 + max_size * 3 / 2 + raw2_size;
    CMR_LOGD("mem_end=%0x", *io_mem_end);

    return 0;
}

int arrange_jpeg_buf(struct cmr_cap_2_frm *cap_2_frm, struct img_size *sn_size,
                     struct img_rect *sn_trim, struct img_size *image_size,
                     uint32_t orig_fmt, struct img_size *cap_size,
                     struct img_size *thum_size,
                     struct cmr_cap_mem *capture_mem, uint32_t need_rot,
                     uint32_t *io_mem_res, uint32_t *io_mem_end,
                     uint32_t *io_channel_size) {
    UNUSED(thum_size);

    uint32_t channel_size;
    uint32_t mem_res = 0, mem_end = 0;
    uint32_t offset = 0;
    uint32_t yy_to_y = 0, tmp = 0;
    uint32_t y_end = 0, uv_end = 0;
    uint32_t gap_size = 0;

    struct cmr_cap_mem *cap_mem = capture_mem; /*&capture_mem[0];*/
    struct img_size align16_image_size, align16_cap_size;

    if (IMG_DATA_TYPE_JPEG != orig_fmt || NULL == io_mem_res ||
        NULL == io_mem_end || NULL == io_channel_size) {
        return -1;
    }
    CMR_LOGI("jpeg fmt buf arrange");

    align16_image_size.width =
        camera_get_aligned_size(cap_2_frm->type, image_size->width);
    align16_image_size.height =
        camera_get_aligned_size(cap_2_frm->type, image_size->height);
    align16_cap_size.width =
        camera_get_aligned_size(cap_2_frm->type, cap_size->width);
    align16_cap_size.height =
        camera_get_aligned_size(cap_2_frm->type, cap_size->height);

    mem_res = *io_mem_res;
    mem_end = *io_mem_end;
    // channel_size = *io_channel_size;

    channel_size =
        (uint32_t)(align16_image_size.width * align16_image_size.height);
    cap_mem->target_yuv.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y;
    cap_mem->target_yuv.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y;
    cap_mem->target_yuv.fd = cap_2_frm->mem_frm.fd;

    yy_to_y =
        (uint32_t)((sn_size->height - sn_trim->start_y - sn_trim->height) *
                   sn_size->width);
    tmp = (uint32_t)(sn_size->height * sn_size->width);
    y_end = yy_to_y + MAX(channel_size, tmp);

    CMR_LOGI("yy_to_y, 0x%x, tmp 0x%x", yy_to_y, tmp);

    cap_mem->target_yuv.addr_phy.addr_u =
        cap_mem->target_yuv.addr_phy.addr_y + y_end;
    cap_mem->target_yuv.addr_vir.addr_u =
        cap_mem->target_yuv.addr_vir.addr_y + y_end;
    cap_mem->cap_yuv.addr_phy.addr_y =
        cap_mem->target_yuv.addr_phy.addr_u - tmp;
    cap_mem->cap_yuv.fd = cap_mem->target_yuv.fd;
    /*to confirm the gap of scaling source and dest should be large than
     * MIN_GAP_LINES*/
    if ((cap_mem->cap_yuv.addr_phy.addr_y -
         cap_mem->target_yuv.addr_phy.addr_y) <
        (MIN_GAP_LINES * align16_image_size.width)) {
        gap_size = (MIN_GAP_LINES * align16_image_size.width) -
                   (cap_mem->cap_yuv.addr_phy.addr_y -
                    cap_mem->target_yuv.addr_phy.addr_y);
        gap_size = (gap_size + ALIGN_PAGE_SIZE - 1) & (~(ALIGN_PAGE_SIZE - 1));
    }

    cap_mem->cap_yuv.addr_phy.addr_y += gap_size;
    cap_mem->cap_yuv.addr_vir.addr_y =
        cap_mem->target_yuv.addr_vir.addr_u - tmp + gap_size;

    cap_mem->target_yuv.addr_phy.addr_u += gap_size;
    cap_mem->target_yuv.addr_vir.addr_u += gap_size;

    cap_mem->cap_yuv.addr_phy.addr_u =
        cap_mem->target_yuv.addr_phy.addr_u + y_end - tmp + gap_size;
    cap_mem->cap_yuv.addr_vir.addr_u =
        cap_mem->target_yuv.addr_vir.addr_u + y_end - tmp + gap_size;

    if (need_rot) {
        offset = ((uint32_t)(y_end * 3) >> 1) + 2 * gap_size;
    } else {
        offset = ((y_end) << 1) + 2 * gap_size;
    }

    CMR_NO_MEM(offset, mem_res);
    mem_end = offset;
    mem_res = mem_res - mem_end;

    if (!need_rot) {
        CMR_NO_MEM(cap_mem->target_jpeg.buf_size, mem_res);
        cap_mem->target_jpeg.addr_phy.addr_y =
            cap_2_frm->mem_frm.addr_phy.addr_y + mem_end;
        cap_mem->target_jpeg.addr_vir.addr_y =
            cap_2_frm->mem_frm.addr_vir.addr_y + mem_end;
        cap_mem->target_jpeg.fd = cap_2_frm->mem_frm.fd;
        mem_end += cap_mem->target_jpeg.buf_size;
        mem_res -= cap_mem->target_jpeg.buf_size;
    }

    cap_mem->cap_yuv.buf_size = channel_size * 3 / 2;
    cap_mem->cap_yuv.size.width = align16_cap_size.width;
    cap_mem->cap_yuv.size.height = align16_cap_size.height;
    cap_mem->cap_yuv.fmt = IMG_DATA_TYPE_YUV420;

    cap_mem->target_yuv.buf_size = channel_size * 3 / 2;
    cap_mem->target_yuv.size.width = align16_image_size.width;
    cap_mem->target_yuv.size.height = align16_image_size.height;
    cap_mem->target_yuv.fmt = IMG_DATA_TYPE_YUV420;

    /* update io param */
    *io_mem_res = mem_res;
    *io_mem_end = mem_end;
    *io_channel_size = channel_size;

    return 0;
}

int arrange_yuv_buf(struct cmr_cap_2_frm *cap_2_frm, struct img_size *sn_size,
                    struct img_rect *sn_trim, struct img_size *image_size,
                    uint32_t orig_fmt, struct img_size *cap_size,
                    struct img_size *thum_size, struct cmr_cap_mem *capture_mem,
                    uint32_t need_rot, uint32_t *io_mem_res,
                    uint32_t *io_mem_end, uint32_t *io_channel_size) {
    // UNUSED(sn_size);
    UNUSED(sn_trim);
    UNUSED(thum_size);

    if (IMG_DATA_TYPE_JPEG == orig_fmt || IMG_DATA_TYPE_RAW == orig_fmt ||
        NULL == io_mem_res || NULL == io_mem_end || NULL == io_channel_size) {
        return -1;
    }

    uint32_t mem_res = 0, mem_end = 0;
    uint32_t offset = 0;
    uint32_t tmp1, tmp2, tmp3, max_size;
    /*&capture_mem[0];*/
    struct cmr_cap_mem *cap_mem = capture_mem;

    mem_res = *io_mem_res;
    mem_end = *io_mem_end;

    tmp1 = image_size->width * image_size->height;
    tmp2 = cap_size->width * cap_size->height;
    tmp3 = sn_size->width * sn_size->height;
    max_size = tmp1 > tmp2 ? tmp1 : tmp2;
    max_size = max_size > tmp3 ? max_size : tmp3;

    cap_mem->target_yuv.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y;
    cap_mem->target_yuv.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y;
    cap_mem->target_yuv.addr_phy.addr_u =
        cap_mem->target_yuv.addr_phy.addr_y +
        image_size->width * image_size->height;
    cap_mem->target_yuv.addr_vir.addr_u =
        cap_mem->target_yuv.addr_vir.addr_y +
        image_size->width * image_size->height;
    cap_mem->target_yuv.fd = cap_2_frm->mem_frm.fd;
    cap_mem->target_yuv.buf_size =
        image_size->width * image_size->height * 3 / 2;
    cap_mem->target_yuv.size.width = image_size->width;
    cap_mem->target_yuv.size.height = image_size->height;
    cap_mem->target_yuv.fmt = IMG_DATA_TYPE_YUV420;

    cap_mem->cap_yuv.addr_phy.addr_y =
        cap_mem->target_yuv.addr_phy.addr_y + max_size * 3 / 2;
    cap_mem->cap_yuv.addr_vir.addr_y =
        cap_mem->target_yuv.addr_vir.addr_y + max_size * 3 / 2;
    cap_mem->cap_yuv.addr_phy.addr_u =
        cap_mem->cap_yuv.addr_phy.addr_y + cap_size->width * cap_size->height;
    cap_mem->cap_yuv.addr_vir.addr_u =
        cap_mem->cap_yuv.addr_vir.addr_y + cap_size->width * cap_size->height;
    cap_mem->cap_yuv.fd = cap_mem->target_yuv.fd;
    cap_mem->cap_yuv.buf_size = cap_size->width * cap_size->height * 3 / 2;
    cap_mem->cap_yuv.size.width = cap_size->width;
    cap_mem->cap_yuv.size.height = cap_size->height;
    cap_mem->cap_yuv.fmt = IMG_DATA_TYPE_YUV420;

    cap_mem->target_jpeg.addr_phy.addr_y = cap_mem->cap_yuv.addr_phy.addr_y;
    cap_mem->target_jpeg.addr_vir.addr_y = cap_mem->cap_yuv.addr_vir.addr_y;
    cap_mem->target_jpeg.fd = cap_mem->cap_yuv.fd;

    tmp1 = max_size * 3 / 2 + max_size * 3 / 2;
    tmp2 = max_size * 3 / 2 + cap_mem->target_jpeg.buf_size;
    offset = tmp1 > tmp2 ? tmp1 : tmp2;

    CMR_LOGI("offset %d", offset);
    CMR_NO_MEM(offset, mem_res);
    mem_end = CMR_ADDR_ALIGNED(offset);
    mem_res = mem_res - mem_end;

    /* update io param */
    *io_mem_res = mem_res;
    *io_mem_end = mem_end;

    return 0;
}

int arrange_misc_buf(struct cmr_cap_2_frm *cap_2_frm, struct img_size *sn_size,
                     struct img_rect *sn_trim, struct img_size *image_size,
                     uint32_t orig_fmt, struct img_size *cap_size,
                     struct img_size *thum_size,
                     struct cmr_cap_mem *capture_mem, uint32_t need_rot,
                     uint32_t *io_mem_res, uint32_t *io_mem_end,
                     uint32_t *io_channel_size) {
    UNUSED(sn_size);
    UNUSED(sn_trim);
    UNUSED(orig_fmt);
    UNUSED(need_rot);

    if (NULL == io_mem_res || NULL == io_mem_end || NULL == io_channel_size) {
        return -1;
    }

    uint32_t size_pixel;
    uint32_t mem_res = 0, mem_end = 0;
    uint32_t i = 0;
    struct cmr_cap_mem *cap_mem = capture_mem; /*&capture_mem[0];*/
    struct img_frm img_frame[BUF_TYPE_NUM];

    mem_res = *io_mem_res;
    mem_end = *io_mem_end;

    for (i = THUM_YUV; i < BUF_TYPE_NUM; i++) {
        /* calculate the address of target_jpeg, start */
        size_pixel = get_size[i](image_size->width, image_size->height,
                                 thum_size->width, thum_size->height);
        CMR_LOGI("i %d %x reseved in buffer, alloc %x buffers\n", i, mem_res,
                 size_pixel);
        if (mem_res >= size_pixel) {
            img_frame[i].buf_size = size_pixel;
            img_frame[i].addr_phy.addr_y =
                cap_2_frm->mem_frm.addr_phy.addr_y + mem_end;
            img_frame[i].addr_vir.addr_y =
                cap_2_frm->mem_frm.addr_vir.addr_y + mem_end;
            img_frame[i].addr_phy.addr_u =
                img_frame[i].addr_phy.addr_y + size_pixel * 2 / 3;
            img_frame[i].addr_vir.addr_u =
                img_frame[i].addr_vir.addr_y + size_pixel * 2 / 3;
            img_frame[i].fd = cap_2_frm->mem_frm.fd;
            mem_res -= size_pixel;
            mem_end += size_pixel;
        } else {
            break;
        }
    }

    if (i != BUF_TYPE_NUM) {
        CMR_LOGE("Failed to alloc all the buffers used in capture");
        return -1;
    }

    cap_mem->thum_yuv.buf_size = img_frame[THUM_YUV].buf_size;
    cap_mem->thum_yuv.addr_phy.addr_y = img_frame[THUM_YUV].addr_phy.addr_y;
    cap_mem->thum_yuv.addr_vir.addr_y = img_frame[THUM_YUV].addr_vir.addr_y;
    cap_mem->thum_yuv.addr_phy.addr_u = img_frame[THUM_YUV].addr_phy.addr_u;
    cap_mem->thum_yuv.fd = img_frame[THUM_YUV].fd;
    cap_mem->thum_yuv.addr_vir.addr_u = img_frame[THUM_YUV].addr_vir.addr_u;
    cap_mem->thum_yuv.size.width = thum_size->width;
    cap_mem->thum_yuv.size.height = thum_size->height;

    cap_mem->thum_jpeg.buf_size = img_frame[THUM_JPEG].buf_size;
    cap_mem->thum_jpeg.addr_phy.addr_y = img_frame[THUM_JPEG].addr_phy.addr_y;
    cap_mem->thum_jpeg.addr_vir.addr_y = img_frame[THUM_JPEG].addr_vir.addr_y;
    cap_mem->thum_jpeg.fd = img_frame[THUM_JPEG].fd;

    /* mem reuse, jpeg_tmp/uv */
    cap_mem->jpeg_tmp.buf_size = cap_mem->cap_yuv.buf_size;
    cap_mem->jpeg_tmp.addr_phy.addr_y = cap_mem->cap_yuv.addr_phy.addr_u;
    cap_mem->jpeg_tmp.addr_vir.addr_y = cap_mem->cap_yuv.addr_vir.addr_u;
    cap_mem->jpeg_tmp.fd = cap_mem->cap_yuv.fd;

    /* update io param */
    *io_mem_res = mem_res;
    *io_mem_end = mem_end;

    CMR_LOGD("mem_res, mem_end 0x%x, 0x%x", mem_res, mem_end);

    return 0;
}

int arrange_rot_buf(struct cmr_cap_2_frm *cap_2_frm, struct img_size *sn_size,
                    struct img_rect *sn_trim, struct img_size *image_size,
                    uint32_t orig_fmt, struct img_size *cap_size,
                    struct img_size *thum_size, struct cmr_cap_mem *capture_mem,
                    uint32_t need_rot, uint32_t need_scale,
                    uint32_t *io_mem_res, uint32_t *io_mem_end,
                    uint32_t *io_channel_size) {
    if (NULL == io_mem_res || NULL == io_mem_end || NULL == io_channel_size) {
        return -1;
    }

    if (!need_rot) {
        return -1;
    }

    uint32_t size_pixel;
    uint32_t mem_res = 0, mem_end = 0;
    uint32_t offset = 0, offset_1;
    uint32_t tmp1, tmp2, tmp3, max_size;
    struct cmr_cap_mem *cap_mem = capture_mem; /*&capture_mem[0];*/

    mem_res = *io_mem_res;
    mem_end = *io_mem_end;

    tmp1 = image_size->width * image_size->height;
    tmp2 = cap_size->width * cap_size->height;
    tmp3 = sn_size->width * sn_size->height;
    max_size = tmp1 > tmp2 ? tmp1 : tmp2;
    max_size = max_size > tmp3 ? max_size : tmp3;

    if (ZOOM_POST_PROCESS == cap_2_frm->zoom_post_proc ||
        ZOOM_POST_PROCESS_WITH_TRIM == cap_2_frm->zoom_post_proc) {
        cap_mem->cap_yuv_rot.addr_phy.addr_y =
            cap_mem->target_yuv.addr_phy.addr_y;
        cap_mem->cap_yuv_rot.addr_vir.addr_y =
            cap_mem->target_yuv.addr_vir.addr_y;
        cap_mem->cap_yuv_rot.addr_phy.addr_u =
            cap_mem->cap_yuv_rot.addr_phy.addr_y +
            cap_size->width * cap_size->height;
        cap_mem->cap_yuv_rot.addr_vir.addr_u =
            cap_mem->cap_yuv_rot.addr_vir.addr_y +
            cap_size->width * cap_size->height;
        cap_mem->cap_yuv_rot.fd = cap_mem->target_yuv.fd;

        cap_mem->cap_yuv_rot.size.width = cap_size->height;
        cap_mem->cap_yuv_rot.size.height = cap_size->width;
        cap_mem->cap_yuv_rot.buf_size =
            cap_size->width * cap_size->width * 3 / 2;
        cap_mem->cap_yuv_rot.fmt = IMG_DATA_TYPE_YUV420;

        cap_mem->target_yuv.addr_phy.addr_y = cap_mem->cap_yuv.addr_phy.addr_y;
        cap_mem->target_yuv.addr_vir.addr_y = cap_mem->cap_yuv.addr_vir.addr_y;
        cap_mem->target_yuv.addr_phy.addr_u =
            cap_mem->target_yuv.addr_phy.addr_y +
            image_size->width * image_size->height;
        cap_mem->target_yuv.addr_vir.addr_u =
            cap_mem->target_yuv.addr_vir.addr_y +
            image_size->width * image_size->height;
        ;
        cap_mem->target_yuv.fd = cap_mem->cap_yuv.fd;

        /* mem reuse when rot */
        cap_mem->target_jpeg.addr_phy.addr_y =
            cap_mem->cap_yuv_rot.addr_phy.addr_y;
        cap_mem->target_jpeg.addr_vir.addr_y =
            cap_mem->cap_yuv_rot.addr_vir.addr_y;
        cap_mem->target_jpeg.fd = cap_mem->cap_yuv_rot.fd;
    } else {
        if (need_scale) {
            cap_mem->cap_yuv_rot.addr_phy.addr_y =
                cap_mem->target_yuv.addr_phy.addr_y;
            cap_mem->cap_yuv_rot.addr_vir.addr_y =
                cap_mem->target_yuv.addr_vir.addr_y;
            cap_mem->cap_yuv_rot.addr_phy.addr_u =
                cap_mem->cap_yuv_rot.addr_phy.addr_y +
                cap_size->width * cap_size->height;
            cap_mem->cap_yuv_rot.addr_vir.addr_u =
                cap_mem->cap_yuv_rot.addr_vir.addr_y +
                cap_size->width * cap_size->height;
            cap_mem->cap_yuv_rot.fd = cap_mem->target_yuv.fd;

            cap_mem->cap_yuv_rot.size.width = cap_size->height;
            cap_mem->cap_yuv_rot.size.height = cap_size->width;
            cap_mem->cap_yuv_rot.buf_size =
                cap_size->height * cap_size->width * 3 / 2;
            cap_mem->cap_yuv_rot.fmt = IMG_DATA_TYPE_YUV420;
        } else {
            cap_mem->cap_yuv_rot.addr_phy.addr_y =
                cap_mem->cap_yuv.addr_phy.addr_y;
            cap_mem->cap_yuv_rot.addr_vir.addr_y =
                cap_mem->cap_yuv.addr_vir.addr_y;
            cap_mem->cap_yuv_rot.addr_phy.addr_u =
                cap_mem->cap_yuv_rot.addr_phy.addr_y +
                cap_size->width * cap_size->height;
            cap_mem->cap_yuv_rot.addr_vir.addr_u =
                cap_mem->cap_yuv_rot.addr_vir.addr_y +
                cap_size->width * cap_size->height;
            cap_mem->cap_yuv_rot.fd = cap_mem->cap_yuv.fd;

            cap_mem->cap_yuv_rot.size.width = cap_size->height;
            cap_mem->cap_yuv_rot.size.height = cap_size->width;
            cap_mem->cap_yuv_rot.buf_size =
                cap_size->height * cap_size->width * 3 / 2;
            cap_mem->cap_yuv_rot.fmt = IMG_DATA_TYPE_YUV420;
        }

        /* mem reuse when rot */
        cap_mem->target_jpeg.addr_phy.addr_y = cap_mem->cap_yuv.addr_phy.addr_y;
        cap_mem->target_jpeg.addr_vir.addr_y = cap_mem->cap_yuv.addr_vir.addr_y;
        cap_mem->target_jpeg.fd = cap_mem->cap_yuv.fd;
    }

    CMR_LOGD("mem_res=%d, mem_end=%d", mem_res, mem_end);
    return 0;
}

uint32_t get_jpeg_size(uint32_t width, uint32_t height, uint32_t thum_width,
                       uint32_t thum_height) {
    uint32_t size;
    (void)thum_width;
    (void)thum_height;

    if ((width * height) <= JPEG_SMALL_SIZE) {
        size = CMR_JPEG_SZIE(width, height) + JPEG_EXIF_SIZE;
    } else {
        size = CMR_JPEG_SZIE(width, height);
    }

    return ADDR_BY_WORD(size);
}

uint32_t get_thum_yuv_size(uint32_t width, uint32_t height, uint32_t thum_width,
                           uint32_t thum_height) {
    (void)width;
    (void)height;
    return (uint32_t)(CMR_ADDR_ALIGNED((thum_width * thum_height)) * 3 / 2);
}
uint32_t get_thum_jpeg_size(uint32_t width, uint32_t height,
                            uint32_t thum_width, uint32_t thum_height) {
    uint32_t size;

    (void)width;
    (void)height;
    size = CMR_JPEG_SZIE(thum_width, thum_height);
    return ADDR_BY_WORD(size);
}

uint32_t get_jpg_tmp_size(uint32_t width, uint32_t height, uint32_t thum_width,
                          uint32_t thum_height) {
    (void)thum_width;
    (void)thum_height;
    return (uint32_t)(width * CMR_SLICE_HEIGHT * 2); // TBD
}
uint32_t get_scaler_tmp_size(uint32_t width, uint32_t height,
                             uint32_t thum_width, uint32_t thum_height) {
    UNUSED(width);
    UNUSED(height);

    uint32_t slice_buffer;

    (void)thum_width;
    (void)thum_height;

    slice_buffer = 0; /*(uint32_t)(width * CMR_SLICE_HEIGHT *
                         CMR_ZOOM_FACTOR);only UV422 need dwon sample to UV420*/

    return slice_buffer;
}

uint32_t get_isp_tmp_size(uint32_t width, uint32_t height, uint32_t thum_width,
                          uint32_t thum_height) {
    uint32_t slice_buffer;

    (void)thum_width;
    (void)thum_height;
    UNUSED(width);
    UNUSED(height);

    slice_buffer = (uint32_t)(
        width * CMR_SLICE_HEIGHT); /*only UV422 need dwon sample to UV420*/

    return slice_buffer;
}

uint32_t camera_get_aligned_size(uint32_t type, uint32_t size) {
    uint32_t size_aligned = 0;

    CMR_LOGI("type %d", type);
    if (CAMERA_MEM_NO_ALIGNED == type) {
        size_aligned = size;
    } else {
        size_aligned = CAMERA_ALIGNED_16(size);
    }

    return size_aligned;
}

void camera_set_mem_multimode(multiCameraMode camera_mode) {
    CMR_LOGD("camera_mode %d", camera_mode);
    is_multi_camera_mode_mem = camera_mode;
}
