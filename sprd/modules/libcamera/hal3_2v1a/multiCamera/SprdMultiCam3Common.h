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

#ifndef SPRDMULTICAMERACOMMON_H_HEADER
#define SPRDMULTICAMERACOMMON_H_HEADER

#include "../SprdCamera3HWI.h"
#include <string>

namespace sprdcamera {

#define WAIT_FRAME_TIMEOUT 2000e6
#define THREAD_TIMEOUT 30e6
#define LIB_GPU_PATH "libimagestitcher.so"
#define CONTEXT_SUCCESS 1
#define CONTEXT_FAIL 0
#define TIME_DIFF (15e6)
#define MAX_CONVERED_VALURE 10
#define MAX_F_FUMBER (10)
#define MIN_F_FUMBER (1)
#define MAX_BLUR_F_FUMBER (20)
#define MIN_BLUR_F_FUMBER (1)

#ifdef CONFIG_SUPERRESOLUTION_ENABLED
#define BOKEH_REFOCUS_COMMON_PARAM_NUM (13)
#else
#define BOKEH_REFOCUS_COMMON_PARAM_NUM (12)
#endif

typedef enum { STATE_NOT_READY, STATE_IDLE, STATE_BUSY } currentStatus;

typedef enum {
    /* There are valid result in both of list*/
    QUEUE_VALID = 0,
    /* There are no valid result in both of list */
    QUEUE_INVALID
} twoQuqueStatus;

typedef enum { MATCH_FAILED = 0, MATCH_SUCCESS } matchResult;

typedef enum { NOTIFY_SUCCESS = 0, NOTIFY_ERROR, NOTIFY_NOT_FOUND } notifytype;

typedef enum {
    PREVIEW_MAIN_BUFFER = 0,
    PREVIEW_DEPTH_BUFFER,
    SNAPSHOT_MAIN_BUFFER,
    SNAPSHOT_DEPTH_BUFFER,
    SNAPSHOT_TRANSFORM_BUFFER,
    DEPTH_OUT_BUFFER,
    DEPTH_OUT_WEIGHTMAP,
    YUV420
} camera_buffer_type_t;

typedef enum {
    CAMERA_LEFT = 0,
    CAMERA_RIGHT,
    MAX_CAMERA_PER_BUNDLE
} cameraIndex;

typedef enum {
    DEFAULT_STREAM = 0,
    PREVIEW_STREAM,
    VIDEO_STREAM,
    CALLBACK_STREAM,
    SNAPSHOT_STREAM,
} streamType_t;

enum sensor_stream_ctrl {
    STREAM_OFF = 0,
    STREAM_ON = 1,
};

typedef struct {
    uint32_t frame_number;
    int32_t vcm_steps;
    uint8_t otp_data[8192];
} depth_input_params_t;

typedef struct {
    uint32_t x_start;
    uint32_t y_start;
    uint32_t x_end;
    uint32_t y_end;
} coordinate_t;

/* Struct@ sprdcamera_physical_descriptor_t
 *
 *  Description@ This structure specifies various attributes
 *      physical cameras enumerated on the device
 */
typedef struct {
    // Userspace Physical Camera ID
    uint8_t id;
    // Camera Info
    camera_info cam_info;
    // Reference to HWI
    SprdCamera3HWI *hwi;
    // Reference to camera device structure
    camera3_device_t *dev;
    // Camera type:main camera or aux camera
} sprdcamera_physical_descriptor_t;

typedef struct {
    // Camera Device to be shared to Frameworks
    camera3_device_t dev;
    // Camera Info
    camera_info cam_info;
    // Logical Camera Facing
    int32_t facing;
    // Main Camera Id
    uint32_t id;
} sprd_virtual_camera_t;

typedef struct {
    uint32_t frame_number;
    uint64_t timestamp;
    buffer_handle_t *buffer;
    int status;
    int vcmSteps;
} hwi_frame_buffer_info_t;

typedef struct {
    uint32_t frame_number;
    const camera3_stream_buffer_t *input_buffer;
    camera3_stream_t *stream;
    buffer_handle_t *buffer1; // main sensor
    int status1;
    buffer_handle_t *buffer2; // aux sensor
    int status2;
    int vcmSteps;
} frame_matched_info_t;

typedef enum { MUXER_MSG_DATA_PROC = 1, MUXER_MSG_EXIT } muxerMsgType;

typedef struct {
    muxerMsgType msg_type;
    frame_matched_info_t combo_frame;
} muxer_queue_msg_t;

typedef struct {
    uint32_t frame_number;
    int32_t request_id;
    bool invalid;
    int showPreviewDeviceId;
    int32_t perfectskinlevel;
    int32_t g_face_info[4];
    buffer_handle_t *buffer;
    camera3_stream_t *stream;
    camera3_stream_t *callback_stream;
    buffer_handle_t *callback_buffer;
    camera3_stream_buffer_t *input_buffer;
    int rotation;
} old_request;

typedef struct {
    const native_handle_t *native_handle;
    MemIon *pHeapIon;
} new_mem_t;

typedef struct {
    struct private_handle_t *left_buf;
    struct private_handle_t *right_buf;
    struct private_handle_t *dst_buf;
    int rot_angle;
} dcam_info_t;

typedef struct {
    void *handle;
    int (*initRenderContext)(struct stream_info_s *resolution,
                             float *homography_matrix, int matrix_size);
    void (*imageStitchingWithGPU)(dcam_info_t *dcam);
    void (*destroyRenderContext)(void);
} GPUAPI_t;

typedef enum {
    /* Main camera device id*/
    CAM_MAIN_ID = 1,
    /* Aux camera device id*/
    CAM_AUX_ID = 3
} CameraID;

typedef enum {
    /* Main camera device id*/
    CAM_BLUR_MAIN_ID = 0,
    /* Main front camera device id*/
    CAM_BLUR_MAIN_ID_2 = 1,
    /* Aux camera device id*/
    CAM_BLUR_AUX_ID = 2,
    /* Aux front camera device id*/
    CAM_BLUR_AUX_ID_2 = 3,
} CameraBlurID;

typedef enum {
    /* Main camera device id*/
    CAM_SIDEBYSIDE_MAIN_ID = 0,
    /* Main front camera device id*/
    CAM_SIDEBYSIDE_MAIN_ID_2 = 1,
    /* Aux camera device id*/
    CAM_SIDEBYSIDE_AUX_ID = 2,
    /* Aux front camera device id*/
    CAM_SIDEBYSIDE_AUX_ID_2 = 3,
} CameraSideBySideID;

typedef enum {
    /* Main camera of the related cam subsystem which controls*/
    CAM_TYPE_MAIN = 0,
    /* Aux camera of the related cam subsystem */
    CAM_TYPE_AUX
} CameraType;

struct stream_info_s {
    int src_width;
    int src_height;
    int dst_width;
    int dst_height;
    int rot_angle;
}; // stream_info_t;

typedef struct line_buf_s {
    float homography_matrix[18];
    float warp_matrix[18];
    int points[32];
} line_buf_t;

typedef enum {
    BLUR_SELFSHOT_LOWLIGHT_DISABLE = 0,
    BLUR_SELFSHOT_NO_CONVERED,
    BLUR_SELFSHOT_CONVERED,
    NORMAL_LIGHT_VALUE = 3,
    LOW_LIGHT_VALUE,
    MAX_EXIT
} sprd_convered_info_t;

enum rot_angle { ROT_0 = 0, ROT_90, ROT_180, ROT_270, ROT_MAX };
typedef enum { DARK_LIGHT = 0, LOW_LIGHT, BRIGHT_LIGHT } scene_Light;
};

#endif
