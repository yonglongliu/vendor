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

#define LOG_TAG "cmr_preview"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include <stdlib.h>
#include <math.h>
#include <cutils/trace.h>
#include <cutils/properties.h>
#include "cmr_preview.h"
#include "cmr_msg.h"
#include "cmr_mem.h"
#include "cmr_ipm.h"
#include "cmr_grab.h"
#include "cmr_sensor.h"
#include "SprdOEMCamera.h"
#include "cmr_cvt.h"
#include "cmr_oem.h"

#undef YUV_TO_ISP
/**************************MCARO
 * DEFINITION********************************************************************/
// abilty, max support buf num
#define PREV_FRM_CNT GRAB_BUF_MAX
#define PREV_DATATYPE_CNT 8
#define PREV_ROT_FRM_CNT GRAB_BUF_MAX
#define ZSL_FRM_CNT GRAB_BUF_MAX
#define ZSL_ROT_FRM_CNT GRAB_BUF_MAX
#define PDAF_FRM_CNT 4

// actually, the num alloced for preview/video/zsl, hal1.0 will use this
#define PREV_FRM_ALLOC_CNT 8
#define PREV_ROT_FRM_ALLOC_CNT 8
#define ZSL_FRM_ALLOC_CNT 8
#define ZSL_ROT_FRM_ALLOC_CNT 8

#define PREV_MSG_QUEUE_SIZE 50
#define PREV_RECOVERY_CNT 3

#define PREV_EVT_BASE (CMR_EVT_PREVIEW_BASE + 0x100)
#define PREV_EVT_INIT (PREV_EVT_BASE + 0x0)
#define PREV_EVT_EXIT (PREV_EVT_BASE + 0x1)
#define PREV_EVT_SET_PARAM (PREV_EVT_BASE + 0x2)
#define PREV_EVT_START (PREV_EVT_BASE + 0x3)
#define PREV_EVT_STOP (PREV_EVT_BASE + 0x4)
#define PREV_EVT_UPDATE_ZOOM (PREV_EVT_BASE + 0x5)
#define PREV_EVT_BEFORE_SET (PREV_EVT_BASE + 0x6)
#define PREV_EVT_AFTER_SET (PREV_EVT_BASE + 0x7)
#define PREV_EVT_RECEIVE_DATA (PREV_EVT_BASE + 0x8)
#define PREV_EVT_GET_POST_PROC_PARAM (PREV_EVT_BASE + 0x9)
#define PREV_EVT_FD_CTRL (PREV_EVT_BASE + 0xA)
#define PREV_EVT_CANCEL_SNP (PREV_EVT_BASE + 0xB)
#define PREV_EVT_SET_PREV_BUFFER (PREV_EVT_BASE + 0xC)
#define PREV_EVT_SET_VIDEO_BUFFER (PREV_EVT_BASE + 0xD)
#define PREV_EVT_SET_ZSL_BUFFER (PREV_EVT_BASE + 0xE)
#define PREV_EVT_SET_PDAF_BUFFER (PREV_EVT_BASE + 0xF)

#define PREV_EVT_CB_INIT (PREV_EVT_BASE + 0x10)
#define PREV_EVT_CB_START (PREV_EVT_BASE + 0x11)
#define PREV_EVT_CB_EXIT (PREV_EVT_BASE + 0x12)
#define PREV_EVT_ASSIST_START (PREV_EVT_BASE + 0x13)
#define PREV_EVT_ASSIST_STOP (PREV_EVT_BASE + 0x14)
#define PREV_EVT_SET_PDAF_DATATYPE_BUFFER (PREV_EVT_BASE + 0x15)

#define ALIGN_16_PIXEL(x) (((x) + 15) & (~15))\

#define UHD_WIDTH 3840
#define UHD_HEIGHT 2160

#define IS_PREVIEW(handle, cam_id)                                             \
    (PREVIEWING ==                                                             \
     (cmr_uint)((struct prev_handle *)handle->prev_cxt[cam_id].prev_status))
#define IS_PREVIEW_FRM(id) ((id & CMR_PREV_ID_BASE) == CMR_PREV_ID_BASE)
#define IS_VIDEO_FRM(id) ((id & CMR_VIDEO_ID_BASE) == CMR_VIDEO_ID_BASE)
#define IS_ZSL_FRM(id) ((id & CMR_CAP1_ID_BASE) == CMR_CAP1_ID_BASE)
#define CAP_SIM_ROT(handle, cam_id)                                            \
    (((struct prev_handle *)handle)                                            \
         ->prev_cxt[cam_id]                                                    \
         .prev_param.is_cfg_rot_cap &&                                         \
     (IMG_ANGLE_0 ==                                                           \
      ((struct prev_handle *)handle)->prev_cxt[cam_id].prev_status))

#define IS_RESEARCH(search_h, h)                                               \
    (search_h != h && h >= search_h * 3 / 2 && search_h >= 1088)

#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            return CMR_CAMERA_INVALID_PARAM;                                   \
        }                                                                      \
    } while (0)

#define CHECK_CAMERA_ID(id)                                                    \
    do {                                                                       \
        if (id >= CAMERA_ID_MAX) {                                             \
            return CMR_CAMERA_INVALID_PARAM;                                   \
        }                                                                      \
    } while (0)

/**************************LOCAL FUNCTION
 * DECLEARATION*********************************************************/
enum isp_status {
    PREV_ISP_IDLE = 0,
    PREV_ISP_COWORK,
    PREV_ISP_POST_PROCESS,
    PREV_ISP_ERR,
    PREV_ISP_MAX,
};

enum cvt_status {
    PREV_CVT_IDLE = 0,
    PREV_CVT_ROTATING,
    PREV_CVT_ROT_DONE,
    PREV_CVT_MAX,
};

enum chn_status { PREV_CHN_IDLE = 0, PREV_CHN_BUSY };

enum recovery_status {
    PREV_RECOVERY_IDLE = 0,
    PREV_RECOVERING,
    PREV_RECOVERY_DONE
};

enum recovery_mode { RECOVERY_LIGHTLY = 0, RECOVERY_MIDDLE, RECOVERY_HEAVY };

struct rot_param {
    cmr_uint angle;
    struct img_frm *src_img;
    struct img_frm *dst_img;
};

struct prev_context {
    cmr_uint camera_id;
    struct preview_param prev_param;

    cmr_int out_ret_val; /*for external function get return value*/

    /*preview*/
    struct img_size actual_prev_size;
    cmr_uint prev_status;
    cmr_uint prev_mode;
    struct img_rect prev_rect;
    cmr_uint skip_mode;
    cmr_uint prev_channel_deci;
    cmr_uint prev_preflash_skip_en;
    cmr_uint prev_skip_num;
    cmr_uint prev_channel_id;
    cmr_uint prev_channel_status;
    struct img_data_end prev_data_endian;
    cmr_uint prev_frm_cnt;
    struct rot_param rot_param;
    cmr_s64 restart_timestamp;
    cmr_uint restart_skip_cnt;
    cmr_uint restart_skip_en;
    struct img_size lv_size;    /*isp lv size*/
    struct img_size video_size; /*isp video and scl size*/

    cmr_uint prev_self_restart;
    cmr_uint prev_buf_id;
    struct img_frm prev_frm[PREV_FRM_CNT];
    struct img_frm prev_reserved_frm[PREV_RESERVED_FRM_CNT];
    cmr_uint prev_rot_index;
    cmr_uint prev_rot_frm_is_lock[PREV_ROT_FRM_CNT];
    struct img_frm prev_rot_frm[PREV_ROT_FRM_CNT];
    cmr_uint prev_phys_addr_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    cmr_uint prev_virt_addr_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    cmr_s32 prev_fd_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    cmr_uint prev_reserved_phys_addr[PREV_RESERVED_FRM_CNT];
    cmr_uint prev_reserved_virt_addr[PREV_RESERVED_FRM_CNT];
    cmr_s32 prev_reserved_fd[PREV_RESERVED_FRM_CNT];
    cmr_uint prev_mem_size;
    cmr_uint prev_mem_num;
    cmr_int prev_mem_valid_num;
#ifdef CONFIG_Y_IMG_TO_ISP
    cmr_uint prev_mem_y_size;
    cmr_uint prev_mem_y_num;
    cmr_uint prev_phys_y_addr_array[2];
    cmr_uint prev_virt_y_addr_array[2];
    cmr_s32 prev_mfd_y_array[2];
#endif
    cmr_uint prev_mem_yuv_size;
    cmr_uint prev_mem_yuv_num;
    cmr_uint prev_phys_yuv_addr;
    cmr_uint prev_virt_yuv_addr;
    cmr_s32 prev_mfd_yuv;
    /*video*/
    struct img_size actual_video_size;
    cmr_uint video_status;
    cmr_uint video_mode;
    struct img_rect video_rect;
    // cmr_uint                        video_skip_mode;
    // cmr_uint                        prev_skip_num;
    cmr_uint video_channel_id;
    cmr_uint video_channel_status;
    struct img_data_end video_data_endian;
    cmr_uint video_frm_cnt;
    struct rot_param video_rot_param;
    cmr_s64 video_restart_timestamp;
    cmr_uint video_restart_skip_cnt;
    cmr_uint video_restart_skip_en;

    cmr_uint video_self_restart;
    cmr_uint video_buf_id;
    struct img_frm video_frm[PREV_FRM_CNT];
    struct img_frm video_reserved_frm[VIDEO_RESERVED_FRM_CNT + 1];
    cmr_uint video_rot_index;
    cmr_uint video_rot_frm_is_lock[PREV_ROT_FRM_CNT];
    struct img_frm video_rot_frm[PREV_ROT_FRM_CNT];
    cmr_uint video_phys_addr_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    cmr_uint video_virt_addr_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    cmr_s32 video_fd_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    cmr_uint video_reserved_phys_addr[VIDEO_RESERVED_FRM_CNT];
    cmr_uint video_reserved_virt_addr[VIDEO_RESERVED_FRM_CNT];
    cmr_s32 video_reserved_fd[VIDEO_RESERVED_FRM_CNT];
    cmr_uint video_mem_size;
    cmr_uint video_mem_num;
    cmr_uint video_mem_valid_num;

    /*capture*/
    cmr_uint cap_mode;
    cmr_uint cap_need_isp;
    cmr_uint cap_need_binning;
    struct img_size max_size;
    struct img_size aligned_pic_size;
    struct img_size actual_pic_size;
    struct img_size dealign_actual_pic_size;
    struct channel_start_param restart_chn_param;
    cmr_uint cap_channel_id;
    cmr_uint cap_channel_status;
    struct img_data_end cap_data_endian;
    cmr_uint cap_frm_cnt;
    cmr_uint cap_skip_num;
    cmr_uint cap_org_fmt;
    struct img_size cap_org_size;
    cmr_uint cap_zoom_mode;
    struct img_rect cap_sn_trim_rect;
    struct img_size cap_sn_size;
    struct img_rect cap_scale_src_rect;

    cmr_uint cap_phys_addr_array[CMR_CAPTURE_MEM_SUM];
    cmr_uint cap_virt_addr_array[CMR_CAPTURE_MEM_SUM];
    cmr_s32 cap_fd_array[CMR_CAPTURE_MEM_SUM];
    cmr_uint cap_phys_addr_path_array[CMR_CAPTURE_MEM_SUM];
    cmr_uint cap_virt_addr_path_array[CMR_CAPTURE_MEM_SUM];
    cmr_s32 cap_fd_path_array[CMR_CAPTURE_MEM_SUM];
    cmr_uint cap_hdr_phys_addr_path_array[HDR_CAP_NUM];
    cmr_uint cap_hdr_virt_addr_path_array[HDR_CAP_NUM];
    cmr_s32 cap_hdr_fd_path_array[HDR_CAP_NUM];

    struct cmr_cap_mem cap_mem[CMR_CAPTURE_MEM_SUM];
    struct img_frm cap_frm[CMR_CAPTURE_MEM_SUM];
    cmr_uint is_zsl_frm;
    cmr_uint cap_zsl_buf_id;
    cmr_s64 cap_zsl_restart_timestamp;
    cmr_uint cap_zsl_restart_skip_cnt;
    cmr_uint cap_zsl_restart_skip_en;
    cmr_uint cap_zsl_frm_cnt;
    struct img_frm cap_zsl_frm[ZSL_FRM_CNT];
    struct img_frm cap_zsl_reserved_frm[CAP_ZSL_RESERVED_FRM_CNT + 1];
    cmr_uint cap_zsl_rot_index;
    cmr_uint cap_zsl_rot_frm_is_lock[ZSL_ROT_FRM_CNT];
    struct img_frm cap_zsl_rot_frm[ZSL_ROT_FRM_CNT];

    cmr_uint cap_zsl_phys_addr_array[ZSL_FRM_CNT + ZSL_ROT_FRM_CNT];
    cmr_uint cap_zsl_virt_addr_array[ZSL_FRM_CNT + ZSL_ROT_FRM_CNT];
    cmr_s32 cap_zsl_fd_array[ZSL_FRM_CNT + ZSL_ROT_FRM_CNT];
    cmr_uint cap_zsl_reserved_phys_addr[CAP_ZSL_RESERVED_FRM_CNT];
    cmr_uint cap_zsl_reserved_virt_addr[CAP_ZSL_RESERVED_FRM_CNT];
    cmr_s32 cap_zsl_reserved_fd[CAP_ZSL_RESERVED_FRM_CNT];

    cmr_uint cap_zsl_mem_size;
    cmr_uint cap_zsl_mem_num;
    cmr_int cap_zsl_mem_valid_num;

    cmr_uint is_reprocessing;

    /*sensor datatype*/
    cmr_uint sensor_datatype_cnt;
    cmr_uint prev_cnt;
    cmr_uint sensor_datatype_channel_id;
    cmr_uint sensor_datatype_channel_status;
    cmr_uint sensor_datatype_frm_cnt;
    cmr_uint sensor_datatype_buf_id;
    struct img_frm sensor_datatype_frm[PREV_FRM_CNT];
    cmr_uint sensor_datatype_phys_addr_array[PREV_FRM_CNT];
    cmr_uint sensor_datatype_virt_addr_array[PREV_FRM_CNT];
    cmr_s32 sensor_datatype_fd_array[PREV_FRM_CNT];
    cmr_u32 sensor_datatype_mem_alloc_flag;
    cmr_uint sensor_datatype_mem_size;
    cmr_uint sensor_datatype_mem_num;
    cmr_int sensor_datatype_mem_valid_num;
    cmr_uint sensor_datatype_reserved_phys_addr;
    cmr_uint sensor_datatype_reserved_virt_addr;
    cmr_s32 sensor_datatype_reserved_fd;
    struct img_frm sensor_datatype_reserved_frm;
    cmr_s64 sensor_datatype_timestamp;
    cmr_uint sensor_datatype_frm_id;
    struct frm_info *sensor_datatype_data;
    struct img_frm *preview_bakcup_frm;
    cmr_s64 preview_bakcup_timestamp;
    struct frm_info *preview_bakcup_data;
    struct camera_frame_type *preview_bakcup_frame_type;
    struct touch_coordinate touch_info;
    enum sensor_mode_size sensor_size_mode; // add for isp tuning mode 0: sensor
                                            // full size. 1:sensor bining size

    /*pdaf raw*/
    struct img_rect pdaf_rect;
    cmr_uint pdaf_channel_id;
    cmr_uint pdaf_channel_status;
    struct img_data_end pdaf_data_endian;
    struct img_frm pdaf_frm[PDAF_FRM_CNT];
    cmr_uint pdaf_phys_addr_array[PDAF_FRM_CNT];
    cmr_uint pdaf_virt_addr_array[PDAF_FRM_CNT];
    cmr_s32 pdaf_fd_array[PDAF_FRM_CNT];
    cmr_u32 pdaf_mem_alloc_flag;
    cmr_uint pdaf_mem_size;
    cmr_uint pdaf_mem_num;
    cmr_int pdaf_mem_valid_num;
    cmr_uint pdaf_reserved_phys_addr;
    cmr_uint pdaf_reserved_virt_addr;
    cmr_s32 pdaf_reserved_fd;
    struct img_frm pdaf_reserved_frm;
    cmr_s64 pdaf_timestamp;
    cmr_uint pdaf_frm_cnt;

    /*common*/
    cmr_handle fd_handle;
    cmr_handle refocus_handle;
    cmr_uint recovery_status;
    cmr_uint recovery_cnt;
    cmr_uint isp_status;
    struct sensor_exp_info sensor_info;
    cmr_uint ae_time;
    void *private_data;
    cmr_uint vcm_step;
};

struct prev_thread_cxt {
    cmr_uint is_inited;
    cmr_handle thread_handle;
    sem_t prev_sync_sem;
    pthread_mutex_t prev_mutex;

    /*callback thread*/
    cmr_handle cb_thread_handle;
    cmr_handle assist_thread_handle;
};

struct prev_handle {
    cmr_handle oem_handle;
    cmr_handle ipm_handle;
    cmr_uint sensor_bits; // multi-sensors need multi mem ? channel_cfg
    preview_cb_func oem_cb;
    struct preview_md_ops ops;
    void *private_data;
    struct prev_thread_cxt thread_cxt;
    struct prev_context prev_cxt[CAMERA_ID_MAX];
    cmr_uint frame_active;
};

struct prev_cb_info {
    enum preview_cb_type cb_type;
    enum preview_func_type func_type;
    struct camera_frame_type *frame_data;
};

struct internal_param {
    void *param1;
    void *param2;
    void *param3;
    void *param4;
};

/**************************LOCAL FUNCTION
 * DECLEARATION*********************************************************/
static cmr_int prev_create_thread(struct prev_handle *handle);

static cmr_int prev_destroy_thread(struct prev_handle *handle);

static cmr_int prev_create_cb_thread(struct prev_handle *handle);

static cmr_int prev_destroy_cb_thread(struct prev_handle *handle);

static cmr_int prev_assist_thread_proc(struct cmr_msg *message, void *p_data);

static cmr_int prev_thread_proc(struct cmr_msg *message, void *p_data);

static cmr_int prev_cb_thread_proc(struct cmr_msg *message, void *p_data);

static cmr_int prev_cb_start(struct prev_handle *handle,
                             struct prev_cb_info *cb_info);

static cmr_int prev_frame_handle(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct frm_info *data);

static cmr_int prev_preview_frame_handle(struct prev_handle *handle,
                                         cmr_u32 camera_id,
                                         struct frm_info *data);

static cmr_int prev_pdaf_datatype_cb(struct pd_frame_in *cb_param);

static cmr_int prev_pdaf_datatype_frame_handle(struct prev_handle *handle,
                                               cmr_u32 camera_id,
                                               struct frm_info *data);

static cmr_int prev_pdaf_raw_cb(struct pd_frame_in *cb_param);

static cmr_int prev_sensor_datatype_frame_handle(struct prev_handle *handle,
                                                 cmr_u32 camera_id,
                                                 struct frm_info *data);

static cmr_int prev_pdaf_raw_frame_handle(struct prev_handle *handle,
                                          cmr_u32 camera_id,
                                          struct frm_info *data);

static cmr_int prev_video_frame_handle(struct prev_handle *handle,
                                       cmr_u32 camera_id,
                                       struct frm_info *data);

static cmr_int prev_capture_frame_handle(struct prev_handle *handle,
                                         cmr_u32 camera_id,
                                         struct frm_info *data);

static cmr_int prev_zsl_frame_handle(struct prev_handle *handle,
                                     cmr_u32 camera_id, struct frm_info *data);

static cmr_int prev_error_handle(struct prev_handle *handle, cmr_u32 camera_id,
                                 cmr_uint evt_type);

static cmr_int prev_recovery_pre_proc(struct prev_handle *handle,
                                      cmr_u32 camera_id,
                                      enum recovery_mode mode);

static cmr_int prev_recovery_post_proc(struct prev_handle *handle,
                                       cmr_u32 camera_id,
                                       enum recovery_mode mode);

static cmr_int prev_recovery_reset(struct prev_handle *handle,
                                   cmr_u32 camera_id);

static cmr_int prev_local_init(struct prev_handle *handle);

static cmr_int prev_local_deinit(struct prev_handle *handle);

static cmr_int prev_pre_set(struct prev_handle *handle, cmr_u32 camera_id);

static cmr_int prev_post_set(struct prev_handle *handle, cmr_u32 camera_id);

static cmr_int prev_start(struct prev_handle *handle, cmr_u32 camera_id,
                          cmr_u32 is_restart, cmr_u32 is_sn_reopen);

static cmr_int prev_stop(struct prev_handle *handle, cmr_u32 camera_id,
                         cmr_u32 is_restart);

static cmr_int prev_cancel_snapshot(struct prev_handle *handle,
                                    cmr_u32 camera_id);

static cmr_int prev_alloc_prev_buf(struct prev_handle *handle,
                                   cmr_u32 camera_id, cmr_u32 is_restart,
                                   struct buffer_cfg *buffer);

static cmr_int prev_free_prev_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                  cmr_u32 is_restart);

static cmr_int prev_alloc_video_buf(struct prev_handle *handle,
                                    cmr_u32 camera_id, cmr_u32 is_restart,
                                    struct buffer_cfg *buffer);

static cmr_int prev_free_video_buf(struct prev_handle *handle,
                                   cmr_u32 camera_id, cmr_u32 is_restart);

static cmr_int prev_alloc_cap_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                  cmr_u32 is_restart,
                                  struct buffer_cfg *buffer);

static cmr_int prev_free_cap_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                 cmr_u32 is_restart);

static cmr_int prev_alloc_zsl_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                  cmr_u32 is_restart,
                                  struct buffer_cfg *buffer);

static cmr_int prev_free_zsl_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                 cmr_u32 is_restart);

static cmr_int prev_free_sensor_datatype_buf(struct prev_handle *handle,
                                             cmr_u32 camera_id,
                                             cmr_u32 is_restart);

static cmr_int prev_free_cap_reserve_buf(struct prev_handle *handle,
                                         cmr_u32 camera_id, cmr_u32 is_restart);

static cmr_int prev_alloc_pdaf_raw_buf(struct prev_handle *handle,
                                       cmr_u32 camera_id, cmr_u32 is_restart,
                                       struct buffer_cfg *buffer);

static cmr_int prev_free_pdaf_raw_buf(struct prev_handle *handle,
                                      cmr_u32 camera_id, cmr_u32 is_restart);

static cmr_int prev_get_sensor_mode(struct prev_handle *handle,
                                    cmr_u32 camera_id);

static cmr_int prev_get_sn_preview_mode(struct prev_handle *handle,
                                        cmr_u32 camera_id,
                                        struct sensor_exp_info *sensor_info,
                                        struct img_size *target_size,
                                        cmr_uint *work_mode);
cmr_int prev_get_sn_capture_mode(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct sensor_exp_info *sensor_info,
                                 struct img_size *target_size,
                                 cmr_uint *work_mode);

static cmr_int prev_get_sn_inf(struct prev_handle *handle, cmr_u32 camera_id,
                               cmr_u32 frm_deci, struct sensor_if *sn_if);

static cmr_int prev_get_cap_max_size(struct prev_handle *handle,
                                     cmr_u32 camera_id,
                                     struct sensor_mode_info *sn_mode,
                                     struct img_size *max_size);

static cmr_int prev_construct_frame(struct prev_handle *handle,
                                    cmr_u32 camera_id, struct frm_info *info,
                                    struct camera_frame_type *frame_type);

static cmr_int prev_construct_video_frame(struct prev_handle *handle,
                                          cmr_u32 camera_id,
                                          struct frm_info *info,
                                          struct camera_frame_type *frame_type);

static cmr_int prev_construct_zsl_frame(struct prev_handle *handle,
                                        cmr_u32 camera_id,
                                        struct frm_info *info,
                                        struct camera_frame_type *frame_type);

static cmr_int prev_set_param_internal(struct prev_handle *handle,
                                       cmr_u32 camera_id, cmr_u32 is_restart,
                                       struct preview_out_param *out_param_ptr);

static cmr_int prev_set_prev_param(struct prev_handle *handle,
                                   cmr_u32 camera_id, cmr_u32 is_restart,
                                   struct preview_out_param *out_param_ptr);

static cmr_int
prev_set_sensor_datatype_param(struct prev_handle *handle, cmr_u32 camera_id,
                               cmr_u32 is_restart,
                               struct preview_out_param *out_param_ptr);

static cmr_int prev_set_pdaf_raw_param(struct prev_handle *handle,
                                       cmr_u32 camera_id, cmr_u32 is_restart,
                                       struct preview_out_param *out_param_ptr);

static cmr_int prev_set_video_param(struct prev_handle *handle,
                                    cmr_u32 camera_id, cmr_u32 is_restart,
                                    struct preview_out_param *out_param_ptr);

static cmr_int prev_set_prev_param_lightly(struct prev_handle *handle,
                                           cmr_u32 camera_id);

static cmr_int prev_set_cap_param(struct prev_handle *handle, cmr_u32 camera_id,
                                  cmr_u32 is_restart, cmr_u32 is_lightly,
                                  struct preview_out_param *out_param_ptr);

static cmr_int prev_set_videocap_param(struct prev_handle *handle,
                                       cmr_u32 camera_id, cmr_u32 is_restart,
                                       cmr_u32 is_lightly,
                                       struct preview_out_param *out_param_ptr);

static cmr_int prev_update_cap_param(struct prev_handle *handle,
                                     cmr_u32 camera_id, cmr_u32 encode_angle,
                                     struct snp_proc_param *snp_proc_param);

static cmr_int prev_set_zsl_param_lightly(struct prev_handle *handle,
                                          cmr_u32 camera_id);

static cmr_int prev_set_cap_param_raw(struct prev_handle *handle,
                                      cmr_u32 camera_id, cmr_u32 is_restart,
                                      struct preview_out_param *out_param_ptr);

static cmr_int prev_set_pdaf_param(struct prev_handle *handle,
                                   cmr_u32 camera_id,
                                   struct preview_out_param *out_param_ptr);

static cmr_int prev_cap_ability(struct prev_handle *handle, cmr_u32 camera_id,
                                struct img_size *cap_size,
                                struct img_frm_cap *img_cap);

static cmr_int prev_get_scale_rect(struct prev_handle *handle,
                                   cmr_u32 camera_id, cmr_u32 rot,
                                   struct snp_proc_param *cap_post_proc_param);

static cmr_int prev_before_set_param(struct prev_handle *handle,
                                     cmr_u32 camera_id,
                                     enum preview_param_mode mode);

static cmr_int prev_after_set_param(struct prev_handle *handle,
                                    cmr_u32 camera_id,
                                    enum preview_param_mode mode,
                                    enum img_skip_mode skip_mode,
                                    cmr_u32 skip_number);

static cmr_uint prev_get_rot_val(cmr_uint rot_enum);

static cmr_uint prev_get_rot_enum(cmr_uint rot_val);

static cmr_uint prev_set_rot_buffer_flag(struct prev_context *prev_cxt,
                                         cmr_uint type, cmr_int index,
                                         cmr_uint flag);

static cmr_uint prev_search_rot_buffer(struct prev_context *prev_cxt,
                                       cmr_uint type);

static cmr_uint prev_get_src_rot_buffer(struct prev_context *prev_cxt,
                                        struct frm_info *data, cmr_uint *index);

static cmr_int prev_start_rotate(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct frm_info *data);

static cmr_uint prev_get_prev_res_buffer_count(struct prev_context *prev_cxt);

static cmr_uint prev_get_capzsl_res_buffer_count(struct prev_context *prev_cxt);

static cmr_int
prev_get_cap_post_proc_param(struct prev_handle *handle, cmr_u32 camera_id,
                             cmr_u32 encode_angle,
                             struct snp_proc_param *out_param_ptr);

static cmr_int prev_receive_data(struct prev_handle *handle, cmr_u32 camera_id,
                                 cmr_uint evt, struct frm_info *data);

static cmr_int prev_pause_cap_channel(struct prev_handle *handle,
                                      cmr_u32 camera_id, struct frm_info *data);

static cmr_int prev_resume_cap_channel(struct prev_handle *handle,
                                       cmr_u32 camera_id,
                                       struct frm_info *data);

static cmr_int prev_restart_cap_channel(struct prev_handle *handle,
                                        cmr_u32 camera_id,
                                        struct frm_info *data);

static cmr_int prev_fd_open(struct prev_handle *handle, cmr_u32 camera_id);

static cmr_int prev_fd_close(struct prev_handle *handle, cmr_u32 camera_id);

static cmr_int prev_fd_send_data(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct img_frm *frm);

static cmr_int prev_fd_cb(cmr_u32 class_type, struct ipm_frame_out *cb_param);

static cmr_int prev_fd_ctrl(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 on_off);

static cmr_int prev_sensor_datatype_open(struct prev_handle *handle,
                                         cmr_u32 camera_id,
                                         struct sensor_data_info otp_data);

static cmr_int prev_sensor_datatype_close(struct prev_handle *handle,
                                          cmr_u32 camera_id);

static cmr_int
prev_sensor_datatype_send_data(struct prev_handle *handle, cmr_u32 camera_id,
                               struct img_frm *frm,
                               struct frm_info *sensor_datatype_frm);

static cmr_int prev_sensor_datatype_cb(cmr_u32 class_type,
                                       struct ipm_frame_out *cb_param);

static cmr_int prev_sensor_datatype_ctrl(struct prev_handle *handle,
                                         cmr_u32 camera_id, cmr_u32 on_off);

static cmr_int prev_set_preview_buffer(struct prev_handle *handle,
                                       cmr_u32 camera_id, cmr_uint src_phy_addr,
                                       cmr_uint src_vir_addr, cmr_s32 fd);

static cmr_int prev_pop_preview_buffer(struct prev_handle *handle,
                                       cmr_u32 camera_id, struct frm_info *data,
                                       cmr_u32 is_to_hal);

static cmr_int prev_set_sensor_datatype_buffer(struct prev_handle *handle,
                                               cmr_u32 camera_id,
                                               cmr_uint src_phy_addr,
                                               cmr_uint src_vir_addr,
                                               cmr_s32 fd);

static cmr_int prev_pop_sensor_datatype_buffer(struct prev_handle *handle,
                                               cmr_u32 camera_id,
                                               struct frm_info *data,
                                               cmr_u32 is_to_hal);

static cmr_int prev_set_pdaf_raw_buffer(struct prev_handle *handle,
                                        cmr_u32 camera_id,
                                        cmr_uint src_phy_addr,
                                        cmr_uint src_vir_addr, cmr_s32 fd);

static cmr_int prev_pop_pdaf_raw_buffer(struct prev_handle *handle,
                                        cmr_u32 camera_id,
                                        struct frm_info *data,
                                        cmr_u32 is_to_hal);

static cmr_int cmr_preview_set_pdaf_raw_buffer(cmr_handle preview_handle,
                                               cmr_u32 camera_id,
                                               cmr_uint src_phy_addr,
                                               cmr_uint src_vir_addr,
                                               cmr_s32 fd, cmr_u32 msg_type);

static cmr_int prev_set_video_buffer(struct prev_handle *handle,
                                     cmr_u32 camera_id, cmr_uint src_phy_addr,
                                     cmr_uint src_vir_addr, cmr_s32 fd);

static cmr_int prev_pop_video_buffer(struct prev_handle *handle,
                                     cmr_u32 camera_id, struct frm_info *data,
                                     cmr_u32 is_to_hal);

static cmr_int prev_set_zsl_buffer(struct prev_handle *handle,
                                   cmr_u32 camera_id, cmr_uint src_phy_addr,
                                   cmr_uint src_vir_addr, cmr_s32 fd);

static cmr_int prev_pop_zsl_buffer(struct prev_handle *handle,
                                   cmr_u32 camera_id, struct frm_info *data,
                                   cmr_u32 is_to_hal);

static cmr_int prev_capture_zoom_post_cap(struct prev_handle *handle,
                                          cmr_int *flag, cmr_u32 camera_id);
cmr_int prev_get_frm_index(struct img_frm *frame, struct frm_info *data);
cmr_int prev_is_need_scaling(cmr_handle preview_handle, cmr_u32 camera_id);
cmr_u32 prev_get_aligned_type(cmr_handle preview_handle, cmr_u32 camera_id);

/**************************FUNCTION
 * ***************************************************************************/
cmr_int cmr_preview_init(struct preview_init_param *init_param_ptr,
                         cmr_handle *preview_handle_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = NULL;

    if (!preview_handle_ptr || !init_param_ptr) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p", preview_handle_ptr,
                 init_param_ptr);
        return CMR_CAMERA_INVALID_PARAM;
    }

    handle = (struct prev_handle *)malloc(sizeof(struct prev_handle));
    if (!handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }
    cmr_bzero(handle, sizeof(struct prev_handle));

    /*save init param*/
    handle->oem_handle = init_param_ptr->oem_handle;
    handle->ipm_handle = init_param_ptr->ipm_handle;
    handle->sensor_bits = init_param_ptr->sensor_bits;
    handle->oem_cb = init_param_ptr->oem_cb;
    handle->ops = init_param_ptr->ops;
    handle->private_data = init_param_ptr->private_data;
    CMR_LOGD("oem_handle: 0x%lx, sensor_bits: %ld, private_data: 0x%lx",
             (cmr_uint)handle->oem_handle, handle->sensor_bits,
             (cmr_uint)handle->private_data);

    /*create thread*/
    ret = prev_create_thread(handle);
    if (ret) {
        CMR_LOGE("create thread failed!");
        ret = CMR_CAMERA_FAIL;
        goto init_end;
    }

    /*create callback thread*/
    ret = prev_create_cb_thread(handle);
    if (ret) {
        CMR_LOGE("create cb thread failed!");
        ret = CMR_CAMERA_FAIL;
        goto init_end;
    }

    /*return handle*/
    *preview_handle_ptr = (cmr_handle)handle;
    CMR_LOGI("handle 0x%lx created!", (cmr_uint)handle);

init_end:
    if (ret) {
        if (handle) {
            free(handle);
            handle = NULL;
        }
    }

    CMR_LOGI("ret %ld", ret);
    return ret;
}

cmr_int cmr_preview_deinit(cmr_handle preview_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 i = 0;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;

    if (!preview_handle) {
        CMR_LOGE("Invalid param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGI("in");

    /*check every device, if previewing, stop it*/
    for (i = 0; i < CAMERA_ID_MAX; i++) {
        CMR_LOGD("id %d, prev_status %ld", i, handle->prev_cxt[i].prev_status);

        if (PREVIEWING == handle->prev_cxt[i].prev_status) {
            /*prev_stop(handle, i, 0);*/
            cmr_preview_stop(preview_handle, i);
        }
    }

    /*destory thread*/
    ret = prev_destroy_thread(handle);
    if (ret) {
        CMR_LOGE("destory thread failed!");
        ret = CMR_CAMERA_FAIL;
        goto deinit_end;
    }

    ret = prev_destroy_cb_thread(handle);
    if (ret) {
        CMR_LOGE("destory cb thread failed!");
        ret = CMR_CAMERA_FAIL;
        goto deinit_end;
    }

    /*release handle*/
    if (handle) {
        free(handle);
        handle = NULL;
    }

deinit_end:
    CMR_LOGI("ret %ld", ret);
    return ret;
}

cmr_int cmr_preview_set_param(cmr_handle preview_handle, cmr_u32 camera_id,
                              struct preview_param *param_ptr,
                              struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_int call_ret = CMR_CAMERA_SUCCESS;
    struct internal_param *inter_param = NULL;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;

    if (!preview_handle || !param_ptr || !out_param_ptr) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p, 0x%p", preview_handle, param_ptr,
                 out_param_ptr);
        return CMR_CAMERA_INVALID_PARAM;
    }

    CHECK_CAMERA_ID(camera_id);
    CMR_LOGI("camera_id %d frame count %d", camera_id, param_ptr->frame_count);

    /*save the preview param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)param_ptr;
    inter_param->param3 = (void *)out_param_ptr;

    message.msg_type = PREV_EVT_SET_PARAM;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
        }
    } else {
        call_ret = handle->prev_cxt[camera_id].out_ret_val;
        CMR_LOGI("call ret %ld", call_ret);
    }

    ATRACE_END();
    return ret | call_ret;
}

cmr_int cmr_preview_start(cmr_handle preview_handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    CMR_LOGI("E");

    message.msg_type = PREV_EVT_ASSIST_START;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }

    message.msg_type = PREV_EVT_START;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)((unsigned long)camera_id);
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGI("X");
    ATRACE_END();
    return ret;
}

cmr_int cmr_preview_stop(cmr_handle preview_handle, cmr_u32 camera_id) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    CMR_LOGI("in");

    message.msg_type = PREV_EVT_ASSIST_STOP;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }

    message.msg_type = PREV_EVT_STOP;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)((unsigned long)camera_id);
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGV("out");
    return ret;
}

cmr_int cmr_preview_cancel_snapshot(cmr_handle preview_handle,
                                    cmr_u32 camera_id) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    CMR_LOGI("in");

    message.msg_type = PREV_EVT_CANCEL_SNP;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)((unsigned long)camera_id);
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGI("out");
    return ret;
}

cmr_int cmr_preview_get_status(cmr_handle preview_handle, cmr_u32 camera_id) {
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;

    if (!handle || (camera_id >= CAMERA_ID_MAX)) {
        CMR_LOGE("invalid param, handle %p, camera_id %d", handle, camera_id);
        return ERROR;
    }

    prev_cxt = &handle->prev_cxt[camera_id];

    /*CMR_LOGD("prev_status %ld", prev_cxt->prev_status); */
    return (cmr_int)prev_cxt->prev_status;
}

cmr_int cmr_preview_get_prev_rect(cmr_handle preview_handle, cmr_u32 camera_id,
                                  struct img_rect *rect) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!rect) {
        CMR_LOGE("rect is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];

    cmr_copy(rect, &prev_cxt->prev_rect, sizeof(struct img_rect));

    return ret;
}

cmr_int cmr_camera_isp_stop_video(cmr_handle preview_handle,
                                  cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    CHECK_HANDLE_VALID(handle);

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!handle->ops.isp_stop_video) {
        CMR_LOGE("ops is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    /*stop isp*/
    if (PREV_ISP_COWORK == prev_cxt->isp_status) {
        ret = handle->ops.isp_stop_video(handle->oem_handle);
        prev_cxt->isp_status = PREV_ISP_IDLE;
        if (ret) {
            CMR_LOGE("Failed to stop ISP video mode, %ld", ret);
        }
    }
exit:
    return ret;
}

cmr_int cmr_preview_receive_data(cmr_handle preview_handle, cmr_u32 camera_id,
                                 cmr_uint evt, void *data) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct frm_info *frm_data = NULL;
    struct internal_param *inter_param = NULL;
    cmr_handle thread = 0;

    CMR_LOGV("handle 0x%p camera id %d evt 0x%lx", preview_handle, camera_id,
             evt);

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    /*copy the frame info*/
    if (data) {
        frm_data = (struct frm_info *)malloc(sizeof(struct frm_info));
        if (!frm_data) {
            CMR_LOGE("alloc frm mem failed!");
            ret = CMR_CAMERA_NO_MEM;
            goto exit;
        }
        cmr_copy(frm_data, data, sizeof(struct frm_info));
    }

    /*deliver the evt and data via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)evt;
    inter_param->param3 = (void *)frm_data;

    if (CMR_GRAB_TX_DONE == evt) {
        thread = handle->thread_cxt.assist_thread_handle;
    } else {
        thread = handle->thread_cxt.thread_handle;
    }

    message.msg_type = PREV_EVT_RECEIVE_DATA;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(thread, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        goto exit;
    }

exit:
    if (ret) {
        if (frm_data) {
            free(frm_data);
        }

        if (inter_param) {
            free(inter_param);
        }
    }

    return ret;
}

cmr_int cmr_preview_update_zoom(cmr_handle preview_handle, cmr_u32 camera_id,
                                struct cmr_zoom_param *param) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!param) {
        CMR_LOGE("zoom param is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    /*deliver the zoom param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)param;

    if (ZOOM_LEVEL == param->mode) {
        CMR_LOGI("update zoom, zoom_level %ld", param->zoom_level);
    } else {
        CMR_LOGI("update zoom, zoom_ratio=%f prev=%f, video=%f, cap=%f",
                 param->zoom_info.zoom_ratio,
                 param->zoom_info.prev_aspect_ratio,
                 param->zoom_info.video_aspect_ratio,
                 param->zoom_info.capture_aspect_ratio);
    }

    message.msg_type = PREV_EVT_UPDATE_ZOOM;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
        }
    }

    CMR_LOGI("ret %ld", ret);
    return ret;
}

cmr_int cmr_preview_release_frame(cmr_handle preview_handle, cmr_u32 camera_id,
                                  cmr_uint index) {
    UNUSED(preview_handle);
    UNUSED(camera_id);
    UNUSED(index);

    cmr_int ret = CMR_CAMERA_SUCCESS;
#if 0
	struct prev_handle     *handle = (struct prev_handle*)preview_handle;
	struct prev_context    *prev_cxt = NULL;
	cmr_u32                channel_id = 0;
	cmr_u32                frm_id = 0;

	CHECK_HANDLE_VALID(handle);
	CHECK_CAMERA_ID(camera_id);

	if (!IS_PREVIEW(handle, camera_id)) {
		CMR_LOGE("warning, not in preview!");
		/*return ret;*/
	}

	if (!handle->ops.channel_free_frame) {
		CMR_LOGE("ops channel_free_frame is null");
		return CMR_CAMERA_INVALID_PARAM;
	}

	CMR_LOGI("index, 0x%lx ", index);

	prev_cxt = &handle->prev_cxt[camera_id];;

	/* only the frame whose rotation angle is zero should be released by app,
	   otherwise, it will be released after rotation done; */
	if (IMG_ANGLE_0 == prev_cxt->prev_param.prev_rot) {
		index += CMR_PREV_ID_BASE;
		if (index >= CMR_PREV_ID_BASE && index < (CMR_PREV_ID_BASE + prev_cxt->prev_mem_num)) {

			frm_id     = index;
			channel_id = prev_cxt->prev_channel_id;
			ret = handle->ops.channel_free_frame(handle->oem_handle, channel_id, frm_id);
			if (ret) {
				CMR_LOGE("release frm failed");
			}

		} else {
			CMR_LOGE("wrong index, 0x%lx ", index);
		}
	} else {
		CMR_LOGI("[prev_rot] unlock %ld", (index % PREV_ROT_FRM_CNT));
		prev_cxt->prev_rot_frm_is_lock[index % PREV_ROT_FRM_CNT] = 0;
	}
#endif
    return ret;
}

cmr_int cmr_preview_ctrl_facedetect(cmr_handle preview_handle,
                                    cmr_u32 camera_id, cmr_uint on_off) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)on_off;

    message.msg_type = PREV_EVT_FD_CTRL;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
        }
    }

    return ret;
}

cmr_int cmr_preview_is_support_zsl(cmr_handle preview_handle, cmr_u32 camera_id,
                                   cmr_uint *is_support) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct sensor_exp_info *sensor_info = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!is_support) {
        CMR_LOGE("invalid param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    sensor_info =
        (struct sensor_exp_info *)malloc(sizeof(struct sensor_exp_info));
    if (!sensor_info) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    if (!handle->ops.get_sensor_info) {
        CMR_LOGE("ops is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret =
        handle->ops.get_sensor_info(handle->oem_handle, camera_id, sensor_info);
    if (ret) {
        CMR_LOGE("get_sensor info failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (IMG_DATA_TYPE_JPEG == sensor_info->sensor_image_type) {
        *is_support = 0;
    } else {
        *is_support = 1;
    }

exit:
    if (sensor_info) {
        free(sensor_info);
        sensor_info = NULL;
    }
    return ret;
}

cmr_int cmr_preview_get_max_cap_size(cmr_handle preview_handle,
                                     cmr_u32 camera_id, cmr_uint *max_w,
                                     cmr_uint *max_h) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!max_w || !max_h) {
        CMR_LOGE("invalid param, 0x%p, 0x%p", max_w, max_h);
        return CMR_CAMERA_INVALID_PARAM;
    }

    *max_w = handle->prev_cxt[camera_id].max_size.width;
    *max_h = handle->prev_cxt[camera_id].max_size.height;

    CMR_LOGI("max size %ld, %ld", *max_w, *max_h);
    return ret;
}
/**add for 3d capture to reset reprocessing capture size begin*/
cmr_int cmr_preview_set_cap_size(cmr_handle preview_handle,
                                 cmr_u32 is_reprocessing, cmr_u32 camera_id,
                                 cmr_u32 width, cmr_u32 height) {
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    CMR_LOGD("before set cap_org_size:%d %d",
             handle->prev_cxt[camera_id].cap_org_size.width,
             handle->prev_cxt[camera_id].cap_org_size.height);
    CMR_LOGD("before set actual_pic_size:%d %d",
             handle->prev_cxt[camera_id].actual_pic_size.width,
             handle->prev_cxt[camera_id].actual_pic_size.height);
    CMR_LOGD("before set aligned_pic_size:%d %d",
             handle->prev_cxt[camera_id].aligned_pic_size.width,
             handle->prev_cxt[camera_id].aligned_pic_size.height);
    CMR_LOGD("before set dealign_actual_pic_size:%d %d",
             handle->prev_cxt[camera_id].dealign_actual_pic_size.width,
             handle->prev_cxt[camera_id].dealign_actual_pic_size.height);
    CMR_LOGD("before set picture_size:%d %d",
             handle->prev_cxt[camera_id].prev_param.picture_size.width,
             handle->prev_cxt[camera_id].prev_param.picture_size.height);
    handle->prev_cxt[camera_id].actual_pic_size.width = width;
    handle->prev_cxt[camera_id].actual_pic_size.height = height;
    handle->prev_cxt[camera_id].aligned_pic_size.width = width;
    handle->prev_cxt[camera_id].aligned_pic_size.height = height;
    handle->prev_cxt[camera_id].dealign_actual_pic_size.width = width;
    handle->prev_cxt[camera_id].dealign_actual_pic_size.height = height;
    handle->prev_cxt[camera_id].cap_org_size.width = width;
    handle->prev_cxt[camera_id].cap_org_size.height = height;
    handle->prev_cxt[camera_id].max_size.width = width;
    handle->prev_cxt[camera_id].max_size.height = height;
    handle->prev_cxt[camera_id].prev_param.picture_size.width = width;
    handle->prev_cxt[camera_id].prev_param.picture_size.height = height;
    handle->prev_cxt[camera_id].is_reprocessing = is_reprocessing;

    return 0;
}
/**add for 3d capture to reset reprocessing capture size end*/

cmr_int cmr_preview_get_post_proc_param(cmr_handle preview_handle,
                                        cmr_u32 camera_id, cmr_u32 encode_angle,
                                        struct snp_proc_param *out_param_ptr) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    CMR_LOGI("in");

    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }
    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)((unsigned long)encode_angle);
    inter_param->param3 = (void *)out_param_ptr;

    message.msg_type = PREV_EVT_GET_POST_PROC_PARAM;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
    }

exit:
    CMR_LOGI("out, ret %ld", ret);
    if (ret) {
        if (inter_param) {
            free(inter_param);
        }
    }

    return ret;
}

cmr_int cmr_preview_before_set_param(cmr_handle preview_handle,
                                     cmr_u32 camera_id,
                                     enum preview_param_mode mode) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGV("in");

    /*deliver the zoom param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)mode;

    message.msg_type = PREV_EVT_BEFORE_SET;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
        }
    }

    CMR_LOGV("out");
    return ret;
}

cmr_int cmr_preview_after_set_param(cmr_handle preview_handle,
                                    cmr_u32 camera_id,
                                    enum preview_param_mode mode,
                                    enum img_skip_mode skip_mode,
                                    cmr_u32 skip_number) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGV("in");

    /*deliver the zoom param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)mode;
    inter_param->param3 = (void *)skip_mode;
    inter_param->param4 = (void *)((unsigned long)skip_number);

    message.msg_type = PREV_EVT_AFTER_SET;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
        }
    }

    CMR_LOGV("out");
    return ret;
}

cmr_int cmr_preview_set_preview_buffer(cmr_handle preview_handle,
                                       cmr_u32 camera_id, cmr_uint src_phy_addr,
                                       cmr_uint src_vir_addr, cmr_s32 fd) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGV("in");

    /*deliver the zoom param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)src_phy_addr;
    inter_param->param3 = (void *)src_vir_addr;
    inter_param->param4 = (void *)(unsigned long)fd;

    message.msg_type = PREV_EVT_SET_PREV_BUFFER;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
        }
    }

    CMR_LOGV("out");
    return ret;
}

cmr_int cmr_preview_set_video_buffer(cmr_handle preview_handle,
                                     cmr_u32 camera_id, cmr_uint src_phy_addr,
                                     cmr_uint src_vir_addr, cmr_s32 fd) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGI("in");

    /*deliver the zoom param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)src_phy_addr;
    inter_param->param3 = (void *)src_vir_addr;
    inter_param->param4 = (void *)(unsigned long)fd;

    message.msg_type = PREV_EVT_SET_VIDEO_BUFFER;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
        }
    }

    CMR_LOGV("out");
    return ret;
}

cmr_int cmr_preview_set_zsl_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                                   cmr_uint src_phy_addr, cmr_uint src_vir_addr,
                                   cmr_s32 fd) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGV("in");

    /*deliver the zoom param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)src_phy_addr;
    inter_param->param3 = (void *)src_vir_addr;
    inter_param->param4 = (void *)(unsigned long)fd;

    message.msg_type = PREV_EVT_SET_ZSL_BUFFER;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
        }
    }

    CMR_LOGV("out");
    return ret;
}

cmr_int cmr_preview_set_pdaf_raw_buffer(cmr_handle preview_handle,
                                        cmr_u32 camera_id,
                                        cmr_uint src_phy_addr,
                                        cmr_uint src_vir_addr, cmr_s32 fd,
                                        cmr_u32 msg_type) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGV("in");

    /*deliver the zoom param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)src_phy_addr;
    inter_param->param3 = (void *)src_vir_addr;
    inter_param->param4 = (void *)(unsigned long)fd;

    message.msg_type = msg_type;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
        }
    }

    CMR_LOGV("out");
    return ret;
}

enum isp_tuning_mode cmr_camera_get_isp_tuning_mode(cmr_handle preview_handle,
                                                    cmr_u32 camera_id,
                                                    cmr_u32 capture_mode) {
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 preview_enable = 0;
    cmr_u32 snapshot_enable = 0;
    cmr_u32 video_enable = 0;
    cmr_u32 tool_eb = 0;
    cmr_u32 sprd_zsl_enabled;
    cmr_u32 video_slowmotion_eb;
    enum sensor_mode_size sensor_size_mode = SENSOR_SIZE_FULL;
    enum isp_tuning_mode tuning_mode = ISP_TUNING_PREVIEW_BINNING;

    CHECK_HANDLE_VALID(handle);

    prev_cxt = &handle->prev_cxt[camera_id];

    preview_enable = prev_cxt->prev_param.preview_eb;
    snapshot_enable = prev_cxt->prev_param.snapshot_eb;
    video_enable = prev_cxt->prev_param.video_eb;
    tool_eb = prev_cxt->prev_param.tool_eb;
    video_slowmotion_eb = prev_cxt->prev_param.video_slowmotion_eb;
    sprd_zsl_enabled = prev_cxt->prev_param.sprd_zsl_enabled;
    sensor_size_mode = prev_cxt->sensor_size_mode;

    CMR_LOGD("sensor_size_mode %d, preview_enable %d, snapshot_enable %d, "
             "video_enable %d, tool_eb %d, video_slowmotion_eb %d, "
             "sprd_zsl_enabled %d, capture_mode %d",
             sensor_size_mode, preview_enable, snapshot_enable, video_enable,
             tool_eb, video_slowmotion_eb, sprd_zsl_enabled, capture_mode);

    if ((ISP_CAP_MODE_HIGHISO == capture_mode) ||
        (ISP_CAP_MODE_BURST == capture_mode && sprd_zsl_enabled)) {
        if (sensor_size_mode == SENSOR_SIZE_FULL) {
            tuning_mode = ISP_TUNING_MULTILAYER_FULL;
        } else if (sensor_size_mode == SENSOR_SIZE_BINNING) {
            tuning_mode = ISP_TUNING_MULTILAYER_BINNING;
        }
        return tuning_mode;
    } else if (ISP_CAP_MODE_HDR == capture_mode && snapshot_enable) {
        if (sensor_size_mode == SENSOR_SIZE_FULL) {
            tuning_mode = ISP_TUNING_HDR_FULL;
        } else if (sensor_size_mode == SENSOR_SIZE_BINNING) {
            tuning_mode = ISP_TUNING_HDR_BINNING;
        }
        return tuning_mode;
    } else if (video_slowmotion_eb) {
        tuning_mode = ISP_TUNING_SLOWVIDEO;
        return tuning_mode;
    }

    if (preview_enable) {
        if (sensor_size_mode == SENSOR_SIZE_FULL) {
            tuning_mode = ISP_TUNING_PREVIEW_FULL;
        } else if (sensor_size_mode == SENSOR_SIZE_BINNING) {
            tuning_mode = ISP_TUNING_PREVIEW_BINNING;
        }
    }

    if (snapshot_enable) {
        if (sensor_size_mode == SENSOR_SIZE_FULL) {
            tuning_mode = ISP_TUNING_CAPTURE_FULL;
        } else if (sensor_size_mode == SENSOR_SIZE_BINNING) {
            tuning_mode = ISP_TUNING_CAPTURE_BINNING;
        }
    }

    if (video_enable) {
        if (sensor_size_mode == SENSOR_SIZE_FULL) {
            tuning_mode = ISP_TUNING_VIDEO_FULL;
        } else if (sensor_size_mode == SENSOR_SIZE_BINNING) {
            tuning_mode = ISP_TUNING_VIDEO_BINNING;
        }
    }

    return tuning_mode;
}

/**************************LOCAL FUNCTION
 * ***************************************************************************/
cmr_int prev_create_thread(struct prev_handle *handle) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);

    CMR_LOGI("is_inited %ld", handle->thread_cxt.is_inited);

    if (!handle->thread_cxt.is_inited) {
        pthread_mutex_init(&handle->thread_cxt.prev_mutex, NULL);
        sem_init(&handle->thread_cxt.prev_sync_sem, 0, 0);

        ret = cmr_thread_create(&handle->thread_cxt.assist_thread_handle,
                                PREV_MSG_QUEUE_SIZE, prev_assist_thread_proc,
                                (void *)handle);
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            goto end;
        }

        ret = cmr_thread_create(&handle->thread_cxt.thread_handle,
                                PREV_MSG_QUEUE_SIZE, prev_thread_proc,
                                (void *)handle);
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            goto end;
        }

        handle->thread_cxt.is_inited = 1;

        message.msg_type = PREV_EVT_INIT;
        message.sync_flag = CMR_MSG_SYNC_RECEIVED;
        ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            goto end;
        }
    }

end:
    if (ret) {
        sem_destroy(&handle->thread_cxt.prev_sync_sem);
        pthread_mutex_destroy(&handle->thread_cxt.prev_mutex);
        handle->thread_cxt.is_inited = 0;
    }

    CMR_LOGI("ret %ld", ret);
    return ret;
}

cmr_int prev_destroy_thread(struct prev_handle *handle) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);

    CMR_LOGI("is_inited %ld", handle->thread_cxt.is_inited);

    if (handle->thread_cxt.is_inited) {
        message.msg_type = PREV_EVT_EXIT;
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
        ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
        if (ret) {
            CMR_LOGE("send msg failed!");
        }

        ret = cmr_thread_destroy(handle->thread_cxt.thread_handle);
        handle->thread_cxt.thread_handle = 0;

        ret = cmr_thread_destroy(handle->thread_cxt.assist_thread_handle);
        handle->thread_cxt.assist_thread_handle = 0;

        sem_destroy(&handle->thread_cxt.prev_sync_sem);
        pthread_mutex_destroy(&handle->thread_cxt.prev_mutex);
        handle->thread_cxt.is_inited = 0;
    }

    CMR_LOGI("out, ret %ld", ret);
    return ret;
}

cmr_int prev_create_cb_thread(struct prev_handle *handle) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);

    CMR_LOGI("in");

    ret = cmr_thread_create(&handle->thread_cxt.cb_thread_handle,
                            PREV_MSG_QUEUE_SIZE, prev_cb_thread_proc,
                            (void *)handle);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto end;
    }

    message.msg_type = PREV_EVT_CB_INIT;
    message.sync_flag = CMR_MSG_SYNC_RECEIVED;
    ret = cmr_thread_msg_send(handle->thread_cxt.cb_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto end;
    }

end:
    CMR_LOGI("ret %ld", ret);
    return ret;
}

cmr_int prev_destroy_cb_thread(struct prev_handle *handle) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);

    CMR_LOGI("in");

    message.msg_type = PREV_EVT_CB_EXIT;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret = cmr_thread_msg_send(handle->thread_cxt.cb_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
    }

    ret = cmr_thread_destroy(handle->thread_cxt.cb_thread_handle);
    handle->thread_cxt.cb_thread_handle = 0;

    CMR_LOGI("ret %ld", ret);
    return ret;
}

cmr_int prev_assist_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 msg_type = 0;
    cmr_uint evt = 0;
    cmr_uint src_phy_addr, src_vir_addr, zsl_private;
    cmr_s32 fd;
    cmr_u32 camera_id = CAMERA_ID_MAX;
    struct internal_param *inter_param = NULL;
    struct frm_info *frm_data = NULL;
    struct prev_handle *handle = (struct prev_handle *)p_data;

    if (!message || !p_data) {
        return CMR_CAMERA_INVALID_PARAM;
    }

    msg_type = (cmr_u32)message->msg_type;
    CMR_LOGV("msg_type 0x%x", msg_type);
    switch (msg_type) {
    case PREV_EVT_ASSIST_START:
        handle->frame_active = 1;
        break;
    case PREV_EVT_RECEIVE_DATA:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        evt = (cmr_uint)inter_param->param2;
        frm_data = (struct frm_info *)inter_param->param3;
        if (handle->frame_active == 1) {
            ret = prev_receive_data(handle, camera_id, evt, frm_data);
            if (frm_data) {
                free(frm_data);
                frm_data = NULL;
            }
        }
        break;
    case PREV_EVT_FD_CTRL:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);

        ret = prev_fd_ctrl(handle, camera_id,
                           (cmr_u32)((cmr_uint)inter_param->param2));
        break;
    case PREV_EVT_ASSIST_STOP:
        handle->frame_active = 0;
        break;

    case PREV_EVT_SET_PREV_BUFFER:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        src_phy_addr = (cmr_uint)inter_param->param2;
        src_vir_addr = (cmr_uint)inter_param->param3;
        fd = (cmr_s32)(unsigned long)inter_param->param4;

        ret = prev_set_preview_buffer(handle, camera_id, src_phy_addr,
                                      src_vir_addr, fd);
        break;

    case PREV_EVT_SET_VIDEO_BUFFER:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        src_phy_addr = (cmr_uint)inter_param->param2;
        src_vir_addr = (cmr_uint)inter_param->param3;
        fd = (cmr_s32)(unsigned long)inter_param->param4;

        ret = prev_set_video_buffer(handle, camera_id, src_phy_addr,
                                    src_vir_addr, fd);
        break;

    case PREV_EVT_SET_ZSL_BUFFER:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        src_phy_addr = (cmr_uint)inter_param->param2;
        src_vir_addr = (cmr_uint)inter_param->param3;
        fd = (cmr_s32)(unsigned long)inter_param->param4;

        ret = prev_set_zsl_buffer(handle, camera_id, src_phy_addr, src_vir_addr,
                                  fd);
        break;
    case PREV_EVT_SET_PDAF_BUFFER:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        src_phy_addr = (cmr_uint)inter_param->param2;
        src_vir_addr = (cmr_uint)inter_param->param3;
        fd = (cmr_s32)(unsigned long)inter_param->param4;

        ret = prev_set_pdaf_raw_buffer(handle, camera_id, src_phy_addr,
                                       src_vir_addr, fd);
        break;
    case PREV_EVT_SET_PDAF_DATATYPE_BUFFER:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        src_phy_addr = (cmr_uint)inter_param->param2;
        src_vir_addr = (cmr_uint)inter_param->param3;
        fd = (cmr_s32)(unsigned long)inter_param->param4;

        ret = prev_set_sensor_datatype_buffer(handle, camera_id, src_phy_addr,
                                              src_vir_addr, fd);
        break;

    default:
        CMR_LOGE("unknown message");
        break;
    }

    return ret;
}

cmr_int prev_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 msg_type = 0;
    cmr_uint evt = 0;
    cmr_u32 camera_id = CAMERA_ID_MAX;
    enum preview_param_mode mode = PARAM_MODE_MAX;
    struct internal_param *inter_param = NULL;
    struct frm_info *frm_data = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    struct prev_handle *handle = (struct prev_handle *)p_data;
    struct prev_cb_info cb_data_info;

    if (!message || !p_data) {
        return CMR_CAMERA_INVALID_PARAM;
    }

    msg_type = (cmr_u32)message->msg_type;
    // CMR_LOGI("msg_type 0x%x", msg_type);

    switch (msg_type) {
    case PREV_EVT_INIT:
        ret = prev_local_init(handle);
        break;

    case PREV_EVT_SET_PARAM:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((unsigned long)inter_param->param1);

        /*save the preview param first*/
        cmr_copy(&handle->prev_cxt[camera_id].prev_param, inter_param->param2,
                 sizeof(struct preview_param));
        CMR_LOGI("encode_angle %d",
                 handle->prev_cxt[camera_id].prev_param.encode_angle);
        /*handle the param*/
        ret = prev_set_param_internal(
            handle, camera_id, 0,
            (struct preview_out_param *)inter_param->param3);
        break;

    case PREV_EVT_RECEIVE_DATA:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((unsigned long)inter_param->param1);
        evt = (cmr_uint)inter_param->param2;
        frm_data = (struct frm_info *)inter_param->param3;

        ret = prev_receive_data(handle, camera_id, evt, frm_data);
        if (frm_data) {
            free(frm_data);
            frm_data = NULL;
        }
        break;

    case PREV_EVT_UPDATE_ZOOM:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((unsigned long)inter_param->param1);
        zoom_param = (struct cmr_zoom_param *)inter_param->param2;
        CMR_LOGV("camera_id %d,  PREV_EVT_UPDATE_ZOOM", camera_id);

        if (ZOOM_LEVEL == zoom_param->mode) {
            CMR_LOGD("update zoom, zoom_level %ld", zoom_param->zoom_level);
        } else {
            CMR_LOGD("update zoom, zoom_ratio=%f, prev=%f, video=%f, cap=%f",
                     zoom_param->zoom_info.zoom_ratio,
                     zoom_param->zoom_info.prev_aspect_ratio,
                     zoom_param->zoom_info.video_aspect_ratio,
                     zoom_param->zoom_info.capture_aspect_ratio);
        }

        /*save zoom param*/
        cmr_copy(&handle->prev_cxt[camera_id].prev_param.zoom_setting,
                 zoom_param, sizeof(struct cmr_zoom_param));
        break;

    case PREV_EVT_BEFORE_SET:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((unsigned long)inter_param->param1);
        mode = (enum preview_param_mode)inter_param->param2;

        ret = prev_before_set_param(handle, camera_id, mode);
        break;

    case PREV_EVT_AFTER_SET:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((unsigned long)inter_param->param1);
        mode = (enum preview_param_mode)inter_param->param2;

        ret = prev_after_set_param(
            handle, camera_id, mode, (enum img_skip_mode)inter_param->param3,
            (cmr_u32)((unsigned long)inter_param->param4));
        break;

    case PREV_EVT_START:
        camera_id = (cmr_u32)((unsigned long)message->data);

        prev_recovery_reset(handle, camera_id);
        ret = prev_start(handle, camera_id, 0, 0);
        /*Notify preview started*/
        cb_data_info.cb_type = PREVIEW_EXIT_CB_PREPARE;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
        break;

    case PREV_EVT_STOP:
        camera_id = (cmr_u32)((unsigned long)message->data);
        ret = prev_stop(handle, camera_id, 0);
        break;

    case PREV_EVT_CANCEL_SNP:
        camera_id = (cmr_u32)((unsigned long)message->data);
        ret = prev_cancel_snapshot(handle, camera_id);
        break;

    case PREV_EVT_GET_POST_PROC_PARAM:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((unsigned long)inter_param->param1);

        ret = prev_update_cap_param(
            handle, camera_id, (cmr_u32)((unsigned long)inter_param->param2),
            (struct snp_proc_param *)inter_param->param3);

        ret = prev_get_cap_post_proc_param(
            handle, camera_id, (cmr_u32)((unsigned long)inter_param->param2),
            (struct snp_proc_param *)inter_param->param3);
        break;

    case PREV_EVT_FD_CTRL:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((unsigned long)inter_param->param1);

        ret = prev_fd_ctrl(handle, camera_id,
                           (cmr_u32)((unsigned long)inter_param->param2));
        break;

    case PREV_EVT_EXIT:
        ret = prev_local_deinit(handle);
        break;

    default:
        CMR_LOGE("unknown message");
        break;
    }

    return ret;
}

static cmr_int prev_cb_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 msg_type = 0;
    cmr_uint evt = 0;
    cmr_u32 camera_id = CAMERA_ID_MAX;
    struct prev_cb_info *cb_data_info = NULL;
    struct prev_handle *handle = (struct prev_handle *)p_data;

    if (!message || !p_data) {
        return CMR_CAMERA_INVALID_PARAM;
    }

    msg_type = (cmr_u32)message->msg_type;
    /*CMR_LOGI("msg_type 0x%x", msg_type); */

    switch (msg_type) {
    case PREV_EVT_CB_INIT:
        CMR_LOGI("cb thread inited");
        break;

    case PREV_EVT_CB_START:
        cb_data_info = (struct prev_cb_info *)message->data;

        if (!handle->oem_cb) {
            CMR_LOGE("oem_cb is null");
            break;
        }

        ret = handle->oem_cb(handle->oem_handle, cb_data_info->cb_type,
                             cb_data_info->func_type, cb_data_info->frame_data);

        if (cb_data_info->frame_data) {
            free(cb_data_info->frame_data);
            cb_data_info->frame_data = NULL;
        }
        break;

    case PREV_EVT_CB_EXIT:
        CMR_LOGI("cb thread exit");
        break;

    default:
        break;
    }

    return ret;
}

cmr_int prev_cb_start(struct prev_handle *handle,
                      struct prev_cb_info *cb_info) {
    ATRACE_BEGIN(__FUNCTION__);

    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_cb_info *cb_data_info = NULL;
    struct camera_frame_type *frame_type_data = NULL;

    cb_data_info = (struct prev_cb_info *)malloc(sizeof(struct prev_cb_info));
    if (!cb_data_info) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }
    cmr_copy(cb_data_info, cb_info, sizeof(struct prev_cb_info));

    if (cb_info->frame_data) {
        cb_data_info->frame_data = (struct camera_frame_type *)malloc(
            sizeof(struct camera_frame_type));
        if (!cb_data_info->frame_data) {
            CMR_LOGE("No mem!");
            ret = CMR_CAMERA_NO_MEM;
            goto exit;
        }
        cmr_copy(cb_data_info->frame_data, cb_info->frame_data,
                 sizeof(struct camera_frame_type));
    }

    CMR_LOGV("cb_type %d, func_type %d, frame_data 0x%p", cb_data_info->cb_type,
             cb_data_info->func_type, cb_data_info->frame_data);

    /*send to callback thread*/
    message.msg_type = PREV_EVT_CB_START;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    message.data = (void *)cb_data_info;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(handle->thread_cxt.cb_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (cb_data_info) {
            if (cb_data_info->frame_data) {
                free(cb_data_info->frame_data);
                cb_data_info->frame_data = NULL;
            }

            free(cb_data_info);
            cb_data_info = NULL;
        }
    }

    ATRACE_END();
    return ret;
}

cmr_int prev_receive_data(struct prev_handle *handle, cmr_u32 camera_id,
                          cmr_uint evt, struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 preview_enable = 0;
    cmr_u32 snapshot_enable = 0;
    cmr_u32 video_enable = 0;
    struct prev_context *prev_cxt = NULL;

    switch (evt) {
    case CMR_GRAB_TX_DONE:
        /*got one frame*/
        ret = prev_frame_handle(handle, camera_id, data);
        break;

    case CMR_GRAB_CANCELED_BUF:
        prev_cxt = &handle->prev_cxt[camera_id];
        preview_enable = prev_cxt->prev_param.preview_eb;
        snapshot_enable = prev_cxt->prev_param.snapshot_eb;
        video_enable = prev_cxt->prev_param.video_eb;

        if (preview_enable && (data->channel_id == prev_cxt->prev_channel_id)) {
            ret = prev_pop_preview_buffer(handle, camera_id, data, 1);
        }

        if (video_enable && (data->channel_id == prev_cxt->video_channel_id)) {
            ret = prev_pop_video_buffer(handle, camera_id, data, 1);
        }

        if (snapshot_enable && (data->channel_id == prev_cxt->cap_channel_id)) {
            ret = prev_pop_zsl_buffer(handle, camera_id, data, 1);
        }
        break;

    case CMR_GRAB_TX_ERROR:
    case CMR_GRAB_TX_NO_MEM:
    case CMR_GRAB_CSI2_ERR:
    case CMR_GRAB_TIME_OUT:
    case CMR_SENSOR_ERROR:
        ret = prev_error_handle(handle, camera_id, evt);
        break;

    case PREVIEW_CHN_PAUSE:
        ret = prev_pause_cap_channel(handle, camera_id, data);
        break;

    case PREVIEW_CHN_RESUME:
        ret = prev_resume_cap_channel(handle, camera_id, data);
        break;

    default:
        break;
    }

    return ret;
}

cmr_int prev_frame_handle(struct prev_handle *handle, cmr_u32 camera_id,
                          struct frm_info *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 preview_enable = 0;
    cmr_u32 snapshot_enable = 0;
    cmr_u32 video_enable = 0;
    enum sensor_data_type sensor_datatype = 0;
    enum sensor_pdaf_type pdaf_mode = 0;
    struct prev_context *prev_cxt = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    preview_enable = prev_cxt->prev_param.preview_eb;
    snapshot_enable = prev_cxt->prev_param.snapshot_eb;
    video_enable = prev_cxt->prev_param.video_eb;
    sensor_datatype = prev_cxt->prev_param.sensor_datatype;
    pdaf_mode = prev_cxt->prev_param.pdaf_mode;

    CMR_LOGV("preview_enable %d, snapshot_enable %d, channel_id %d, "
             "prev_channel_id %ld, cap_channel_id %ld",
             preview_enable, snapshot_enable, data->channel_id,
             prev_cxt->prev_channel_id, prev_cxt->cap_channel_id);

    if (preview_enable && (data->channel_id == prev_cxt->prev_channel_id)) {
        ret = prev_preview_frame_handle(handle, camera_id, data);
    }

    if ((sensor_datatype > SENSOR_DATATYPE_DISABLED) &&
        (data->channel_id == prev_cxt->sensor_datatype_channel_id)) {
        ret = prev_sensor_datatype_frame_handle(handle, camera_id, data);
    }

    if ((pdaf_mode == SENSOR_PDAF_TYPE3_ENABLE) &&
        (data->channel_id == prev_cxt->pdaf_channel_id)) {
        ret = prev_pdaf_raw_frame_handle(handle, camera_id, data);
    }

    if (video_enable && (data->channel_id == prev_cxt->video_channel_id)) {
        ret = prev_video_frame_handle(handle, camera_id, data);
    }

    if (snapshot_enable && (data->channel_id == prev_cxt->cap_channel_id)) {
        if (prev_cxt->is_zsl_frm) {
            ret = prev_zsl_frame_handle(handle, camera_id, data);
        } else {
            ret = prev_capture_frame_handle(handle, camera_id, data);
        }
    }

    /*received frame, reset recovery status*/
    if (prev_cxt->recovery_status) {
        CMR_LOGI("reset the recover status");
        prev_cxt->recovery_status = PREV_RECOVERY_IDLE;
    }

    ATRACE_END();
    return ret;
}

cmr_int prev_preview_frame_handle(struct prev_handle *handle, cmr_u32 camera_id,
                                  struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct camera_frame_type frame_type;
    cmr_s64 timestamp = 0;
    struct prev_cb_info cb_data_info;
    cmr_uint rot_index = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&cb_data_info, sizeof(struct prev_cb_info));
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!IS_PREVIEW(handle, camera_id)) {
        CMR_LOGE("preview stopped, skip this frame");
        return ret;
    }

    if (!handle->oem_cb || !handle->ops.channel_free_frame) {
        CMR_LOGE("ops oem_cb or channel_free_frame is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGD("frame_id=0x%x, frame_real_id=%d, channel_id=%d fd=0x%x",
             data->frame_id, data->frame_real_id, data->channel_id, data->fd);

    if (0 /*prev_cxt->prev_frm_cnt < 50 && (prev_cxt->prev_frm_cnt%2) == 0*/) {
        camera_save_yuv_to_file(
            FORM_DUMPINDEX(PREV_OUT_DATA, prev_cxt->prev_frm_cnt, 0),
            IMG_DATA_TYPE_YUV420, prev_cxt->actual_prev_size.width,
            prev_cxt->actual_prev_size.height,
            (struct img_addr *)&data->yaddr_vir);
    }

    if (0 == prev_cxt->prev_frm_cnt) {
        /*response*/
        cb_data_info.cb_type = PREVIEW_RSP_CB_SUCCESS;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }

    prev_cxt->prev_frm_cnt++;

    if (IMG_ANGLE_0 != prev_cxt->prev_param.prev_rot) {
        ret = prev_get_src_rot_buffer(prev_cxt, data, &rot_index);
        CMR_LOGI("rot_index %ld", rot_index);
        if (ret) {
            CMR_LOGE("get src rot buffer failed");
            return ret;
        }
    }

    /* skip num frames for pre-flash, because the frame is black*/
    if (prev_cxt->prev_preflash_skip_en &&
        IMG_SKIP_SW_KER == prev_cxt->skip_mode) {
        if (prev_cxt->prev_frm_cnt <= prev_cxt->prev_skip_num) {
            CMR_LOGI("ignore this frame, preview cnt %ld, total skip num %ld, "
                     "channed_id %d",
                     prev_cxt->prev_frm_cnt, prev_cxt->prev_skip_num,
                     data->channel_id);

            if (IMG_ANGLE_0 != prev_cxt->prev_param.prev_rot) {
                ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_PREVIEW,
                                               rot_index, 0);
                if (ret) {
                    CMR_LOGE("prev_set_rot_buffer_flag failed");
                    goto exit;
                }
                CMR_LOGI("rot_index %ld prev_rot_frm_is_lock %ld", rot_index,
                         prev_cxt->prev_rot_frm_is_lock[rot_index]);
                data->fd = prev_cxt->prev_frm[0].fd;
                data->yaddr = prev_cxt->prev_frm[0].addr_phy.addr_y;
                data->uaddr = prev_cxt->prev_frm[0].addr_phy.addr_u;
                data->vaddr = prev_cxt->prev_frm[0].addr_phy.addr_v;
                data->yaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_y;
                data->uaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_u;
                data->vaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_v;
            }
            ret = prev_pop_preview_buffer(handle, camera_id, data, 1);
            if (ret) {
                CMR_LOGE("pop frm failed");
            }

            return ret;
        }
    }

    /*skip frame if SW skip mode*/
    if (IMG_SKIP_SW == prev_cxt->skip_mode) {
        if (prev_cxt->prev_frm_cnt <= prev_cxt->prev_skip_num) {
            CMR_LOGI("ignore this frame, preview cnt %ld, total skip num %ld, "
                     "channed_id %d",
                     prev_cxt->prev_frm_cnt, prev_cxt->prev_skip_num,
                     data->channel_id);

            if (IMG_ANGLE_0 != prev_cxt->prev_param.prev_rot) {
                ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_PREVIEW,
                                               rot_index, 0);
                if (ret) {
                    CMR_LOGE("prev_set_rot_buffer_flag failed");
                    goto exit;
                }
                CMR_LOGI("rot_index %ld prev_rot_frm_is_lock %ld", rot_index,
                         prev_cxt->prev_rot_frm_is_lock[rot_index]);
                data->fd = prev_cxt->prev_frm[0].fd;
                data->yaddr = prev_cxt->prev_frm[0].addr_phy.addr_y;
                data->uaddr = prev_cxt->prev_frm[0].addr_phy.addr_u;
                data->vaddr = prev_cxt->prev_frm[0].addr_phy.addr_v;
                data->yaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_y;
                data->uaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_u;
                data->vaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_v;
            }
            ret = prev_pop_preview_buffer(handle, camera_id, data, 1);
            if (ret) {
                CMR_LOGE("pop frm failed");
            }

            return ret;
        }
    }

    /*skip frame when set param*/
    if ((IMG_SKIP_SW == prev_cxt->skip_mode) && prev_cxt->restart_skip_en) {

        timestamp = data->sec * 1000000000LL + data->usec * 1000;
        CMR_LOGI("Restart skip: frame time = %" PRId64
                 ", restart time=%" PRId64,
                 timestamp, prev_cxt->restart_timestamp);

        if (timestamp > prev_cxt->restart_timestamp) {

            pthread_mutex_lock(&handle->thread_cxt.prev_mutex);
            prev_cxt->restart_skip_cnt++;
            pthread_mutex_unlock(&handle->thread_cxt.prev_mutex);

            if (prev_cxt->restart_skip_cnt <= prev_cxt->prev_skip_num) {
                CMR_LOGI(
                    "skip this frm, skip_cnt %ld, skip_num %ld channed_id %d",
                    prev_cxt->restart_skip_cnt, prev_cxt->prev_skip_num,
                    data->channel_id);

                if (IMG_ANGLE_0 != prev_cxt->prev_param.prev_rot) {
                    ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_PREVIEW,
                                                   rot_index, 0);
                    if (ret) {
                        CMR_LOGE("prev_set_rot_buffer_flag failed");
                        goto exit;
                    }
                    CMR_LOGI("rot_index %ld prev_rot_frm_is_lock %ld",
                             rot_index,
                             prev_cxt->prev_rot_frm_is_lock[rot_index]);
                    data->fd = prev_cxt->prev_frm[0].fd;
                    data->yaddr = prev_cxt->prev_frm[0].addr_phy.addr_y;
                    data->uaddr = prev_cxt->prev_frm[0].addr_phy.addr_u;
                    data->vaddr = prev_cxt->prev_frm[0].addr_phy.addr_v;
                    data->yaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_y;
                    data->uaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_u;
                    data->vaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_v;
                }
                ret = prev_pop_preview_buffer(handle, camera_id, data, 1);
                if (ret) {
                    CMR_LOGE("pop frm failed");
                }
                return ret;

            } else {

                pthread_mutex_lock(&handle->thread_cxt.prev_mutex);
                prev_cxt->restart_skip_cnt = 0;
                prev_cxt->restart_skip_en = 0;
                pthread_mutex_unlock(&handle->thread_cxt.prev_mutex);

                CMR_LOGI("Restart skip: end");
            }
        } else {
            CMR_LOGI("Restart skip: frame is before restart, no need to skip "
                     "this frame");
        }
    }

    switch (prev_cxt->prev_param.sensor_datatype) {
    case SENSOR_REAL_DEPTH_ENABLE:
        prev_cxt->prev_cnt++;
        if (prev_cxt->prev_cnt <= 1) {
            CMR_LOGD("prev_cxt->prev_cnt %ld ", prev_cxt->prev_cnt);
        } else {

            /*notify frame*/
            ret = prev_pop_preview_buffer(handle, camera_id,
                                          prev_cxt->preview_bakcup_data, 0);
            if (ret) {
                CMR_LOGE("pop frm 0x%x err",
                         prev_cxt->preview_bakcup_data->channel_id);
                goto exit;
            }

            /*copy refoucs-detect info*/
            cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
            frame_type.width = prev_cxt->prev_param.preview_size.width;
            frame_type.height = prev_cxt->prev_param.preview_size.height;

            frame_type.y_phy_addr = prev_cxt->preview_bakcup_data->yaddr;
            frame_type.y_vir_addr = prev_cxt->preview_bakcup_data->yaddr_vir;
            frame_type.fd = prev_cxt->preview_bakcup_data->fd;
            frame_type.type = PREVIEW_FRAME;
            frame_type.timestamp =
                prev_cxt->preview_bakcup_data->sec * 1000000000LL +
                prev_cxt->preview_bakcup_data->usec * 1000;

            /*notify refoucs info directly*/
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);
        }
        break;
    case SENSOR_EMBEDDED_INFO_ENABLE:
        /*notify embedded info directly*/
        cb_data_info.cb_type = PREVIEW_EVT_SENSOR_DATATYPE;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = &frame_type;
        prev_cb_start(handle, &cb_data_info);

        break;
    case SENSOR_DATATYPE_PDAF_ENABLE:
        break;
    default:
        break;
    }

    if (IMG_ANGLE_0 == prev_cxt->prev_param.prev_rot) {
        ret = prev_construct_frame(handle, camera_id, data, &frame_type);
        if (ret) {
            CMR_LOGE("construct frm err");
            goto exit;
        }
        prev_cxt->prev_buf_id = frame_type.buf_id;
        if (prev_cxt->prev_param.sensor_datatype == SENSOR_REAL_DEPTH_ENABLE) {
            CMR_LOGE("prev_cxt->prev_cnt %lu ,prev_cxt->prev_frm_cnt %lu "
                     "sensor_datatype %d",
                     prev_cxt->prev_cnt, prev_cxt->prev_frm_cnt,
                     prev_cxt->prev_param.sensor_datatype);
            return CMR_CAMERA_SUCCESS;
        }

        ret = prev_pop_preview_buffer(handle, camera_id, data, 0);
        if (ret) {
            CMR_LOGE("pop frm 0x%x err", data->channel_id);
            goto exit;
        }
        /*notify frame via callback*/
        cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = &frame_type;
        prev_cb_start(handle, &cb_data_info);

    } else {
        /*need rotation*/
        if (prev_cxt->prev_mem_valid_num > 0) {
            ret = prev_start_rotate(handle, camera_id, data);
            if (ret) {
                CMR_LOGE("rot failed, skip this frm");
                ret = CMR_CAMERA_SUCCESS;
                goto exit;
            }
            CMR_LOGI("rot done");

            /*construct frame*/
            ret = prev_construct_frame(handle, camera_id, data, &frame_type);
            if (ret) {
                CMR_LOGE("construct frm 0x%x err", data->frame_id);
                goto exit;
            }

            if (prev_cxt->prev_param.sensor_datatype ==
                SENSOR_REAL_DEPTH_ENABLE) {
                CMR_LOGE("prev_cxt->prev_cnt %lu ,prev_cxt->prev_frm_cnt %lu "
                         "sensor_datatype %d",
                         prev_cxt->prev_cnt, prev_cxt->prev_frm_cnt,
                         prev_cxt->prev_param.sensor_datatype);
                return CMR_CAMERA_SUCCESS;
            }

            ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_PREVIEW, rot_index,
                                           0);
            if (ret) {
                CMR_LOGE("prev_set_rot_buffer_flag failed");
                goto exit;
            }
            /*notify frame*/
            ret = prev_pop_preview_buffer(handle, camera_id, data, 0);
            if (ret) {
                CMR_LOGE("pop frm 0x%x err", data->channel_id);
                goto exit;
            }
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);

        } else {
            CMR_LOGW("no available buf, drop! channel_id 0x%x",
                     data->channel_id);
            ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_PREVIEW, rot_index,
                                           0);
            if (ret) {
                CMR_LOGE("prev_set_rot_buffer_flag failed");
                goto exit;
            }
        }
    }

exit:
    if (ret) {
        cb_data_info.cb_type = PREVIEW_EXIT_CB_FAILED;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }

    return ret;
}

cmr_int prev_pdaf_datatype_cb(struct pd_frame_in *cb_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct prev_handle *handle = NULL;
    cmr_u32 camera_id = CAMERA_ID_MAX;

    if (!cb_param || !cb_param->caller_handle) {
        CMR_LOGE("error param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    handle = (struct prev_handle *)cb_param->caller_handle;
    camera_id = cb_param->camera_id;
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    if (cb_param->src_vir_addr)
        ret = prev_set_sensor_datatype_buffer(
            handle, camera_id, cb_param->src_phy_addr, cb_param->src_vir_addr,
            cb_param->fd);

    return ret;
}

cmr_int prev_pdaf_datatype_frame_handle(struct prev_handle *handle,
                                        cmr_u32 camera_id,
                                        struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_s64 timestamp = 0;
    cmr_uint rot_index = 0;
    struct img_addr addr_vir;
    struct img_frm *frm_ptr = NULL;
    struct pd_raw_info pd_raw;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&pd_raw, sizeof(struct pd_raw_info));

    prev_cxt = &handle->prev_cxt[camera_id];
    prev_cxt->pdaf_timestamp = data->sec * 1000000000LL + data->usec * 1000;

    if (!IS_PREVIEW(handle, camera_id)) {
        CMR_LOGE("preview stopped, skip this frame,prev_status %ld ",
                 prev_cxt->prev_status);
        return ret;
    }

    if (!handle->oem_cb || !handle->ops.channel_free_frame) {
        CMR_LOGE("ops oem_cb or channel_free_frame is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGV("got one frame, data->yaddr_vir 0x%x, frame_id 0x%x, "
             "frame_real_id %d, channel_id %d pdaf_timestamp %" PRId64,
             data->yaddr_vir, data->frame_id, data->frame_real_id,
             data->channel_id, prev_cxt->pdaf_timestamp);

    /*skip pdaf data when not preview*/
    if (0 == prev_cxt->pdaf_frm_cnt) {
        CMR_LOGI("skip pdaf data when not preview");
        ret = prev_pop_sensor_datatype_buffer(handle, camera_id, data, 0);
        ret = prev_set_sensor_datatype_buffer(handle, camera_id, data->yaddr,
                                              data->yaddr_vir, data->fd);
    } else {
        CMR_LOGV("send pdaf frame to isp");
        addr_vir.addr_y = data->yaddr_vir;
        pd_raw.frame_id = prev_cxt->pdaf_frm_cnt;
        pd_raw.addr = (void *)addr_vir.addr_y;
        pd_raw.len = prev_cxt->pdaf_mem_size;
        pd_raw.sec = data->sec;
        pd_raw.usec = data->usec;
        pd_raw.format = prev_cxt->sensor_info.image_format;
        pd_raw.pattern = prev_cxt->sensor_info.image_pattern;
        pd_raw.roi.image_width = prev_cxt->sensor_info.source_width_max;
        pd_raw.roi.image_height = prev_cxt->sensor_info.source_height_max;
        pd_raw.pd_in.caller_handle = (cmr_handle)handle;
        pd_raw.pd_in.camera_id = camera_id;
        pd_raw.pd_in.src_phy_addr = data->yaddr;
        pd_raw.pd_in.src_vir_addr = data->yaddr_vir;
        pd_raw.pd_in.fd = data->fd;
        pd_raw.pd_callback = prev_pdaf_datatype_cb;
        CMR_LOGV("addr=%p, len=%d, format=%d, pattern=%d, roi max_w=%d, "
                 "max_h=%d,start_x=%d, start_y=%d, w=%d, h=%d ",
                 pd_raw.addr, pd_raw.len, pd_raw.format, pd_raw.pattern,
                 pd_raw.roi.image_width, pd_raw.roi.image_height,
                 pd_raw.roi.trim_start_x, pd_raw.roi.trim_start_y,
                 pd_raw.roi.trim_width, pd_raw.roi.trim_height);
        ret = prev_pop_sensor_datatype_buffer(handle, camera_id, data, 0);
        handle->ops.set_preview_pd_raw(handle->oem_handle, &pd_raw);
    }
exit:

    return ret;
}

cmr_int prev_sensor_datatype_frame_handle(struct prev_handle *handle,
                                          cmr_u32 camera_id,
                                          struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct camera_frame_type frame_type;
    cmr_s64 timestamp = 0;
    struct prev_cb_info cb_data_info;
    cmr_uint rot_index = 0;
    struct img_addr addr_vir;
    struct img_frm *frm_ptr = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&cb_data_info, sizeof(struct prev_cb_info));
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));

    prev_cxt = &handle->prev_cxt[camera_id];
    prev_cxt->sensor_datatype_timestamp =
        data->sec * 1000000000LL + data->usec * 1000;
    prev_cxt->sensor_datatype_frm_id =
        prev_get_frm_index(prev_cxt->sensor_datatype_frm, data);
    prev_cxt->sensor_datatype_cnt++;
    prev_cxt->prev_cnt = 0;
    // cmr_bzero(prev_cxt->sensor_datatype_data,sizeof(struct frm_info));

    if (!IS_PREVIEW(handle, camera_id)) {
        CMR_LOGE(
            "preview stopped, skip this frame,prev_status,prev_status %ld ",
            prev_cxt->prev_status);
#if 0
		char *psPath_depthData = "data/misc/cameraserver/depth.bin";
		save_file(psPath_depthData, data->yaddr_vir, 480*360);
#endif
        return ret;
    }

    if (!handle->oem_cb || !handle->ops.channel_free_frame) {
        CMR_LOGE("ops oem_cb or channel_free_frame is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGV("got one frame, frame_id 0x%x, frame_real_id %d, channel_id %d "
             "yaddr 0x%x sensor_datatype_timestamp %" PRId64
             ", data->yaddr_vir 0x%x",
             data->frame_id, data->frame_real_id, data->channel_id, data->yaddr,
             prev_cxt->sensor_datatype_timestamp, data->yaddr_vir);

#if 0 // re-depthmap image save
	if (prev_cxt->prev_frm_cnt %20 == 0) {
		addr_vir.addr_y = data->yaddr_vir;
		camera_save_mipi_raw_to_file(prev_cxt->prev_frm_cnt,
				IMG_DATA_TYPE_RAW,
				1920,
				1088,
				&addr_vir);
	}
#endif

    switch (prev_cxt->prev_param.sensor_datatype) {
    case SENSOR_REAL_DEPTH_ENABLE:
        if (prev_cxt->sensor_datatype_cnt > 2) {
            ret = prev_pop_sensor_datatype_buffer(handle, camera_id, data, 0);
            ret = prev_set_sensor_datatype_buffer(
                handle, camera_id, data->yaddr, data->yaddr_vir, data->fd);
        } else {

            prev_cxt->sensor_datatype_data = data;

            CMR_LOGV("sensor_datatype_cnt "
                     "%lu,prev_cxt->sensor_datatype_timestamp %" PRId64
                     " preview_bakcup_timestamp %" PRId64,
                     prev_cxt->sensor_datatype_cnt,
                     prev_cxt->sensor_datatype_timestamp,
                     prev_cxt->preview_bakcup_timestamp);
            if (prev_cxt->sensor_datatype_timestamp != 0 &&
                prev_cxt->preview_bakcup_frm != 0) {
                if (llabs(prev_cxt->sensor_datatype_timestamp -
                          prev_cxt->preview_bakcup_timestamp) <=
                    10000 * 1000 * 1.0f) {
                    prev_sensor_datatype_send_data(
                        handle, camera_id, prev_cxt->preview_bakcup_frm, data);
                    CMR_LOGV(" prev_sensor_datatype_send_data ");
                    return ret;
                } else
                    CMR_LOGD("sensor_datatype_timestamp %" PRId64
                             "preview_bakcup_frm %p ",
                             prev_cxt->sensor_datatype_timestamp,
                             prev_cxt->preview_bakcup_frm);
                if (prev_cxt->preview_bakcup_frm != NULL) {
                    /*notify frame*/
                    ret = prev_pop_preview_buffer(
                        handle, camera_id, prev_cxt->preview_bakcup_data, 0);
                    if (ret) {
                        CMR_LOGE("pop frm 0x%x err",
                                 prev_cxt->preview_bakcup_data->channel_id);
                        // goto exit;
                    }

                    /*copy refoucs-detect info*/
                    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
                    frame_type.width = prev_cxt->prev_param.preview_size.width;
                    frame_type.height =
                        prev_cxt->prev_param.preview_size.height;

                    frame_type.y_phy_addr =
                        prev_cxt->preview_bakcup_data->yaddr;
                    frame_type.y_vir_addr =
                        prev_cxt->preview_bakcup_data->yaddr_vir;
                    frame_type.fd = prev_cxt->preview_bakcup_data->fd;
                    frame_type.type = PREVIEW_FRAME;
                    frame_type.timestamp =
                        prev_cxt->preview_bakcup_data->sec * 1000000000LL +
                        prev_cxt->preview_bakcup_data->usec * 1000;

                    /*notify refoucs info directly*/
                    cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
                    cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
                    cb_data_info.frame_data = &frame_type;
                    prev_cb_start(handle, &cb_data_info);
                }

                ret =
                    prev_pop_sensor_datatype_buffer(handle, camera_id, data, 0);
                ret = prev_set_sensor_datatype_buffer(
                    handle, camera_id, data->yaddr, data->yaddr_vir, data->fd);
                return ret;
            } else {
                CMR_LOGD("sensor_datatype_timestamp %" PRId64
                         "preview_bakcup_frm %p ",
                         prev_cxt->sensor_datatype_timestamp,
                         prev_cxt->preview_bakcup_frm);
                ret =
                    prev_pop_sensor_datatype_buffer(handle, camera_id, data, 0);
                ret = prev_set_sensor_datatype_buffer(
                    handle, camera_id, data->yaddr, data->yaddr_vir, data->fd);
            }
        }

        break;
    case SENSOR_EMBEDDED_INFO_ENABLE:
        prev_cxt->sensor_datatype_data = data;
        CMR_LOGV("sensor_datatype %u ,sensor_datatype_cnt "
                 "%lu,prev_cxt->sensor_datatype_timestamp %" PRId64
                 " preview_bakcup_timestamp %" PRId64,
                 prev_cxt->prev_param.sensor_datatype,
                 prev_cxt->sensor_datatype_cnt,
                 prev_cxt->sensor_datatype_timestamp,
                 prev_cxt->preview_bakcup_timestamp);

        /*notify embedded info directly*/
        cb_data_info.cb_type = PREVIEW_EVT_SENSOR_DATATYPE;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = &frame_type;
        prev_cb_start(handle, &cb_data_info);

        break;
    case SENSOR_DATATYPE_PDAF_ENABLE:
        prev_pdaf_datatype_frame_handle(handle, camera_id, data);
        break;
    default:
        break;
    }

exit:
    if (ret) {
        cb_data_info.cb_type = PREVIEW_EXIT_CB_FAILED;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }

    return ret;
}

cmr_int prev_pdaf_datatype_thread_cb(struct pd_frame_in *cb_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct prev_handle *handle = NULL;
    cmr_u32 camera_id = CAMERA_ID_MAX;

    if (!cb_param || !cb_param->caller_handle) {
        CMR_LOGE("error param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    handle = (struct prev_handle *)cb_param->caller_handle;
    camera_id = cb_param->camera_id;
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    if (cb_param->src_vir_addr)
        ret = cmr_preview_set_pdaf_raw_buffer(
            (cmr_handle)handle, camera_id, cb_param->src_phy_addr,
            cb_param->src_vir_addr, cb_param->fd,
            PREV_EVT_SET_PDAF_DATATYPE_BUFFER);

    return ret;
}

cmr_int prev_pdaf_raw_thread_cb(struct pd_frame_in *cb_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct prev_handle *handle = NULL;
    cmr_u32 camera_id = CAMERA_ID_MAX;

    if (!cb_param || !cb_param->caller_handle) {
        CMR_LOGE("error param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    handle = (struct prev_handle *)cb_param->caller_handle;
    camera_id = cb_param->camera_id;
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    if (cb_param->src_vir_addr)
        ret = cmr_preview_set_pdaf_raw_buffer(
            (cmr_handle)handle, camera_id, cb_param->src_phy_addr,
            cb_param->src_vir_addr, cb_param->fd, PREV_EVT_SET_PDAF_BUFFER);

    return ret;
}

cmr_int prev_pdaf_raw_cb(struct pd_frame_in *cb_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct prev_handle *handle = NULL;
    cmr_u32 camera_id = CAMERA_ID_MAX;

    if (!cb_param || !cb_param->caller_handle) {
        CMR_LOGE("error param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    handle = (struct prev_handle *)cb_param->caller_handle;
    camera_id = cb_param->camera_id;
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    if (cb_param->src_vir_addr)
        ret =
            prev_set_pdaf_raw_buffer(handle, camera_id, cb_param->src_phy_addr,
                                     cb_param->src_vir_addr, cb_param->fd);

    return ret;
}

cmr_int prev_pdaf_raw_frame_handle(struct prev_handle *handle,
                                   cmr_u32 camera_id, struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_s64 timestamp = 0;
    cmr_uint rot_index = 0;
    struct img_addr addr_vir;
    struct img_frm *frm_ptr = NULL;
    struct pd_raw_info pd_raw;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&pd_raw, sizeof(struct pd_raw_info));

    prev_cxt = &handle->prev_cxt[camera_id];
    prev_cxt->pdaf_timestamp = data->sec * 1000000000LL + data->usec * 1000;

    if (!IS_PREVIEW(handle, camera_id)) {
        CMR_LOGE("preview stopped, skip this frame,prev_status %ld ",
                 prev_cxt->prev_status);
        return ret;
    }

    if (!handle->oem_cb || !handle->ops.channel_free_frame) {
        CMR_LOGE("ops oem_cb or channel_free_frame is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGI("got one frame, data->yaddr_vir 0x%x, frame_id 0x%x, "
             "frame_real_id %d, channel_id %d pdaf_timestamp %" PRId64,
             data->yaddr_vir, data->frame_id, data->frame_real_id,
             data->channel_id, prev_cxt->pdaf_timestamp);

    /*skip pdaf data when not preview*/
    if (0 == prev_cxt->pdaf_frm_cnt) {
        CMR_LOGI("skip pdaf data when not preview");
        ret = prev_pop_pdaf_raw_buffer(handle, camera_id, data, 0);
        ret = prev_set_pdaf_raw_buffer(handle, camera_id, data->yaddr,
                                       data->yaddr_vir, data->fd);
    } else {
        CMR_LOGI("send pdaf frame to isp");
        addr_vir.addr_y = data->yaddr_vir;
#if 0 // pdaf image save
			cmr_s8 value[PROPERTY_VALUE_MAX];
			property_get("debug.camera.dump.frame",value,"video");
			if(!strcmp(value,"pdafraw")) {
				if (prev_cxt->pdaf_frm_cnt %20 == 0) {
					camera_save_to_file(prev_cxt->pdaf_frm_cnt,
							IMG_DATA_TYPE_RAW,
							prev_cxt->pdaf_rect.width,
							prev_cxt->pdaf_rect.height,
							&addr_vir);
				}
			}
#endif
        pd_raw.frame_id = prev_cxt->pdaf_frm_cnt;
        pd_raw.addr = (void *)addr_vir.addr_y;
        pd_raw.len = prev_cxt->pdaf_mem_size;
        pd_raw.sec = data->sec;
        pd_raw.usec = data->usec;
        pd_raw.format = prev_cxt->sensor_info.image_format;
        pd_raw.pattern = prev_cxt->sensor_info.image_pattern;
        pd_raw.roi.image_width = prev_cxt->sensor_info.source_width_max;
        pd_raw.roi.image_height = prev_cxt->sensor_info.source_height_max;
        pd_raw.roi.trim_start_x = prev_cxt->pdaf_rect.start_x;
        pd_raw.roi.trim_start_y = prev_cxt->pdaf_rect.start_y;
        pd_raw.roi.trim_width = prev_cxt->pdaf_rect.width;
        pd_raw.roi.trim_height = prev_cxt->pdaf_rect.height;
        pd_raw.pd_in.caller_handle = (cmr_handle)handle;
        pd_raw.pd_in.camera_id = camera_id;
        pd_raw.pd_in.src_phy_addr = data->yaddr;
        pd_raw.pd_in.src_vir_addr = data->yaddr_vir;
        pd_raw.pd_in.fd = data->fd;
        pd_raw.pd_callback = prev_pdaf_raw_cb;
        CMR_LOGV("addr=%p, len=%d, format=%d, pattern=%d, roi max_w=%d, "
                 "max_h=%d,start_x=%d, start_y=%d, w=%d, h=%d ",
                 pd_raw.addr, pd_raw.len, pd_raw.format, pd_raw.pattern,
                 pd_raw.roi.image_width, pd_raw.roi.image_height,
                 pd_raw.roi.trim_start_x, pd_raw.roi.trim_start_y,
                 pd_raw.roi.trim_width, pd_raw.roi.trim_height);
        ret = prev_pop_pdaf_raw_buffer(handle, camera_id, data, 0);
        handle->ops.set_preview_pd_raw(handle->oem_handle, &pd_raw);
    }
exit:

    return ret;
}

cmr_int prev_video_frame_handle(struct prev_handle *handle, cmr_u32 camera_id,
                                struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct camera_frame_type frame_type;
    cmr_s64 timestamp = 0;
    struct prev_cb_info cb_data_info;
    cmr_uint rot_index = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&cb_data_info, sizeof(struct prev_cb_info));
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!IS_PREVIEW(handle, camera_id)) {
        CMR_LOGE("preview stopped, skip this frame");
        return ret;
    }

    if (!handle->oem_cb || !handle->ops.channel_free_frame) {
        CMR_LOGE("ops oem_cb or channel_free_frame is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGD("got one frame, frame_id 0x%x, frame_real_id %d, channel_id %d",
             data->frame_id, data->frame_real_id, data->channel_id);

    if (0 == prev_cxt->video_frm_cnt) {
        /*response*/
        cb_data_info.cb_type = PREVIEW_RSP_CB_SUCCESS;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }
    prev_cxt->video_frm_cnt++;

    if (IMG_ANGLE_0 != prev_cxt->prev_param.prev_rot) {
        ret = prev_get_src_rot_buffer(prev_cxt, data, &rot_index);
        CMR_LOGI("rot_index %ld", rot_index);
        if (ret) {
            CMR_LOGE("get src rot buffer failed");
            return ret;
        }
    }

    /*skip frame if SW skip mode*/
    if (IMG_SKIP_SW == prev_cxt->skip_mode) {
        if (prev_cxt->video_frm_cnt <= prev_cxt->prev_skip_num) {
            CMR_LOGI("ignore this frame, preview cnt %ld, total skip num %ld, "
                     "channed_id %d",
                     prev_cxt->video_frm_cnt, prev_cxt->prev_skip_num,
                     data->channel_id);

            if (IMG_ANGLE_0 != prev_cxt->prev_param.prev_rot) {
                ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_VIDEO,
                                               rot_index, 0);
                if (ret) {
                    CMR_LOGE("prev_set_rot_buffer_flag failed");
                    goto exit;
                }
                data->fd = prev_cxt->video_frm[0].fd;
                data->yaddr = prev_cxt->video_frm[0].addr_phy.addr_y;
                data->uaddr = prev_cxt->video_frm[0].addr_phy.addr_u;
                data->vaddr = prev_cxt->video_frm[0].addr_phy.addr_v;
                data->yaddr_vir = prev_cxt->video_frm[0].addr_vir.addr_y;
                data->uaddr_vir = prev_cxt->video_frm[0].addr_vir.addr_u;
                data->vaddr_vir = prev_cxt->video_frm[0].addr_vir.addr_v;
            }
            ret = prev_pop_video_buffer(handle, camera_id, data, 1);
            if (ret) {
                CMR_LOGE("pop frm failed");
            }

            return ret;
        }
    }

    /*skip frame when set param*/
    if ((IMG_SKIP_SW == prev_cxt->skip_mode) &&
        prev_cxt->video_restart_skip_en) {

        timestamp = data->sec * 1000000000LL + data->usec * 1000;
        CMR_LOGI("Restart skip: frame time = %" PRId64
                 ", restart time=%" PRId64,
                 timestamp, prev_cxt->video_restart_timestamp);

        if (timestamp > prev_cxt->video_restart_timestamp) {

            pthread_mutex_lock(&handle->thread_cxt.prev_mutex);
            prev_cxt->video_restart_skip_cnt++;
            pthread_mutex_unlock(&handle->thread_cxt.prev_mutex);

            if (prev_cxt->video_restart_skip_cnt <= prev_cxt->prev_skip_num) {
                CMR_LOGI(
                    "skip this frm, skip_cnt %ld, skip_num %ld channed_id %d",
                    prev_cxt->video_restart_skip_cnt, prev_cxt->prev_skip_num,
                    data->channel_id);

                if (IMG_ANGLE_0 != prev_cxt->prev_param.prev_rot) {
                    ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_VIDEO,
                                                   rot_index, 0);
                    if (ret) {
                        CMR_LOGE("prev_set_rot_buffer_flag failed");
                        goto exit;
                    }
                    data->fd = prev_cxt->video_frm[0].fd;
                    data->yaddr = prev_cxt->video_frm[0].addr_phy.addr_y;
                    data->uaddr = prev_cxt->video_frm[0].addr_phy.addr_u;
                    data->vaddr = prev_cxt->video_frm[0].addr_phy.addr_v;
                    data->yaddr_vir = prev_cxt->video_frm[0].addr_vir.addr_y;
                    data->uaddr_vir = prev_cxt->video_frm[0].addr_vir.addr_u;
                    data->vaddr_vir = prev_cxt->video_frm[0].addr_vir.addr_v;
                }
                ret = prev_pop_video_buffer(handle, camera_id, data, 1);
                if (ret) {
                    CMR_LOGE("pop frm failed");
                }
                return ret;

            } else {

                pthread_mutex_lock(&handle->thread_cxt.prev_mutex);
                prev_cxt->video_restart_skip_cnt = 0;
                prev_cxt->video_restart_skip_en = 0;
                pthread_mutex_unlock(&handle->thread_cxt.prev_mutex);

                CMR_LOGI("Restart skip: end");
            }
        } else {
            CMR_LOGI("Restart skip: frame is before restart, no need to skip "
                     "this frame");
        }
    }

    if (IMG_ANGLE_0 == prev_cxt->prev_param.prev_rot) {
        ret = prev_construct_video_frame(handle, camera_id, data, &frame_type);
        if (ret) {
            CMR_LOGE("construct frm err");
            goto exit;
        }
        prev_cxt->video_buf_id = frame_type.buf_id;

        /*notify frame via callback*/
        /*need rotation*/
        /*construct frame*/
        /*notify frame*/

        ret = prev_pop_video_buffer(handle, camera_id, data, 0);
        if (ret) {
            CMR_LOGE("pop frm 0x%x err", data->channel_id);
            goto exit;
        }
        cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = &frame_type;
        prev_cb_start(handle, &cb_data_info);
    } else {
        if (prev_cxt->video_mem_valid_num > 0) {
            ret = prev_start_rotate(handle, camera_id, data);
            if (ret) {
                CMR_LOGE("rot failed, skip this frm");
                ret = CMR_CAMERA_SUCCESS;
                goto exit;
            }
            CMR_LOGI("rot done");
            ret = prev_construct_video_frame(handle, camera_id, data,
                                             &frame_type);
            if (ret) {
                CMR_LOGE("construct frm 0x%x err", data->frame_id);
                goto exit;
            }
            ret =
                prev_set_rot_buffer_flag(prev_cxt, CAMERA_VIDEO, rot_index, 0);
            if (ret) {
                CMR_LOGE("prev_set_rot_buffer_flag failed");
                goto exit;
            }
            ret = prev_pop_video_buffer(handle, camera_id, data, 0);
            if (ret) {
                CMR_LOGE("pop frm 0x%x err", data->channel_id);
                goto exit;
            }
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);
        } else {
            CMR_LOGW("no available buf, drop! channel_id 0x%x",
                     data->channel_id);
            ret =
                prev_set_rot_buffer_flag(prev_cxt, CAMERA_VIDEO, rot_index, 0);
            if (ret) {
                CMR_LOGE("prev_set_rot_buffer_flag failed");
                goto exit;
            }
        }
    }
exit:
    if (ret) {
        cb_data_info.cb_type = PREVIEW_EXIT_CB_FAILED;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }

    return ret;
}

cmr_int prev_zsl_frame_handle(struct prev_handle *handle, cmr_u32 camera_id,
                              struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct camera_frame_type frame_type;
    cmr_s64 timestamp = 0;
    struct prev_cb_info cb_data_info;
    cmr_uint rot_index = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&cb_data_info, sizeof(struct prev_cb_info));
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!IS_PREVIEW(handle, camera_id)) {
        CMR_LOGE("preview stopped, skip this frame");
        return ret;
    }

    if (!handle->oem_cb || !handle->ops.channel_free_frame) {
        CMR_LOGE("ops oem_cb or channel_free_frame is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGD("frame_id=0x%x, frame_real_id=%d, channel_id=%d, fd=0x%x",
             data->frame_id, data->frame_real_id, data->channel_id, data->fd);
    CMR_LOGV("cap_zsl_frm_cnt %ld", prev_cxt->cap_zsl_frm_cnt);
    if (0 == prev_cxt->cap_zsl_frm_cnt) {
        /*response*/
        cb_data_info.cb_type = PREVIEW_RSP_CB_SUCCESS;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }
    prev_cxt->cap_zsl_frm_cnt++;

    if (IMG_ANGLE_0 != prev_cxt->prev_param.prev_rot) {
        ret = prev_get_src_rot_buffer(prev_cxt, data, &rot_index);
        if (ret) {
            CMR_LOGE("get src rot buffer failed");
            return ret;
        }
    }

    /*skip frame if SW skip mode*/
    if (IMG_SKIP_SW == prev_cxt->skip_mode &&
        !prev_cxt->prev_param.sprd_burstmode_enabled) {
        if (prev_cxt->cap_zsl_frm_cnt <= prev_cxt->prev_skip_num &&
            prev_cxt->cap_zsl_frm_cnt < CMR_MAX_SKIP_NUM) {
            CMR_LOGI("ignore this frame, preview cnt %ld, total skip num %ld, "
                     "channed_id %d",
                     prev_cxt->cap_zsl_frm_cnt, prev_cxt->prev_skip_num,
                     data->channel_id);

            ret = prev_pop_zsl_buffer(handle, camera_id, data, 1);
            if (ret) {
                CMR_LOGE("pop frm failed");
            }

            return ret;
        }
    }

    /*skip frame when set param*/
    if ((IMG_SKIP_SW == prev_cxt->skip_mode) &&
        prev_cxt->cap_zsl_restart_skip_en &&
        !prev_cxt->prev_param.sprd_burstmode_enabled) {

        timestamp = data->sec * 1000000000LL + data->usec * 1000;
        CMR_LOGI("Restart skip: frame time = %" PRId64
                 ", restart time=%" PRId64,
                 timestamp, prev_cxt->cap_zsl_restart_timestamp);

        if (timestamp > prev_cxt->cap_zsl_restart_timestamp) {

            pthread_mutex_lock(&handle->thread_cxt.prev_mutex);
            prev_cxt->cap_zsl_restart_skip_cnt++;
            pthread_mutex_unlock(&handle->thread_cxt.prev_mutex);

            if (prev_cxt->cap_zsl_restart_skip_cnt <= prev_cxt->prev_skip_num) {
                CMR_LOGI(
                    "skip this frm, skip_cnt %ld, skip_num %ld channed_id %d",
                    prev_cxt->cap_zsl_restart_skip_cnt, prev_cxt->prev_skip_num,
                    data->channel_id);

                ret = prev_pop_zsl_buffer(handle, camera_id, data, 1);
                if (ret) {
                    CMR_LOGE("pop frm failed");
                }
                return ret;

            } else {

                pthread_mutex_lock(&handle->thread_cxt.prev_mutex);
                prev_cxt->cap_zsl_restart_skip_cnt = 0;
                prev_cxt->cap_zsl_restart_skip_en = 0;
                pthread_mutex_unlock(&handle->thread_cxt.prev_mutex);

                CMR_LOGI("Restart skip: end");
            }
        } else {
            CMR_LOGI("Restart skip: frame is before restart, no need to skip "
                     "this frame");
        }
    }

    if (0 /*prev_cxt->cap_zsl_frm_cnt < 50 && (prev_cxt->cap_zsl_frm_cnt%5) == 0*/) {
        camera_save_yuv_to_file(
            FORM_DUMPINDEX(ZSL_OUT_DATA, prev_cxt->cap_zsl_frm_cnt, 0),
            IMG_DATA_TYPE_YUV420, prev_cxt->actual_pic_size.width,
            prev_cxt->actual_pic_size.height,
            (struct img_addr *)&data->yaddr_vir);
    }

    if (IMG_ANGLE_0 == prev_cxt->prev_param.cap_rot) {
        ret = prev_construct_zsl_frame(handle, camera_id, data, &frame_type);
        if (ret) {
            CMR_LOGE("construct frm err");
            goto exit;
        }
        prev_cxt->cap_zsl_buf_id = frame_type.buf_id;

        ret = prev_pop_zsl_buffer(handle, camera_id, data, 0);
        if (ret) {
            CMR_LOGE("pop frm 0x%x err", data->channel_id);
            goto exit;
        }
        /*notify frame via callback*/
        cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = &frame_type;
        prev_cb_start(handle, &cb_data_info);
    } else {
        if (prev_cxt->cap_zsl_mem_valid_num > 0) {
            ret = prev_start_rotate(handle, camera_id, data);
            if (ret) {
                CMR_LOGE("rot failed, skip this frm");
                ret = CMR_CAMERA_SUCCESS;
                goto exit;
            }
            ret =
                prev_construct_zsl_frame(handle, camera_id, data, &frame_type);
            if (ret) {
                CMR_LOGE("construct frm 0x%x err", data->frame_id);
                goto exit;
            }
            ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_SNAPSHOT_ZSL,
                                           rot_index, 0);
            if (ret) {
                CMR_LOGE("prev_set_rot_buffer_flag failed");
                goto exit;
            }
            ret = prev_pop_zsl_buffer(handle, camera_id, data, 0);
            if (ret) {
                CMR_LOGE("pop frm 0x%x err", data->channel_id);
                goto exit;
            }
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);
        } else {
            CMR_LOGW("no available buf, drop! channel_id 0x%x",
                     data->channel_id);
            ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_SNAPSHOT_ZSL,
                                           rot_index, 0);
            if (ret) {
                CMR_LOGE("prev_set_rot_buffer_flag failed");
                goto exit;
            }
        }
    }

exit:
    if (ret) {
        cb_data_info.cb_type = PREVIEW_EXIT_CB_FAILED;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }

    return ret;
}

cmr_int prev_capture_frame_handle(struct prev_handle *handle, cmr_u32 camera_id,
                                  struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 channel_bits = 0;
    struct buffer_cfg buf_cfg;
    cmr_uint i;
    cmr_uint hdr_num = HDR_CAP_NUM;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!handle->ops.channel_stop || !handle->ops.isp_stop_video) {
        CMR_LOGE("ops is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    channel_bits = 1 << prev_cxt->cap_channel_id;

    CMR_LOGI("frame_id 0x%x", data->frame_id);
    if (!(CMR_CAP0_ID_BASE == (data->frame_id & CMR_CAP0_ID_BASE) ||
          CMR_CAP1_ID_BASE == (data->frame_id & CMR_CAP1_ID_BASE))) {
        CMR_LOGE("0x%x not capture frame, drop it", data->frame_id);
        return ret;
    }
    if (FRAME_FLASH_MAX != prev_cxt->prev_param.frame_count) {
        prev_cxt->cap_frm_cnt++;
    }
    CMR_LOGI("frame_ctrl %d, frame_count %d, cap_frm_cnt %ld, isp_status %ld",
             prev_cxt->prev_param.frame_ctrl, prev_cxt->prev_param.frame_count,
             prev_cxt->cap_frm_cnt, prev_cxt->isp_status);

    if (prev_cxt->cap_frm_cnt < prev_cxt->prev_param.frame_count) {
        CMR_LOGI("got one cap frm, restart another");

        /*stop channel*/
        if (FRAME_STOP == prev_cxt->prev_param.frame_ctrl) {
            ret = handle->ops.channel_stop(handle->oem_handle, channel_bits);
            if (ret) {
                CMR_LOGE("channel_stop failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }

            /*stop isp*/
            if (PREV_ISP_COWORK == prev_cxt->isp_status) {
                ret = handle->ops.isp_stop_video(handle->oem_handle);
                prev_cxt->isp_status = PREV_ISP_IDLE;
                if (ret) {
                    CMR_LOGE("Failed to stop ISP video mode, %ld", ret);
                }
            }

            /*got one frame, start another*/
            ret = prev_restart_cap_channel(handle, camera_id, data);
        }
        if (FRAME_HDR_PROC == prev_cxt->prev_param.frame_ctrl) {
#if 0
				ret = handle->ops.channel_pause(handle->oem_handle, prev_cxt->cap_channel_id, 1);
				if (ret) {
					CMR_LOGE("pause chn failed");
					ret = CMR_CAMERA_FAIL;
					goto exit;
				}
#endif

            if (prev_cxt->prev_param.snapshot_eb &&
                !prev_cxt->prev_param.preview_eb) {
                if (handle->ops.capture_pre_proc) {
                    if (prev_cxt->cap_frm_cnt <=
                        prev_cxt->prev_param.frame_count - hdr_num)
                        handle->ops.capture_pre_proc(
                            handle->oem_handle, camera_id, prev_cxt->prev_mode,
                            prev_cxt->cap_mode, 1, 0);
                } else {
                    CMR_LOGE("err,capture_pre_proc is null");
                    ret = CMR_CAMERA_INVALID_PARAM;
                    goto exit;
                }
#if 0
					if (handle->ops.hdr_set_ev) {
						if (prev_cxt->cap_frm_cnt <= prev_cxt->prev_param.frame_count - hdr_num)
							handle->ops.hdr_set_ev(handle->oem_handle);
					} else {
						CMR_LOGE("err,hdr_set_ev is null");
						ret = CMR_CAMERA_INVALID_PARAM;
						goto exit;
					}
#endif
            }
            if (prev_cxt->cap_frm_cnt <=
                prev_cxt->prev_param.frame_count - hdr_num) {
                cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
                buf_cfg.channel_id = prev_cxt->cap_channel_id;
                buf_cfg.base_id = CMR_CAP0_ID_BASE;
                buf_cfg.count = 1;
                buf_cfg.flag = BUF_FLAG_RUNNING;
                buf_cfg.length = prev_cxt->actual_pic_size.width *
                                 prev_cxt->actual_pic_size.height * 3 / 2;

                buf_cfg.addr[0].addr_y = data->yaddr;
                buf_cfg.addr[0].addr_u = buf_cfg.addr[0].addr_y +
                                         prev_cxt->actual_pic_size.width *
                                             prev_cxt->actual_pic_size.height;
                buf_cfg.addr_vir[0].addr_y = data->yaddr_vir;
                buf_cfg.addr_vir[0].addr_u =
                    buf_cfg.addr_vir[0].addr_y +
                    prev_cxt->actual_pic_size.width *
                        prev_cxt->actual_pic_size.height;
                buf_cfg.fd[0] = data->fd;

                ret =
                    handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
                if (ret) {
                    CMR_LOGE("channel_buff_cfg failed");
                    ret = CMR_CAMERA_FAIL;
                    goto exit;
                }
            }
#if 0
				ret = handle->ops.channel_resume(handle->oem_handle, prev_cxt->cap_channel_id, 0, 0, 1);
				if (ret) {
					CMR_LOGE("resume chn failed");
					ret = CMR_CAMERA_FAIL;
					goto exit;
				}
#endif
        }
    }

    /*capture done, stop isp and channel*/
    if (prev_cxt->cap_frm_cnt == prev_cxt->prev_param.frame_count ||
        (FRAME_FLASH_MAX == prev_cxt->prev_param.frame_count)) {

        CMR_LOGI("got total %ld cap frm, stop chn and isp",
                 prev_cxt->cap_frm_cnt);

        /*stop channel*/
        ret = handle->ops.channel_stop(handle->oem_handle, channel_bits);
        if (ret) {
            CMR_LOGE("channel_stop failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        prev_cxt->cap_channel_status = PREV_CHN_IDLE;

        /*stop isp*/
        if (PREV_ISP_COWORK == prev_cxt->isp_status) {
            ret = handle->ops.isp_stop_video(handle->oem_handle);
            prev_cxt->isp_status = PREV_ISP_IDLE;
            if (ret) {
                CMR_LOGE("Failed to stop ISP video mode, %ld", ret);
            }
        }

        /*post proc*/
        CMR_LOGI("post proc");
        ret = handle->ops.capture_post_proc(handle->oem_handle, camera_id);
        if (ret) {
            CMR_LOGE("post proc failed");
        }
    }

exit:
    return ret;
}

cmr_int prev_error_handle(struct prev_handle *handle, cmr_u32 camera_id,
                          cmr_uint evt_type) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    enum recovery_mode mode = 0;
    struct prev_context *prev_cxt = NULL;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!handle->oem_cb) {
        CMR_LOGE("oem_cb is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&cb_data_info, sizeof(struct prev_cb_info));

    CMR_LOGI("error type 0x%lx, camera_id %d", evt_type, camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    CMR_LOGI("prev_status %ld, preview_eb %d, snapshot_eb %d",
             prev_cxt->prev_status, prev_cxt->prev_param.preview_eb,
             prev_cxt->prev_param.snapshot_eb);

    CMR_LOGI("recovery_status %ld, mode %d", prev_cxt->recovery_status, mode);

    if (PREV_RECOVERY_DONE == prev_cxt->recovery_status) {
        CMR_LOGE("recovery failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    switch (evt_type) {
    case CMR_GRAB_TX_ERROR:
    case CMR_GRAB_CSI2_ERR:
        if (PREV_RECOVERING == prev_cxt->recovery_status) {
            /* when in recovering */
            prev_cxt->recovery_cnt--;

            CMR_LOGI("recovery_cnt, %ld", prev_cxt->recovery_cnt);
            if (prev_cxt->recovery_cnt) {
                /* try once more */
                mode = RECOVERY_MIDDLE;
            } else {
                /* tried three times, it hasn't recovered yet, restart */
                mode = RECOVERY_HEAVY;
                prev_cxt->recovery_status = PREV_RECOVERY_DONE;
            }
        } else {
            /* not in recovering, start to recover three times */
            mode = RECOVERY_MIDDLE;
            prev_cxt->recovery_status = PREV_RECOVERING;
            prev_cxt->recovery_cnt = PREV_RECOVERY_CNT;
            CMR_LOGD("Need recover, recovery_cnt, %ld", prev_cxt->recovery_cnt);
        }
        break;

    case CMR_SENSOR_ERROR:
    case CMR_GRAB_TIME_OUT:
        mode = RECOVERY_HEAVY;
        prev_cxt->recovery_status = PREV_RECOVERY_DONE;
        CMR_LOGD("Sensor error, restart preview");
        break;

    default:
        CMR_LOGE("invalid evt_type");
        break;
    }

    ret = prev_recovery_pre_proc(handle, camera_id, mode);
    if (ret) {
        CMR_LOGE("stop failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret = prev_recovery_post_proc(handle, camera_id, mode);
    if (ret) {
        CMR_LOGE("start failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (prev_cxt->prev_param.preview_eb) {
            CMR_LOGE("Call cb to notice the upper layer something error "
                     "blocked preview");
            cb_data_info.cb_type = PREVIEW_EXIT_CB_FAILED;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = NULL;
            prev_cb_start(handle, &cb_data_info);
        }

        if (prev_cxt->prev_param.snapshot_eb) {
            CMR_LOGE("Call cb to notice the upper layer something error "
                     "blocked capture");
            cb_data_info.cb_type = PREVIEW_EXIT_CB_FAILED;
            cb_data_info.func_type = PREVIEW_FUNC_START_CAPTURE;
            cb_data_info.frame_data = NULL;
            prev_cb_start(handle, &cb_data_info);
        }
    }

    return 0;
}

cmr_int prev_recovery_pre_proc(struct prev_handle *handle, cmr_u32 camera_id,
                               enum recovery_mode mode) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;

    if (!handle->ops.sensor_close) {
        CMR_LOGE("ops is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGI("mode %d, camera_id %d", mode, camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    switch (mode) {
    case RECOVERY_HEAVY:
    case RECOVERY_MIDDLE:
        ret = prev_stop(handle, camera_id, 1);
        if (RECOVERY_HEAVY == mode) {
            /*close sensor*/
            handle->ops.sensor_close(handle->oem_handle, camera_id);
        }
        break;

    default:
        break;
    }

    return ret;
}

cmr_int prev_recovery_post_proc(struct prev_handle *handle, cmr_u32 camera_id,
                                enum recovery_mode mode) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;

    if (!handle->ops.sensor_open) {
        CMR_LOGE("ops is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGI("mode %d, camera_id %d", mode, camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    if (IDLE == prev_cxt->prev_status && prev_cxt->prev_param.preview_eb &&
        prev_cxt->prev_param.snapshot_eb) {
        CMR_LOGE("is idle now, do nothing");
        return ret;
    }

    switch (mode) {
    case RECOVERY_HEAVY:
    case RECOVERY_MIDDLE:
        if (RECOVERY_HEAVY == mode) {
            /*open sesnor*/
            handle->ops.sensor_open(handle->oem_handle, camera_id);
        }

        ret = prev_set_param_internal(handle, camera_id, 1, NULL);
        if (ret) {
            CMR_LOGE("set param err");
            return ret;
        }

        ret = prev_start(handle, camera_id, 1, 1);

        break;

    default:
        break;
    }

    return ret;
}

cmr_int prev_recovery_reset(struct prev_handle *handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;

    prev_cxt = &handle->prev_cxt[camera_id];

    /*reset recovery status*/
    prev_cxt->recovery_status = PREV_RECOVERY_IDLE;

    return ret;
}

cmr_int prev_local_init(struct prev_handle *handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);

    return ret;
}

cmr_int prev_local_deinit(struct prev_handle *handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);

    return ret;
}

cmr_int prev_pre_set(struct prev_handle *handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 sensor_mode = 0;
    struct prev_context *prev_cxt = NULL;

    CHECK_HANDLE_VALID(handle);

    prev_cxt = &handle->prev_cxt[camera_id];

    if (prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb) {
        sensor_mode = prev_cxt->cap_mode;
    } else {
        sensor_mode = prev_cxt->prev_mode;
    }

    if (handle->ops.preview_pre_proc) {
        handle->ops.preview_pre_proc(handle->oem_handle, camera_id,
                                     sensor_mode);
        CMR_LOGI("preview_pre_proc called");
    } else {
        CMR_LOGE("pre proc is null");
        ret = CMR_CAMERA_FAIL;
    }

    ATRACE_END();
    return ret;
}

cmr_int prev_post_set(struct prev_handle *handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);

    if (handle->ops.preview_post_proc) {
        handle->ops.preview_post_proc(handle->oem_handle, camera_id);
    } else {
        CMR_LOGE("post proc is null");
        ret = CMR_CAMERA_FAIL;
    }

    return ret;
}

cmr_int prev_start(struct prev_handle *handle, cmr_u32 camera_id,
                   cmr_u32 is_restart, cmr_u32 is_sn_reopen) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 preview_enable = 0;
    cmr_u32 snapshot_enable = 0;
    cmr_u32 video_enable = 0;
    enum sensor_pdaf_type pdaf_mode = 0;
    cmr_u32 tool_eb = 0;
    cmr_u32 channel_bits = 0;
    cmr_uint skip_num = 0;
    struct video_start_param video_param;
    struct sensor_mode_info *sensor_mode_info = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    sensor_mode_info = &prev_cxt->sensor_info.mode_info[prev_cxt->cap_mode];

    if (!handle->ops.channel_start || !handle->ops.capture_pre_proc) {
        CMR_LOGE("ops is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    preview_enable = prev_cxt->prev_param.preview_eb;
    snapshot_enable = prev_cxt->prev_param.snapshot_eb;
    video_enable = prev_cxt->prev_param.video_eb;
    tool_eb = prev_cxt->prev_param.tool_eb;
    pdaf_mode = prev_cxt->prev_param.pdaf_mode;
    CMR_LOGI("camera_id %d, prev_status %ld, preview_eb %d, snapshot_eb %d",
             camera_id, prev_cxt->prev_status, preview_enable, snapshot_enable);

    if (preview_enable && PREVIEWING == prev_cxt->prev_status) {
        CMR_LOGE("is previewing now, do nothing");
        return ret;
    }

    if (preview_enable && snapshot_enable) {
        channel_bits =
            (1 << prev_cxt->prev_channel_id) | (1 << prev_cxt->cap_channel_id);
        skip_num = prev_cxt->prev_skip_num;
    } else if (preview_enable) {
        channel_bits = 1 << prev_cxt->prev_channel_id;
        skip_num = prev_cxt->prev_skip_num;
    } else if (snapshot_enable) {
        channel_bits = 1 << prev_cxt->cap_channel_id;
        skip_num = prev_cxt->cap_skip_num;
    } else {
        CMR_LOGE("invalid param");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGI("channel_bits %d, skip_num %ld", channel_bits, skip_num);

    if (snapshot_enable && !preview_enable) {
        if (handle->ops.capture_pre_proc) {
            handle->ops.capture_pre_proc(
                handle->oem_handle, camera_id, prev_cxt->prev_mode,
                prev_cxt->cap_mode, is_restart, is_sn_reopen);
        } else {
            CMR_LOGE("err,capture_pre_proc is null");
            ret = CMR_CAMERA_INVALID_PARAM;
            goto exit;
        }
    }

    /*start isp for cap*/
    if (snapshot_enable && !preview_enable && !tool_eb) {
        if (prev_cxt->cap_need_isp && (PREV_ISP_IDLE == prev_cxt->isp_status)) {

#if defined(CONFIG_CAMERA_NO_DCAM_DATA_PATH)
            video_param.size.width = prev_cxt->actual_pic_size.width;
            video_param.size.height = prev_cxt->actual_pic_size.height;
#else
            video_param.size.width = sensor_mode_info->trim_width;
            if (prev_cxt->cap_need_binning) {
                video_param.size.width = video_param.size.width >> 1;
            }
            video_param.size.height = sensor_mode_info->trim_height;
#endif
            CMR_LOGI("snapshot_eb=%d video_param.size.width=%d, "
                     "video_param.size.height=%d\n",
                     prev_cxt->prev_param.snapshot_eb, video_param.size.width,
                     video_param.size.height);
            video_param.img_format = ISP_DATA_NORMAL_RAW10;
            video_param.work_mode = 1;
            video_param.capture_skip_num = prev_cxt->cap_skip_num;
            if (FRAME_FLASH_MAX == prev_cxt->prev_param.frame_count) {
                video_param.is_need_flash = 1;
                video_param.video_mode = ISP_VIDEO_MODE_CONTINUE;
            } else {
                video_param.is_need_flash = 0;
                video_param.video_mode = ISP_VIDEO_MODE_SINGLE;
            }

            if (!handle->ops.isp_start_video) {
                CMR_LOGE("ops isp_start_video is null");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
            video_param.live_view_sz.width = prev_cxt->actual_prev_size.width;
            video_param.live_view_sz.height = prev_cxt->actual_prev_size.height;
            video_param.lv_size = prev_cxt->lv_size;
            video_param.video_size = prev_cxt->video_size;
            ret = handle->ops.isp_start_video(handle->oem_handle, &video_param);
            if (ret) {
                CMR_LOGE("isp start video failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
            prev_cxt->isp_status = PREV_ISP_COWORK;
        }
    } else if (prev_cxt->prev_param.sprd_burstmode_enabled) {
        video_param.size.width = sensor_mode_info->trim_width;
        if (prev_cxt->cap_need_binning)
            video_param.size.width = video_param.size.width >> 1;

        video_param.size.height = sensor_mode_info->trim_height;
        video_param.img_format = ISP_DATA_NORMAL_RAW10;
        video_param.video_mode = ISP_VIDEO_MODE_CONTINUE;
        video_param.work_mode = 0;
        video_param.lv_size = prev_cxt->lv_size;
        video_param.video_size = prev_cxt->video_size;
        video_param.capture_skip_num = prev_cxt->cap_skip_num;
        if (!handle->ops.isp_start_video) {
            CMR_LOGE("ops isp_start_video is null");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        CMR_LOGI("width %d, height %d", video_param.size.width,
                 video_param.size.height);
        ret = handle->ops.isp_start_video(handle->oem_handle, &video_param);
        if (ret) {
            CMR_LOGE("isp start video failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        prev_cxt->isp_status = PREV_ISP_COWORK;
    }

    if (snapshot_enable && !preview_enable) {
        if (handle->ops.hdr_set_ev) {
            handle->ops.hdr_set_ev(handle->oem_handle);
        } else {
            CMR_LOGE("err,hdr_set_ev is null");
            ret = CMR_CAMERA_INVALID_PARAM;
            goto exit;
        }
    }

    /*start channel*/
    if (FRAME_HDR_PROC == prev_cxt->prev_param.frame_ctrl) {
        skip_num = 0;
    }
    if (prev_cxt->skip_mode != IMG_SKIP_HW)
		skip_num = 0;

    ret = handle->ops.channel_start(handle->oem_handle, channel_bits, skip_num);
    if (ret) {
        CMR_LOGE("channel_start failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*update preview status*/
    if (preview_enable) {
        prev_cxt->prev_status = PREVIEWING;

        /*init fd*/
        CMR_LOGI("is_support_fd %ld", prev_cxt->prev_param.is_support_fd);
        if (prev_cxt->prev_param.is_support_fd) {
            prev_fd_open(handle, camera_id);
        }
        /*init sensor_datatype*/
        CMR_LOGI("sensor_datatype %u", prev_cxt->prev_param.sensor_datatype);
#if 1
        struct sensor_otp_cust_info sensor_otp;
        switch (prev_cxt->prev_param.sensor_datatype) {
        case SENSOR_REAL_DEPTH_ENABLE:
            /* get otp  buffer  */
            ret = handle->ops.get_sensor_otp(handle->oem_handle, &sensor_otp);
            if (ret) {
                CMR_LOGE("get sensor otp error");
                goto exit;
            }
            CMR_LOGI("sensor_otp %p, dual flag %d",
                     sensor_otp.total_otp.data_ptr,
                     sensor_otp.dual_otp.dual_flag);
            if (sensor_otp.total_otp.data_ptr != NULL &&
                sensor_otp.dual_otp.dual_flag)
                prev_sensor_datatype_open(handle, camera_id,
                                          sensor_otp.total_otp);

            break;
        case SENSOR_EMBEDDED_INFO_ENABLE:
            break;
        case SENSOR_DATATYPE_PDAF_ENABLE: {
            struct pd_raw_open pd_open;

            pd_open.open = 1;
            pd_open.pd_set_buffer = prev_pdaf_datatype_thread_cb;
            handle->ops.set_preview_pd_open(handle->oem_handle, &pd_open);
        } break;
        default:
            break;
        }
#endif
    }

    if (pdaf_mode == SENSOR_PDAF_TYPE3_ENABLE) {
        struct pd_raw_open pd_open;

        pd_open.open = 1;
        pd_open.size = prev_cxt->pdaf_mem_size;
        pd_open.pd_set_buffer = prev_pdaf_raw_thread_cb;
        handle->ops.set_preview_pd_open(handle->oem_handle, &pd_open);
    }
exit:
    if (ret) {
        if (preview_enable) {
            prev_post_set(handle, camera_id);
            prev_free_prev_buf(handle, camera_id, 0);
        }

        if (video_enable) {
            prev_free_video_buf(handle, camera_id, 0);
        }

        if (snapshot_enable) {
            prev_free_cap_buf(handle, camera_id, 0);
            if (!prev_cxt->prev_param.sprd_burstmode_enabled) {
                prev_free_cap_reserve_buf(handle, camera_id, 0);
            }
            prev_free_zsl_buf(handle, camera_id, 0);
        }
        if (pdaf_mode == SENSOR_PDAF_TYPE3_ENABLE) {
            prev_free_pdaf_raw_buf(handle, camera_id, 0);
        }
    }

    CMR_LOGD("out");
    ATRACE_END();
    return ret;
}

cmr_int prev_stop(struct prev_handle *handle, cmr_u32 camera_id,
                  cmr_u32 is_restart) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 preview_enable = 0;
    cmr_u32 snapshot_enable = 0;
    cmr_u32 video_enable = 0;
    enum sensor_pdaf_type pdaf_mode = 0;
    cmr_u32 channel_bits = 0;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!handle->ops.channel_stop || !handle->ops.isp_stop_video) {
        CMR_LOGE("ops is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    preview_enable = prev_cxt->prev_param.preview_eb;
    snapshot_enable = prev_cxt->prev_param.snapshot_eb;
    video_enable = prev_cxt->prev_param.video_eb;
    pdaf_mode = prev_cxt->prev_param.pdaf_mode;

    CMR_LOGI("camera_id %d, prev_status %ld, isp_status %ld, preview_eb %d, "
             "snapshot_eb %d",
             camera_id, prev_cxt->prev_status, prev_cxt->isp_status,
             preview_enable, snapshot_enable);

    if (IDLE == prev_cxt->prev_status && preview_enable) {
        CMR_LOGE("is idle now, do nothing");
        return ret;
    }

    if (preview_enable && snapshot_enable) {

        channel_bits =
            (1 << prev_cxt->prev_channel_id) | (1 << prev_cxt->cap_channel_id);
        prev_cxt->prev_channel_status = PREV_CHN_IDLE;
        prev_cxt->cap_channel_status = PREV_CHN_IDLE;

    } else if (preview_enable) {

        channel_bits = 1 << prev_cxt->prev_channel_id;
        prev_cxt->prev_channel_status = PREV_CHN_IDLE;

    } else if (snapshot_enable) {

        channel_bits = 1 << prev_cxt->cap_channel_id;
        prev_cxt->cap_channel_status = PREV_CHN_IDLE;
    }

    /*stop channel*/
    CMR_LOGI("channel_bits %d", channel_bits);
    ret = handle->ops.channel_stop(handle->oem_handle, channel_bits);
    if (ret) {
        CMR_LOGE("channel_stop failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (preview_enable) {
        if (is_restart && PREV_RECOVERY_IDLE != prev_cxt->recovery_status) {
            prev_cxt->prev_status = RECOVERING_IDLE;
        } else {
            prev_cxt->prev_status = IDLE;
        }

        /*deinit fd*/
        if (prev_cxt->prev_param.is_support_fd) {
            prev_fd_close(handle, camera_id);
        }

        /*deinit sensor_datatype*/
        if (prev_cxt->prev_param.sensor_datatype == SENSOR_REAL_DEPTH_ENABLE) {
            prev_sensor_datatype_close(handle, camera_id);
        }
    }

    /*stop isp*/
    if (PREV_ISP_COWORK == prev_cxt->isp_status) {
        ret = handle->ops.isp_stop_video(handle->oem_handle);
        prev_cxt->isp_status = PREV_ISP_IDLE;
        if (ret) {
            CMR_LOGE("Failed to stop ISP video mode, %ld", ret);
        }
    }

    if (preview_enable) {
        prev_post_set(handle, camera_id);
        prev_free_prev_buf(handle, camera_id, is_restart);

        CMR_LOGD("is_restart %d, recovery_status %ld", is_restart,
                 prev_cxt->recovery_status);
        if (!is_restart) {
            /*stop response*/
            cb_data_info.cb_type = PREVIEW_RSP_CB_SUCCESS;
            cb_data_info.func_type = PREVIEW_FUNC_STOP_PREVIEW;
            cb_data_info.frame_data = NULL;
            prev_cb_start(handle, &cb_data_info);
        }
    }
    if (handle->prev_cxt[camera_id].prev_param.sensor_datatype >
        SENSOR_DATATYPE_DISABLED) {
        prev_free_sensor_datatype_buf(handle, camera_id, 0);
    }
    if (pdaf_mode == SENSOR_PDAF_TYPE3_ENABLE) {
        struct pd_raw_open pd_open;

        cmr_bzero(&pd_open, sizeof(pd_open));
        pd_open.open = 0;
        pd_open.size = 0;
        handle->ops.set_preview_pd_open(handle->oem_handle, &pd_open);
        prev_free_pdaf_raw_buf(handle, camera_id, is_restart);
    }

    if (video_enable) {
        prev_free_video_buf(handle, camera_id, is_restart);
    }

    if (snapshot_enable) {
        /*capture post proc*/
        ret = handle->ops.capture_post_proc(handle->oem_handle, camera_id);
        if (ret) {
            CMR_LOGE("post proc failed");
        }
        prev_free_cap_buf(handle, camera_id, is_restart);
    }

    if (!prev_cxt->prev_param.sprd_burstmode_enabled) {
        prev_free_cap_reserve_buf(handle, camera_id, is_restart);
    }

    prev_free_zsl_buf(handle, camera_id, is_restart);

    prev_cxt->prev_frm_cnt = 0;
    prev_cxt->video_frm_cnt = 0;
    prev_cxt->cap_zsl_frm_cnt = 0;
    prev_cxt->pdaf_frm_cnt = 0;
    if (!is_restart) {
        prev_cxt->cap_frm_cnt = 0;
    }
    pthread_mutex_lock(&handle->thread_cxt.prev_mutex);
    prev_cxt->prev_preflash_skip_en = 0;
    prev_cxt->restart_skip_cnt = 0;
    prev_cxt->restart_skip_en = 0;
    prev_cxt->video_restart_skip_cnt = 0;
    prev_cxt->video_restart_skip_en = 0;
    prev_cxt->cap_zsl_restart_skip_cnt = 0;
    prev_cxt->cap_zsl_restart_skip_en = 0;
    pthread_mutex_unlock(&handle->thread_cxt.prev_mutex);

exit:

    CMR_LOGD("out");
    ATRACE_END();
    return ret;
}

cmr_int prev_cancel_snapshot(struct prev_handle *handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 channel_bits = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!handle->ops.channel_stop || !handle->ops.isp_stop_video) {
        CMR_LOGE("ops is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    channel_bits = 1 << prev_cxt->cap_channel_id;

    CMR_LOGI("channel_bits %d, channel_status %ld", channel_bits,
             prev_cxt->cap_channel_status);

    /*capture done, stop isp and channel*/
    if (PREV_CHN_BUSY == prev_cxt->cap_channel_status) {

        /*stop channel*/
        ret = handle->ops.channel_stop(handle->oem_handle, channel_bits);
        if (ret) {
            CMR_LOGE("channel_stop failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        prev_cxt->cap_channel_status = PREV_CHN_IDLE;

        /*stop isp*/
        if (PREV_ISP_COWORK == prev_cxt->isp_status) {
            ret = handle->ops.isp_stop_video(handle->oem_handle);
            prev_cxt->isp_status = PREV_ISP_IDLE;
            if (ret) {
                CMR_LOGE("Failed to stop ISP video mode, %ld", ret);
            }
        }
    }

    /*post proc*/
    ret = handle->ops.capture_post_proc(handle->oem_handle, camera_id);
    if (ret) {
        CMR_LOGE("post proc failed");
    }

exit:
    return ret;
}

#ifdef CONFIG_CAMERA_NO_DCAM_DATA_PATH
cmr_int prev_alloc_prev_buf(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart, struct buffer_cfg *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_size = 0;
    cmr_u32 frame_num = 0;
    cmr_uint i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_uint reserved_count = 0;
    cmr_u32 aligned_type = 0;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    cmr_u32 reserved_width = 0;
    cmr_u32 reserved_height = 0;
    cmr_u32 reserved_frame_size = 0;
    cmr_u32 reserved_buffer_size = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    reserved_count = prev_get_prev_res_buffer_count(prev_cxt);

    sensor_mode_info = &prev_cxt->sensor_info.mode_info[prev_cxt->prev_mode];
    reserved_width = prev_cxt->actual_prev_size.width;
    reserved_height = prev_cxt->actual_prev_size.height;

    if (prev_cxt->prev_mode && prev_cxt->video_mode &&
        reserved_width < prev_cxt->actual_video_size.width) {
        reserved_width = prev_cxt->actual_video_size.width;
        reserved_height = prev_cxt->actual_video_size.height;
    }

    width = prev_cxt->actual_prev_size.width;
    height = prev_cxt->actual_prev_size.height;

    CMR_LOGI("reserved out w=%d, h=%d\n", reserved_width, reserved_height);

    CMR_LOGI("preview out w=%d, h=%d\n", width, height);

    aligned_type = CAMERA_MEM_NO_ALIGNED;

    /*init preview memory info*/
    buffer_size = width * height;
    reserved_buffer_size = reserved_width * reserved_height;
    if (IMG_DATA_TYPE_YUV420 == prev_cxt->prev_param.preview_fmt ||
        IMG_DATA_TYPE_YVU420 == prev_cxt->prev_param.preview_fmt) {
        prev_cxt->prev_mem_size = (width * height * 3) >> 1;
        reserved_frame_size = (reserved_width * reserved_height * 3) >> 1;
    } else if (IMG_DATA_TYPE_YUV422 == prev_cxt->prev_param.preview_fmt) {
        prev_cxt->prev_mem_size = (width * height) << 1;
        reserved_frame_size = (reserved_width * reserved_height) << 1;
    } else if (IMG_DATA_TYPE_YV12 == prev_cxt->prev_param.preview_fmt) {
        if (IMG_ANGLE_90 == prev_cxt->prev_param.prev_rot ||
            IMG_ANGLE_270 == prev_cxt->prev_param.prev_rot) {
            prev_cxt->prev_mem_size =
                (height + camera_get_aligned_size(aligned_type, height / 2)) *
                width;
            reserved_frame_size =
                (reserved_height +
                 camera_get_aligned_size(aligned_type, reserved_height / 2)) *
                reserved_width;
        } else {
            prev_cxt->prev_mem_size =
                (width + camera_get_aligned_size(aligned_type, width / 2)) *
                height;
            reserved_frame_size =
                (reserved_width +
                 camera_get_aligned_size(aligned_type, reserved_width / 2)) *
                reserved_height;
        }
        prev_cxt->prev_param.preview_fmt = IMG_DATA_TYPE_YUV420;
    } else {
        CMR_LOGE("unsupprot fmt %ld", prev_cxt->prev_param.preview_fmt);
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt->prev_mem_num = PREV_FRM_ALLOC_CNT;
    if (prev_cxt->prev_param.prev_rot) {
        CMR_LOGI("need increase buf for rotation");
        prev_cxt->prev_mem_num += PREV_ROT_FRM_ALLOC_CNT;
    }

    /*alloc preview buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        prev_cxt->prev_mem_valid_num = 0;
        mem_ops->alloc_mem(
            CAMERA_PREVIEW, handle->oem_handle,
            (cmr_u32 *)&prev_cxt->prev_mem_size,
            (cmr_u32 *)&prev_cxt->prev_mem_num, prev_cxt->prev_phys_addr_array,
            prev_cxt->prev_virt_addr_array, prev_cxt->prev_fd_array);
        /*check memory valid*/
        CMR_LOGI("prev_mem_size 0x%lx, mem_num %ld", prev_cxt->prev_mem_size,
                 prev_cxt->prev_mem_num);
#ifdef CONFIG_Y_IMG_TO_ISP
        prev_cxt->prev_mem_y_size = prev_cxt->prev_mem_size * 2 / 3;
        prev_cxt->prev_mem_y_num = 2;
        mem_ops->alloc_mem(CAMERA_ISP_PREVIEW_Y, handle->oem_handle,
                           (cmr_u32 *)&prev_cxt->prev_mem_y_size,
                           (cmr_u32 *)&prev_cxt->prev_mem_y_num,
                           prev_cxt->prev_phys_y_addr_array,
                           prev_cxt->prev_virt_y_addr_array,
                           prev_cxt->prev_mfd_y_array);
        CMR_LOGI("phys 0x%lx, virt %lx", prev_cxt->prev_phys_y_addr_array[0],
                 prev_cxt->prev_virt_y_addr_array[0]);
        CMR_LOGI("phys 0x%lx, virt %lx", prev_cxt->prev_phys_y_addr_array[1],
                 prev_cxt->prev_virt_y_addr_array[1]);
#endif
#ifdef YUV_TO_ISP
        prev_cxt->prev_mem_yuv_size = prev_cxt->prev_mem_size;
        prev_cxt->prev_mem_yuv_num = 1;
        mem_ops->alloc_mem(CAMERA_ISP_PREVIEW_YUV, handle->oem_handle,
                           (cmr_u32 *)&prev_cxt->prev_mem_yuv_size,
                           (cmr_u32 *)&prev_cxt->prev_mem_yuv_num,
                           &prev_cxt->prev_phys_yuv_addr,
                           &prev_cxt->prev_virt_yuv_addr,
                           &prev_cxt->prev_mfd_yuv);
#endif

        for (i = 0; i < prev_cxt->prev_mem_num; i++) {
            CMR_LOGI("%ld, virt_addr 0x%lx, fd 0x%x", i,
                     prev_cxt->prev_virt_addr_array[i],
                     prev_cxt->prev_fd_array[i]);

            if ((0 == prev_cxt->prev_virt_addr_array[i]) ||
                (0 == prev_cxt->prev_fd_array[i])) {
                if (i >= PREV_FRM_ALLOC_CNT) {
                    CMR_LOGE("memory is invalid");
                    return CMR_CAMERA_NO_MEM;
                }
            } else {
                if (i < PREV_FRM_ALLOC_CNT) {
                    prev_cxt->prev_mem_valid_num++;
                }
            }
        }

        mem_ops->alloc_mem(
            CAMERA_PREVIEW_RESERVED, handle->oem_handle,
            (cmr_u32 *)&reserved_frame_size, (cmr_u32 *)&reserved_count,
            prev_cxt->prev_reserved_phys_addr,
            prev_cxt->prev_reserved_virt_addr, prev_cxt->prev_reserved_fd);
        CMR_LOGI("reserved_frame_size = %d\n", reserved_frame_size);

        CMR_LOGI("reserved, virt_addr 0x%lx, fd 0x%x",
                 prev_cxt->prev_reserved_virt_addr[0],
                 prev_cxt->prev_reserved_fd[0]);
    }

    frame_size = prev_cxt->prev_mem_size;
    prev_num = prev_cxt->prev_mem_num;
    if (prev_cxt->prev_param.prev_rot) {
        prev_num = prev_cxt->prev_mem_num - PREV_ROT_FRM_ALLOC_CNT;
    }

    /*arrange the buffer*/
    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_PREV_ID_BASE;
    buffer->count = prev_cxt->prev_mem_valid_num;
    buffer->length = frame_size;
    buffer->flag = BUF_FLAG_INIT;

    for (i = 0; i < (cmr_uint)prev_cxt->prev_mem_valid_num; i++) {
        prev_cxt->prev_frm[i].buf_size = frame_size;
        prev_cxt->prev_frm[i].addr_vir.addr_y =
            prev_cxt->prev_virt_addr_array[i];
        prev_cxt->prev_frm[i].addr_vir.addr_u =
            prev_cxt->prev_frm[i].addr_vir.addr_y + buffer_size;
        prev_cxt->prev_frm[i].addr_phy.addr_y =
            prev_cxt->prev_phys_addr_array[i];
        prev_cxt->prev_frm[i].addr_phy.addr_u =
            prev_cxt->prev_frm[i].addr_phy.addr_y + buffer_size;
        prev_cxt->prev_frm[i].fd = prev_cxt->prev_fd_array[i];
        prev_cxt->prev_frm[i].fmt = prev_cxt->prev_param.preview_fmt;
        prev_cxt->prev_frm[i].size.width = prev_cxt->actual_prev_size.width;
        prev_cxt->prev_frm[i].size.height = prev_cxt->actual_prev_size.height;
        prev_cxt->prev_frm[i].addr_phy.addr_v = 0;

        buffer->addr[i].addr_y = prev_cxt->prev_frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->prev_frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->prev_frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->prev_frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->prev_frm[i].fd;
    }

    for (i = 0; i < reserved_count; i++) {
        prev_cxt->prev_reserved_frm[i].buf_size = reserved_frame_size;
        prev_cxt->prev_reserved_frm[i].addr_vir.addr_y =
            prev_cxt->prev_reserved_virt_addr[i];
        prev_cxt->prev_reserved_frm[i].addr_vir.addr_u =
            prev_cxt->prev_reserved_frm[i].addr_vir.addr_y +
            reserved_buffer_size;
        prev_cxt->prev_reserved_frm[i].addr_phy.addr_y =
            prev_cxt->prev_reserved_phys_addr[i];
        prev_cxt->prev_reserved_frm[i].addr_phy.addr_u =
            prev_cxt->prev_reserved_frm[i].addr_phy.addr_y +
            reserved_buffer_size;
        prev_cxt->prev_reserved_frm[i].fd = prev_cxt->prev_reserved_fd[i];

        prev_cxt->prev_reserved_frm[i].fmt = prev_cxt->prev_param.preview_fmt;
        prev_cxt->prev_reserved_frm[i].size.width = reserved_width;
        prev_cxt->prev_reserved_frm[i].size.height = reserved_height;
    }

    if (prev_cxt->prev_param.prev_rot) {
        for (i = 0; i < PREV_ROT_FRM_ALLOC_CNT; i++) {
            prev_cxt->prev_rot_frm[i].buf_size = frame_size;
            prev_cxt->prev_rot_frm[i].addr_vir.addr_y =
                prev_cxt->prev_virt_addr_array[prev_num + i];
            prev_cxt->prev_rot_frm[i].addr_vir.addr_u =
                prev_cxt->prev_rot_frm[i].addr_vir.addr_y + buffer_size;
            prev_cxt->prev_rot_frm[i].addr_phy.addr_y =
                prev_cxt->prev_phys_addr_array[prev_num + i];
            prev_cxt->prev_rot_frm[i].addr_phy.addr_u =
                prev_cxt->prev_rot_frm[i].addr_phy.addr_y + buffer_size;
            prev_cxt->prev_rot_frm[i].addr_phy.addr_v = 0;
            prev_cxt->prev_rot_frm[i].fd =
                prev_cxt->prev_fd_array[prev_num + i];
            prev_cxt->prev_rot_frm[i].fmt = prev_cxt->prev_param.preview_fmt;
            prev_cxt->prev_rot_frm[i].size.width =
                prev_cxt->actual_prev_size.width;
            prev_cxt->prev_rot_frm[i].size.height =
                prev_cxt->actual_prev_size.height;
        }
    }
    CMR_LOGI("out %ld", ret);
    ATRACE_END();
    return ret;
}
#else
cmr_int prev_alloc_prev_buf(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart, struct buffer_cfg *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_size = 0;
    cmr_u32 frame_num = 0;
    cmr_uint i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_uint reserved_count = 0;
    cmr_u32 aligned_type = 0;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;
    width = prev_cxt->actual_prev_size.width;
    height = prev_cxt->actual_prev_size.height;
    aligned_type = CAMERA_MEM_NO_ALIGNED;

    reserved_count = prev_get_prev_res_buffer_count(prev_cxt);

    /*init preview memory info*/
    buffer_size = width * height;
    if (IMG_DATA_TYPE_YUV420 == prev_cxt->prev_param.preview_fmt ||
        IMG_DATA_TYPE_YVU420 == prev_cxt->prev_param.preview_fmt) {
        prev_cxt->prev_mem_size = (width * height * 3) >> 1;
    } else if (IMG_DATA_TYPE_YUV422 == prev_cxt->prev_param.preview_fmt) {
        prev_cxt->prev_mem_size = (width * height) << 1;
    } else if (IMG_DATA_TYPE_YV12 == prev_cxt->prev_param.preview_fmt) {
        if (IMG_ANGLE_90 == prev_cxt->prev_param.prev_rot ||
            IMG_ANGLE_270 == prev_cxt->prev_param.prev_rot) {
            prev_cxt->prev_mem_size =
                (height + camera_get_aligned_size(aligned_type, height / 2)) *
                width;
        } else {
            prev_cxt->prev_mem_size =
                (width + camera_get_aligned_size(aligned_type, width / 2)) *
                height;
        }
        prev_cxt->prev_param.preview_fmt = IMG_DATA_TYPE_YUV420;
    } else {
        CMR_LOGE("unsupprot fmt %ld", prev_cxt->prev_param.preview_fmt);
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt->prev_mem_num = PREV_FRM_ALLOC_CNT;
    if (prev_cxt->prev_param.prev_rot) {
        CMR_LOGI("need increase buf for rotation");
        prev_cxt->prev_mem_num += PREV_ROT_FRM_ALLOC_CNT;
    }

    /*alloc preview buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        prev_cxt->prev_mem_valid_num = 0;
        mem_ops->alloc_mem(
            CAMERA_PREVIEW, handle->oem_handle,
            (cmr_u32 *)&prev_cxt->prev_mem_size,
            (cmr_u32 *)&prev_cxt->prev_mem_num, prev_cxt->prev_phys_addr_array,
            prev_cxt->prev_virt_addr_array, prev_cxt->prev_fd_array);
        /*check memory valid*/
        CMR_LOGI("prev_mem_size 0x%lx, mem_num %ld", prev_cxt->prev_mem_size,
                 prev_cxt->prev_mem_num);
#ifdef CONFIG_Y_IMG_TO_ISP
        prev_cxt->prev_mem_y_size = prev_cxt->prev_mem_size * 2 / 3;
        prev_cxt->prev_mem_y_num = 2;
        mem_ops->alloc_mem(CAMERA_ISP_PREVIEW_Y, handle->oem_handle,
                           (cmr_u32 *)&prev_cxt->prev_mem_y_size,
                           (cmr_u32 *)&prev_cxt->prev_mem_y_num,
                           prev_cxt->prev_phys_y_addr_array,
                           prev_cxt->prev_virt_y_addr_array,
                           prev_cxt->prev_mfd_y_array);
        CMR_LOGI("phys 0x%lx, virt %lx", prev_cxt->prev_phys_y_addr_array[0],
                 prev_cxt->prev_virt_y_addr_array[0]);
        CMR_LOGI("phys 0x%lx, virt %lx", prev_cxt->prev_phys_y_addr_array[1],
                 prev_cxt->prev_virt_y_addr_array[1]);
#endif
#ifdef YUV_TO_ISP
        prev_cxt->prev_mem_yuv_size = prev_cxt->prev_mem_size;
        prev_cxt->prev_mem_yuv_num = 1;
        mem_ops->alloc_mem(CAMERA_ISP_PREVIEW_YUV, handle->oem_handle,
                           (cmr_u32 *)&prev_cxt->prev_mem_yuv_size,
                           (cmr_u32 *)&prev_cxt->prev_mem_yuv_num,
                           &prev_cxt->prev_phys_yuv_addr,
                           &prev_cxt->prev_virt_yuv_addr,
                           &prev_cxt->prev_mfd_yuv);
#endif

        for (i = 0; i < prev_cxt->prev_mem_num; i++) {
            CMR_LOGI("%ld, virt_addr 0x%lx, fd 0x%x", i,
                     prev_cxt->prev_virt_addr_array[i],
                     prev_cxt->prev_fd_array[i]);

            if ((0 == prev_cxt->prev_virt_addr_array[i]) ||
                (0 == prev_cxt->prev_fd_array[i])) {
                if (i >= PREV_FRM_ALLOC_CNT) {
                    CMR_LOGE("memory is invalid");
                    return CMR_CAMERA_NO_MEM;
                }
            } else {
                if (i < PREV_FRM_ALLOC_CNT) {
                    prev_cxt->prev_mem_valid_num++;
                }
            }
        }

        mem_ops->alloc_mem(
            CAMERA_PREVIEW_RESERVED, handle->oem_handle,
            (cmr_u32 *)&prev_cxt->prev_mem_size, (cmr_u32 *)&reserved_count,
            prev_cxt->prev_reserved_phys_addr,
            prev_cxt->prev_reserved_virt_addr, prev_cxt->prev_reserved_fd);
        CMR_LOGI("reserved, virt_addr 0x%lx, fd 0x%x",
                 prev_cxt->prev_reserved_virt_addr[0],
                 prev_cxt->prev_reserved_fd[0]);
    }

    frame_size = prev_cxt->prev_mem_size;
    prev_num = prev_cxt->prev_mem_num;
    if (prev_cxt->prev_param.prev_rot) {
        prev_num = prev_cxt->prev_mem_num - PREV_ROT_FRM_ALLOC_CNT;
    }

    /*arrange the buffer*/
    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_PREV_ID_BASE;
    buffer->count = prev_cxt->prev_mem_valid_num;
    buffer->length = frame_size;
    buffer->flag = BUF_FLAG_INIT;

    for (i = 0; i < (cmr_uint)prev_cxt->prev_mem_valid_num; i++) {
        prev_cxt->prev_frm[i].buf_size = frame_size;
        prev_cxt->prev_frm[i].addr_vir.addr_y =
            prev_cxt->prev_virt_addr_array[i];
        prev_cxt->prev_frm[i].addr_vir.addr_u =
            prev_cxt->prev_frm[i].addr_vir.addr_y + buffer_size;
        prev_cxt->prev_frm[i].addr_phy.addr_y =
            prev_cxt->prev_phys_addr_array[i];
        prev_cxt->prev_frm[i].addr_phy.addr_u =
            prev_cxt->prev_frm[i].addr_phy.addr_y + buffer_size;
        prev_cxt->prev_frm[i].fd = prev_cxt->prev_fd_array[i];
        prev_cxt->prev_frm[i].fmt = prev_cxt->prev_param.preview_fmt;
        prev_cxt->prev_frm[i].size.width = prev_cxt->actual_prev_size.width;
        prev_cxt->prev_frm[i].size.height = prev_cxt->actual_prev_size.height;
        prev_cxt->prev_frm[i].addr_phy.addr_v = 0;

        buffer->addr[i].addr_y = prev_cxt->prev_frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->prev_frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->prev_frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->prev_frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->prev_frm[i].fd;
    }
    for (i = 0; i < reserved_count; i++) {
        prev_cxt->prev_reserved_frm[i].buf_size = frame_size;
        prev_cxt->prev_reserved_frm[i].addr_vir.addr_y =
            prev_cxt->prev_reserved_virt_addr[i];
        prev_cxt->prev_reserved_frm[i].addr_vir.addr_u =
            prev_cxt->prev_reserved_frm[i].addr_vir.addr_y + buffer_size;
        prev_cxt->prev_reserved_frm[i].addr_phy.addr_y =
            prev_cxt->prev_reserved_phys_addr[i];
        prev_cxt->prev_reserved_frm[i].addr_phy.addr_u =
            prev_cxt->prev_reserved_frm[i].addr_phy.addr_y + buffer_size;
        prev_cxt->prev_reserved_frm[i].fd = prev_cxt->prev_reserved_fd[i];

        prev_cxt->prev_reserved_frm[i].fmt = prev_cxt->prev_param.preview_fmt;
        prev_cxt->prev_reserved_frm[i].size.width =
            prev_cxt->actual_prev_size.width;
        prev_cxt->prev_reserved_frm[i].size.height =
            prev_cxt->actual_prev_size.height;
        prev_cxt->prev_reserved_frm[i].addr_phy.addr_v = 0;
    }

    if (prev_cxt->prev_param.prev_rot) {
        for (i = 0; i < PREV_ROT_FRM_ALLOC_CNT; i++) {
            prev_cxt->prev_rot_frm[i].buf_size = frame_size;
            prev_cxt->prev_rot_frm[i].addr_vir.addr_y =
                prev_cxt->prev_virt_addr_array[prev_num + i];
            prev_cxt->prev_rot_frm[i].addr_vir.addr_u =
                prev_cxt->prev_rot_frm[i].addr_vir.addr_y + buffer_size;
            prev_cxt->prev_rot_frm[i].addr_phy.addr_y =
                prev_cxt->prev_phys_addr_array[prev_num + i];
            prev_cxt->prev_rot_frm[i].addr_phy.addr_u =
                prev_cxt->prev_rot_frm[i].addr_phy.addr_y + buffer_size;
            prev_cxt->prev_rot_frm[i].addr_phy.addr_v = 0;
            prev_cxt->prev_rot_frm[i].fd =
                prev_cxt->prev_fd_array[prev_num + i];
            prev_cxt->prev_rot_frm[i].fmt = prev_cxt->prev_param.preview_fmt;
            prev_cxt->prev_rot_frm[i].size.width =
                prev_cxt->actual_prev_size.width;
            prev_cxt->prev_rot_frm[i].size.height =
                prev_cxt->actual_prev_size.height;
        }
    }
    CMR_LOGI("out %ld", ret);
    ATRACE_END();
    return ret;
}
#endif

cmr_int prev_free_prev_buf(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;
    cmr_uint reserved_count = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;
    reserved_count = prev_get_prev_res_buffer_count(prev_cxt);

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        mem_ops->free_mem(CAMERA_PREVIEW, handle->oem_handle,
                          prev_cxt->prev_phys_addr_array,
                          prev_cxt->prev_virt_addr_array,
                          prev_cxt->prev_fd_array, prev_cxt->prev_mem_num);
        cmr_bzero(prev_cxt->prev_phys_addr_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->prev_virt_addr_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->prev_fd_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_s32));

#ifdef CONFIG_Y_IMG_TO_ISP
        mem_ops->free_mem(CAMERA_ISP_PREVIEW_Y, handle->oem_handle,
                          prev_cxt->prev_phys_y_addr_array,
                          prev_cxt->prev_virt_y_addr_array,
                          prev_cxt->prev_mfd_y_array, prev_cxt->prev_mem_y_num);
        cmr_bzero(prev_cxt->prev_phys_y_addr_array, 2 * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->prev_virt_y_addr_array, 2 * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->prev_mfd_y_array, 2 * sizeof(cmr_s32));
#endif

#ifdef YUV_TO_ISP
        mem_ops->free_mem(CAMERA_ISP_PREVIEW_YUV, handle->oem_handle,
                          (cmr_uint *)prev_cxt->prev_phys_yuv_addr,
                          (cmr_uint *)prev_cxt->prev_virt_yuv_addr,
                          &prev_cxt->prev_mfd_yuv, prev_cxt->prev_mem_yuv_num);
        prev_cxt->prev_phys_yuv_addr = 0;
        prev_cxt->prev_virt_yuv_addr = 0;
        prev_cxt->prev_mfd_yuv = 0;
#endif

        mem_ops->free_mem(CAMERA_PREVIEW_RESERVED, handle->oem_handle,
                          prev_cxt->prev_reserved_phys_addr,
                          prev_cxt->prev_reserved_virt_addr,
                          prev_cxt->prev_reserved_fd, (cmr_u32)reserved_count);

        cmr_bzero(prev_cxt->prev_reserved_phys_addr,
                  sizeof(prev_cxt->prev_reserved_phys_addr));
        cmr_bzero(prev_cxt->prev_reserved_virt_addr,
                  sizeof(prev_cxt->prev_reserved_virt_addr));
        cmr_bzero(prev_cxt->prev_reserved_fd,
                  sizeof(prev_cxt->prev_reserved_fd));
    }

    CMR_LOGI("out");
    return ret;
}

cmr_int prev_alloc_video_buf(struct prev_handle *handle, cmr_u32 camera_id,
                             cmr_u32 is_restart, struct buffer_cfg *buffer) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_size = 0;
    cmr_u32 frame_num = 0;
    cmr_uint i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_uint reserved_count = VIDEO_RESERVED_FRM_CNT;
    cmr_u32 aligned_type = 0;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;
    width = prev_cxt->actual_video_size.width;
    height = prev_cxt->actual_video_size.height;
    aligned_type = CAMERA_MEM_NO_ALIGNED;

    /*init video memory info*/
    buffer_size = width * height;
    if (IMG_DATA_TYPE_YUV420 == prev_cxt->prev_param.preview_fmt ||
        IMG_DATA_TYPE_YVU420 == prev_cxt->prev_param.preview_fmt) {
        prev_cxt->video_mem_size = (width * height * 3) >> 1;
    } else if (IMG_DATA_TYPE_YUV422 == prev_cxt->prev_param.preview_fmt) {
        prev_cxt->video_mem_size = (width * height) << 1;
    } else if (IMG_DATA_TYPE_YV12 == prev_cxt->prev_param.preview_fmt) {
        if (IMG_ANGLE_90 == prev_cxt->prev_param.prev_rot ||
            IMG_ANGLE_270 == prev_cxt->prev_param.prev_rot) {
            prev_cxt->video_mem_size =
                (height + camera_get_aligned_size(aligned_type, height / 2)) *
                width;
        } else {
            prev_cxt->video_mem_size =
                (width + camera_get_aligned_size(aligned_type, width / 2)) *
                height;
        }
        prev_cxt->prev_param.preview_fmt = IMG_DATA_TYPE_YUV420;
    } else {
        CMR_LOGE("unsupprot fmt %ld", prev_cxt->prev_param.preview_fmt);
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt->video_mem_num = PREV_FRM_ALLOC_CNT;
    if (prev_cxt->prev_param.prev_rot) {
        CMR_LOGI("need increase buf for rotation");
        prev_cxt->video_mem_num += PREV_ROT_FRM_ALLOC_CNT;
    }

    /*alloc preview buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }
    if (!is_restart) {
        prev_cxt->video_mem_valid_num = 0;
        for (i = 0; i < prev_cxt->video_mem_num; i++) {
            prev_cxt->video_phys_addr_array[i] = 0;
            prev_cxt->video_virt_addr_array[i] = 0;
            prev_cxt->video_fd_array[i] = 0;
        }
        mem_ops->alloc_mem(CAMERA_VIDEO, handle->oem_handle,
                           (cmr_u32 *)&prev_cxt->video_mem_size,
                           (cmr_u32 *)&prev_cxt->video_mem_num,
                           prev_cxt->video_phys_addr_array,
                           prev_cxt->video_virt_addr_array,
                           prev_cxt->video_fd_array);
        /*check memory valid*/
        CMR_LOGI("video_mem_size 0x%lx, mem_num %ld", prev_cxt->video_mem_size,
                 prev_cxt->video_mem_num);
        for (i = 0; i < prev_cxt->video_mem_num; i++) {
            CMR_LOGV("%ld, virt_addr 0x%lx, fd 0x%x", i,
                     prev_cxt->video_virt_addr_array[i],
                     prev_cxt->video_fd_array[i]);

            if ((0 == prev_cxt->video_virt_addr_array[i]) ||
                (0 == prev_cxt->video_fd_array[i])) {
                if (i >= PREV_FRM_ALLOC_CNT) {
                    CMR_LOGE("memory is invalid");
                    return CMR_CAMERA_NO_MEM;
                }
            } else {
                if (i < PREV_FRM_ALLOC_CNT) {
                    prev_cxt->video_mem_valid_num++;
                }
            }
        }

        mem_ops->alloc_mem(
            CAMERA_VIDEO_RESERVED, handle->oem_handle,
            (cmr_u32 *)&prev_cxt->video_mem_size, (cmr_u32 *)&reserved_count,
            prev_cxt->video_reserved_phys_addr,
            prev_cxt->video_reserved_virt_addr, prev_cxt->video_reserved_fd);
    }

    frame_size = prev_cxt->video_mem_size;
    prev_num = prev_cxt->video_mem_num;
    if (prev_cxt->prev_param.prev_rot) {
        prev_num = prev_cxt->video_mem_num - PREV_ROT_FRM_ALLOC_CNT;
    }

    /*arrange the buffer*/

    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_VIDEO_ID_BASE;
    buffer->count = prev_cxt->video_mem_valid_num;
    buffer->length = frame_size;
    buffer->flag = BUF_FLAG_INIT;

    for (i = 0; i < prev_cxt->video_mem_valid_num; i++) {
        prev_cxt->video_frm[i].buf_size = frame_size;
        prev_cxt->video_frm[i].addr_vir.addr_y =
            prev_cxt->video_virt_addr_array[i];
        prev_cxt->video_frm[i].addr_vir.addr_u =
            prev_cxt->video_frm[i].addr_vir.addr_y + buffer_size;
        prev_cxt->video_frm[i].addr_phy.addr_y =
            prev_cxt->video_phys_addr_array[i];
        prev_cxt->video_frm[i].addr_phy.addr_u =
            prev_cxt->video_frm[i].addr_phy.addr_y + buffer_size;
        prev_cxt->video_frm[i].fd = prev_cxt->video_fd_array[i];
        prev_cxt->video_frm[i].fmt = prev_cxt->prev_param.preview_fmt;
        prev_cxt->video_frm[i].size.width = prev_cxt->actual_video_size.width;
        prev_cxt->video_frm[i].size.height = prev_cxt->actual_video_size.height;
        prev_cxt->video_frm[i].addr_phy.addr_v = 0;

        buffer->addr[i].addr_y = prev_cxt->video_frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->video_frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->video_frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->video_frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->video_frm[i].fd;
    }

    for (i = 0; i < VIDEO_RESERVED_FRM_CNT; i++) {
        prev_cxt->video_reserved_frm[i].buf_size = frame_size;
        prev_cxt->video_reserved_frm[i].addr_vir.addr_y =
            prev_cxt->video_reserved_virt_addr[i];
        prev_cxt->video_reserved_frm[i].addr_vir.addr_u =
            prev_cxt->video_reserved_frm[i].addr_vir.addr_y + buffer_size;
        prev_cxt->video_reserved_frm[i].addr_phy.addr_y =
            prev_cxt->video_reserved_phys_addr[i];
        prev_cxt->video_reserved_frm[i].addr_phy.addr_u =
            prev_cxt->video_reserved_frm[i].addr_phy.addr_y + buffer_size;
        prev_cxt->video_reserved_frm[i].fd = prev_cxt->video_reserved_fd[i];
        prev_cxt->video_reserved_frm[i].fmt = prev_cxt->prev_param.preview_fmt;
        prev_cxt->video_reserved_frm[i].size.width =
            prev_cxt->actual_video_size.width;
        prev_cxt->video_reserved_frm[i].size.height =
            prev_cxt->actual_video_size.height;
        prev_cxt->video_reserved_frm[i].addr_phy.addr_v = 0;
    }

    if (prev_cxt->prev_param.prev_rot) {
        for (i = 0; i < PREV_ROT_FRM_ALLOC_CNT; i++) {
            prev_cxt->video_rot_frm[i].buf_size = frame_size;
            prev_cxt->video_rot_frm[i].addr_vir.addr_y =
                prev_cxt->video_virt_addr_array[prev_num + i];
            prev_cxt->video_rot_frm[i].addr_vir.addr_u =
                prev_cxt->video_rot_frm[i].addr_vir.addr_y + buffer_size;
            prev_cxt->video_rot_frm[i].addr_phy.addr_y =
                prev_cxt->video_phys_addr_array[prev_num + i];
            prev_cxt->video_rot_frm[i].addr_phy.addr_u =
                prev_cxt->video_rot_frm[i].addr_phy.addr_y + buffer_size;
            prev_cxt->video_rot_frm[i].addr_phy.addr_v = 0;
            prev_cxt->video_rot_frm[i].fd =
                prev_cxt->video_fd_array[prev_num + i];
            prev_cxt->video_rot_frm[i].fmt = prev_cxt->prev_param.preview_fmt;
            prev_cxt->video_rot_frm[i].size.width =
                prev_cxt->actual_video_size.width;
            prev_cxt->video_rot_frm[i].size.height =
                prev_cxt->actual_video_size.height;
        }
    }
    CMR_LOGI("out %ld", ret);
    return ret;
}

cmr_int prev_free_video_buf(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;
    cmr_uint reserved_count = VIDEO_RESERVED_FRM_CNT;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        mem_ops->free_mem(CAMERA_VIDEO, handle->oem_handle,
                          prev_cxt->video_phys_addr_array,
                          prev_cxt->video_virt_addr_array,
                          prev_cxt->video_fd_array, prev_cxt->video_mem_num);

        cmr_bzero(prev_cxt->video_phys_addr_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->video_virt_addr_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->video_fd_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_s32));

        mem_ops->free_mem(CAMERA_VIDEO_RESERVED, handle->oem_handle,
                          prev_cxt->video_reserved_phys_addr,
                          prev_cxt->video_reserved_virt_addr,
                          prev_cxt->video_reserved_fd, (cmr_u32)reserved_count);
        cmr_bzero(prev_cxt->video_reserved_phys_addr,
                  sizeof(prev_cxt->video_reserved_phys_addr));
        cmr_bzero(prev_cxt->video_reserved_virt_addr,
                  sizeof(prev_cxt->video_reserved_virt_addr));
        cmr_bzero(prev_cxt->video_reserved_fd,
                  sizeof(prev_cxt->video_reserved_fd));
    }

    CMR_LOGI("out");
    return ret;
}

cmr_int prev_alloc_cap_buf(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart, struct buffer_cfg *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 total_mem_size = 0;
    cmr_u32 i = 0;
    cmr_u32 mem_size, buffer_size, frame_size, y_addr, u_addr = 0;
    cmr_s32 fd = 0;
    cmr_u32 y_addr_vir, u_addr_vir = 0;
    cmr_u32 no_scaling = 0;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;
    struct img_size *cap_max_size = NULL;
    struct sensor_mode_info *sensor_mode = NULL;
    struct cmr_cap_2_frm cap_2_mems;
    struct img_frm *cur_img_frm = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 sum = 0;
    cmr_u32 is_normal_cap = 0;
    cmr_int zoom_post_proc = 0;
    cmr_u32 channel_size = 0;
    cmr_u32 channel_buffer_size = 0;
    cmr_u32 cap_sum = 0;
    int32_t buffer_id = 0;
    cmr_int is_need_scaling = 1;
    cmr_u32 hdr_cap_sum = HDR_CAP_NUM - 1;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    sum = 1;
    cap_sum = CMR_CAPTURE_MEM_SUM;
    CMR_LOGI("camera_id %d, is_restart %d", camera_id, is_restart);
    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;
    cap_max_size = &prev_cxt->max_size;
    sensor_mode = &prev_cxt->sensor_info.mode_info[prev_cxt->cap_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);
    if (ZOOM_POST_PROCESS == zoom_post_proc ||
        ZOOM_POST_PROCESS_WITH_TRIM == zoom_post_proc) {
        channel_size = prev_cxt->max_size.width * prev_cxt->max_size.height;
    } else {
        channel_size =
            prev_cxt->actual_pic_size.width * prev_cxt->actual_pic_size.height;
    }
    channel_buffer_size = channel_size * 3 / 2;

    if (!prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb) {
        is_normal_cap = 1;
    } else {
        is_normal_cap = 0;
    }

    if (prev_cxt->cap_org_size.width * prev_cxt->cap_org_size.height >
        cap_max_size->width * cap_max_size->height) {
        cap_max_size = &prev_cxt->cap_org_size;
    }

    is_need_scaling = prev_is_need_scaling(handle, camera_id);
    /*caculate memory size for capture*/

    cmr_u16 width, height;
    camera_pre_capture_sensor_size_get(camera_id, &width, &height);
    CMR_LOGI("get capture size width = %d, height = %d", width, height);
    buffer_id = camera_pre_capture_buf_id(camera_id, width, height);
    ret = camera_pre_capture_buf_size(camera_id, buffer_id, &total_mem_size,
                                      &sum);
    if (ret) {
        CMR_LOGE("get mem size err");
        return CMR_CAMERA_FAIL;
    }

    /*alloc capture buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        mem_ops->alloc_mem(CAMERA_SNAPSHOT, handle->oem_handle, &total_mem_size,
                           &sum, prev_cxt->cap_phys_addr_array,
                           prev_cxt->cap_virt_addr_array,
                           prev_cxt->cap_fd_array);
#if 0 // for coverity 181595
        for (i = 1; i < CMR_CAPTURE_MEM_SUM; i++) {
            prev_cxt->cap_phys_addr_array[i] = prev_cxt->cap_phys_addr_array[0];
            prev_cxt->cap_virt_addr_array[i] = prev_cxt->cap_virt_addr_array[0];
            prev_cxt->cap_fd_array[i] = prev_cxt->cap_fd_array[0];
        }
#endif
        CMR_LOGI("virt_addr 0x%lx, fd 0x%x", prev_cxt->cap_virt_addr_array[0],
                 prev_cxt->cap_fd_array[0]);

        /*check memory valid*/
        CMR_LOGI("cap mem size 0x%x, mem_num %d", total_mem_size,
                 CMR_CAPTURE_MEM_SUM);
        for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
            if ((0 == prev_cxt->cap_virt_addr_array[i]) ||
                (0 == prev_cxt->cap_fd_array[i])) {
                CMR_LOGE("memory is invalid");
                return CMR_CAMERA_NO_MEM;
            }
        }

        if ((FRAME_HDR_PROC == prev_cxt->prev_param.frame_ctrl) &&
            (IMG_DATA_TYPE_YUV420 == prev_cxt->cap_org_fmt ||
             IMG_DATA_TYPE_YVU420 == prev_cxt->cap_org_fmt) &&
            is_normal_cap) {
            mem_ops->alloc_mem(CAMERA_SNAPSHOT_PATH, handle->oem_handle,
                               &channel_buffer_size, &hdr_cap_sum,
                               prev_cxt->cap_hdr_phys_addr_path_array,
                               prev_cxt->cap_hdr_virt_addr_path_array,
                               prev_cxt->cap_hdr_fd_path_array);

            /*check memory valid*/
            CMR_LOGI("hdr CAMERA_SNAPSHOT_PATH mem size 0x%x, mem_num %d",
                     channel_buffer_size, hdr_cap_sum);
            for (i = 0; i < hdr_cap_sum; i++) {
                CMR_LOGI("%d, virt_addr 0x%lx, fd 0x%x", i,
                         prev_cxt->cap_hdr_virt_addr_path_array[i],
                         prev_cxt->cap_hdr_fd_path_array[i]);
                if ((0 == prev_cxt->cap_hdr_virt_addr_path_array[i]) ||
                    (0 == prev_cxt->cap_hdr_fd_path_array[i])) {
                    CMR_LOGE("CAMERA_SNAPSHOT_PATH memory is invalid");
                    return CMR_CAMERA_NO_MEM;
                }
            }
        }

#ifdef CONFIG_MULTI_CAP_MEM
        if ((IMG_DATA_TYPE_YUV420 == prev_cxt->cap_org_fmt ||
             IMG_DATA_TYPE_YVU420 == prev_cxt->cap_org_fmt) &&
            is_normal_cap) {
            mem_ops->alloc_mem(CAMERA_SNAPSHOT_PATH, handle->oem_handle,
                               &channel_buffer_size, &cap_sum,
                               prev_cxt->cap_phys_addr_path_array,
                               prev_cxt->cap_virt_addr_path_array,
                               prev_cxt->cap_fd_path_array);

            /*check memory valid*/
            CMR_LOGI("CAMERA_SNAPSHOT_PATH mem size 0x%x, mem_num %d",
                     channel_buffer_size, cap_sum);
            for (i = 0; i < cap_sum; i++) {
                CMR_LOGI("%d, virt_addr 0x%lx, fd 0x%x", i,
                         prev_cxt->cap_virt_addr_path_array[i],
                         prev_cxt->cap_fd_path_array[i]);
                if ((0 == prev_cxt->cap_virt_addr_path_array[i]) ||
                    (0 == prev_cxt->cap_fd_path_array[i])) {
                    CMR_LOGE("CAMERA_SNAPSHOT_PATH memory is invalid");
                    return CMR_CAMERA_NO_MEM;
                }
            }
        }
#endif
    }

    /*arrange the buffer*/
    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        cmr_bzero(&cap_2_mems, sizeof(struct cmr_cap_2_frm));
        cap_2_mems.mem_frm.buf_size = total_mem_size;
        cap_2_mems.mem_frm.addr_phy.addr_y = prev_cxt->cap_phys_addr_array[i];
        cap_2_mems.mem_frm.addr_vir.addr_y = prev_cxt->cap_virt_addr_array[i];
        cap_2_mems.mem_frm.fd = prev_cxt->cap_fd_array[i];
        cap_2_mems.type = CAMERA_MEM_NO_ALIGNED;
        cap_2_mems.zoom_post_proc = zoom_post_proc;

        if (is_normal_cap) {
            ret = camera_arrange_capture_buf(
                &cap_2_mems, &prev_cxt->cap_sn_size,
                &prev_cxt->cap_sn_trim_rect, &prev_cxt->max_size,
                prev_cxt->cap_org_fmt, &prev_cxt->cap_org_size,
                &prev_cxt->prev_param.thumb_size, &prev_cxt->cap_mem[i],
                ((IMG_ANGLE_0 != prev_cxt->prev_param.cap_rot) ||
                 prev_cxt->prev_param.is_cfg_rot_cap),
                is_need_scaling, 1);
        } else {
            ret = camera_arrange_capture_buf(
                &cap_2_mems, &prev_cxt->cap_sn_size,
                &prev_cxt->cap_sn_trim_rect, &prev_cxt->max_size,
                prev_cxt->cap_org_fmt, &prev_cxt->cap_org_size,
                &prev_cxt->prev_param.thumb_size, &prev_cxt->cap_mem[i],
                (prev_cxt->prev_param.is_cfg_rot_cap &&
                 (IMG_ANGLE_0 != prev_cxt->prev_param.encode_angle)),
                is_need_scaling, 1);
        }
    }

    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_CAP0_ID_BASE;
    buffer->count = CMR_CAPTURE_MEM_SUM;
    buffer->flag = BUF_FLAG_INIT;
    buffer_size =
        prev_cxt->actual_pic_size.width * prev_cxt->actual_pic_size.height;
    frame_size = buffer_size * 3 / 2;
    CMR_LOGI("prev_cxt->cap_org_fmt: %ld, encode_angle %d",
             prev_cxt->cap_org_fmt, prev_cxt->prev_param.encode_angle);
    for (i = 0; i < buffer->count; i++) {
        if (IMG_DATA_TYPE_RAW == prev_cxt->cap_org_fmt) {
            if ((IMG_ANGLE_0 != prev_cxt->prev_param.cap_rot) ||
                prev_cxt->prev_param.is_cfg_rot_cap) {
                if (prev_cxt->cap_mem[i].cap_yuv_rot.fd ==
                    prev_cxt->cap_mem[i].cap_yuv.fd) {
                    prev_cxt->cap_mem[i].cap_raw.addr_phy.addr_y =
                        prev_cxt->cap_mem[i].target_yuv.addr_phy.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.addr_vir.addr_y =
                        prev_cxt->cap_mem[i].target_yuv.addr_vir.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.fd =
                        prev_cxt->cap_mem[i].target_yuv.fd;
                } else {
                    prev_cxt->cap_mem[i].cap_raw.addr_phy.addr_y =
                        prev_cxt->cap_mem[i].cap_yuv.addr_phy.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.addr_vir.addr_y =
                        prev_cxt->cap_mem[i].cap_yuv.addr_vir.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.fd =
                        prev_cxt->cap_mem[i].cap_yuv.fd;
                }
            } else {
                if (!is_need_scaling) {
                    CMR_LOGD("raw no scale, no rotation");
                    prev_cxt->cap_mem[i].cap_raw.addr_phy.addr_y =
                        prev_cxt->cap_mem[i].cap_yuv.addr_phy.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.addr_vir.addr_y =
                        prev_cxt->cap_mem[i].cap_yuv.addr_vir.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.fd =
                        prev_cxt->cap_mem[i].cap_yuv.fd;
                } else {
                    CMR_LOGD("raw has scale, no rotation");
                    prev_cxt->cap_mem[i].cap_raw.addr_phy.addr_y =
                        prev_cxt->cap_mem[i].target_yuv.addr_phy.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.addr_vir.addr_y =
                        prev_cxt->cap_mem[i].target_yuv.addr_vir.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.fd =
                        prev_cxt->cap_mem[i].target_yuv.fd;
                }
            }

            buffer_size = sensor_mode->trim_width * sensor_mode->trim_height;
            mem_size = prev_cxt->cap_mem[i].cap_raw.buf_size;
            fd = prev_cxt->cap_mem[i].cap_raw.fd;
            y_addr = prev_cxt->cap_mem[i].cap_raw.addr_phy.addr_y;
            u_addr = y_addr;
            y_addr_vir = prev_cxt->cap_mem[i].cap_raw.addr_vir.addr_y;
            u_addr_vir = y_addr_vir;
            frame_size = buffer_size * RAWRGB_BIT_WIDTH / 8;
            cur_img_frm = &prev_cxt->cap_mem[i].cap_raw;
        } else if (IMG_DATA_TYPE_JPEG == prev_cxt->cap_org_fmt) {
            mem_size = prev_cxt->cap_mem[i].target_jpeg.buf_size;
            if (CAP_SIM_ROT(handle, camera_id)) {
                fd = prev_cxt->cap_mem[i].cap_yuv.fd;
                y_addr = prev_cxt->cap_mem[i].cap_yuv.addr_phy.addr_y;
                y_addr_vir = prev_cxt->cap_mem[i].cap_yuv.addr_vir.addr_y;
                cur_img_frm = &prev_cxt->cap_mem[i].cap_yuv;
            } else {
                fd = prev_cxt->cap_mem[i].target_jpeg.fd;
                y_addr = prev_cxt->cap_mem[i].target_jpeg.addr_phy.addr_y;
                y_addr_vir = prev_cxt->cap_mem[i].target_jpeg.addr_vir.addr_y;
                cur_img_frm = &prev_cxt->cap_mem[i].target_jpeg;
            }
            u_addr = y_addr;
            u_addr_vir = y_addr_vir;
            frame_size = CMR_JPEG_SZIE(prev_cxt->actual_pic_size.width,
                                       prev_cxt->actual_pic_size.height);
        } else if (IMG_DATA_TYPE_YUV420 == prev_cxt->cap_org_fmt ||
                   IMG_DATA_TYPE_YVU420 == prev_cxt->cap_org_fmt) {
            if (is_normal_cap) {
                if ((IMG_ANGLE_0 != prev_cxt->prev_param.cap_rot) ||
                    (prev_cxt->prev_param.is_cfg_rot_cap &&
                     (IMG_ANGLE_0 != prev_cxt->prev_param.encode_angle))) {
                    mem_size = prev_cxt->cap_mem[i].cap_yuv_rot.buf_size;
                    fd = prev_cxt->cap_mem[i].cap_yuv_rot.fd;
                    y_addr = prev_cxt->cap_mem[i].cap_yuv_rot.addr_phy.addr_y;
                    u_addr = prev_cxt->cap_mem[i].cap_yuv_rot.addr_phy.addr_u;
                    y_addr_vir =
                        prev_cxt->cap_mem[i].cap_yuv_rot.addr_vir.addr_y;
                    u_addr_vir =
                        prev_cxt->cap_mem[i].cap_yuv_rot.addr_vir.addr_u;
                    frame_size = prev_cxt->cap_org_size.width *
                                 prev_cxt->cap_org_size.height * 3 / 2;
                    cur_img_frm = &prev_cxt->cap_mem[i].cap_yuv_rot;
                } else {
                    if (!is_need_scaling) {
                        mem_size = prev_cxt->cap_mem[i].target_yuv.buf_size;
                        fd = prev_cxt->cap_mem[i].target_yuv.fd;
                        y_addr =
                            prev_cxt->cap_mem[i].target_yuv.addr_phy.addr_y;
                        u_addr =
                            prev_cxt->cap_mem[i].target_yuv.addr_phy.addr_u;
                        y_addr_vir =
                            prev_cxt->cap_mem[i].target_yuv.addr_vir.addr_y;
                        u_addr_vir =
                            prev_cxt->cap_mem[i].target_yuv.addr_vir.addr_u;
                        cur_img_frm = &prev_cxt->cap_mem[i].target_yuv;
                    } else {
                        mem_size = prev_cxt->cap_mem[i].cap_yuv.buf_size;
                        fd = prev_cxt->cap_mem[i].cap_yuv.fd;
                        y_addr = prev_cxt->cap_mem[i].cap_yuv.addr_phy.addr_y;
                        u_addr = prev_cxt->cap_mem[i].cap_yuv.addr_phy.addr_u;
                        y_addr_vir =
                            prev_cxt->cap_mem[i].cap_yuv.addr_vir.addr_y;
                        u_addr_vir =
                            prev_cxt->cap_mem[i].cap_yuv.addr_vir.addr_u;
                        cur_img_frm = &prev_cxt->cap_mem[i].cap_yuv;
                    }
                    frame_size = buffer_size * 3 / 2;
                }
            } else {
                if (prev_cxt->prev_param.is_cfg_rot_cap &&
                    (IMG_ANGLE_0 != prev_cxt->prev_param.encode_angle)) {
                    mem_size = prev_cxt->cap_mem[i].cap_yuv_rot.buf_size;
                    fd = prev_cxt->cap_mem[i].cap_yuv_rot.fd;
                    y_addr = prev_cxt->cap_mem[i].cap_yuv_rot.addr_phy.addr_y;
                    u_addr = prev_cxt->cap_mem[i].cap_yuv_rot.addr_phy.addr_u;
                    y_addr_vir =
                        prev_cxt->cap_mem[i].cap_yuv_rot.addr_vir.addr_y;
                    u_addr_vir =
                        prev_cxt->cap_mem[i].cap_yuv_rot.addr_vir.addr_u;
                    frame_size = prev_cxt->cap_org_size.width *
                                 prev_cxt->cap_org_size.height * 3 / 2;
                    cur_img_frm = &prev_cxt->cap_mem[i].cap_yuv_rot;

                } else {
                    if (!is_need_scaling) {
                        mem_size = prev_cxt->cap_mem[i].target_yuv.buf_size;
                        fd = prev_cxt->cap_mem[i].target_yuv.fd;
                        y_addr =
                            prev_cxt->cap_mem[i].target_yuv.addr_phy.addr_y;
                        u_addr =
                            prev_cxt->cap_mem[i].target_yuv.addr_phy.addr_u;
                        y_addr_vir =
                            prev_cxt->cap_mem[i].target_yuv.addr_vir.addr_y;
                        u_addr_vir =
                            prev_cxt->cap_mem[i].target_yuv.addr_vir.addr_u;
                        cur_img_frm = &prev_cxt->cap_mem[i].target_yuv;
                    } else {
                        mem_size = prev_cxt->cap_mem[i].cap_yuv.buf_size;
                        fd = prev_cxt->cap_mem[i].cap_yuv.fd;
                        y_addr = prev_cxt->cap_mem[i].cap_yuv.addr_phy.addr_y;
                        u_addr = prev_cxt->cap_mem[i].cap_yuv.addr_phy.addr_u;
                        y_addr_vir =
                            prev_cxt->cap_mem[i].cap_yuv.addr_vir.addr_y;
                        u_addr_vir =
                            prev_cxt->cap_mem[i].cap_yuv.addr_vir.addr_u;
                        cur_img_frm = &prev_cxt->cap_mem[i].cap_yuv;
                    }
                    frame_size = buffer_size * 3 / 2;
                }
            }
        } else {
            CMR_LOGE("Unsupported capture format!");
            ret = CMR_CAMERA_NO_SUPPORT;
            break;
        }

        CMR_LOGI("%d capture addr, fd 0x%x", i, cur_img_frm->fd);
        if (0 == cur_img_frm->fd) {
            ret = CMR_CAMERA_FAIL;
            CMR_LOGI("cur_img_frm->fd is null");
            break;
        }

        prev_cxt->cap_frm[i].size.width = prev_cxt->cap_org_size.width;
        prev_cxt->cap_frm[i].size.height = prev_cxt->cap_org_size.height;
        prev_cxt->cap_frm[i].fmt = prev_cxt->cap_org_fmt;
        prev_cxt->cap_frm[i].buf_size = cur_img_frm->buf_size;
        prev_cxt->cap_frm[i].addr_phy.addr_y = y_addr;
        prev_cxt->cap_frm[i].addr_phy.addr_u = u_addr;
        prev_cxt->cap_frm[i].addr_vir.addr_y = y_addr_vir;
        prev_cxt->cap_frm[i].addr_vir.addr_u = u_addr_vir;
        prev_cxt->cap_frm[i].fd = fd;
        buffer->addr[i].addr_y = prev_cxt->cap_frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->cap_frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->cap_frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->cap_frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->cap_frm[i].fd;

#ifdef CONFIG_MULTI_CAP_MEM
        if ((IMG_DATA_TYPE_YUV420 == prev_cxt->cap_org_fmt ||
             IMG_DATA_TYPE_YVU420 == prev_cxt->cap_org_fmt) &&
            is_normal_cap /* && i > 0*/) {
            /*prev_cxt->cap_frm[i].addr_phy.addr_y =
            prev_cxt->cap_phys_addr_path_array[i];
            prev_cxt->cap_frm[i].addr_phy.addr_u =
            prev_cxt->cap_frm[i].addr_phy.addr_y + buffer_size;
            prev_cxt->cap_frm[i].addr_phy.addr_v = 0;
            prev_cxt->cap_frm[i].addr_vir.addr_y =
            prev_cxt->cap_virt_addr_path_array[i];
            prev_cxt->cap_frm[i].addr_vir.addr_u =
            prev_cxt->cap_frm[i].addr_vir.addr_y + buffer_size;
            prev_cxt->cap_frm[i].addr_vir.addr_v = 0;

            buffer->addr[i].addr_y = prev_cxt->cap_frm[i].addr_phy.addr_y;
            buffer->addr[i].addr_u = prev_cxt->cap_frm[i].addr_phy.addr_u;
            buffer->addr_vir[i].addr_y = prev_cxt->cap_frm[i].addr_vir.addr_y;
            buffer->addr_vir[i].addr_u = prev_cxt->cap_frm[i].addr_vir.addr_u;*/

            buffer->addr[i].addr_y = prev_cxt->cap_phys_addr_path_array[i];
            buffer->addr[i].addr_u = buffer->addr[i].addr_y + channel_size;
            buffer->addr_vir[i].addr_y = prev_cxt->cap_virt_addr_path_array[i];
            buffer->addr_vir[i].addr_u =
                buffer->addr_vir[i].addr_y + channel_size;
            buffer->fd[i] = prev_cxt->cap_fd_path_array[i];
        }
#endif
    }

    buffer->length = frame_size;
    if (FRAME_HDR_PROC == prev_cxt->prev_param.frame_ctrl) {
        for (i = 1; i < HDR_CAP_NUM; i++) {
            if ((IMG_DATA_TYPE_YUV420 == prev_cxt->cap_org_fmt ||
                 IMG_DATA_TYPE_YVU420 == prev_cxt->cap_org_fmt) &&
                is_normal_cap) {
                buffer->addr[i].addr_y =
                    prev_cxt->cap_hdr_phys_addr_path_array[i - 1];
                buffer->addr[i].addr_u = buffer->addr[i].addr_y + channel_size;
                buffer->addr_vir[i].addr_y =
                    prev_cxt->cap_hdr_virt_addr_path_array[i - 1];
                buffer->addr_vir[i].addr_u =
                    buffer->addr_vir[i].addr_y + channel_size;
                buffer->fd[i] = prev_cxt->cap_hdr_fd_path_array[i - 1];
            }
        }
        buffer->count = HDR_CAP_NUM;
    };

    ATRACE_END();
    CMR_LOGI("out");
    return ret;
}

cmr_int prev_free_cap_buf(struct prev_handle *handle, cmr_u32 camera_id,
                          cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;
    cmr_u32 sum = 0;
    cmr_u32 is_pre_alloc_cap_mem = 0;
    cmr_u32 cap_sum = 0;
    cmr_u32 hdr_cap_sum = HDR_CAP_NUM - 1;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

#ifdef CONFIG_PRE_ALLOC_CAPTURE_MEM
    is_pre_alloc_cap_mem = 1;
#endif

    sum = 1;
    cap_sum = CMR_CAPTURE_MEM_SUM;
    CMR_LOGI("camera_id %d, is_restart %d, is_pre_alloc_cap_mem %d", camera_id,
             is_restart, is_pre_alloc_cap_mem);
    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if ((FRAME_HDR_PROC == prev_cxt->prev_param.frame_ctrl) &&
        (0 != prev_cxt->cap_phys_addr_path_array[0]) && !is_restart) {
        mem_ops->free_mem(CAMERA_SNAPSHOT_PATH, handle->oem_handle,
                          prev_cxt->cap_hdr_phys_addr_path_array,
                          prev_cxt->cap_hdr_virt_addr_path_array,
                          prev_cxt->cap_hdr_fd_path_array, hdr_cap_sum);
        cmr_bzero(prev_cxt->cap_hdr_phys_addr_path_array,
                  HDR_CAP_NUM * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_hdr_virt_addr_path_array,
                  HDR_CAP_NUM * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_hdr_fd_path_array,
                  HDR_CAP_NUM * sizeof(cmr_s32));
    }

#ifdef CONFIG_MULTI_CAP_MEM
    if (0 != prev_cxt->cap_phys_addr_path_array[0] && !is_restart) {
        mem_ops->free_mem(CAMERA_SNAPSHOT_PATH, handle->oem_handle,
                          prev_cxt->cap_phys_addr_path_array,
                          prev_cxt->cap_virt_addr_path_array,
                          prev_cxt->cap_fd_path_array, cap_sum);
        cmr_bzero(prev_cxt->cap_phys_addr_path_array,
                  CMR_CAPTURE_MEM_SUM * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_virt_addr_path_array,
                  CMR_CAPTURE_MEM_SUM * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_fd_path_array,
                  CMR_CAPTURE_MEM_SUM * sizeof(cmr_s32));
    }
#endif

    if (0 == prev_cxt->cap_fd_array[0]) {
        CMR_LOGE("already freed");
        return ret;
    }

    if (!is_pre_alloc_cap_mem && !is_restart) {
        CMR_LOGI("fre cap mem really");
        mem_ops->free_mem(
            CAMERA_SNAPSHOT, handle->oem_handle, prev_cxt->cap_phys_addr_array,
            prev_cxt->cap_virt_addr_array, prev_cxt->cap_fd_array, sum);

        cmr_bzero(prev_cxt->cap_phys_addr_array,
                  CMR_CAPTURE_MEM_SUM * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_virt_addr_array,
                  CMR_CAPTURE_MEM_SUM * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_fd_array,
                  CMR_CAPTURE_MEM_SUM * sizeof(cmr_s32));
    }

    CMR_LOGI("out");
    return ret;
}

cmr_int prev_alloc_cap_reserve_buf(struct prev_handle *handle,
                                   cmr_u32 camera_id, cmr_u32 is_restart) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_size = 0;
    cmr_u32 frame_num = 0;
    cmr_u32 i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 cap_rot = 0;
    cmr_uint reserved_count = 0;
    cmr_u32 aligned_type = 0;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;
    cmr_int zoom_post_proc = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    CMR_LOGI("is_restart %d", is_restart);

    reserved_count = prev_get_capzsl_res_buffer_count(prev_cxt);

    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);
    mem_ops = &prev_cxt->prev_param.memory_setting;
    if (ZOOM_POST_PROCESS == zoom_post_proc) {
        width = prev_cxt->max_size.width;
        height = prev_cxt->max_size.height;
    } else {
        width = prev_cxt->actual_pic_size.width;
        height = prev_cxt->actual_pic_size.height;
    }
    CMR_LOGI("width %d height %d", width, height);
    cap_rot = 0; // prev_cxt->prev_param.cap_rot;
    aligned_type = CAMERA_MEM_NO_ALIGNED;

    /*init preview memory info*/
    buffer_size = width * height;
    if (IMG_DATA_TYPE_YUV420 == prev_cxt->cap_org_fmt ||
        IMG_DATA_TYPE_YVU420 == prev_cxt->cap_org_fmt) {
        prev_cxt->cap_zsl_mem_size = (width * height * 3) >> 1;
    } else if (IMG_DATA_TYPE_YUV422 == prev_cxt->cap_org_fmt) {
        prev_cxt->cap_zsl_mem_size = (width * height) << 1;
    } else if (IMG_DATA_TYPE_YV12 == prev_cxt->cap_org_fmt) {
        if (IMG_ANGLE_90 == prev_cxt->prev_param.cap_rot ||
            IMG_ANGLE_270 == prev_cxt->prev_param.cap_rot) {
            prev_cxt->cap_zsl_mem_size =
                (height + camera_get_aligned_size(aligned_type, height / 2)) *
                width;
        } else {
            prev_cxt->cap_zsl_mem_size =
                (width + camera_get_aligned_size(aligned_type, width / 2)) *
                height;
        }
        prev_cxt->cap_org_fmt = IMG_DATA_TYPE_YUV420;
    } else if (IMG_DATA_TYPE_RAW == prev_cxt->cap_org_fmt) {
        prev_cxt->cap_zsl_mem_size = (width * height * 3) >> 1;
    } else {
        CMR_LOGE("unsupprot fmt %ld", prev_cxt->cap_org_fmt);
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt->cap_zsl_mem_num = ZSL_FRM_ALLOC_CNT;

    /*alloc preview buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }
    if (!is_restart) {
        mem_ops->alloc_mem(CAMERA_SNAPSHOT_ZSL_RESERVED, handle->oem_handle,
                           (cmr_u32 *)&prev_cxt->cap_zsl_mem_size,
                           (cmr_u32 *)&reserved_count,
                           prev_cxt->cap_zsl_reserved_phys_addr,
                           prev_cxt->cap_zsl_reserved_virt_addr,
                           prev_cxt->cap_zsl_reserved_fd);
    }

    frame_size = prev_cxt->cap_zsl_mem_size;

    for (i = 0; i < reserved_count; i++) {
        prev_cxt->cap_zsl_reserved_frm[i].buf_size = frame_size;
        prev_cxt->cap_zsl_reserved_frm[i].addr_vir.addr_y =
            prev_cxt->cap_zsl_reserved_virt_addr[i];
        prev_cxt->cap_zsl_reserved_frm[i].addr_vir.addr_u =
            prev_cxt->cap_zsl_reserved_frm[i].addr_vir.addr_y + buffer_size;
        prev_cxt->cap_zsl_reserved_frm[i].addr_phy.addr_y =
            prev_cxt->cap_zsl_reserved_phys_addr[i];
        prev_cxt->cap_zsl_reserved_frm[i].addr_phy.addr_u =
            prev_cxt->cap_zsl_reserved_frm[i].addr_phy.addr_y + buffer_size;
        prev_cxt->cap_zsl_reserved_frm[i].fd = prev_cxt->cap_zsl_reserved_fd[i];
        prev_cxt->cap_zsl_reserved_frm[i].fmt = prev_cxt->cap_org_fmt;
        prev_cxt->cap_zsl_reserved_frm[i].size.width = width;
        prev_cxt->cap_zsl_reserved_frm[i].size.height = height;

        prev_cxt->cap_zsl_reserved_frm[i].addr_phy.addr_v = 0;
    }

    CMR_LOGI("out %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int prev_free_cap_reserve_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                  cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;
    cmr_uint reserved_count = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    reserved_count = prev_get_capzsl_res_buffer_count(prev_cxt);

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        mem_ops->free_mem(CAMERA_SNAPSHOT_ZSL_RESERVED, handle->oem_handle,
                          prev_cxt->cap_zsl_reserved_phys_addr,
                          prev_cxt->cap_zsl_reserved_virt_addr,
                          prev_cxt->cap_zsl_reserved_fd,
                          (cmr_u32)reserved_count);
        cmr_bzero(prev_cxt->cap_zsl_reserved_phys_addr,
                  sizeof(prev_cxt->cap_zsl_reserved_phys_addr));
        cmr_bzero(prev_cxt->cap_zsl_reserved_virt_addr,
                  sizeof(prev_cxt->cap_zsl_reserved_virt_addr));
        cmr_bzero(prev_cxt->cap_zsl_reserved_fd,
                  sizeof(prev_cxt->cap_zsl_reserved_fd));
    }

    CMR_LOGI("out");
    return ret;
}

cmr_int prev_alloc_zsl_buf(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart, struct buffer_cfg *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_size = 0;
    cmr_u32 frame_num = 0;
    cmr_u32 i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 cap_rot = 0;
    cmr_uint reserved_count = 1;
    cmr_u32 aligned_type = 0;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;
    cmr_int zoom_post_proc = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    CMR_LOGI("is_restart %d", is_restart);

    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);
    mem_ops = &prev_cxt->prev_param.memory_setting;
    if (ZOOM_POST_PROCESS == zoom_post_proc) {
        width = prev_cxt->cap_sn_size.width;
        height = prev_cxt->cap_sn_size.height;
    } else if (ZOOM_POST_PROCESS_WITH_TRIM == zoom_post_proc) {
        width = prev_cxt->max_size.width;
        height = prev_cxt->max_size.height;
    } else {
        width = prev_cxt->actual_pic_size.width;
        height = prev_cxt->actual_pic_size.height;
    }
    CMR_LOGI("width %d height %d", width, height);
    cap_rot = 0; // prev_cxt->prev_param.cap_rot;
    aligned_type = CAMERA_MEM_NO_ALIGNED;

    /*init preview memory info*/
    buffer_size = width * height;
    if (IMG_DATA_TYPE_YUV420 == prev_cxt->cap_org_fmt ||
        IMG_DATA_TYPE_YVU420 == prev_cxt->cap_org_fmt) {
        prev_cxt->cap_zsl_mem_size = (width * height * 3) >> 1;
    } else if (IMG_DATA_TYPE_YUV422 == prev_cxt->cap_org_fmt) {
        prev_cxt->cap_zsl_mem_size = (width * height) << 1;
    } else if (IMG_DATA_TYPE_YV12 == prev_cxt->cap_org_fmt) {
        if (IMG_ANGLE_90 == prev_cxt->prev_param.cap_rot ||
            IMG_ANGLE_270 == prev_cxt->prev_param.cap_rot) {
            prev_cxt->cap_zsl_mem_size =
                (height + camera_get_aligned_size(aligned_type, height / 2)) *
                width;
        } else {
            prev_cxt->cap_zsl_mem_size =
                (width + camera_get_aligned_size(aligned_type, width / 2)) *
                height;
        }
        prev_cxt->cap_org_fmt = IMG_DATA_TYPE_YUV420;
    } else {
        CMR_LOGE("unsupprot fmt %ld", prev_cxt->cap_org_fmt);
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt->cap_zsl_mem_num = ZSL_FRM_ALLOC_CNT;
    if (prev_cxt->prev_param.cap_rot) {
        CMR_LOGI("need increase buf for rotation");
        prev_cxt->cap_zsl_mem_num += PREV_ROT_FRM_CNT;
    }

    /*alloc preview buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }
    if (!is_restart) {
        prev_cxt->cap_zsl_mem_valid_num = 0;
        mem_ops->alloc_mem(CAMERA_SNAPSHOT_ZSL, handle->oem_handle,
                           (cmr_u32 *)&prev_cxt->cap_zsl_mem_size,
                           (cmr_u32 *)&prev_cxt->cap_zsl_mem_num,
                           prev_cxt->cap_zsl_phys_addr_array,
                           prev_cxt->cap_zsl_virt_addr_array,
                           prev_cxt->cap_zsl_fd_array);

        /*check memory valid*/
        CMR_LOGI("prev_mem_size 0x%lx, mem_num %ld", prev_cxt->cap_zsl_mem_size,
                 prev_cxt->cap_zsl_mem_num);
        for (i = 0; i < prev_cxt->cap_zsl_mem_num; i++) {
            CMR_LOGI("%d, virt_addr 0x%lx, fd 0x%x", i,
                     prev_cxt->cap_zsl_virt_addr_array[i],
                     prev_cxt->cap_zsl_fd_array[i]);

            if ((0 == prev_cxt->cap_zsl_virt_addr_array[i]) ||
                0 == prev_cxt->cap_zsl_fd_array[i]) {
                if (i >= ZSL_FRM_ALLOC_CNT) {
                    CMR_LOGE("memory is invalid");
                    return CMR_CAMERA_NO_MEM;
                }
            } else {
                if (i < ZSL_FRM_ALLOC_CNT) {
                    prev_cxt->cap_zsl_mem_valid_num++;
                }
            }
        }
    }

    frame_size = prev_cxt->cap_zsl_mem_size;
    prev_num = prev_cxt->cap_zsl_mem_num;
    if (prev_cxt->prev_param.cap_rot) {
        prev_num = prev_cxt->cap_zsl_mem_num - PREV_ROT_FRM_CNT;
    }

    /*arrange the buffer*/
    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_CAP1_ID_BASE;
    buffer->count = prev_cxt->cap_zsl_mem_valid_num;
    buffer->length = frame_size;
    buffer->flag = BUF_FLAG_INIT;

    for (i = 0; i < (cmr_u32)prev_cxt->cap_zsl_mem_valid_num; i++) {
        prev_cxt->cap_zsl_frm[i].buf_size = frame_size;
        prev_cxt->cap_zsl_frm[i].addr_vir.addr_y =
            prev_cxt->cap_zsl_virt_addr_array[i];
        prev_cxt->cap_zsl_frm[i].addr_vir.addr_u =
            prev_cxt->cap_zsl_frm[i].addr_vir.addr_y + buffer_size;
        prev_cxt->cap_zsl_frm[i].addr_phy.addr_y =
            prev_cxt->cap_zsl_phys_addr_array[i];
        prev_cxt->cap_zsl_frm[i].addr_phy.addr_u =
            prev_cxt->cap_zsl_frm[i].addr_phy.addr_y + buffer_size;
        prev_cxt->cap_zsl_frm[i].fd = prev_cxt->cap_zsl_fd_array[i];
        prev_cxt->cap_zsl_frm[i].fmt = prev_cxt->cap_org_fmt;
        prev_cxt->cap_zsl_frm[i].size.width = width;
        prev_cxt->cap_zsl_frm[i].size.height = height;

        buffer->addr[i].addr_y = prev_cxt->cap_zsl_frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->cap_zsl_frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->cap_zsl_frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->cap_zsl_frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->cap_zsl_frm[i].fd;
    }
    /*prev_cxt->cap_zsl_reserved_frm.buf_size        = frame_size;
    prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_y =
    prev_cxt->cap_zsl_reserved_virt_addr;
    prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_u =
    prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_y + buffer_size;
    prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_y =
    prev_cxt->cap_zsl_reserved_phys_addr;
    prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_u =
    prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_y + buffer_size;
    prev_cxt->cap_zsl_reserved_frm.fmt             = prev_cxt->cap_org_fmt;
    prev_cxt->cap_zsl_reserved_frm.size.width      = width;
    prev_cxt->cap_zsl_reserved_frm.size.height     = height;*/

    prev_cxt->cap_zsl_frm[i].addr_phy.addr_v = 0;
    // prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_v = 0;

    if (prev_cxt->prev_param.cap_rot) {
        for (i = 0; i < PREV_ROT_FRM_CNT; i++) {
            prev_cxt->cap_zsl_rot_frm[i].buf_size = frame_size;
            prev_cxt->cap_zsl_rot_frm[i].addr_vir.addr_y =
                prev_cxt->cap_zsl_virt_addr_array[prev_num + i];
            prev_cxt->cap_zsl_rot_frm[i].addr_vir.addr_u =
                prev_cxt->cap_zsl_rot_frm[i].addr_vir.addr_y + buffer_size;
            prev_cxt->cap_zsl_rot_frm[i].addr_phy.addr_y =
                prev_cxt->cap_zsl_phys_addr_array[prev_num + i];
            prev_cxt->cap_zsl_rot_frm[i].addr_phy.addr_u =
                prev_cxt->cap_zsl_rot_frm[i].addr_phy.addr_y + buffer_size;
            prev_cxt->cap_zsl_rot_frm[i].addr_phy.addr_v = 0;
            prev_cxt->cap_zsl_rot_frm[i].fd =
                prev_cxt->cap_zsl_fd_array[prev_num + i];
            prev_cxt->cap_zsl_rot_frm[i].fmt = prev_cxt->cap_org_fmt;
            prev_cxt->cap_zsl_rot_frm[i].size.width = width;
            prev_cxt->cap_zsl_rot_frm[i].size.height = height;
        }
    }

    ATRACE_END();
    CMR_LOGI("out %ld", ret);
    return ret;
}

cmr_int prev_free_zsl_buf(struct prev_handle *handle, cmr_u32 camera_id,
                          cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        mem_ops->free_mem(CAMERA_SNAPSHOT_ZSL, handle->oem_handle,
                          prev_cxt->cap_zsl_phys_addr_array,
                          prev_cxt->cap_zsl_virt_addr_array,
                          prev_cxt->cap_zsl_fd_array,
                          prev_cxt->cap_zsl_mem_num);

        cmr_bzero(prev_cxt->cap_zsl_phys_addr_array,
                  (ZSL_FRM_CNT + ZSL_ROT_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_zsl_virt_addr_array,
                  (ZSL_FRM_CNT + ZSL_ROT_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_zsl_fd_array,
                  (ZSL_FRM_CNT + ZSL_ROT_FRM_CNT) * sizeof(cmr_s32));
        cmr_bzero(&prev_cxt->cap_zsl_frm[0],
                  sizeof(struct img_frm) * ZSL_FRM_CNT);

        cmr_bzero(prev_cxt->cap_zsl_reserved_phys_addr,
                  sizeof(prev_cxt->cap_zsl_reserved_phys_addr));
        cmr_bzero(prev_cxt->cap_zsl_reserved_virt_addr,
                  sizeof(prev_cxt->cap_zsl_reserved_virt_addr));
        cmr_bzero(prev_cxt->cap_zsl_reserved_fd,
                  sizeof(prev_cxt->cap_zsl_reserved_fd));
    }

    CMR_LOGI("out");
    return ret;
}

cmr_int prev_alloc_sensor_datatype_buf(struct prev_handle *handle,
                                       cmr_u32 camera_id, cmr_u32 is_restart,
                                       struct buffer_cfg *buffer) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint i = 0;
    cmr_uint reserved_count = 1;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;
    CMR_LOGI("allo flag %d", prev_cxt->sensor_datatype_mem_alloc_flag);

    switch (prev_cxt->prev_param.sensor_datatype) {
    case SENSOR_REAL_DEPTH_ENABLE:
        /*for depth param*/
        prev_cxt->sensor_datatype_mem_size = CAMERA_DEPTH_META_SIZE;
        prev_cxt->sensor_datatype_mem_num = PREV_FRM_CNT;
        break;
    case SENSOR_EMBEDDED_INFO_ENABLE:
        /*for embedded info param*/
        prev_cxt->sensor_datatype_mem_size = CAMERA_EMBEDDED_INFO_META_SIZE;
        prev_cxt->sensor_datatype_mem_num = PREV_DATATYPE_CNT;
        break;
    case SENSOR_DATATYPE_PDAF_ENABLE:
        /*for pdaf param*/
        prev_cxt->sensor_datatype_mem_size = prev_cxt->prev_param.datatype_size;
        prev_cxt->sensor_datatype_mem_num = PDAF_FRM_CNT; // PREV_DATATYPE_CNT;
        break;
    default:
        return CMR_CAMERA_INVALID_PARAM;
    }

    /*alloc preview buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart && (1 != prev_cxt->sensor_datatype_mem_alloc_flag)) {
        prev_cxt->sensor_datatype_mem_valid_num = 0;
        mem_ops->alloc_mem(CAMERA_SENSOR_DATATYPE_MAP, handle->oem_handle,
                           (cmr_u32 *)&prev_cxt->sensor_datatype_mem_size,
                           (cmr_u32 *)&prev_cxt->sensor_datatype_mem_num,
                           prev_cxt->sensor_datatype_phys_addr_array,
                           prev_cxt->sensor_datatype_virt_addr_array,
                           prev_cxt->sensor_datatype_fd_array);
        /*check memory valid*/
        CMR_LOGI("sensor_datatype_mem_size 0x%lx, mem_num %ld",
                 prev_cxt->sensor_datatype_mem_size,
                 prev_cxt->sensor_datatype_mem_num);
        for (i = 0; i < prev_cxt->sensor_datatype_mem_num; i++) {
            CMR_LOGI("%lu, phys_addr 0x%lx, virt_addr 0x%lx, fd 0x%x", i,
                     prev_cxt->sensor_datatype_phys_addr_array[i],
                     prev_cxt->sensor_datatype_virt_addr_array[i],
                     prev_cxt->sensor_datatype_fd_array[i]);

            if (((0 == prev_cxt->sensor_datatype_virt_addr_array[i])) ||
                (prev_cxt->sensor_datatype_fd_array[i] == 0)) {
                CMR_LOGE("memory is invalid");
                return CMR_CAMERA_NO_MEM;
            } else {
                prev_cxt->sensor_datatype_mem_valid_num++;
            }
        }
        mem_ops->alloc_mem(CAMERA_SENSOR_DATATYPE_MAP_RESERVED,
                           handle->oem_handle,
                           (cmr_u32 *)&prev_cxt->sensor_datatype_mem_size,
                           (cmr_u32 *)&reserved_count,
                           &prev_cxt->sensor_datatype_reserved_phys_addr,
                           &prev_cxt->sensor_datatype_reserved_virt_addr,
                           &prev_cxt->sensor_datatype_reserved_fd);
        prev_cxt->sensor_datatype_mem_alloc_flag = 1;
        CMR_LOGI("reserved, phys_addr 0x%lx, virt_addr 0x%lx, fd 0x%x",
                 prev_cxt->sensor_datatype_reserved_phys_addr,
                 prev_cxt->sensor_datatype_reserved_virt_addr,
                 prev_cxt->sensor_datatype_reserved_fd);
    }

    /*arrange the buffer*/
    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_SENSOR_DATATYPE_ID_BASE;
    buffer->count = prev_cxt->sensor_datatype_mem_valid_num;
    buffer->length = prev_cxt->sensor_datatype_mem_size;
    buffer->flag = BUF_FLAG_INIT;

    for (i = 0; i < (cmr_uint)prev_cxt->sensor_datatype_mem_valid_num; i++) {
        prev_cxt->sensor_datatype_frm[i].buf_size =
            prev_cxt->sensor_datatype_mem_size;
        prev_cxt->sensor_datatype_frm[i].addr_vir.addr_y =
            prev_cxt->sensor_datatype_virt_addr_array[i];
        prev_cxt->sensor_datatype_frm[i].addr_phy.addr_y =
            prev_cxt->sensor_datatype_phys_addr_array[i];
        prev_cxt->sensor_datatype_frm[i].fd =
            prev_cxt->sensor_datatype_fd_array[i];
        prev_cxt->sensor_datatype_frm[i].fmt = IMG_DATA_TYPE_RAW;

        buffer->addr[i].addr_y =
            prev_cxt->sensor_datatype_frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u =
            prev_cxt->sensor_datatype_frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y =
            prev_cxt->sensor_datatype_frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u =
            prev_cxt->sensor_datatype_frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->sensor_datatype_frm[i].fd;
    }
    prev_cxt->sensor_datatype_reserved_frm.buf_size =
        prev_cxt->sensor_datatype_mem_size;
    prev_cxt->sensor_datatype_reserved_frm.addr_vir.addr_y =
        prev_cxt->sensor_datatype_reserved_virt_addr;
    prev_cxt->sensor_datatype_reserved_frm.addr_vir.addr_u =
        prev_cxt->sensor_datatype_reserved_virt_addr;
    prev_cxt->sensor_datatype_reserved_frm.addr_phy.addr_y =
        prev_cxt->sensor_datatype_reserved_phys_addr;
    prev_cxt->sensor_datatype_reserved_frm.addr_phy.addr_u =
        prev_cxt->sensor_datatype_reserved_phys_addr;
    prev_cxt->sensor_datatype_reserved_frm.fd =
        prev_cxt->sensor_datatype_reserved_fd;
    prev_cxt->sensor_datatype_reserved_frm.fmt = IMG_DATA_TYPE_RAW;

    CMR_LOGI("out %ld", ret);
    return ret;
}

cmr_int prev_free_sensor_datatype_buf(struct prev_handle *handle,
                                      cmr_u32 camera_id, cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart && (1 == prev_cxt->sensor_datatype_mem_alloc_flag)) {
        mem_ops->free_mem(CAMERA_SENSOR_DATATYPE_MAP, handle->oem_handle,
                          prev_cxt->sensor_datatype_phys_addr_array,
                          prev_cxt->sensor_datatype_virt_addr_array,
                          prev_cxt->sensor_datatype_fd_array,
                          prev_cxt->sensor_datatype_mem_num);
        cmr_bzero(prev_cxt->sensor_datatype_phys_addr_array,
                  (PREV_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->sensor_datatype_virt_addr_array,
                  (PREV_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->sensor_datatype_fd_array,
                  (PREV_FRM_CNT) * sizeof(cmr_s32));
        prev_cxt->sensor_datatype_reserved_phys_addr = 0;
        prev_cxt->sensor_datatype_reserved_virt_addr = 0;
        prev_cxt->sensor_datatype_mem_alloc_flag = 0;
    }

    CMR_LOGI("out");
    return ret;
}

cmr_int prev_alloc_pdaf_raw_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                cmr_u32 is_restart, struct buffer_cfg *buffer) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint i = 0;
    cmr_uint reserved_count = 1;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;
    CMR_LOGI("allo flag %d", prev_cxt->pdaf_mem_alloc_flag);

    prev_cxt->pdaf_mem_size =
        prev_cxt->pdaf_rect.width * prev_cxt->pdaf_rect.height * 10 / 8;

    prev_cxt->pdaf_mem_num = PDAF_FRM_CNT;

    /*alloc preview buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart && (1 != prev_cxt->pdaf_mem_alloc_flag)) {
        prev_cxt->pdaf_mem_valid_num = 0;
        mem_ops->alloc_mem(
            CAMERA_PDAF_RAW, handle->oem_handle,
            (cmr_u32 *)&prev_cxt->pdaf_mem_size,
            (cmr_u32 *)&prev_cxt->pdaf_mem_num, prev_cxt->pdaf_phys_addr_array,
            prev_cxt->pdaf_virt_addr_array, prev_cxt->pdaf_fd_array);
        /*check memory valid*/
        CMR_LOGI("pdaf_mem_size 0x%lx, mem_num %ld", prev_cxt->pdaf_mem_size,
                 prev_cxt->pdaf_mem_num);
        for (i = 0; i < prev_cxt->pdaf_mem_num; i++) {
            CMR_LOGI("%ld, phys_addr 0x%lx, virt_addr 0x%lx, fd 0x%x", i,
                     prev_cxt->pdaf_phys_addr_array[i],
                     prev_cxt->pdaf_virt_addr_array[i],
                     prev_cxt->pdaf_fd_array[i]);

            if (((0 == prev_cxt->pdaf_virt_addr_array[i])) ||
                (prev_cxt->pdaf_fd_array[i] == 0)) {
                CMR_LOGE("memory is invalid");
                return CMR_CAMERA_NO_MEM;
            } else {
                prev_cxt->pdaf_mem_valid_num++;
            }
        }

        mem_ops->alloc_mem(
            CAMERA_PDAF_RAW_RESERVED, handle->oem_handle,
            (cmr_u32 *)&prev_cxt->pdaf_mem_size, (cmr_u32 *)&reserved_count,
            &prev_cxt->pdaf_reserved_phys_addr,
            &prev_cxt->pdaf_reserved_virt_addr, &prev_cxt->pdaf_reserved_fd);

        CMR_LOGI("reserved, phys_addr 0x%lx, virt_addr 0x%lx, fd 0x%x",
                 prev_cxt->pdaf_reserved_phys_addr,
                 prev_cxt->pdaf_reserved_virt_addr, prev_cxt->pdaf_reserved_fd);

        prev_cxt->pdaf_mem_alloc_flag = 1;
    }

    /*arrange the buffer*/
    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_PDAF_ID_BASE;
    buffer->count = prev_cxt->pdaf_mem_valid_num;
    buffer->length = prev_cxt->pdaf_mem_size;
    buffer->flag = BUF_FLAG_INIT;

    for (i = 0; i < (cmr_uint)prev_cxt->pdaf_mem_valid_num; i++) {
        prev_cxt->pdaf_frm[i].buf_size = prev_cxt->pdaf_mem_size;
        prev_cxt->pdaf_frm[i].addr_vir.addr_y =
            prev_cxt->pdaf_virt_addr_array[i];
        prev_cxt->pdaf_frm[i].addr_phy.addr_y =
            prev_cxt->pdaf_phys_addr_array[i];
        prev_cxt->pdaf_frm[i].fd = prev_cxt->pdaf_fd_array[i];
        prev_cxt->pdaf_frm[i].fmt =
            IMG_DATA_TYPE_RAW; // IMG_DATA_TYPE_PDAF_TYPE3;

        buffer->addr[i].addr_y = prev_cxt->pdaf_frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->pdaf_frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->pdaf_frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->pdaf_frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->pdaf_frm[i].fd;
    }

    prev_cxt->pdaf_reserved_frm.buf_size = prev_cxt->pdaf_mem_size;
    prev_cxt->pdaf_reserved_frm.addr_vir.addr_y =
        prev_cxt->pdaf_reserved_virt_addr;
    prev_cxt->pdaf_reserved_frm.addr_vir.addr_u =
        prev_cxt->pdaf_reserved_virt_addr;
    prev_cxt->pdaf_reserved_frm.addr_phy.addr_y =
        prev_cxt->pdaf_reserved_phys_addr;
    prev_cxt->pdaf_reserved_frm.addr_phy.addr_u =
        prev_cxt->pdaf_reserved_phys_addr;
    prev_cxt->pdaf_reserved_frm.fd = prev_cxt->pdaf_reserved_fd;
    prev_cxt->pdaf_reserved_frm.fmt =
        IMG_DATA_TYPE_RAW; // IMG_DATA_TYPE_PDAF_TYPE3;

    CMR_LOGI("out %ld", ret);
    return ret;
}

cmr_int prev_free_pdaf_raw_buf(struct prev_handle *handle, cmr_u32 camera_id,
                               cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart && (1 == prev_cxt->pdaf_mem_alloc_flag)) {
        mem_ops->free_mem(CAMERA_PDAF_RAW, handle->oem_handle,
                          prev_cxt->pdaf_phys_addr_array,
                          prev_cxt->pdaf_virt_addr_array,
                          prev_cxt->pdaf_fd_array, prev_cxt->pdaf_mem_num);
        cmr_bzero(prev_cxt->pdaf_phys_addr_array,
                  (PDAF_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->pdaf_virt_addr_array,
                  (PDAF_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->pdaf_fd_array, (PDAF_FRM_CNT) * sizeof(cmr_s32));

        mem_ops->free_mem(CAMERA_PDAF_RAW_RESERVED, handle->oem_handle,
                          (cmr_uint *)prev_cxt->pdaf_reserved_phys_addr,
                          (cmr_uint *)prev_cxt->pdaf_reserved_virt_addr,
                          &prev_cxt->pdaf_reserved_fd, (cmr_u32)1);

        prev_cxt->pdaf_reserved_phys_addr = 0;
        prev_cxt->pdaf_reserved_virt_addr = 0;
        prev_cxt->pdaf_reserved_fd = 0;
        prev_cxt->pdaf_mem_alloc_flag = 0;
    }

    CMR_LOGI("out");
    return ret;
}

cmr_int prev_get_sensor_mode(struct prev_handle *handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct img_size *prev_size = NULL;
    struct img_size *act_prev_size = NULL;
    struct img_size *video_size = NULL;
    struct img_size *act_video_size = NULL;
    struct img_size *org_pic_size = NULL;
    struct img_size *act_pic_size = NULL;
    struct img_size *alg_pic_size = NULL;
    struct sensor_exp_info *sensor_info = NULL;
    cmr_u32 prev_rot = 0;
    cmr_u32 cap_rot = 0;
    cmr_u32 cfg_cap_rot = 0;
    cmr_u32 is_cfg_rot_cap = 0;
    cmr_u32 aligned_type = CAMERA_MEM_NO_ALIGNED;
    cmr_u32 mode_flag = 0;
    cmr_int sn_mode = 0;
    struct sensor_mode_fps_tag fps_info;

    CHECK_HANDLE_VALID(handle);

    prev_size = &handle->prev_cxt[camera_id].prev_param.preview_size;
    act_prev_size = &handle->prev_cxt[camera_id].actual_prev_size;
    video_size = &handle->prev_cxt[camera_id].prev_param.video_size;
    act_video_size = &handle->prev_cxt[camera_id].actual_video_size;
    org_pic_size = &handle->prev_cxt[camera_id].prev_param.picture_size;
    alg_pic_size = &handle->prev_cxt[camera_id].aligned_pic_size;
    act_pic_size = &handle->prev_cxt[camera_id].actual_pic_size;
    prev_rot = handle->prev_cxt[camera_id].prev_param.prev_rot;
    cap_rot = handle->prev_cxt[camera_id].prev_param.cap_rot;
    cfg_cap_rot = handle->prev_cxt[camera_id].prev_param.encode_angle;
    is_cfg_rot_cap = handle->prev_cxt[camera_id].prev_param.is_cfg_rot_cap;
    sensor_info = &handle->prev_cxt[camera_id].sensor_info;

    CMR_LOGI("preview_eb %d, snapshot_eb %d, video_eb %d, sprd_zsl_enabled %d",
             handle->prev_cxt[camera_id].prev_param.preview_eb,
             handle->prev_cxt[camera_id].prev_param.snapshot_eb,
             handle->prev_cxt[camera_id].prev_param.video_eb,
             handle->prev_cxt[camera_id].prev_param.sprd_zsl_enabled);

    CMR_LOGI("camera_id %d, sensor_datatype %d", camera_id,
             handle->prev_cxt[camera_id].prev_param.sensor_datatype);

    CMR_LOGI("camera_id %d, prev size %d %d, cap size %d %d", camera_id,
             prev_size->width, prev_size->height, org_pic_size->width,
             org_pic_size->height);

    CMR_LOGI("prev_rot %d, cap_rot %d, is_cfg_rot_cap %d, cfg_cap_rot %d",
             prev_rot, cap_rot, is_cfg_rot_cap, cfg_cap_rot);

    aligned_type = prev_get_aligned_type(handle, camera_id);

    /* w/h aligned by 16 */
    alg_pic_size->width =
        camera_get_aligned_size(aligned_type, org_pic_size->width);
    alg_pic_size->height =
        camera_get_aligned_size(aligned_type, org_pic_size->height);

    /*consider preview and capture rotation*/
    if (IMG_ANGLE_90 == prev_rot || IMG_ANGLE_270 == prev_rot) {
        act_prev_size->width = prev_size->height;
        act_prev_size->height = prev_size->width;
        act_video_size->width = video_size->height;
        act_video_size->height = video_size->width;
        act_pic_size->width = alg_pic_size->height;
        act_pic_size->height = alg_pic_size->width;
    } else {
        act_prev_size->width = prev_size->width;
        act_prev_size->height = prev_size->height;
        act_video_size->width = video_size->width;
        act_video_size->height = video_size->height;
        act_pic_size->width = alg_pic_size->width;
        act_pic_size->height = alg_pic_size->height;
    }

    CMR_LOGI(
        "org_pic_size %d %d, aligned_pic_size %d %d, actual_pic_size %d %d",
        org_pic_size->width, org_pic_size->height, alg_pic_size->width,
        alg_pic_size->height, act_pic_size->width, act_pic_size->height);

    if (!handle->ops.get_sensor_info) {
        CMR_LOGE("ops is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret =
        handle->ops.get_sensor_info(handle->oem_handle, camera_id, sensor_info);
    if (ret) {
        CMR_LOGE("get_sensor info failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*get sensor preview work mode*/
    if (handle->prev_cxt[camera_id].prev_param.preview_eb) {
        ret = prev_get_sn_preview_mode(handle, camera_id, sensor_info,
                                       act_prev_size,
                                       &handle->prev_cxt[camera_id].prev_mode);
        if (ret) {
            CMR_LOGE("get preview mode failed!");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    /*get sensor video work mode*/
    if (handle->prev_cxt[camera_id].prev_param.video_eb) {
        ret = prev_get_sn_preview_mode(handle, camera_id, sensor_info,
                                       act_video_size,
                                       &handle->prev_cxt[camera_id].video_mode);
        if (ret) {
            CMR_LOGE("get video mode failed!");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        if (handle->prev_cxt[camera_id].prev_mode >
            handle->prev_cxt[camera_id].video_mode) {
            handle->prev_cxt[camera_id].video_mode =
                handle->prev_cxt[camera_id].prev_mode;
        } else {
            handle->prev_cxt[camera_id].prev_mode =
                handle->prev_cxt[camera_id].video_mode;
        }
    }

    if (handle->prev_cxt[camera_id].prev_param.video_slowmotion_eb) {
        for (sn_mode = SENSOR_MODE_PREVIEW_ONE; sn_mode < SENSOR_MODE_MAX;
             sn_mode++) {
            ret = handle->ops.get_sensor_fps_info(handle->oem_handle, camera_id,
                                                  sn_mode, &fps_info);
            if (ret) {
                CMR_LOGE("get_sensor info failed!");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }

            CMR_LOGD("mode=%d, max_fps=%d, min_fps=%d, is_high_fps=%d, "
                     "high_fps_skip_num=%d",
                     fps_info.mode, fps_info.max_fps, fps_info.min_fps,
                     fps_info.is_high_fps, fps_info.high_fps_skip_num);

            CMR_LOGD("trim_height=%d, video_height=%d, prev_height=%d",
                     sensor_info->mode_info[fps_info.mode].trim_height,
                     act_video_size->height, act_prev_size->height);

            /* we want to make sure that high fps setting is bigger than preview
             * and video size */
            if (fps_info.is_high_fps &&
                sensor_info->mode_info[fps_info.mode].trim_height >=
                    act_prev_size->height &&
                sensor_info->mode_info[fps_info.mode].trim_height >=
                    act_video_size->height) {
                CMR_LOGD("slow motion, find a high fps setting: %d",
                         fps_info.mode);
                CMR_LOGD("prev_mode=%d, video_mode=%d, prev_channel_deci=%d",
                         fps_info.mode, fps_info.mode,
                         fps_info.high_fps_skip_num);
                handle->prev_cxt[camera_id].prev_mode = fps_info.mode;
                if (handle->prev_cxt[camera_id].prev_param.video_eb)
                    handle->prev_cxt[camera_id].video_mode = fps_info.mode;
                handle->prev_cxt[camera_id].prev_channel_deci =
                    fps_info.high_fps_skip_num - 1;
                break;
            }
        }
    }

    /*get sensor capture work mode*/
    if (handle->prev_cxt[camera_id].prev_param.snapshot_eb) {
        ret = prev_get_sn_capture_mode(handle, camera_id, sensor_info,
                                       act_pic_size,
                                       &handle->prev_cxt[camera_id].cap_mode);
        if (ret) {
            CMR_LOGE("get capture mode failed!");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        if (handle->prev_cxt[camera_id].prev_param.preview_eb &&
            (handle->prev_cxt[camera_id].prev_mode >
             handle->prev_cxt[camera_id].cap_mode)) {
            handle->prev_cxt[camera_id].cap_mode =
                handle->prev_cxt[camera_id].prev_mode;
        }
        /*caculate max size for capture*/
        handle->prev_cxt[camera_id].max_size.width = alg_pic_size->width;
        handle->prev_cxt[camera_id].max_size.height = alg_pic_size->height;
        ret = prev_get_cap_max_size(
            handle, camera_id,
            &sensor_info->mode_info[handle->prev_cxt[camera_id].cap_mode],
            &handle->prev_cxt[camera_id].max_size);
    }

exit:
    CMR_LOGI("prev_mode %ld, video_mode %ld, cap_mode %ld",
             handle->prev_cxt[camera_id].prev_mode,
             handle->prev_cxt[camera_id].video_mode,
             handle->prev_cxt[camera_id].cap_mode);

    ATRACE_END();
    return ret;
}

cmr_int prev_get_sn_preview_mode(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct sensor_exp_info *sensor_info,
                                 struct img_size *target_size,
                                 cmr_uint *work_mode) {
    cmr_int ret = CMR_CAMERA_FAIL;
    cmr_u32 width = 0, height = 0, i;
    cmr_u32 maxwidth = 0, maxheight = 0;
    cmr_u32 search_height;
    cmr_u32 search_width;
    cmr_u32 target_mode = SENSOR_MODE_PREVIEW_ONE;
    cmr_u32 last_mode = SENSOR_MODE_PREVIEW_ONE;
    cmr_int offset1 = 0, offset2 = 0;
    struct sensor_mode_fps_tag fps_info;
    char value[PROPERTY_VALUE_MAX];
    cmr_u32 is_raw_capture = 0;
    cmr_u32 is_3D_video = 0;
    cmr_u32 is_3D_caputre = 0;
    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);

    if (!sensor_info) {
        CMR_LOGE("sn info is null!");
        return CMR_CAMERA_FAIL;
    }

    property_get("persist.sys.camera.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }

    if (cxt->is_multi_mode == MODE_3D_VIDEO) {
        is_3D_video = 1;
    } else if (cxt->is_multi_mode == MODE_3D_CAPTURE) {
        is_3D_caputre = 1;
    }

    if (SENSOR_PDAF_TYPE3_ENABLE ==
        handle->prev_cxt[camera_id].prev_param.pdaf_mode) {
        search_width = sensor_info->source_width_max;
        search_height = sensor_info->source_height_max;
    } else if (1 == is_3D_video || 1 == is_3D_caputre) {
        search_width = sensor_info->source_width_max / 2;
        search_height = sensor_info->source_height_max / 2;
    } else {
        search_width = target_size->width;
        search_height = target_size->height;
    }

    CMR_LOGI("search_height = %d, search_width = %d", search_height,
             search_width);
    for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
        if (SENSOR_MODE_MAX != sensor_info->mode_info[i].mode) {
            height = sensor_info->mode_info[i].trim_height;
            width = sensor_info->mode_info[i].trim_width;
            CMR_LOGI("candidate height = %d, width = %d", height, width);
            height = CAMERA_ALIGNED_16(height);
            if (IMG_DATA_TYPE_JPEG != sensor_info->mode_info[i].image_format) {
                if (search_height <= height && search_width <= width) {
                    if (is_raw_capture == 0) {
                        /* dont choose high fps setting for no-slowmotion */
                        ret = handle->ops.get_sensor_fps_info(
                            handle->oem_handle, camera_id, i, &fps_info);
                        CMR_LOGV("mode=%d, is_high_fps=%d", i,
                                 fps_info.is_high_fps);
                        if (fps_info.is_high_fps) {
                            CMR_LOGD("dont choose high fps setting");
                            continue;
                        }
                    }

                    target_mode = i;
                    ret = CMR_CAMERA_SUCCESS;
                    break;
                } else {
                    last_mode = i;
                }
            }
        }
    }

    if (i == SENSOR_MODE_MAX) {
        CMR_LOGI("can't find the right mode, %d", i);
        target_mode = last_mode;
        ret = CMR_CAMERA_SUCCESS;
    }

    height = sensor_info->mode_info[target_mode].trim_height;
    width = sensor_info->mode_info[target_mode].trim_width;
    // CMR_LOGI("i %d, height %ld, width %ld, last_mode %ld, target_mode %ld",
    // i, height, width, last_mode, target_mode);
    for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
        if (SENSOR_MODE_MAX != sensor_info->mode_info[i].mode) {
            cmr_u32 tmpwidth = 0, tmpheight = 0;
            tmpheight = sensor_info->mode_info[i].trim_height;
            tmpwidth = sensor_info->mode_info[i].trim_width;
            // CMR_LOGV("i: %ld, sensor size tmpheight = %d, tmpwidth = %d", i,
            // tmpheight, tmpwidth);
            if (tmpheight >= maxheight && tmpwidth >= maxwidth) {
                maxheight = tmpheight;
                maxwidth = tmpwidth;
            }
        }
    }
    if (height == maxheight && (width == maxwidth)) {
        handle->prev_cxt[camera_id].sensor_size_mode = SENSOR_SIZE_FULL;
    } else {
        handle->prev_cxt[camera_id].sensor_size_mode = SENSOR_SIZE_BINNING;
    }

    CMR_LOGI("target_mode %d, sensor_size_mode %d", target_mode,
             handle->prev_cxt[camera_id].sensor_size_mode);
    *work_mode = target_mode;

    return ret;
}
cmr_int prev_get_sn_capture_mode(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct sensor_exp_info *sensor_info,
                                 struct img_size *target_size,
                                 cmr_uint *work_mode) {
    cmr_int ret = CMR_CAMERA_FAIL;
    cmr_u32 width = 0, height = 0, i;
    cmr_u32 maxwidth = 0, maxheight = 0;
    cmr_u32 search_width;
    cmr_u32 search_height;
    cmr_u32 target_mode = SENSOR_MODE_PREVIEW_ONE;
    cmr_u32 last_mode = SENSOR_MODE_PREVIEW_ONE;
    struct sensor_mode_fps_tag fps_info;
    char value[PROPERTY_VALUE_MAX];
    cmr_u32 is_raw_capture = 0;
    cmr_u32 is_3D_video = 0;
    struct camera_context *cxt = (struct camera_context *)handle->oem_handle;

    if (!sensor_info) {
        CMR_LOGE("sn info is null!");
        return CMR_CAMERA_FAIL;
    }

    if (cxt->is_multi_mode == MODE_3D_VIDEO) {
        is_3D_video = 1;
    }

    if (1 == is_3D_video) {
        search_width = sensor_info->source_width_max / 2;
        search_height = sensor_info->source_height_max / 2;
    } else {
        search_width = target_size->width;
        search_height = target_size->height;
    }

    property_get("persist.sys.camera.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }

    CMR_LOGI("search_height = %d, search_width = %d", search_height,
             search_width);
    for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
        if (SENSOR_MODE_MAX != sensor_info->mode_info[i].mode) {
            height = sensor_info->mode_info[i].trim_height;
            width = sensor_info->mode_info[i].trim_width;
            CMR_LOGI("height = %d, width = %d", height, width);
            height = CAMERA_ALIGNED_16(height);
            if (search_height <= height && search_width <= width) {
                if (is_raw_capture == 0) {
                    /* dont choose high fps setting for no-slowmotion */
                    ret = handle->ops.get_sensor_fps_info(
                        handle->oem_handle, camera_id, i, &fps_info);
                    CMR_LOGV("mode=%d, is_high_fps=%d", i,
                             fps_info.is_high_fps);
                    if (fps_info.is_high_fps) {
                        CMR_LOGD("dont choose high fps setting");
                        continue;
                    }
                }
                target_mode = i;
                ret = CMR_CAMERA_SUCCESS;
                break;
            } else {
                last_mode = i;
            }
        }
    }

    if (i == SENSOR_MODE_MAX) {
        CMR_LOGI("can't find the right mode, use last available mode %d",
                 last_mode);
        i = last_mode;
        target_mode = last_mode;
        ret = CMR_CAMERA_SUCCESS;
    }

    height = sensor_info->mode_info[target_mode].trim_height;
    width = sensor_info->mode_info[target_mode].trim_width;
    if (handle->prev_cxt[camera_id].sensor_size_mode == SENSOR_SIZE_FULL) {
        CMR_LOGV("preview mode selected full sensor mode");
    } else if (handle->prev_cxt[camera_id].sensor_size_mode ==
               SENSOR_SIZE_BINNING) {
        for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
            if (SENSOR_MODE_MAX != sensor_info->mode_info[i].mode) {
                cmr_u32 tmpwidth = 0, tmpheight = 0;
                tmpheight = sensor_info->mode_info[i].trim_height;
                tmpwidth = sensor_info->mode_info[i].trim_width;
                if (tmpheight >= maxheight && tmpwidth >= maxwidth) {
                    maxheight = tmpheight;
                    maxwidth = tmpwidth;
                }
            }
        }
        if (height == maxheight && width == maxwidth) {
            handle->prev_cxt[camera_id].sensor_size_mode = SENSOR_SIZE_FULL;
        } else {
            handle->prev_cxt[camera_id].sensor_size_mode = SENSOR_SIZE_BINNING;
        }
    }

    CMR_LOGI("mode %d, width %d height %d, sensor_size_mode %d", target_mode,
             sensor_info->mode_info[target_mode].trim_width,
             sensor_info->mode_info[target_mode].trim_height,
             handle->prev_cxt[camera_id].sensor_size_mode);

    *work_mode = target_mode;

    return ret;
}

cmr_int prev_get_sn_inf(struct prev_handle *handle, cmr_u32 camera_id,
                        cmr_u32 frm_deci, struct sensor_if *sn_if) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    sensor_info = &handle->prev_cxt[camera_id].sensor_info;

    if (IMG_DATA_TYPE_RAW == sensor_info->image_format) {
        sn_if->img_fmt = GRAB_SENSOR_FORMAT_RAWRGB;
        CMR_LOGI("this is RAW sensor");
    } else {
        sn_if->img_fmt = GRAB_SENSOR_FORMAT_YUV;
    }

    if (SENSOR_INTERFACE_TYPE_CSI2 == sensor_info->sn_interface.type) {
        sn_if->if_type = 1;
        sn_if->if_spec.mipi.lane_num = sensor_info->sn_interface.bus_width;
        sn_if->if_spec.mipi.bits_per_pxl =
            sensor_info->sn_interface.pixel_width;
        sn_if->if_spec.mipi.is_loose = sensor_info->sn_interface.is_loose;
        sn_if->if_spec.mipi.use_href = 0;
        CMR_LOGI("lane_num %d, bits_per_pxl %d, is_loose %d",
                 sn_if->if_spec.mipi.lane_num, sn_if->if_spec.mipi.bits_per_pxl,
                 sn_if->if_spec.mipi.is_loose);
    } else {
        sn_if->if_type = 0;
        sn_if->if_spec.ccir.v_sync_pol = sensor_info->vsync_polarity;
        sn_if->if_spec.ccir.h_sync_pol = sensor_info->hsync_polarity;
        sn_if->if_spec.ccir.pclk_pol = sensor_info->pclk_polarity;
    }

    sn_if->img_ptn = sensor_info->image_pattern;
    sn_if->frm_deci = frm_deci;

exit:
    return ret;
}

cmr_int prev_get_cap_max_size(struct prev_handle *handle, cmr_u32 camera_id,
                              struct sensor_mode_info *sn_mode,
                              struct img_size *max_size) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 zoom_proc_mode = ZOOM_BY_CAP;
    cmr_u32 original_fmt = IMG_DATA_TYPE_YUV420;
    cmr_u32 need_isp = 0;
    struct img_rect img_rc;
    struct img_size img_sz;
    struct img_size trim_sz;
    cmr_u32 tmp_width;
    cmr_u32 isp_width_limit = 0;
    cmr_u32 sc_factor = 0, sc_capability = 0, sc_threshold = 0;
    struct cmr_zoom_param *zoom_param = NULL;
    struct img_size *cap_size = NULL;
    cmr_int zoom_post_proc = 0;

    img_sz.width = max_size->width;
    img_sz.height = max_size->height;
    CMR_LOGI("camera_id %d", camera_id);
    isp_width_limit = handle->prev_cxt[camera_id].prev_param.isp_width_limit;
    zoom_param = &handle->prev_cxt[camera_id].prev_param.zoom_setting;
    cap_size = &handle->prev_cxt[camera_id].actual_pic_size;

    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);
    if (IMG_DATA_TYPE_YUV422 == sn_mode->image_format) {
        original_fmt = IMG_DATA_TYPE_YUV420;
        zoom_proc_mode = zoom_post_proc;
    } else if (IMG_DATA_TYPE_RAW == sn_mode->image_format) {
        if (sn_mode->trim_width <= isp_width_limit) {
            CMR_LOGI("Need ISP to work at video mode");
            need_isp = 1;
            original_fmt = IMG_DATA_TYPE_YUV420;
            zoom_proc_mode = zoom_post_proc;
        } else {
            CMR_LOGI("Need to process raw data");
            need_isp = 0;
            original_fmt = IMG_DATA_TYPE_RAW;
            zoom_proc_mode = ZOOM_POST_PROCESS;
        }
    } else if (IMG_DATA_TYPE_JPEG == sn_mode->image_format) {
        original_fmt = IMG_DATA_TYPE_JPEG;
        zoom_proc_mode = ZOOM_POST_PROCESS;
    } else {
        CMR_LOGE("Unsupported sensor format %d for capture",
                 sn_mode->image_format);
        ret = -CMR_CAMERA_INVALID_FORMAT;
        goto exit;
    }

    img_rc.start_x = sn_mode->trim_start_x;
    img_rc.start_y = sn_mode->trim_start_y;
    img_rc.width = sn_mode->trim_width;
    img_rc.height = sn_mode->trim_height;

    trim_sz.width = sn_mode->trim_width;
    trim_sz.height = sn_mode->trim_height;
    CMR_LOGI("rect %d %d %d %d", img_rc.start_x, img_rc.start_y, img_rc.width,
             img_rc.height);
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&img_rc, zoom_param->zoom_level, &trim_sz);
        if (ret) {
            CMR_LOGE("Failed to get trimming window for %ld zoom level ",
                     zoom_param->zoom_level);
            goto exit;
        }
    } else {
        ret = camera_get_trim_rect2(
            &img_rc, zoom_param->zoom_info.zoom_ratio,
            zoom_param->zoom_info.capture_aspect_ratio, sn_mode->trim_width,
            sn_mode->trim_height,
            handle->prev_cxt[camera_id].prev_param.cap_rot);
        if (ret) {
            CMR_LOGE("Failed to get trimming window");
            goto exit;
        }
    }
    CMR_LOGI("after rect %d %d %d %d", img_rc.start_x, img_rc.start_y,
             img_rc.width, img_rc.height);

    if (ZOOM_POST_PROCESS == zoom_proc_mode) {
        if (zoom_post_proc) {
            if ((max_size->width < sn_mode->trim_width) ||
                (max_size->height < sn_mode->trim_height)) {
                max_size->width = sn_mode->trim_width;
                max_size->height = sn_mode->trim_height;
            }
        } else {
            if (max_size->width < sn_mode->trim_width) {
                max_size->width = sn_mode->trim_width;
                max_size->height = sn_mode->trim_height;
            }
        }
    } else {
        if (handle->ops.channel_scale_capability) {
            ret = handle->ops.channel_scale_capability(
                handle->oem_handle, &sc_capability, &sc_factor, &sc_threshold);
            if (ret) {
                CMR_LOGE("ops return %ld", ret);
                goto exit;
            }
        } else {
            CMR_LOGE("ops is null");
            goto exit;
        }

        tmp_width = (cmr_u32)(sc_factor * img_rc.width);
        if ((img_rc.width >= cap_size->width ||
             cap_size->width <= sc_threshold) &&
            ZOOM_BY_CAP == zoom_proc_mode) {
            /*if the out size is smaller than the in size, try to use scaler on
             * the fly*/
            if (cap_size->width > tmp_width) {
                if (tmp_width > sc_capability) {
                    img_sz.width = sc_capability;
                } else {
                    img_sz.width = tmp_width;
                }
                img_sz.height = (cmr_u32)(img_rc.height * sc_factor);
            } else {
                /*just use scaler on the fly*/
                img_sz.width = cap_size->width;
                img_sz.height = cap_size->height;
            }
        } else {
            /*if the out size is larger than the in size*/
            img_sz.width = img_rc.width;
            img_sz.height = img_rc.height;
        }

        if (!(max_size->width == img_sz.height &&
              max_size->height == img_sz.width)) {
            max_size->width = MAX(max_size->width, img_sz.width);
            max_size->height = MAX(max_size->height, img_sz.height);
        }
    }

    CMR_LOGI("max_size width %d, height %d", max_size->width, max_size->height);

exit:
    return ret;
}

cmr_int prev_get_frm_index(struct img_frm *frame, struct frm_info *data) {
    cmr_int i;

    for (i = 0; i < PREV_FRM_CNT; i++) {
        if (data->fd == (cmr_u32)(frame + i)->fd) {
            break;
        }
    }
    CMR_LOGV("frm id %ld", i);

    return i;
}

cmr_int prev_zsl_get_frm_index(struct img_frm *frame, struct frm_info *data) {
    cmr_int i;

    for (i = 0; i < ZSL_FRM_CNT; i++) {
        if (data->fd == (cmr_u32)(frame + i)->fd) {
            break;
        }
    }
    CMR_LOGV("frm id %ld", i);

    return i;
}

#ifdef CONFIG_Y_IMG_TO_ISP
cmr_int prev_y_info_copy_to_isp(struct prev_handle *handle, cmr_uint camera_id,
                                struct frm_info *info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint i = 0;
    cmr_uint index = 0xff;
    struct prev_context *prev_cxt = NULL;
    struct yimg_info y_info = {0};
    struct isp_yimg_info isp_yimg = {0};

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!handle->ops.get_isp_yimg || !handle->ops.set_preview_yimg) {
        CMR_LOGE("ops null");
        goto exit;
    }
    /* get isp user buffer status */
    ret = handle->ops.get_isp_yimg(handle->oem_handle, camera_id, &isp_yimg);
    if (ret) {
        CMR_LOGE("get isp yimg error");
        goto exit;
    }

    for (i = 0; i < 2; i++) {
        if (0 == isp_yimg.lock[i]) {
            index = i;
            break;
        }
    }
    if (0xff == index) {
        CMR_LOGE("two buffer all in lock");
        goto exit;
    }
    CMR_LOGI("index %ld", index);
    /* set buffer to isp */
    cmr_bzero(prev_cxt->prev_virt_y_addr_array[index],
              prev_cxt->prev_mem_y_size);
    y_info.camera_id = camera_id;
    y_info.y_size = prev_cxt->prev_mem_y_size;
    y_info.sec = info->sec;
    y_info.usec = info->usec;
    memcpy(prev_cxt->prev_virt_y_addr_array[index], info->yaddr_vir,
           prev_cxt->prev_mem_y_size);
    y_info.ready[index] = 1;
    for (i = 0; i < 2; i++)
        y_info.y_addr[i] = prev_cxt->prev_virt_y_addr_array[i];
    ret = handle->ops.set_preview_yimg(handle->oem_handle, camera_id, &y_info);
    if (ret)
        CMR_LOGE("set_preview_yimg err %d", ret);

exit:
    return ret;
}
#endif

static cmr_int prev_yuv_info_copy_to_isp(struct prev_handle *handle,
                                         cmr_uint camera_id,
                                         struct frm_info *info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint i = 0;
    cmr_uint index = 0xff;
    struct prev_context *prev_cxt = NULL;
    struct yuv_info_t yuv_info;
    cmr_u8 *info_yaddr = NULL;
    cmr_u8 *info_uaddr = NULL;
    cmr_u8 *uv_addr = NULL;
    cmr_uint uv_size = 0;

    prev_cxt = &handle->prev_cxt[camera_id];

    if (0 != prev_cxt->prev_frm_cnt % 30)
        goto exit;

    cmr_bzero(&yuv_info, sizeof(struct yuv_info_t));
    /* set buffer to isp */
    yuv_info.camera_id = camera_id;
    yuv_info.yuv_addr = (cmr_u8 *)prev_cxt->prev_virt_yuv_addr;
    yuv_info.width = prev_cxt->actual_prev_size.width;
    yuv_info.height = prev_cxt->actual_prev_size.height;
    info_yaddr = (cmr_u8 *)((cmr_uint)info->yaddr_vir);
    info_uaddr = (cmr_u8 *)((cmr_uint)info->uaddr_vir);
    memcpy(yuv_info.yuv_addr, info_yaddr, yuv_info.width * yuv_info.height);
    uv_addr = yuv_info.yuv_addr + yuv_info.width * yuv_info.height;
    uv_size = yuv_info.width * yuv_info.height / 2;
    memcpy(uv_addr, info_uaddr, uv_size);

    ret = handle->ops.set_preview_yuv(handle->oem_handle, camera_id, &yuv_info);
    if (ret)
        CMR_LOGE("set_preview_yimg err %ld", ret);

exit:
    return ret;
}

// cmr_uint g_preview_frame_dump_cnt = 0;
cmr_int prev_construct_frame(struct prev_handle *handle, cmr_u32 camera_id,
                             struct frm_info *info,
                             struct camera_frame_type *frame_type) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 frm_id = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 prev_chn_id = 0;
    cmr_u32 cap_chn_id = 0;
    cmr_u32 prev_rot = 0;
    struct prev_context *prev_cxt = NULL;
    struct img_frm *frm_ptr = NULL;
    cmr_s64 ae_time = 0;

    if (!handle || !frame_type || !info) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p, 0x%p", handle, frame_type, info);
        ret = CMR_CAMERA_FAIL;
        return ret;
    }

    prev_chn_id = handle->prev_cxt[camera_id].prev_channel_id;
    cap_chn_id = handle->prev_cxt[camera_id].cap_channel_id;
    prev_rot = handle->prev_cxt[camera_id].prev_param.prev_rot;
    prev_cxt = &handle->prev_cxt[camera_id];
    ae_time = prev_cxt->ae_time;
    prev_cxt->sensor_datatype_cnt = 0;
    prev_cxt->pdaf_frm_cnt = prev_cxt->prev_frm_cnt;
    if (prev_chn_id == info->channel_id) {
        if (prev_rot) {
            /*prev_num = prev_cxt->prev_mem_num - PREV_ROT_FRM_CNT;
            frm_id   = prev_cxt->prev_rot_index % PREV_ROT_FRM_CNT;
            frm_ptr  = &prev_cxt->prev_rot_frm[frm_id];

            frame_type->buf_id       = frm_id;
            frame_type->order_buf_id = frm_id + prev_num;
            frame_type->y_vir_addr   =
            prev_cxt->prev_rot_frm[frm_id].addr_vir.addr_y;
            frame_type->y_phy_addr   =
            prev_cxt->prev_rot_frm[frm_id].addr_phy.addr_y;

            CMR_LOGE("[prev_rot] lock %d", frm_id);
            prev_cxt->prev_rot_frm_is_lock[frm_id] = 1;*/

            info->fd = prev_cxt->prev_frm[0].fd;
            info->yaddr = prev_cxt->prev_frm[0].addr_phy.addr_y;
            info->uaddr = prev_cxt->prev_frm[0].addr_phy.addr_u;
            info->vaddr = prev_cxt->prev_frm[0].addr_phy.addr_v;
            info->yaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_y;
            info->uaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_u;
            info->vaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_v;
        }
        // frm_id = info->frame_id - CMR_PREV_ID_BASE;
        frm_id = prev_get_frm_index(prev_cxt->prev_frm, info);
        frm_ptr = &prev_cxt->prev_frm[frm_id];

        frame_type->buf_id = frm_id;
        frame_type->order_buf_id = frm_id;
        frame_type->y_vir_addr = prev_cxt->prev_frm[frm_id].addr_vir.addr_y;
        frame_type->y_phy_addr = prev_cxt->prev_frm[frm_id].addr_phy.addr_y;
        frame_type->fd = prev_cxt->prev_frm[frm_id].fd;

        frame_type->width = prev_cxt->prev_param.preview_size.width;
        frame_type->height = prev_cxt->prev_param.preview_size.height;
        frame_type->timestamp = info->sec * 1000000000LL + info->usec * 1000;
        frame_type->monoboottime = info->monoboottime;
        frame_type->zoom_ratio =
            prev_cxt->prev_param.zoom_setting.zoom_info.zoom_ratio;
        frame_type->ae_time = ae_time;
        CMR_LOGV("ae_time: %" PRId64 ", zoom_ratio: %f", frame_type->ae_time,
                 frame_type->zoom_ratio);
        frame_type->vcm_step = (cmr_u32)prev_cxt->vcm_step;
        CMR_LOGV("vcm_step: %d", frame_type->vcm_step);
        frame_type->type = PREVIEW_FRAME;
        CMR_LOGV("timestamp %" PRId64, frame_type->timestamp);
        if (prev_cxt->prev_param.is_support_fd &&
            prev_cxt->prev_param.is_fd_on) {
            prev_fd_send_data(handle, camera_id, frm_ptr);
        }

        char value[PROPERTY_VALUE_MAX];
        property_get("debug.camera.dump.frame", value, "null");
        if (!strcmp(value, "preview")) {
            // if (g_preview_frame_dump_cnt < 10)
            {
                camera_save_yuv_to_file(prev_cxt->prev_frm_cnt,
                                        IMG_DATA_TYPE_YUV420, frame_type->width,
                                        frame_type->height,
                                        &prev_cxt->prev_frm[frm_id].addr_vir);
                //	g_preview_frame_dump_cnt++;
            }
        }

#if 1
        switch (prev_cxt->prev_param.sensor_datatype) {
        case SENSOR_REAL_DEPTH_ENABLE:
            prev_cxt->preview_bakcup_timestamp = frame_type->timestamp;
            prev_cxt->preview_bakcup_frm = frm_ptr;
            prev_cxt->preview_bakcup_data = info;
            prev_cxt->preview_bakcup_frame_type = frame_type;
            CMR_LOGV(" frm_ptr %p addr_vir.addr_y 0x%lx  frame_type->fd 0x%x",
                     frm_ptr, frm_ptr->addr_vir.addr_y, frame_type->fd);

            break;
        case SENSOR_EMBEDDED_INFO_ENABLE:
            break;
        case SENSOR_DATATYPE_PDAF_ENABLE:
            break;
        default:
            CMR_LOGV("not sensor_datatype mode, channel id %d, frame id %d",
                     info->channel_id, info->frame_id);
            break;
        }
#endif
#ifdef CONFIG_Y_IMG_TO_ISP
        prev_y_info_copy_to_isp(handle, camera_id, info);
#endif
#ifdef YUV_TO_ISP
        prev_yuv_info_copy_to_isp(handle, camera_id, info);
#endif
    } else {
        CMR_LOGE("ignored, channel id %d, frame id %d", info->channel_id,
                 info->frame_id);
    }

    ATRACE_END();
    return ret;
}

// cmr_uint g_video_frame_dump_cnt = 0;
cmr_int prev_construct_video_frame(struct prev_handle *handle,
                                   cmr_u32 camera_id, struct frm_info *info,
                                   struct camera_frame_type *frame_type) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 frm_id = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 video_chn_id = 0;
    cmr_u32 cap_chn_id = 0;
    cmr_u32 prev_rot = 0;
    struct prev_context *prev_cxt = NULL;
    struct img_frm *frm_ptr = NULL;
    cmr_s64 ae_time = 0;

    if (!handle || !frame_type || !info) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p, 0x%p", handle, frame_type, info);
        ret = CMR_CAMERA_FAIL;
        return ret;
    }

    video_chn_id = handle->prev_cxt[camera_id].video_channel_id;
    cap_chn_id = handle->prev_cxt[camera_id].cap_channel_id;
    prev_rot = handle->prev_cxt[camera_id].prev_param.prev_rot;
    prev_cxt = &handle->prev_cxt[camera_id];
    ae_time = prev_cxt->ae_time;

    if (video_chn_id == info->channel_id) {
        if (prev_rot) {
            /*prev_num = prev_cxt->video_mem_num - PREV_ROT_FRM_CNT;
            frm_id   = prev_cxt->video_rot_index % PREV_ROT_FRM_CNT;
            frm_ptr  = &prev_cxt->video_rot_frm[frm_id];

            frame_type->buf_id       = frm_id;
            frame_type->order_buf_id = frm_id + prev_num;
            frame_type->y_vir_addr   =
            prev_cxt->video_rot_frm[frm_id].addr_vir.addr_y;
            frame_type->y_phy_addr   =
            prev_cxt->video_rot_frm[frm_id].addr_phy.addr_y;

            CMR_LOGE("[prev_rot] lock %d", frm_id);
            prev_cxt->video_rot_frm_is_lock[frm_id] = 1;*/

            info->fd = prev_cxt->video_frm[0].fd;
            info->yaddr = prev_cxt->video_frm[0].addr_phy.addr_y;
            info->uaddr = prev_cxt->video_frm[0].addr_phy.addr_u;
            info->vaddr = prev_cxt->video_frm[0].addr_phy.addr_v;
            info->yaddr_vir = prev_cxt->video_frm[0].addr_vir.addr_y;
            info->uaddr_vir = prev_cxt->video_frm[0].addr_vir.addr_u;
            info->vaddr_vir = prev_cxt->video_frm[0].addr_vir.addr_v;
        }
        // frm_id = info->frame_id - CMR_PREV_ID_BASE;
        frm_id = prev_get_frm_index(prev_cxt->video_frm, info);
        frm_ptr = &prev_cxt->video_frm[frm_id];

        frame_type->buf_id = frm_id;
        frame_type->order_buf_id = frm_id;
        frame_type->y_vir_addr = prev_cxt->video_frm[frm_id].addr_vir.addr_y;
        frame_type->uv_vir_addr = prev_cxt->video_frm[frm_id].addr_vir.addr_u;
        frame_type->fd = prev_cxt->video_frm[frm_id].fd;
        frame_type->y_phy_addr = prev_cxt->video_frm[frm_id].addr_phy.addr_y;
        frame_type->uv_phy_addr = prev_cxt->video_frm[frm_id].addr_phy.addr_u;
        frame_type->width = prev_cxt->prev_param.video_size.width;
        frame_type->height = prev_cxt->prev_param.video_size.height;
        frame_type->timestamp = info->sec * 1000000000LL + info->usec * 1000;
        frame_type->monoboottime = info->monoboottime;
        frame_type->zoom_ratio =
            prev_cxt->prev_param.zoom_setting.zoom_info.zoom_ratio;
        frame_type->ae_time = ae_time;
        CMR_LOGI("ae_time: %" PRId64 ", zoom_ratio: %f", frame_type->ae_time,
                 frame_type->zoom_ratio);
        frame_type->type = PREVIEW_VIDEO_FRAME;

        char value[PROPERTY_VALUE_MAX];
        property_get("debug.camera.dump.frame", value, "null");
        if (!strcmp(value, "video")) {
            // if (g_video_frame_dump_cnt < 10)
            {
                camera_save_yuv_to_file(prev_cxt->prev_frm_cnt,
                                        IMG_DATA_TYPE_YUV420, frame_type->width,
                                        frame_type->height,
                                        &prev_cxt->video_frm[frm_id].addr_vir);
                //	g_video_frame_dump_cnt++;
            }
        }

    } else {
        CMR_LOGE("ignored, channel id %d, frame id %d", info->channel_id,
                 info->frame_id);
    }

    return ret;
}

// cmr_uint g_zsl_frame_dump_cnt = 0;
cmr_int prev_construct_zsl_frame(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct frm_info *info,
                                 struct camera_frame_type *frame_type) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 frm_id = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 prev_chn_id = 0;
    cmr_u32 cap_chn_id = 0;
    cmr_u32 prev_rot = 0;
    struct prev_context *prev_cxt = NULL;
    struct img_frm *frm_ptr = NULL;
    cmr_int zoom_post_proc = 0;

    if (!handle || !frame_type || !info) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p, 0x%p", handle, frame_type, info);
        ret = CMR_CAMERA_FAIL;
        return ret;
    }

    prev_chn_id = handle->prev_cxt[camera_id].prev_channel_id;
    cap_chn_id = handle->prev_cxt[camera_id].cap_channel_id;
    prev_rot = handle->prev_cxt[camera_id].prev_param.cap_rot;
    prev_cxt = &handle->prev_cxt[camera_id];
    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);
    if (cap_chn_id == info->channel_id) {
        if (prev_rot) {
            info->fd = prev_cxt->cap_zsl_frm[0].fd;
            info->yaddr = prev_cxt->cap_zsl_frm[0].addr_phy.addr_y;
            info->uaddr = prev_cxt->cap_zsl_frm[0].addr_phy.addr_u;
            info->vaddr = prev_cxt->cap_zsl_frm[0].addr_phy.addr_v;
            info->yaddr_vir = prev_cxt->cap_zsl_frm[0].addr_vir.addr_y;
            info->uaddr_vir = prev_cxt->cap_zsl_frm[0].addr_vir.addr_u;
            info->vaddr_vir = prev_cxt->cap_zsl_frm[0].addr_vir.addr_v;
        }
        frm_id = prev_zsl_get_frm_index(prev_cxt->cap_zsl_frm, info);
        frm_ptr = &prev_cxt->cap_zsl_frm[frm_id];

        frame_type->buf_id = frm_id;
        frame_type->order_buf_id = frm_id;
        frame_type->y_vir_addr = prev_cxt->cap_zsl_frm[frm_id].addr_vir.addr_y;
        frame_type->uv_vir_addr = prev_cxt->cap_zsl_frm[frm_id].addr_vir.addr_u;
        frame_type->fd = prev_cxt->cap_zsl_frm[frm_id].fd;
        frame_type->y_phy_addr = prev_cxt->cap_zsl_frm[frm_id].addr_phy.addr_y;
        frame_type->uv_phy_addr = prev_cxt->cap_zsl_frm[frm_id].addr_phy.addr_u;
        frame_type->width = prev_cxt->cap_zsl_frm[frm_id].size.width;
        frame_type->height = prev_cxt->cap_zsl_frm[frm_id].size.height;
        frame_type->timestamp = info->sec * 1000000000LL + info->usec * 1000;
        frame_type->type = PREVIEW_ZSL_FRAME;
        CMR_LOGV("timestamp=%" PRId64 ", width=%d, height=%d, fd=0x%x",
                 frame_type->timestamp, frame_type->width, frame_type->height,
                 frame_type->fd);

        char value[PROPERTY_VALUE_MAX];
        property_get("debug.camera.dump.frame", value, "null");
        if (!strcmp(value, "zsl")) {
            // if (g_zsl_frame_dump_cnt < 10)
            {
                camera_save_yuv_to_file(
                    prev_cxt->prev_frm_cnt, IMG_DATA_TYPE_YUV420,
                    frame_type->width, frame_type->height,
                    &prev_cxt->cap_zsl_frm[frm_id].addr_vir);
                //	g_zsl_frame_dump_cnt++;
            }
        }

    } else {
        CMR_LOGE("ignored, channel id %d, frame id %d", info->channel_id,
                 info->frame_id);
    }

    return ret;
}

cmr_int prev_set_param_internal(struct prev_handle *handle, cmr_u32 camera_id,
                                cmr_u32 is_restart,
                                struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!out_param_ptr) {
        CMR_LOGE("out_param_ptr is null");
        goto exit;
    }

    /*cmr_bzero(out_param_ptr, sizeof(struct preview_out_param));*/

    handle->prev_cxt[camera_id].camera_id = camera_id;
    handle->prev_cxt[camera_id].out_ret_val = CMR_CAMERA_SUCCESS;

    CMR_LOGI("camera_id %ld, preview_eb %d, snapshot_eb %d, video_eb %d, "
             "tool_eb %d, prev_status %ld",
             handle->prev_cxt[camera_id].camera_id,
             handle->prev_cxt[camera_id].prev_param.preview_eb,
             handle->prev_cxt[camera_id].prev_param.snapshot_eb,
             handle->prev_cxt[camera_id].prev_param.video_eb,
             handle->prev_cxt[camera_id].prev_param.tool_eb,
             handle->prev_cxt[camera_id].prev_status);

    CMR_LOGI("preview_size %d %d, picture_size %d %d, video_size %d %d",
             handle->prev_cxt[camera_id].prev_param.preview_size.width,
             handle->prev_cxt[camera_id].prev_param.preview_size.height,
             handle->prev_cxt[camera_id].prev_param.picture_size.width,
             handle->prev_cxt[camera_id].prev_param.picture_size.height,
             handle->prev_cxt[camera_id].prev_param.video_size.width,
             handle->prev_cxt[camera_id].prev_param.video_size.height);

    ret = prev_get_sensor_mode(handle, camera_id);
    if (ret) {
        CMR_LOGE("get sensor mode failed");
        goto exit;
    }

    if (handle->prev_cxt[camera_id].prev_param.preview_eb) {
        ret = prev_pre_set(handle, camera_id);
        if (ret) {
            CMR_LOGE("pre set failed");
            goto exit;
        }

        ret = prev_set_prev_param(handle, camera_id, is_restart, out_param_ptr);
        if (ret) {
            CMR_LOGE("set prev param failed");
            goto exit;
        }
    }

    if (handle->prev_cxt[camera_id].prev_param.video_eb) {
        ret = prev_pre_set(handle, camera_id);
        if (ret) {
            CMR_LOGE("video set failed");
            goto exit;
        }

        ret =
            prev_set_video_param(handle, camera_id, is_restart, out_param_ptr);
        if (ret) {
            CMR_LOGE("set video param failed");
            goto exit;
        }
    }

    if (handle->prev_cxt[camera_id].prev_param.pdaf_mode ==
        SENSOR_PDAF_TYPE3_ENABLE) {
        ret = prev_set_pdaf_raw_param(handle, camera_id, is_restart,
                                      out_param_ptr);
        if (ret) {
            CMR_LOGE("set pdaf param failed");
            goto exit;
        }
    }

    if (handle->prev_cxt[camera_id].prev_param.sensor_datatype >
        SENSOR_DATATYPE_DISABLED) {
        ret = prev_set_sensor_datatype_param(handle, camera_id, is_restart,
                                             out_param_ptr);
        if (ret) {
            CMR_LOGE("set sensosr_datatype param failed");
            goto exit;
        }
    }

    if (handle->prev_cxt[camera_id].prev_param.pdaf_mode ==
        SENSOR_PDAF_TYPE2_ENABLE) {
        char value[PROPERTY_VALUE_MAX];
        property_get("persist.sys.camera.raw.mode", value, "jpeg");
        if (!strcmp(value, "raw")) {
            ret = prev_set_pdaf_param(handle, camera_id, out_param_ptr);
            if (ret) {
                CMR_LOGE("set sensosr_datatype param failed");
                goto exit;
            }
        }
    }
    if (handle->prev_cxt[camera_id].prev_param.snapshot_eb) {
        if (handle->prev_cxt[camera_id].prev_param.tool_eb) {
            // raw capture
            ret = prev_set_cap_param_raw(handle, camera_id, is_restart,
                                         out_param_ptr);
            if (ret) {
                CMR_LOGE("prev_set_cap_param_raw failed");
                goto exit;
            }
        } else if (handle->prev_cxt[camera_id].prev_param.video_snapshot_type ==
                   VIDEO_SNAPSHOT_VIDEO) {
            // copy video stream for video snapshot
            ret = prev_set_videocap_param(handle, camera_id, is_restart, 0,
                                          out_param_ptr);
            if (ret) {
                CMR_LOGE("prev_set_videocap_param failed");
                goto exit;
            }
        } else {
            if (!handle->prev_cxt[camera_id].prev_param.preview_eb) {
                ret = prev_pre_set(handle, camera_id);
                if (ret) {
                    CMR_LOGE("pre set failed");
                    goto exit;
                }
            }
            ret = prev_set_cap_param(handle, camera_id, is_restart, 0,
                                     out_param_ptr);
            if (ret) {
                CMR_LOGE("prev_set_cap_param failed");
                goto exit;
            }
        }
    }

exit:
    CMR_LOGD(" out, ret %ld", ret);
    handle->prev_cxt[camera_id].out_ret_val = ret;
    ATRACE_END();
    return ret;
}

static cmr_int prev_get_matched_burstmode_lv_size(struct img_size sensor_size,
                                                  struct img_size dst_img_size,
                                                  struct img_size *lvsize,
                                                  struct img_size *video_size) {
    int i, last = -1;
    cmr_u32 min_rate = 0x7fffffff, del_rate;

    struct {
        struct img_size el;
        struct img_size rate;
    } list[] = {
        {{960, 720}, {12, 9}},
        {{1920, 1080}, {16, 9}},
        {{1920, 1440}, {12, 9}},
#if 0 // VIDEO_USE
			{{4200, 2362}, {16, 9}},
			{{4200, 3158}, {12, 9}},
#endif
        {{0, 0}, {0, 0}},
    };

    CMR_LOGD("sensor_size %d %d, dst_img_size %d %d", sensor_size.width,
             sensor_size.height, dst_img_size.width, dst_img_size.height);

#if 0

	for (i = 0; list[i].el.width != 0; i++) {
		if (sensor_size.width < list[i].el.width || sensor_size.height < list[i].el.height){
			break;
		}
		if((list[i].rate.width * sensor_size.height) == (list[i].rate.height * sensor_size.width)) {
			last = i;
			min_rate = 0;
		} else if((list[i].rate.width * sensor_size.height) > (list[i].rate.height * sensor_size.width)) {
			del_rate = (list[i].rate.width * sensor_size.height) - (list[i].rate.height * sensor_size.width);
			CMR_LOGI("i %d del_rate %u", i, del_rate);
			if(min_rate >= del_rate) {
				last = i;
				min_rate = del_rate;
				CMR_LOGI("min_rate %d", min_rate);
			}
		} else {
			del_rate = (list[i].rate.height *sensor_size.width) - (list[i].rate.width * sensor_size.height);
			CMR_LOGI("i %d del_rate %u", i, del_rate);
			if(min_rate >= del_rate) {
				last = i;
				min_rate = del_rate;
				CMR_LOGI("min_rate %d", min_rate);
			}
		}
	}
	CMR_LOGI("select %d", last);

	if(last == -1) {
		cmr_u32 min_w = 0x7fffffff, min_h = 0x7fffffff;
		cmr_u32 del_w , del_h;

		for (i = 0; list[i].el.width != 0; i++) {
			del_w = list[i].el.width > dst_img_size.width ? list[i].el.width - dst_img_size.width:
				dst_img_size.width - list[i].el.width;
			del_h =	list[i].el.height > dst_img_size.height ? list[i].el.height - dst_img_size.height:
					dst_img_size.height - list[i].el.height;

			if(del_w <= min_w && del_h <= min_h) {
				min_w = del_w;
				min_h = del_h;
				last = i;
			} else if (sensor_size.width < list[i].el.width || sensor_size.height < list[i].el.height){
				break;
			}
		}

		if(last == -1)
			last = 0;
		CMR_LOGI("select %d", last);
	}
	video_size->width = list[last].el.width;
	video_size->height = list[last].el.height;
	if(list[last].rate.width == 12) {
		lvsize->width = 960;
		lvsize->height = 720;
	} else {
		lvsize->width = 1920;
		lvsize->height = 1080;
	}
#//else
	if(sensor_size.width > 4224/*4900 && ((sensor_size.width % 24) == 0)*/) {
		video_size->width = 4224;
		video_size->height = 4224 * sensor_size.height / sensor_size.width;
	} else {
		video_size->width = sensor_size.width;
		video_size->height = sensor_size.height;
	}
	lvsize->width = 960;
	lvsize->height = 720;
#else

    if (dst_img_size.width < sensor_size.width &&
        dst_img_size.height < sensor_size.height) {
        video_size->width = dst_img_size.width;
        video_size->height = dst_img_size.height;

        if (sensor_size.width * 1000 / sensor_size.height >
            video_size->width * 1000 / video_size->height) {
            video_size->width =
                sensor_size.width * video_size->height / sensor_size.height;
            video_size->width = CAMERA_WIDTH(video_size->width);
        } else if (sensor_size.width * 1000 / sensor_size.height <
                   video_size->width * 1000 / video_size->height) {
            video_size->height =
                video_size->width * sensor_size.height / sensor_size.width;
            video_size->height = CAMERA_HEIGHT(video_size->height);
        }
    } else {
        video_size->width = sensor_size.width;
        video_size->height = sensor_size.height;
    }

    lvsize->width = 960;
    lvsize->height = 720;
    CMR_LOGI("video width %d height %d", video_size->width, video_size->height);

#endif
    return 0;
}

cmr_int prev_set_prev_param(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart,
                            struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct video_start_param video_param;
    struct img_data_end endian;
    struct buffer_cfg buf_cfg;
    cmr_u32 i;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    cmr_bzero(&video_param, sizeof(struct video_start_param));
    CMR_LOGI("camera_id %d", camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];

    if (prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb) {
        chn_param.sensor_mode = prev_cxt->cap_mode;
    } else {
        chn_param.sensor_mode = prev_cxt->prev_mode;
    }

    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    cmr_bzero(prev_cxt->prev_rot_frm_is_lock,
              PREV_ROT_FRM_CNT * sizeof(cmr_uint));
    prev_cxt->prev_rot_index = 0;
    prev_cxt->prev_frm_cnt = 0;
    prev_cxt->prev_preflash_skip_en = 0;
    prev_cxt->prev_skip_num = sensor_info->preview_skip_num;
    prev_cxt->skip_mode = IMG_SKIP_HW;

    chn_param.is_lightly = 0;
    chn_param.frm_num = -1;
    if (IMG_DATA_TYPE_RAW == sensor_mode_info->image_format) {
        chn_param.skip_num = 0;
    } else {
        chn_param.skip_num = prev_cxt->prev_skip_num;
    }

    /* in slowmotion, we want do decimation in preview channel,
    bug video buffer and preview buffer is in one request, so we cant
    do decimation for now, otherwise the fps is low */
    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.need_isp = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.preview_fmt;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;

    if (IMG_DATA_TYPE_RAW == sensor_mode_info->image_format) {
        prev_cxt->skip_mode = IMG_SKIP_SW_KER;
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    }

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->actual_prev_size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->actual_prev_size.height;

    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;
    chn_param.cap_inf_cfg.cfg.sence_mode = DCAM_SCENE_MODE_PREVIEW;

    CMR_LOGI("skip_mode %ld, skip_num %ld, image_format %d",
             prev_cxt->skip_mode, prev_cxt->prev_skip_num,
             sensor_mode_info->image_format);

    CMR_LOGI("src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    if (prev_cxt->prev_param.sprd_burstmode_enabled ||
        (!prev_cxt->prev_param.snapshot_eb && !prev_cxt->prev_param.video_eb)) {

        struct img_size sensor_size;
        struct img_size lv_size;
        struct img_size video_size;

        sensor_size.width = sensor_mode_info->scaler_trim.width;
        sensor_size.height = sensor_mode_info->scaler_trim.height;
        prev_get_matched_burstmode_lv_size(
            sensor_size, chn_param.cap_inf_cfg.cfg.dst_img_size, &lv_size,
            &video_size);
        chn_param.cap_inf_cfg.cfg.src_img_change = 1;
        chn_param.cap_inf_cfg.cfg.src_img_size.width = video_size.width;
        chn_param.cap_inf_cfg.cfg.src_img_size.height = video_size.height;
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_x = 0;
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_y = 0;
        chn_param.cap_inf_cfg.cfg.src_img_rect.width = video_size.width;
        chn_param.cap_inf_cfg.cfg.src_img_rect.height = video_size.height;

        prev_cxt->lv_size = lv_size;
        prev_cxt->video_size = video_size;
    } else /* if (prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb)*/ {
        prev_cxt->video_size.width = sensor_mode_info->scaler_trim.width;
        prev_cxt->video_size.height = sensor_mode_info->scaler_trim.height;
    }

    /*caculate trim rect*/
    if (ZOOM_INFO != zoom_param->mode) {
        CMR_LOGI("zoom level %ld, dst_img_size %d %d", zoom_param->zoom_level,
                 chn_param.cap_inf_cfg.cfg.dst_img_size.width,
                 chn_param.cap_inf_cfg.cfg.dst_img_size.height);
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level,
                                   &chn_param.cap_inf_cfg.cfg.dst_img_size);
    } else if (prev_cxt->prev_param.sprd_burstmode_enabled ||
               (!prev_cxt->prev_param.snapshot_eb &&
                !prev_cxt->prev_param.video_eb)) {
        ret =
            camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                  zoom_param->zoom_info.zoom_ratio,
                                  zoom_param->zoom_info.prev_aspect_ratio,
                                  chn_param.cap_inf_cfg.cfg.src_img_size.width,
                                  chn_param.cap_inf_cfg.cfg.src_img_size.height,
                                  prev_cxt->prev_param.prev_rot);
    } else {
        ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    zoom_param->zoom_info.zoom_ratio,
                                    zoom_param->zoom_info.prev_aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.prev_rot);
    }
    if (ret) {
        CMR_LOGE("prev get trim failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGI("after src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    /*save the rect*/
    prev_cxt->prev_rect.start_x =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_x;
    prev_cxt->prev_rect.start_y =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_y;
    prev_cxt->prev_rect.width = chn_param.cap_inf_cfg.cfg.src_img_rect.width;
    prev_cxt->prev_rect.height = chn_param.cap_inf_cfg.cfg.src_img_rect.height;

    /*get sensor interface info*/
    ret = prev_get_sn_inf(handle, camera_id, chn_param.skip_num,
                          &chn_param.sn_if);
    if (ret) {
        CMR_LOGE("get sn inf failed");
        goto exit;
    }

    /*alloc preview buffer*/
    ret = prev_alloc_prev_buf(handle, camera_id, is_restart, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc prev buf failed");
        goto exit;
    }
    if (!handle->ops.channel_cfg) {
        CMR_LOGE("ops channel_cfg is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    chn_param.cap_inf_cfg.cfg.flip_on = 0;

    /*config channel*/
    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    prev_cxt->prev_channel_id = channel_id;
    CMR_LOGI("prev chn id is %ld", prev_cxt->prev_channel_id);
    prev_cxt->prev_channel_status = PREV_CHN_BUSY;
    prev_cxt->prev_data_endian = endian;

    if (prev_cxt->skip_mode == IMG_SKIP_SW_KER) {
        /*config skip num buffer*/
        for (i = 0; i < prev_cxt->prev_skip_num; i++) {
            cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
            buf_cfg.channel_id = prev_cxt->prev_channel_id;
            buf_cfg.base_id = CMR_PREV_ID_BASE;
            buf_cfg.count = 1;
            buf_cfg.length = prev_cxt->prev_mem_size;
            buf_cfg.is_reserved_buf = 0;
            buf_cfg.flag = BUF_FLAG_INIT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->prev_reserved_frm[0].addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->prev_reserved_frm[0].addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->prev_reserved_frm[0].addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->prev_reserved_frm[0].addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->prev_reserved_frm[0].fd;
            ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
            if (ret) {
                CMR_LOGE("channel buff config failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }
    }

    chn_param.buffer.channel_id = prev_cxt->prev_channel_id;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config reserved buffer*/
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    buf_cfg.channel_id = prev_cxt->prev_channel_id;
    buf_cfg.base_id = CMR_PREV_ID_BASE;
    buf_cfg.count = prev_get_prev_res_buffer_count(prev_cxt);
    buf_cfg.length = prev_cxt->prev_mem_size;
    buf_cfg.is_reserved_buf = 1;
    buf_cfg.flag = BUF_FLAG_INIT;
    for (i = 0; i < buf_cfg.count; i++) {
        buf_cfg.addr[i].addr_y = prev_cxt->prev_reserved_frm[i].addr_phy.addr_y;
        buf_cfg.addr[i].addr_u = prev_cxt->prev_reserved_frm[i].addr_phy.addr_u;
        buf_cfg.addr_vir[i].addr_y =
            prev_cxt->prev_reserved_frm[i].addr_vir.addr_y;
        buf_cfg.addr_vir[i].addr_u =
            prev_cxt->prev_reserved_frm[i].addr_vir.addr_u;
        buf_cfg.fd[i] = prev_cxt->prev_reserved_frm[i].fd;
    }
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*start isp*/
    CMR_LOGI("need_isp %d, isp_status %ld", chn_param.cap_inf_cfg.cfg.need_isp,
             prev_cxt->isp_status);
    if (chn_param.cap_inf_cfg.cfg.need_isp) {
        if (!prev_cxt->prev_param.sprd_burstmode_enabled) {
            video_param.size.width = sensor_mode_info->trim_width;
            if (chn_param.cap_inf_cfg.cfg.need_binning) {
                video_param.size.width = video_param.size.width >> 1;
            }
            video_param.size.height = sensor_mode_info->trim_height;
            video_param.img_format = ISP_DATA_NORMAL_RAW10;
            video_param.video_mode = ISP_VIDEO_MODE_CONTINUE;
            video_param.work_mode = 0;

            if (!handle->ops.isp_start_video) {
                CMR_LOGE("ops isp_start_video is null");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }

            video_param.live_view_sz.width = prev_cxt->actual_prev_size.width;
            video_param.live_view_sz.height = prev_cxt->actual_prev_size.height;
            video_param.lv_size = prev_cxt->lv_size;
            video_param.video_size = prev_cxt->video_size;
            ret = handle->ops.isp_start_video(handle->oem_handle, &video_param);
            if (ret) {
                CMR_LOGE("isp start video failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
            prev_cxt->isp_status = PREV_ISP_COWORK;
        }
        prev_cxt->cap_need_isp = chn_param.cap_inf_cfg.cfg.need_isp;
        prev_cxt->cap_need_binning = chn_param.cap_inf_cfg.cfg.need_binning;
    }

    /*return preview out params*/
    if (out_param_ptr) {
        out_param_ptr->preview_chn_bits = 1 << prev_cxt->prev_channel_id;
        out_param_ptr->preview_sn_mode = chn_param.sensor_mode;
        out_param_ptr->preview_data_endian = prev_cxt->prev_data_endian;
    }

exit:
    CMR_LOGI("ret %ld", ret);
    if (ret) {
        prev_free_prev_buf(handle, camera_id, 0);
    }

    ATRACE_END();
    return ret;
}

cmr_int prev_set_prev_param_lightly(struct prev_handle *handle,
                                    cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGD(" in");

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    prev_cxt = &handle->prev_cxt[camera_id];

    if (prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb) {
        chn_param.sensor_mode = prev_cxt->cap_mode;
    } else {
        chn_param.sensor_mode = prev_cxt->prev_mode;
    }
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    //	cmr_bzero(prev_cxt->prev_rot_frm_is_lock, PREV_ROT_FRM_CNT *
    // sizeof(cmr_uint));
    prev_cxt->prev_rot_index = 0;
    prev_cxt->skip_mode = IMG_SKIP_HW;

    chn_param.is_lightly = 1; /*config channel lightly*/
    chn_param.frm_num = -1;
    if (IMG_DATA_TYPE_RAW == sensor_mode_info->image_format) {
        chn_param.skip_num = 0;
    } else {
        chn_param.skip_num = prev_cxt->prev_skip_num;
    }

    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.need_isp = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.preview_fmt;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;

    if (IMG_DATA_TYPE_RAW == sensor_mode_info->image_format) {
        prev_cxt->skip_mode = IMG_SKIP_SW_KER;
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    }

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->actual_prev_size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->actual_prev_size.height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;

    CMR_LOGI("src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    if (prev_cxt->prev_param.sprd_burstmode_enabled ||
        (!prev_cxt->prev_param.snapshot_eb && !prev_cxt->prev_param.video_eb)) {
        struct img_size sensor_size;
        struct img_size lv_size;
        struct img_size video_size;

        sensor_size.width = sensor_mode_info->scaler_trim.width;
        sensor_size.height = sensor_mode_info->scaler_trim.height;
        prev_get_matched_burstmode_lv_size(
            sensor_size, chn_param.cap_inf_cfg.cfg.dst_img_size, &lv_size,
            &video_size);
        chn_param.cap_inf_cfg.cfg.src_img_change = 1;
        chn_param.cap_inf_cfg.cfg.src_img_size.width = video_size.width;
        chn_param.cap_inf_cfg.cfg.src_img_size.height = video_size.height;
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_x = 0;
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_y = 0;
        chn_param.cap_inf_cfg.cfg.src_img_rect.width = video_size.width;
        chn_param.cap_inf_cfg.cfg.src_img_rect.height = video_size.height;

        prev_cxt->lv_size = lv_size;
        prev_cxt->video_size = video_size;
    } else {
        prev_cxt->video_size.width = sensor_mode_info->scaler_trim.width;
        prev_cxt->video_size.height = sensor_mode_info->scaler_trim.height;
    }

    /*caculate trim rect*/
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level,
                                   &chn_param.cap_inf_cfg.cfg.dst_img_size);
    } else if (prev_cxt->prev_param.sprd_burstmode_enabled ||
               (!prev_cxt->prev_param.snapshot_eb &&
                !prev_cxt->prev_param.video_eb)) {
        ret =
            camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                  zoom_param->zoom_info.zoom_ratio,
                                  zoom_param->zoom_info.prev_aspect_ratio,
                                  chn_param.cap_inf_cfg.cfg.src_img_size.width,
                                  chn_param.cap_inf_cfg.cfg.src_img_size.height,
                                  prev_cxt->prev_param.prev_rot);
    } else {
        ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    zoom_param->zoom_info.zoom_ratio,
                                    zoom_param->zoom_info.prev_aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.prev_rot);
    }
    if (ret) {
        CMR_LOGE("prev get trim failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGI("after src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    /*save the rect*/
    prev_cxt->prev_rect.start_x =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_x;
    prev_cxt->prev_rect.start_y =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_y;
    prev_cxt->prev_rect.width = chn_param.cap_inf_cfg.cfg.src_img_rect.width;
    prev_cxt->prev_rect.height = chn_param.cap_inf_cfg.cfg.src_img_rect.height;

    /*config channel*/
    if (!handle->ops.channel_cfg) {
        CMR_LOGE("ops channel_cfg is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    channel_id = prev_cxt->prev_channel_id;
    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    CMR_LOGD("returned chn id is %d", channel_id);

exit:
    CMR_LOGD(" out");
    return ret;
}

cmr_int prev_set_video_param(struct prev_handle *handle, cmr_u32 camera_id,
                             cmr_u32 is_restart,
                             struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct video_start_param video_param;
    struct img_data_end endian;
    struct buffer_cfg buf_cfg;
    struct img_size trim_sz;
    cmr_u32 i;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    cmr_bzero(&video_param, sizeof(struct video_start_param));
    CMR_LOGI("camera_id %d", camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];

    if (prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb) {
        chn_param.sensor_mode = prev_cxt->cap_mode;
    } else {
        chn_param.sensor_mode = prev_cxt->video_mode;
    }

    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    cmr_bzero(prev_cxt->video_rot_frm_is_lock,
              PREV_ROT_FRM_CNT * sizeof(cmr_uint));
    prev_cxt->video_rot_index = 0;
    prev_cxt->video_frm_cnt = 0;
    prev_cxt->prev_skip_num = sensor_info->preview_skip_num;
    prev_cxt->skip_mode = IMG_SKIP_HW;

    chn_param.is_lightly = 0;
    chn_param.frm_num = -1;
    if (IMG_DATA_TYPE_RAW == sensor_mode_info->image_format) {
        chn_param.skip_num = 0;
    } else {
        chn_param.skip_num = prev_cxt->prev_skip_num;
    }

    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.need_isp = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.preview_fmt;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 1;
    chn_param.cap_inf_cfg.cfg.sence_mode = DCAM_SCENE_MODE_RECORDING;

    if (IMG_DATA_TYPE_RAW == sensor_mode_info->image_format) {
        prev_cxt->skip_mode = IMG_SKIP_SW_KER;
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    }

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->actual_video_size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->actual_video_size.height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;

    CMR_LOGI("skip_mode %ld, skip_num %ld, image_format %d w=%d h=%d",
             prev_cxt->skip_mode, prev_cxt->prev_skip_num,
             sensor_mode_info->image_format, prev_cxt->actual_video_size.width,
             prev_cxt->actual_video_size.height);

    CMR_LOGI("src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    trim_sz.width = sensor_mode_info->scaler_trim.width;
    trim_sz.height = sensor_mode_info->scaler_trim.height;
    /*caculate trim rect*/
    if (ZOOM_INFO != zoom_param->mode) {
        CMR_LOGI("zoom level %ld, dst_img_size %d %d", zoom_param->zoom_level,
                 chn_param.cap_inf_cfg.cfg.dst_img_size.width,
                 chn_param.cap_inf_cfg.cfg.dst_img_size.height);
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level, &trim_sz);
    } else {
        ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    zoom_param->zoom_info.zoom_ratio,
                                    zoom_param->zoom_info.video_aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.prev_rot);
    }
    if (ret) {
        CMR_LOGE("video get trim failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGI("after src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    /*save the rect*/
    prev_cxt->video_rect.start_x =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_x;
    prev_cxt->video_rect.start_y =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_y;
    prev_cxt->video_rect.width = chn_param.cap_inf_cfg.cfg.src_img_rect.width;
    prev_cxt->video_rect.height = chn_param.cap_inf_cfg.cfg.src_img_rect.height;

    /*get sensor interface info*/
    ret = prev_get_sn_inf(handle, camera_id, chn_param.skip_num,
                          &chn_param.sn_if);
    if (ret) {
        CMR_LOGE("get sn inf failed");
        goto exit;
    }

    /*alloc preview buffer*/
    ret =
        prev_alloc_video_buf(handle, camera_id, is_restart, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc prev buf failed");
        goto exit;
    }
    if (!handle->ops.channel_cfg) {
        CMR_LOGE("ops channel_cfg is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    enum cmr_mirror_type mirror_type = CMR_MIRROR_DEFAULT;
    cmr_get_mirror(&mirror_type);
    if ((CMR_MIRROR_DCAM == mirror_type) &&
        (handle->prev_cxt[camera_id].prev_param.flip_on) && (1 == camera_id))
        chn_param.cap_inf_cfg.cfg.flip_on =
            handle->prev_cxt[camera_id].prev_param.flip_on;
    else
        chn_param.cap_inf_cfg.cfg.flip_on = 0;

    CMR_LOGD("channel config flip:%d", chn_param.cap_inf_cfg.cfg.flip_on);

    /*config channel*/
    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    prev_cxt->video_channel_id = channel_id;
    CMR_LOGI("video chn id is %ld", prev_cxt->video_channel_id);
    prev_cxt->video_channel_status = PREV_CHN_BUSY;
    prev_cxt->video_data_endian = endian;

    if (prev_cxt->skip_mode == IMG_SKIP_SW_KER) {
        /*config skip buffer*/
        for (i = 0; i < prev_cxt->prev_skip_num; i++) {
            cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
            buf_cfg.channel_id = prev_cxt->video_channel_id;
            buf_cfg.base_id = CMR_VIDEO_ID_BASE;
            buf_cfg.count = 1;
            buf_cfg.length = prev_cxt->video_mem_size;
            buf_cfg.is_reserved_buf = 0;
            buf_cfg.flag = BUF_FLAG_INIT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->video_reserved_frm[0].addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->video_reserved_frm[0].addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->video_reserved_frm[0].addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->video_reserved_frm[0].addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->video_reserved_frm[0].fd;
            ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
            if (ret) {
                CMR_LOGE("channel buff config failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }
    }

    chn_param.buffer.channel_id = prev_cxt->video_channel_id;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config reserved buffer*/
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    buf_cfg.channel_id = prev_cxt->video_channel_id;
    buf_cfg.base_id = CMR_VIDEO_ID_BASE;
    buf_cfg.count = VIDEO_RESERVED_FRM_CNT;
    buf_cfg.length = prev_cxt->video_mem_size;
    buf_cfg.is_reserved_buf = 1;
    buf_cfg.flag = BUF_FLAG_INIT;
    for (i = 0; i < VIDEO_RESERVED_FRM_CNT; i++) {
        buf_cfg.addr[i].addr_y =
            prev_cxt->video_reserved_frm[i].addr_phy.addr_y;
        buf_cfg.addr[i].addr_u =
            prev_cxt->video_reserved_frm[i].addr_phy.addr_u;
        buf_cfg.addr_vir[i].addr_y =
            prev_cxt->video_reserved_frm[i].addr_vir.addr_y;
        buf_cfg.addr_vir[i].addr_u =
            prev_cxt->video_reserved_frm[i].addr_vir.addr_u;
        buf_cfg.fd[i] = prev_cxt->video_reserved_frm[i].fd;
    }
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*start isp*/
    CMR_LOGI("need_isp %d, isp_status %ld", chn_param.cap_inf_cfg.cfg.need_isp,
             prev_cxt->isp_status);

    /*return preview out params*/
    if (out_param_ptr) {
        out_param_ptr->video_chn_bits = 1 << prev_cxt->video_channel_id;
        out_param_ptr->video_sn_mode = chn_param.sensor_mode;
        out_param_ptr->video_data_endian = prev_cxt->video_data_endian;
        out_param_ptr->snapshot_data_endian = prev_cxt->video_data_endian;
        out_param_ptr->actual_video_size = prev_cxt->actual_video_size;
    }

exit:
    CMR_LOGI("ret %ld", ret);
    if (ret) {
        prev_free_video_buf(handle, camera_id, 0);
    }

    ATRACE_END();
    return ret;
}

cmr_int prev_set_video_param_lightly(struct prev_handle *handle,
                                     cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;
    struct img_size trim_sz;
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGD(" in");

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    prev_cxt = &handle->prev_cxt[camera_id];

    if (prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb) {
        chn_param.sensor_mode = prev_cxt->cap_mode;
    } else {
        chn_param.sensor_mode = prev_cxt->video_mode;
    }
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    cmr_bzero(prev_cxt->video_rot_frm_is_lock,
              PREV_ROT_FRM_CNT * sizeof(cmr_uint));
    prev_cxt->prev_rot_index = 0;
    prev_cxt->skip_mode = IMG_SKIP_HW;

    chn_param.is_lightly = 1; /*config channel lightly*/
    chn_param.frm_num = -1;
    if (IMG_DATA_TYPE_RAW == sensor_mode_info->image_format) {
        chn_param.skip_num = 0;
    } else {
        chn_param.skip_num = prev_cxt->prev_skip_num;
    }

    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.need_isp = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.preview_fmt;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 1;

    if (IMG_DATA_TYPE_RAW == sensor_mode_info->image_format) {
        prev_cxt->skip_mode = IMG_SKIP_SW_KER;
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    }

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->actual_video_size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->actual_video_size.height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;

    CMR_LOGI("src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    trim_sz.width = sensor_mode_info->scaler_trim.width;
    trim_sz.height = sensor_mode_info->scaler_trim.height;
    /*caculate trim rect*/
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level, &trim_sz);
    } else {
        ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    zoom_param->zoom_info.zoom_ratio,
                                    zoom_param->zoom_info.video_aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.prev_rot);
    }
    if (ret) {
        CMR_LOGE("prev get trim failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGI("after src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    /*save the rect*/
    prev_cxt->video_rect.start_x =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_x;
    prev_cxt->video_rect.start_y =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_y;
    prev_cxt->video_rect.width = chn_param.cap_inf_cfg.cfg.src_img_rect.width;
    prev_cxt->video_rect.height = chn_param.cap_inf_cfg.cfg.src_img_rect.height;

    /*config channel*/
    if (!handle->ops.channel_cfg) {
        CMR_LOGE("ops channel_cfg is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    channel_id = prev_cxt->video_channel_id;
    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    CMR_LOGD("returned chn id is %d", channel_id);

exit:
    CMR_LOGD(" out");
    return ret;
}

cmr_int prev_set_cap_param(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart, cmr_u32 is_lightly,
                           struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct video_start_param video_param;
    struct img_data_end endian;
    struct cmr_path_capability capability;
    cmr_u32 is_capture_zsl = 0;
    struct buffer_cfg buf_cfg;
    cmr_u32 i;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    cmr_bzero(&video_param, sizeof(struct video_start_param));
    CMR_LOGI("camera_id %d", camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->cap_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

#ifdef CONFIG_CAMERA_UHD_VIDEOSNAPSHOT_SUPPORT
    CMR_LOGI("is_uhd_recording_mode %d support",
             prev_cxt->prev_param.is_uhd_recording_mode);
#else
    if (prev_cxt->prev_param.is_uhd_recording_mode == 1 &&
        prev_cxt->prev_param.video_eb) {
        CMR_LOGI("is_uhd_recording_mode %d not support",
                 prev_cxt->prev_param.is_uhd_recording_mode);
        goto exit;
    }
#endif
    if (!is_restart) {
        prev_cxt->cap_frm_cnt = 0;
        prev_cxt->cap_zsl_frm_cnt = 0;
    }

    chn_param.is_lightly = is_lightly;
    ;
    chn_param.sensor_mode = prev_cxt->cap_mode;
    prev_cxt->skip_mode = IMG_SKIP_SW_KER;
    prev_cxt->cap_skip_num = sensor_info->capture_skip_num;
    if (IMG_DATA_TYPE_RAW == sensor_mode_info->image_format) {
        chn_param.skip_num = 0;
    } else {
        chn_param.skip_num = prev_cxt->cap_skip_num;
    }

    CMR_LOGI("preview_eb %d , snapshot_eb %d, frame_ctrl %d, frame_count %d, "
             "is_restart %d",
             prev_cxt->prev_param.preview_eb, prev_cxt->prev_param.snapshot_eb,
             prev_cxt->prev_param.frame_ctrl, prev_cxt->prev_param.frame_count,
             is_restart);

    if (prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb) {
        /*zsl*/
        if (FRAME_CONTINUE == prev_cxt->prev_param.frame_ctrl) {
            chn_param.frm_num = prev_cxt->prev_param.frame_count;
            if (prev_cxt->prev_param.video_snapshot_type !=
                VIDEO_SNAPSHOT_VIDEO) {
                CMR_LOGI("enable zsl for non video snapshot mode");
                is_capture_zsl = 1;
            }
        } else {
            CMR_LOGE("wrong cap param!");
        }

    } else {
        /*no-zsl, if frame_ctrl is stop, get one frame everytime, else get the
         * frames one time*/
        if (FRAME_STOP == prev_cxt->prev_param.frame_ctrl) {
            chn_param.frm_num = 1;
        } else {
            chn_param.frm_num = prev_cxt->prev_param.frame_count;
        }
    }

    CMR_LOGI("frm_num 0x%x", chn_param.frm_num);

    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = chn_param.frm_num;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;
    chn_param.cap_inf_cfg.cfg.sence_mode = DCAM_SCENE_MODE_CAPTURE;

    if (prev_cxt->prev_param.video_eb ||
        (prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb)) {
        prev_get_sensor_mode(handle, camera_id);
    }

    /*config capture ability*/
    ret = prev_cap_ability(handle, camera_id, &prev_cxt->actual_pic_size,
                           &chn_param.cap_inf_cfg.cfg);
    if (ret) {
        CMR_LOGE("calc ability failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*get sensor interface info*/
    ret = prev_get_sn_inf(handle, camera_id,
                          chn_param.cap_inf_cfg.chn_deci_factor,
                          &chn_param.sn_if);
    if (ret) {
        CMR_LOGE("get sn inf failed");
        goto exit;
    }

    /*alloc capture buffer*/
    ret = prev_alloc_cap_buf(handle, camera_id, is_restart, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc cap buf failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (!handle->ops.channel_path_capability) {
        CMR_LOGE("ops channel_path_capability is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (is_capture_zsl) {
        ret = prev_alloc_zsl_buf(handle, camera_id, is_restart,
                                 &chn_param.buffer);
        if (ret) {
            CMR_LOGE("alloc zsl buf failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }
    if (!prev_cxt->prev_param.sprd_burstmode_enabled) {
        ret = prev_alloc_cap_reserve_buf(handle, camera_id, is_restart);
        if (ret) {
            CMR_LOGE("alloc cap reserve buf failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }
    ret = handle->ops.channel_path_capability(handle->oem_handle, &capability);
    if (ret) {
        CMR_LOGE("channel_path_capability failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (capability.yuv_available_cnt && !chn_param.is_lightly) {
        /*config channel*/
        if (!handle->ops.channel_cfg) {
            CMR_LOGE("ops channel_cfg is null");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        enum cmr_mirror_type mirror_type = CMR_MIRROR_DEFAULT;
        cmr_get_mirror(&mirror_type);
        if ((CMR_MIRROR_DCAM == mirror_type) &&
            (handle->prev_cxt[camera_id].prev_param.flip_on) &&
            (1 == camera_id))
            chn_param.cap_inf_cfg.cfg.flip_on =
                handle->prev_cxt[camera_id].prev_param.flip_on;
        else
            chn_param.cap_inf_cfg.cfg.flip_on = 0;

        CMR_LOGI("channel config flip:%d", chn_param.cap_inf_cfg.cfg.flip_on);

        ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                      &chn_param, &channel_id, &endian);
        if (ret) {
            CMR_LOGE("channel config failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        prev_cxt->cap_channel_id = channel_id;
        CMR_LOGI("cap chn id is %ld", prev_cxt->cap_channel_id);
        prev_cxt->cap_channel_status = PREV_CHN_BUSY;
        prev_cxt->cap_data_endian = endian;

        /* for capture, not skip frame for now, for cts
         * testMandatoryOutputCombinations issue */
        //if ((prev_cxt->skip_mode == IMG_SKIP_SW_KER) && 0) {
        if (prev_cxt->skip_mode == IMG_SKIP_SW_KER &&
		!prev_cxt->prev_param.sprd_burstmode_enabled &&
		prev_cxt->prev_param.frame_ctrl !=FRAME_HDR_PROC) {
            /*config skip num buffer*/
            for (i = 0; i < prev_cxt->cap_skip_num; i++) {
                cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
                buf_cfg.channel_id = prev_cxt->cap_channel_id;
                if (is_capture_zsl) {
                    buf_cfg.base_id = CMR_CAP1_ID_BASE;
                } else {
                    buf_cfg.base_id = CMR_CAP0_ID_BASE;
                }
                buf_cfg.count = 1;
                buf_cfg.length = prev_cxt->cap_zsl_mem_size;
                buf_cfg.is_reserved_buf = 0;
                buf_cfg.flag = BUF_FLAG_INIT;
                buf_cfg.addr[0].addr_y =
                    prev_cxt->cap_zsl_reserved_frm[0].addr_phy.addr_y;
                buf_cfg.addr[0].addr_u =
                    prev_cxt->cap_zsl_reserved_frm[0].addr_phy.addr_u;
                buf_cfg.addr_vir[0].addr_y =
                    prev_cxt->cap_zsl_reserved_frm[0].addr_vir.addr_y;
                buf_cfg.addr_vir[0].addr_u =
                    prev_cxt->cap_zsl_reserved_frm[0].addr_vir.addr_u;
                buf_cfg.fd[0] = prev_cxt->cap_zsl_reserved_frm[0].fd;
                ret =
                    handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
                if (ret) {
                    CMR_LOGE("channel buff config failed");
                    ret = CMR_CAMERA_FAIL;
                    goto exit;
                }
            }
        }

        chn_param.buffer.channel_id = prev_cxt->cap_channel_id;
        ret =
            handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
        if (ret) {
            CMR_LOGE("channel buff config failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        if (!prev_cxt->prev_param.sprd_burstmode_enabled) {
            /*config reserved buffer*/
            cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
            buf_cfg.channel_id = prev_cxt->cap_channel_id;
            if (is_capture_zsl) {
                buf_cfg.base_id = CMR_CAP1_ID_BASE;
            } else {
                buf_cfg.base_id = CMR_CAP0_ID_BASE;
            }
            buf_cfg.count = prev_get_capzsl_res_buffer_count(prev_cxt);;
            buf_cfg.length = prev_cxt->cap_zsl_mem_size;
            buf_cfg.is_reserved_buf = 1;
            buf_cfg.flag = BUF_FLAG_INIT;
            for (i = 0; i < buf_cfg.count; i++) {
                buf_cfg.addr[i].addr_y =
                    prev_cxt->cap_zsl_reserved_frm[i].addr_phy.addr_y;
                buf_cfg.addr[i].addr_u =
                    prev_cxt->cap_zsl_reserved_frm[i].addr_phy.addr_u;
                buf_cfg.addr_vir[i].addr_y =
                    prev_cxt->cap_zsl_reserved_frm[i].addr_vir.addr_y;
                buf_cfg.addr_vir[i].addr_u =
                    prev_cxt->cap_zsl_reserved_frm[i].addr_vir.addr_u;
                buf_cfg.fd[i] = prev_cxt->cap_zsl_reserved_frm[i].fd;
            }
            ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
            if (ret) {
                CMR_LOGE("channel buff config failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }
    } else {
        if (!is_capture_zsl) {
            prev_cxt->cap_channel_id = CHN_MAX;
            prev_cxt->cap_data_endian = prev_cxt->video_data_endian;
        }
    }

    /*save channel start param for restart*/
    cmr_copy(&prev_cxt->restart_chn_param, &chn_param,
             sizeof(struct channel_start_param));

    /*start isp*/
    CMR_LOGI("need_isp %d, isp_status %ld", chn_param.cap_inf_cfg.cfg.need_isp,
             prev_cxt->isp_status);
    prev_cxt->cap_need_isp = chn_param.cap_inf_cfg.cfg.need_isp;
    prev_cxt->cap_need_binning = chn_param.cap_inf_cfg.cfg.need_binning;

    /*return capture out params*/
    if (out_param_ptr) {
        out_param_ptr->snapshot_chn_bits = 1 << prev_cxt->cap_channel_id;
        out_param_ptr->preview_sn_mode = prev_cxt->prev_mode;
        out_param_ptr->snapshot_sn_mode = prev_cxt->cap_mode;
        out_param_ptr->snapshot_data_endian = prev_cxt->cap_data_endian;
        out_param_ptr->actual_snapshot_size = prev_cxt->actual_pic_size;
        if (is_capture_zsl) {
            out_param_ptr->zsl_frame = 1;
            prev_cxt->is_zsl_frm = 1;
        } else {
            out_param_ptr->zsl_frame = 0;
            prev_cxt->is_zsl_frm = 0;
        }

        CMR_LOGD(
            "chn_bits 0x%x, prev_mode %d, cap_mode %d, encode_angle %d",
            out_param_ptr->snapshot_chn_bits, out_param_ptr->preview_sn_mode,
            out_param_ptr->snapshot_sn_mode, prev_cxt->prev_param.encode_angle);

        ret = prev_get_cap_post_proc_param(handle, camera_id,
                                           prev_cxt->prev_param.encode_angle,
                                           &out_param_ptr->post_proc_setting);
        if (ret) {
            CMR_LOGE("get cap post proc param failed");
        }
    }

exit:
    CMR_LOGD("out");
    ATRACE_END();
    return ret;
}

cmr_int prev_set_videocap_param(struct prev_handle *handle, cmr_u32 camera_id,
                                cmr_u32 is_restart, cmr_u32 is_lightly,
                                struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct video_start_param video_param;
    struct img_data_end endian;
    struct cmr_path_capability capability;
    cmr_u32 is_capture_zsl = 0;
    struct buffer_cfg buf_cfg;
    cmr_u32 i;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&video_param, sizeof(struct video_start_param));

    prev_cxt = &handle->prev_cxt[camera_id];
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->cap_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    if (!is_restart) {
        prev_cxt->cap_frm_cnt = 0;
        prev_cxt->cap_zsl_frm_cnt = 0;
    }

    CMR_LOGI("preview_eb %d , snapshot_eb %d, frame_ctrl %d, frame_count %d, "
             "is_restart %d",
             prev_cxt->prev_param.preview_eb, prev_cxt->prev_param.snapshot_eb,
             prev_cxt->prev_param.frame_ctrl, prev_cxt->prev_param.frame_count,
             is_restart);

#ifdef CONFIG_CAMERA_UHD_VIDEOSNAPSHOT_SUPPORT
    CMR_LOGI("is_uhd_recording_mode %d support",
             prev_cxt->prev_param.is_uhd_recording_mode);
#else
    if (prev_cxt->prev_param.is_uhd_recording_mode == 1 &&
        prev_cxt->prev_param.video_eb) {
        CMR_LOGI("is_uhd_recording_mode %d not support",
                 prev_cxt->prev_param.is_uhd_recording_mode);
        goto exit;
    }
#endif

    if (prev_cxt->prev_param.video_eb ||
        (prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb)) {
        prev_get_sensor_mode(handle, camera_id);
    }

    /*config capture ability*/
    ret = prev_cap_ability(handle, camera_id, &prev_cxt->actual_pic_size,
                           &chn_param.cap_inf_cfg.cfg);
    if (ret) {
        CMR_LOGE("prev_cap_ability failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*alloc capture buffer*/
    ret = prev_alloc_cap_buf(handle, camera_id, is_restart, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc cap buf failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    prev_cxt->cap_channel_id = CHN_MAX;
    prev_cxt->cap_mode = prev_cxt->prev_mode;
    prev_cxt->cap_data_endian = prev_cxt->video_data_endian;

    /*return capture out params*/
    if (out_param_ptr) {
        out_param_ptr->snapshot_chn_bits = 1 << prev_cxt->cap_channel_id;
        out_param_ptr->preview_sn_mode = prev_cxt->prev_mode;
        out_param_ptr->snapshot_sn_mode = prev_cxt->cap_mode;
        out_param_ptr->snapshot_data_endian = prev_cxt->cap_data_endian;
        out_param_ptr->actual_snapshot_size = prev_cxt->actual_pic_size;
        out_param_ptr->zsl_frame = 0;
        prev_cxt->is_zsl_frm = 0;

        CMR_LOGD(
            "chn_bits 0x%x, prev_mode %d, cap_mode %d, encode_angle %d",
            out_param_ptr->snapshot_chn_bits, out_param_ptr->preview_sn_mode,
            out_param_ptr->snapshot_sn_mode, prev_cxt->prev_param.encode_angle);

        ret = prev_get_cap_post_proc_param(handle, camera_id,
                                           prev_cxt->prev_param.encode_angle,
                                           &out_param_ptr->post_proc_setting);
        if (ret) {
            CMR_LOGE("get cap post proc param failed");
        }
    }

exit:
    ATRACE_END();
    return ret;
}

static cmr_int prev_update_cap_param(struct prev_handle *handle,
                                     cmr_u32 camera_id, cmr_u32 encode_angle,
                                     struct snp_proc_param *snp_proc_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;
    struct cmr_path_capability capability;
    cmr_u32 is_capture_zsl = 0;
    struct buffer_cfg buf_cfg;

    /*for new video snapshot or zsl snapshot, update cap mem info via encode
     * angle*/
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->cap_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;
    prev_cxt->prev_param.thumb_size = snp_proc_param->thumb_size;

    CMR_LOGI("@xin preview_eb %d , snapshot_eb %d, frame_ctrl %d, frame_count "
             "%d, encode_angle %d",
             prev_cxt->prev_param.preview_eb, prev_cxt->prev_param.snapshot_eb,
             prev_cxt->prev_param.frame_ctrl, prev_cxt->prev_param.frame_count,
             encode_angle);

    if (!prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb) {
        /*normal cap ignore this*/
        return ret;
    }

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    prev_cxt->prev_param.encode_angle = encode_angle;

    /*trigger cap mem re-arrange*/
    ret = prev_alloc_cap_buf(handle, camera_id, 1, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("update cap buf failed");
        ret = CMR_CAMERA_FAIL;
    }

    return ret;
}

cmr_int prev_set_zsl_param_lightly(struct prev_handle *handle,
                                   cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGD(" in");

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    prev_cxt = &handle->prev_cxt[camera_id];

    if (prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb) {
        chn_param.sensor_mode = prev_cxt->cap_mode;
    }
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    cmr_bzero(prev_cxt->cap_zsl_rot_frm_is_lock,
              PREV_ROT_FRM_CNT * sizeof(cmr_uint));
    prev_cxt->prev_rot_index = 0;
    prev_cxt->skip_mode = IMG_SKIP_HW;

    chn_param.is_lightly = 1; /*config channel lightly*/
    chn_param.frm_num = -1;
    if (IMG_DATA_TYPE_RAW == sensor_mode_info->image_format) {
        chn_param.skip_num = 0;
    } else {
        chn_param.skip_num = prev_cxt->cap_skip_num;
    }

    chn_param.buffer.base_id = CMR_CAP1_ID_BASE;
    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.need_isp = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.cap_fmt;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;

    if (IMG_DATA_TYPE_RAW == sensor_mode_info->image_format) {
        prev_cxt->skip_mode = IMG_SKIP_SW_KER;
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    }

    /*config capture ability*/
    ret = prev_cap_ability(handle, camera_id, &prev_cxt->actual_pic_size,
                           &chn_param.cap_inf_cfg.cfg);
    if (ret) {
        CMR_LOGE("calc ability failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config channel*/
    if (!handle->ops.channel_cfg) {
        CMR_LOGE("ops channel_cfg is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    channel_id = prev_cxt->cap_channel_id;
    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    CMR_LOGD("returned chn id is %d", channel_id);

exit:
    CMR_LOGD(" out");
    return ret;
}

cmr_int prev_set_cap_param_raw(struct prev_handle *handle, cmr_u32 camera_id,
                               cmr_u32 is_restart,
                               struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct video_start_param video_param;
    cmr_uint is_autotest = 0;
    struct img_data_end endian;
    struct buffer_cfg buf_cfg;
    int i = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!handle->ops.get_sensor_autotest_mode) {
        CMR_LOGE("ops get_sensor_autotest_mode null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    CMR_LOGD(" in");

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    cmr_bzero(&video_param, sizeof(struct video_start_param));
    CMR_LOGI("camera_id %d", camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->cap_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    ret = handle->ops.get_sensor_autotest_mode(handle->oem_handle, camera_id,
                                               &is_autotest);
    if (ret) {
        CMR_LOGE("get mode err");
    }
    if (is_autotest) {
        CMR_LOGE("0 sensor_mode->image_format =%d \n",
                 sensor_mode_info->image_format);
        CMR_LOGE("inorde to out yuv raw data ,so force set yuv to "
                 "SENSOR_IMAGE_FORMAT_RAW \n");
        sensor_mode_info->image_format = SENSOR_IMAGE_FORMAT_RAW;
        CMR_LOGE("1 sensor_mode->image_format =%d \n",
                 sensor_mode_info->image_format);
    }

    if (!is_restart) {
        prev_cxt->cap_frm_cnt = 0;
    }

    chn_param.is_lightly = 0;
    chn_param.sensor_mode = prev_cxt->cap_mode;
    chn_param.skip_num = sensor_info->capture_skip_num;
    prev_cxt->cap_skip_num = chn_param.skip_num;

    CMR_LOGI("preview_eb %d , snapshot_eb %d, frame_ctrl %d, frame_count %d, "
             "is_restart %d",
             prev_cxt->prev_param.preview_eb, prev_cxt->prev_param.snapshot_eb,
             prev_cxt->prev_param.frame_ctrl, prev_cxt->prev_param.frame_count,
             is_restart);

    chn_param.frm_num = 1;
    CMR_LOGI("frm_num 0x%x", chn_param.frm_num);

    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = chn_param.frm_num;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;

    /*config capture ability*/
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = IMG_DATA_TYPE_RAW;
    chn_param.cap_inf_cfg.cfg.need_isp_tool = 1;

    if (SENSOR_PDAF_TYPE2_ENABLE == prev_cxt->prev_param.pdaf_mode) {
        chn_param.cap_inf_cfg.cfg.pdaf_ctrl.mode = 1;
        chn_param.cap_inf_cfg.cfg.pdaf_ctrl.phase_data_type =
            prev_cxt->prev_param.datatype;
        chn_param.cap_inf_cfg.cfg.pdaf_ctrl.isp_tool_mode = 1;
    }
    ret = prev_cap_ability(handle, camera_id, &prev_cxt->actual_pic_size,
                           &chn_param.cap_inf_cfg.cfg);
    if (ret) {
        CMR_LOGE("calc ability failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*get sensor interface info*/
    ret = prev_get_sn_inf(handle, camera_id, chn_param.skip_num,
                          &chn_param.sn_if);
    if (ret) {
        CMR_LOGE("get sn inf failed");
        goto exit;
    }

    /*alloc capture buffer*/
    ret = prev_alloc_cap_buf(handle, camera_id, 0, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc cap buf failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (!prev_cxt->prev_param.sprd_burstmode_enabled) {
        ret = prev_alloc_cap_reserve_buf(handle, camera_id, is_restart);
        if (ret) {
            CMR_LOGE("alloc cap reserve buf failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    /*config channel*/
    if (!handle->ops.channel_cfg) {
        CMR_LOGE("ops channel_cfg is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    prev_cxt->cap_channel_id = channel_id;
    CMR_LOGI("cap chn id is %ld", prev_cxt->cap_channel_id);
    prev_cxt->cap_channel_status = PREV_CHN_BUSY;
    if (prev_cxt->prev_param.tool_eb) {
        prev_cxt->cap_data_endian = prev_cxt->prev_data_endian;
    } else {
        prev_cxt->cap_data_endian = endian;
    }

    chn_param.buffer.channel_id = prev_cxt->cap_channel_id;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (!prev_cxt->prev_param.sprd_burstmode_enabled) {
        /*config reserved buffer*/
        cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
        buf_cfg.channel_id = prev_cxt->cap_channel_id;
        buf_cfg.base_id = CMR_CAP0_ID_BASE;
        buf_cfg.count = CAP_ZSL_RESERVED_FRM_CNT;
        buf_cfg.length = prev_cxt->cap_zsl_mem_size;
        buf_cfg.is_reserved_buf = 1;
        buf_cfg.flag = BUF_FLAG_INIT;
        for (i = 0; i < CAP_ZSL_RESERVED_FRM_CNT; i++) {
            buf_cfg.addr[i].addr_y =
                prev_cxt->cap_zsl_reserved_frm[i].addr_phy.addr_y;
            buf_cfg.addr[i].addr_u =
                prev_cxt->cap_zsl_reserved_frm[i].addr_phy.addr_u;
            buf_cfg.addr_vir[i].addr_y =
                prev_cxt->cap_zsl_reserved_frm[i].addr_vir.addr_y;
            buf_cfg.addr_vir[i].addr_u =
                prev_cxt->cap_zsl_reserved_frm[i].addr_vir.addr_u;
            buf_cfg.fd[i] = prev_cxt->cap_zsl_reserved_frm[i].fd;
        }
        ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
        if (ret) {
            CMR_LOGE("channel buff config failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    /*save channel start param for restart*/
    cmr_copy(&prev_cxt->restart_chn_param, &chn_param,
             sizeof(struct channel_start_param));

    /*start isp*/
    CMR_LOGI("need_isp %d, isp_status %ld", chn_param.cap_inf_cfg.cfg.need_isp,
             prev_cxt->isp_status);
    prev_cxt->cap_need_isp = chn_param.cap_inf_cfg.cfg.need_isp;
    prev_cxt->cap_need_binning = chn_param.cap_inf_cfg.cfg.need_binning;

    /*return capture out params*/
    if (out_param_ptr) {
        out_param_ptr->snapshot_chn_bits = 1 << prev_cxt->cap_channel_id;
        out_param_ptr->preview_sn_mode = prev_cxt->prev_mode;
        out_param_ptr->snapshot_sn_mode = prev_cxt->cap_mode;
        out_param_ptr->snapshot_data_endian = prev_cxt->cap_data_endian;

        CMR_LOGD("chn_bits 0x%x, prev_mode %d, cap_mode %d",
                 out_param_ptr->snapshot_chn_bits,
                 out_param_ptr->preview_sn_mode,
                 out_param_ptr->snapshot_sn_mode);

        ret = prev_get_cap_post_proc_param(handle, camera_id,
                                           prev_cxt->prev_param.encode_angle,
                                           &out_param_ptr->post_proc_setting);
        if (ret) {
            CMR_LOGE("get cap post proc param failed");
        }
    }

exit:
    CMR_LOGD("X");
    ATRACE_END();
    return ret;
}

cmr_int prev_set_pdaf_param(struct prev_handle *handle, cmr_u32 camera_id,
                            struct preview_out_param *out_param_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 channel_id = 0;
    struct prev_context *prev_cxt = NULL;
    struct img_frm_cap cfg;

    prev_cxt = &handle->prev_cxt[camera_id];
    memset(&cfg, 0x00, sizeof(cfg));
    cfg.pdaf_ctrl.mode = 1;
    cfg.pdaf_ctrl.phase_data_type = prev_cxt->prev_param.datatype;
    cfg.pdaf_ctrl.isp_tool_mode = 1;
    ret = handle->ops.channel_pdaf_cfg(handle->oem_handle, camera_id, &cfg,
                                       channel_id);

    return ret;
}

cmr_int
prev_set_sensor_datatype_param(struct prev_handle *handle, cmr_u32 camera_id,
                               cmr_u32 is_restart,
                               struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct buffer_cfg buf_cfg;
    struct img_data_end endian;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    CMR_LOGI("camera_id %d", camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];

    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.need_isp = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = IMG_DATA_TYPE_RAW;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;
    // chn_param.cap_inf_cfg.cfg.dst_img_size.width   =
    // prev_cxt->actual_prev_size.width;
    // chn_param.cap_inf_cfg.cfg.dst_img_size.height  =
    // prev_cxt->actual_prev_size.height;
    // chn_param.cap_inf_cfg.cfg.notice_slice_height  =
    // chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    // chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
    // sensor_mode_info->scaler_trim.start_x;
    // chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
    // sensor_mode_info->scaler_trim.start_y;
    // chn_param.cap_inf_cfg.cfg.src_img_rect.width   =
    // sensor_mode_info->scaler_trim.width;
    // chn_param.cap_inf_cfg.cfg.src_img_rect.height  =
    // sensor_mode_info->scaler_trim.height;
    chn_param.cap_inf_cfg.cfg.flip_on = 0;

    chn_param.cap_inf_cfg.cfg.pdaf_ctrl.mode = 1;
    chn_param.cap_inf_cfg.cfg.pdaf_ctrl.isp_tool_mode = 0;

    switch (prev_cxt->prev_param.sensor_datatype) {
    case SENSOR_REAL_DEPTH_ENABLE:
        chn_param.cap_inf_cfg.cfg.pdaf_ctrl.phase_data_type =
            CAMERA_DEPTH_META_DATA_TYPE;
        break;
    case SENSOR_EMBEDDED_INFO_ENABLE:
        chn_param.cap_inf_cfg.cfg.pdaf_ctrl.phase_data_type =
            CAMERA_EMBEDDED_INFO_TYPE;
        break;
    case SENSOR_DATATYPE_PDAF_ENABLE:
        chn_param.cap_inf_cfg.cfg.pdaf_ctrl.phase_data_type =
            prev_cxt->prev_param.datatype;
        break;
    default:
        return CMR_CAMERA_INVALID_PARAM;
    }

    /*alloc sensor_datatype metadata buffer*/
    ret = prev_alloc_sensor_datatype_buf(handle, camera_id, is_restart,
                                         &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc sensor_datatype buf failed");
        goto exit;
    }
    if (!handle->ops.channel_cap_cfg) {
        CMR_LOGE("ops channel_cap_cfg is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config channel*/
    ret = handle->ops.channel_cap_cfg(handle->oem_handle, handle, camera_id,
                                      &chn_param.cap_inf_cfg, &channel_id,
                                      &endian);
    if (ret) {
        CMR_LOGE("channel cap cfg failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    prev_cxt->sensor_datatype_channel_id = channel_id;
    CMR_LOGI("sensor_datatype chn id is %ld",
             prev_cxt->sensor_datatype_channel_id);
    prev_cxt->sensor_datatype_channel_status = PREV_CHN_BUSY;

    chn_param.buffer.channel_id = channel_id;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config reserved buffer*/
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    buf_cfg.channel_id = prev_cxt->sensor_datatype_channel_id;
    buf_cfg.base_id = CMR_SENSOR_DATATYPE_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->sensor_datatype_mem_size;
    buf_cfg.is_reserved_buf = 1;
    buf_cfg.flag = BUF_FLAG_INIT;
    buf_cfg.addr[0].addr_y =
        prev_cxt->sensor_datatype_reserved_frm.addr_phy.addr_y;
    buf_cfg.addr[0].addr_u =
        prev_cxt->sensor_datatype_reserved_frm.addr_phy.addr_u;
    buf_cfg.addr_vir[0].addr_y =
        prev_cxt->sensor_datatype_reserved_frm.addr_vir.addr_y;
    buf_cfg.addr_vir[0].addr_u =
        prev_cxt->sensor_datatype_reserved_frm.addr_vir.addr_u;
    buf_cfg.fd[0] = prev_cxt->sensor_datatype_reserved_frm.fd;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*return preview out params*/
    if (out_param_ptr) {
        out_param_ptr->sensor_datatype_chn_bits =
            1 << prev_cxt->sensor_datatype_channel_id;
    }

exit:
    CMR_LOGI("ret %ld", ret);
    if (ret) {
        prev_free_sensor_datatype_buf(handle, camera_id, 0);
    }

    ATRACE_END();
    return ret;
}

cmr_int prev_set_pdaf_raw_param(struct prev_handle *handle, cmr_u32 camera_id,
                                cmr_u32 is_restart,
                                struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;
    struct buffer_cfg buf_cfg;
    struct img_size trim_sz;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    CMR_LOGI("camera_id %d", camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];

    chn_param.sensor_mode = prev_cxt->prev_mode;

    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    prev_cxt->pdaf_frm_cnt = 0;

    if ((sensor_mode_info->width != sensor_info->source_width_max) ||
        (sensor_mode_info->height != sensor_info->source_height_max)) {
        CMR_LOGE(
            "sensor mode setting err,type3 only support full size setting");
        return -CMR_CAMERA_FAIL;
    }

    chn_param.is_lightly = 0;
    chn_param.frm_num = -1;
    // chn_param.skip_num   = 0; //prev_cxt->prev_skip_num;
    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.need_isp = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt =
        IMG_DATA_TYPE_RAW; // IMG_DATA_TYPE_PDAF_TYPE3;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;
    chn_param.cap_inf_cfg.cfg.flip_on = 0;
    chn_param.cap_inf_cfg.cfg.need_isp_tool = 1;
    chn_param.cap_inf_cfg.cfg.pdaf_type3 = 1;
#if 0 /*not crop*/
	prev_cxt->pdaf_rect.start_x = sensor_mode_info->trim_start_x;
	prev_cxt->pdaf_rect.start_y = sensor_mode_info->trim_start_y;
	prev_cxt->pdaf_rect.width = sensor_mode_info->trim_width;
	prev_cxt->pdaf_rect.height = sensor_mode_info->trim_height;
#else /*crop roi*/
    prev_cxt->pdaf_rect.start_x = sensor_mode_info->trim_width / 4;
    prev_cxt->pdaf_rect.start_y = sensor_mode_info->trim_height / 4;
    prev_cxt->pdaf_rect.width = sensor_mode_info->trim_width / 2;
    prev_cxt->pdaf_rect.height = sensor_mode_info->trim_height / 2;
    prev_cxt->pdaf_rect.start_x = ALIGN_16_PIXEL(prev_cxt->pdaf_rect.start_x);
    prev_cxt->pdaf_rect.start_y = ALIGN_16_PIXEL(prev_cxt->pdaf_rect.start_y);
    prev_cxt->pdaf_rect.width = ALIGN_16_PIXEL(prev_cxt->pdaf_rect.width);
    prev_cxt->pdaf_rect.height = ALIGN_16_PIXEL(prev_cxt->pdaf_rect.height);
#endif

    CMR_LOGI("image_format %d image_pattern %d w=%d h=%d, max_w=%d, max_h=%d",
             sensor_mode_info->image_format, sensor_info->image_pattern,
             sensor_mode_info->width, sensor_mode_info->height,
             sensor_info->source_width_max, sensor_info->source_height_max);
    CMR_LOGI("pdaf_rect %d %d %d %d", prev_cxt->pdaf_rect.start_x,
             prev_cxt->pdaf_rect.start_y, prev_cxt->pdaf_rect.width,
             prev_cxt->pdaf_rect.height);

    chn_param.cap_inf_cfg.cfg.dst_img_size.width = prev_cxt->pdaf_rect.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height = prev_cxt->pdaf_rect.height;

    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        prev_cxt->pdaf_rect.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        prev_cxt->pdaf_rect.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width = prev_cxt->pdaf_rect.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height = prev_cxt->pdaf_rect.height;

    /*alloc pdaf buffer*/
    ret = prev_alloc_pdaf_raw_buf(handle, camera_id, is_restart,
                                  &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc pdaf buf failed");
        goto exit;
    }
    if (!handle->ops.channel_cap_cfg) {
        CMR_LOGE("ops channel_cap_cfg is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config channel*/
    ret = handle->ops.channel_cap_cfg(handle->oem_handle, handle, camera_id,
                                      &chn_param.cap_inf_cfg, &channel_id,
                                      &endian);
    if (ret) {
        CMR_LOGE("channel cap cfg failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    prev_cxt->pdaf_channel_id = channel_id;
    CMR_LOGI("pdaf chn id is %ld", prev_cxt->pdaf_channel_id);
    prev_cxt->pdaf_channel_status = PREV_CHN_BUSY;
    // prev_cxt->pdaf_data_endian = endian;

    chn_param.buffer.channel_id = channel_id;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config reserved buffer*/
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    buf_cfg.channel_id = prev_cxt->pdaf_channel_id;
    buf_cfg.base_id = CMR_PDAF_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->pdaf_mem_size;
    buf_cfg.is_reserved_buf = 1;
    buf_cfg.flag = BUF_FLAG_INIT;
    buf_cfg.addr[0].addr_y = prev_cxt->pdaf_reserved_frm.addr_phy.addr_y;
    buf_cfg.addr[0].addr_u = prev_cxt->pdaf_reserved_frm.addr_phy.addr_u;
    buf_cfg.addr_vir[0].addr_y = prev_cxt->pdaf_reserved_frm.addr_vir.addr_y;
    buf_cfg.addr_vir[0].addr_u = prev_cxt->pdaf_reserved_frm.addr_vir.addr_u;
    buf_cfg.fd[0] = prev_cxt->pdaf_reserved_frm.fd;

    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*return preview out params*/
    if (out_param_ptr) {
        out_param_ptr->pdaf_chn_bits = 1 << prev_cxt->pdaf_channel_id;
    }

exit:
    CMR_LOGI("ret %ld", ret);
    if (ret) {
        prev_free_pdaf_raw_buf(handle, camera_id, 0);
    }

    ATRACE_END();
    return ret;
}

cmr_int prev_cap_ability(struct prev_handle *handle, cmr_u32 camera_id,
                         struct img_size *cap_size,
                         struct img_frm_cap *img_cap) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 tmp_width = 0;
    struct img_size *sensor_size = NULL;
    struct img_rect *sn_trim_rect = NULL;
    struct sensor_mode_info *sn_mode_info = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 sc_factor = 0, sc_capability = 0, sc_threshold = 0;
    cmr_int zoom_post_proc = 0;
    struct img_size trim_sz;

    if (!handle || !cap_size || !img_cap) {
        CMR_LOGE("invalid param, 0x%p, 0x%p, 0x%p", handle, cap_size, img_cap);
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGI("camera_id %d", camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];
    sensor_size = &prev_cxt->cap_sn_size;
    sn_trim_rect = &prev_cxt->cap_sn_trim_rect;
    sn_mode_info = &prev_cxt->sensor_info.mode_info[prev_cxt->cap_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;
    CMR_LOGI("image_format %d, dst_img_fmt %d , "
             "prev_cxt->prev_param.sprd_highiso_enabled %d",
             sn_mode_info->image_format, img_cap->dst_img_fmt,
             prev_cxt->prev_param.sprd_highiso_enabled);
    CMR_LOGI("isp_to_dram %d", prev_cxt->prev_param.isp_to_dram);
    img_cap->need_isp = 0;
    sensor_size->width = sn_mode_info->trim_width;
    sensor_size->height = sn_mode_info->trim_height;
    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);

    switch (sn_mode_info->image_format) {
    case IMG_DATA_TYPE_YUV422:
        prev_cxt->cap_org_fmt = prev_cxt->prev_param.cap_fmt;
        if (ZOOM_POST_PROCESS == zoom_post_proc) {
            prev_cxt->cap_zoom_mode = zoom_post_proc;
            sensor_size->width = sn_mode_info->width;
            sensor_size->height = sn_mode_info->height;
        } else {
            prev_cxt->cap_zoom_mode = zoom_post_proc;
        }
        break;

    case IMG_DATA_TYPE_RAW:
        if (IMG_DATA_TYPE_RAW == img_cap->dst_img_fmt) {
            CMR_LOGI("Get RawData From RawRGB senosr");
            img_cap->need_isp = 0;
            prev_cxt->cap_org_fmt = IMG_DATA_TYPE_RAW;
            prev_cxt->cap_zoom_mode = ZOOM_POST_PROCESS;
            sensor_size->width = sn_mode_info->width;
            sensor_size->height = sn_mode_info->height;
        } else {
            if (sn_mode_info->width <= prev_cxt->prev_param.isp_width_limit) {
                CMR_LOGI("Need ISP");
                img_cap->need_isp = 1;
                prev_cxt->cap_org_fmt = prev_cxt->prev_param.cap_fmt;
                if (ZOOM_POST_PROCESS == zoom_post_proc) {
                    prev_cxt->cap_zoom_mode = zoom_post_proc;
                    sensor_size->width = sn_mode_info->width;
                    sensor_size->height = sn_mode_info->height;
                } else {
                    prev_cxt->cap_zoom_mode = zoom_post_proc;
                }
            } else {
                CMR_LOGI("change to rgbraw type");
                img_cap->need_isp = 0;
                prev_cxt->cap_org_fmt = IMG_DATA_TYPE_RAW;
                prev_cxt->cap_zoom_mode = ZOOM_POST_PROCESS;
                sensor_size->width = sn_mode_info->width;
                sensor_size->height = sn_mode_info->height;
            }
        }
        break;

    case IMG_DATA_TYPE_JPEG:
        prev_cxt->cap_org_fmt = IMG_DATA_TYPE_JPEG;
        prev_cxt->cap_zoom_mode = ZOOM_POST_PROCESS;
        break;

    default:
        CMR_LOGE("Unsupport sn format %d", sn_mode_info->image_format);
        return CMR_CAMERA_NO_SUPPORT;
        break;
    }

    CMR_LOGI("sn_image_format %d, dst_img_fmt %d, cap_org_fmt %ld, "
             "cap_zoom_mode %ld",
             sn_mode_info->image_format, img_cap->dst_img_fmt,
             prev_cxt->cap_org_fmt, prev_cxt->cap_zoom_mode);

    img_cap->dst_img_fmt = prev_cxt->cap_org_fmt;
    img_cap->notice_slice_height = sn_mode_info->scaler_trim.height;
    img_cap->src_img_rect.start_x = sn_mode_info->scaler_trim.start_x;
    img_cap->src_img_rect.start_y = sn_mode_info->scaler_trim.start_y;
    img_cap->src_img_rect.width = sn_mode_info->scaler_trim.width;
    img_cap->src_img_rect.height = sn_mode_info->scaler_trim.height;

    CMR_LOGI("src_img_rect %d %d %d %d", img_cap->src_img_rect.start_x,
             img_cap->src_img_rect.start_y, img_cap->src_img_rect.width,
             img_cap->src_img_rect.height);

    if (!prev_cxt->prev_param.preview_eb) {
        struct img_size sensor_size;
        struct img_size lv_size;
        struct img_size video_size;

        sensor_size.width = sn_mode_info->scaler_trim.width;
        sensor_size.height = sn_mode_info->scaler_trim.height;
        prev_get_matched_burstmode_lv_size(sensor_size, *cap_size, &lv_size,
                                           &video_size);
        img_cap->src_img_change = 1;
        img_cap->src_img_size.width = video_size.width;
        img_cap->src_img_size.height = video_size.height;
        img_cap->src_img_rect.start_x = 0;
        img_cap->src_img_rect.start_y = 0;
        img_cap->src_img_rect.width = video_size.width;
        img_cap->src_img_rect.height = video_size.height;

        prev_cxt->lv_size = lv_size;
        prev_cxt->video_size = video_size;
    }

    trim_sz.width = sn_mode_info->scaler_trim.width;
    trim_sz.height = sn_mode_info->scaler_trim.height;
    /*caculate trim rect*/
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&img_cap->src_img_rect,
                                   zoom_param->zoom_level, &trim_sz);
    } else if (!prev_cxt->prev_param.preview_eb) {
        ret = camera_get_trim_rect2(
            &img_cap->src_img_rect, zoom_param->zoom_info.zoom_ratio,
            zoom_param->zoom_info.capture_aspect_ratio,
            img_cap->src_img_size.width, img_cap->src_img_size.height,
            prev_cxt->prev_param.cap_rot);
    } else {
        ret = camera_get_trim_rect2(
            &img_cap->src_img_rect, zoom_param->zoom_info.zoom_ratio,
            zoom_param->zoom_info.capture_aspect_ratio,
            sn_mode_info->scaler_trim.width, sn_mode_info->scaler_trim.height,
            prev_cxt->prev_param.cap_rot);
    }

    if (ret) {
        CMR_LOGE("cap get trim failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGI("after src_img_rect %d %d %d %d", img_cap->src_img_rect.start_x,
             img_cap->src_img_rect.start_y, img_cap->src_img_rect.width,
             img_cap->src_img_rect.height);

    /*save sensor trim rect*/
    sn_trim_rect->start_x = img_cap->src_img_rect.start_x;
    sn_trim_rect->start_y = img_cap->src_img_rect.start_y;
    sn_trim_rect->width = img_cap->src_img_rect.width;
    sn_trim_rect->height = img_cap->src_img_rect.height;

    trim_sz.width = sn_trim_rect->width;
    trim_sz.height = sn_trim_rect->height;

    /*handle zoom process mode*/
    if (ZOOM_POST_PROCESS == prev_cxt->cap_zoom_mode) {
        img_cap->src_img_rect.start_x = sn_mode_info->trim_start_x;
        img_cap->src_img_rect.start_y = sn_mode_info->trim_start_y;
        img_cap->src_img_rect.width = sn_mode_info->trim_width;
        img_cap->src_img_rect.height = sn_mode_info->trim_height;
        img_cap->dst_img_size.width = sn_mode_info->trim_width;
        img_cap->dst_img_size.height = sn_mode_info->trim_height;

        if (IMG_DATA_TYPE_RAW == prev_cxt->cap_org_fmt ||
            ZOOM_POST_PROCESS == zoom_post_proc) {
            sn_trim_rect->start_x = img_cap->src_img_rect.start_x;
            sn_trim_rect->start_y = img_cap->src_img_rect.start_y;
            sn_trim_rect->width = img_cap->src_img_rect.width;
            sn_trim_rect->height = img_cap->src_img_rect.height;
            if (ZOOM_INFO != zoom_param->mode) {
                ret = camera_get_trim_rect(sn_trim_rect, zoom_param->zoom_level,
                                           &trim_sz);
            } else {
                ret = camera_get_trim_rect2(
                    sn_trim_rect, zoom_param->zoom_info.zoom_ratio,
                    zoom_param->zoom_info.capture_aspect_ratio,
                    sn_trim_rect->width, sn_trim_rect->height,
                    prev_cxt->prev_param.cap_rot);
            }
            if (ret) {
                CMR_LOGE("Failed to get trimming window for %ld zoom level ",
                         zoom_param->zoom_level);
                goto exit;
            }
        }
    } else {
        if (handle->ops.channel_scale_capability) {
            ret = handle->ops.channel_scale_capability(
                handle->oem_handle, &sc_capability, &sc_factor, &sc_threshold);
            if (ret) {
                CMR_LOGE("ops return %ld", ret);
                goto exit;
            }
        } else {
            CMR_LOGE("ops channel_scale_capability is null");
            goto exit;
        }

        tmp_width = (cmr_u32)(sc_factor * img_cap->src_img_rect.width);
        CMR_LOGI("%d, %d, %d, %d, %d", tmp_width, img_cap->src_img_rect.width,
                 cap_size->width, cap_size->height, sc_threshold);
        if ((img_cap->src_img_rect.width >= cap_size->width ||
             cap_size->width <= sc_threshold) &&
            ZOOM_BY_CAP == prev_cxt->cap_zoom_mode) {
            /*if the out size is smaller than the in size, try to use scaler on
             * the fly*/
            if (cap_size->width > tmp_width) {
                if (tmp_width > sc_capability) {
                    img_cap->dst_img_size.width = sc_capability;
                } else {
                    img_cap->dst_img_size.width = tmp_width;
                }
                img_cap->dst_img_size.height =
                    (cmr_u32)(img_cap->src_img_rect.height * sc_factor);
            } else {
                /*just use scaler on the fly*/
                img_cap->dst_img_size.width = cap_size->width;
                img_cap->dst_img_size.height = cap_size->height;
            }
        } else if (prev_cxt->prev_param.sprd_highiso_enabled) {
            /*if the out size is larger than the in size*/
            img_cap->dst_img_size.width = img_cap->src_img_rect.width;
            img_cap->dst_img_size.height = img_cap->src_img_rect.height;
        } else {
            img_cap->dst_img_size.width = cap_size->width;
            img_cap->dst_img_size.height = cap_size->height;
        }
    }

    if (prev_cxt->prev_param.sprd_highiso_enabled) {
        struct img_size video_size = {960, 720};

        prev_cxt->lv_size = video_size;
        prev_cxt->video_size = video_size;
    }

    /*save original cap size*/
    if (prev_cxt->prev_param.video_snapshot_type == VIDEO_SNAPSHOT_VIDEO) {
        prev_cxt->cap_org_size.width = prev_cxt->actual_video_size.width;
        prev_cxt->cap_org_size.height = prev_cxt->actual_video_size.height;
    } else {
        prev_cxt->cap_org_size.width = img_cap->dst_img_size.width;
        prev_cxt->cap_org_size.height = img_cap->dst_img_size.height;
    }
    CMR_LOGI("cap_orig_size %d %d", prev_cxt->cap_org_size.width,
             prev_cxt->cap_org_size.height);

exit:
    CMR_LOGD("X");
    ATRACE_END();
    return ret;
}

cmr_int prev_get_scale_rect(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 rot,
                            struct snp_proc_param *cap_post_proc_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 i = 0;
    cmr_int is_need_scaling = 1;
    struct img_rect rect;
    struct prev_context *prev_cxt = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    struct img_size trim_sz;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!cap_post_proc_param) {
        CMR_LOGE("out param is null");
        return ret;
    }
    CMR_LOGI("camera_id %d", camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];
    sensor_mode_info = &prev_cxt->sensor_info.mode_info[prev_cxt->cap_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    CMR_LOGI("rot %d, cap_zoom_mode %ld, cap_org_size %d %d, aligned_pic_size "
             "%d %d, actual_pic_size %d %d",
             rot, prev_cxt->cap_zoom_mode, prev_cxt->cap_org_size.width,
             prev_cxt->cap_org_size.height, prev_cxt->aligned_pic_size.width,
             prev_cxt->aligned_pic_size.height, prev_cxt->actual_pic_size.width,
             prev_cxt->actual_pic_size.height);

    is_need_scaling = prev_is_need_scaling(handle, camera_id);
    if (is_need_scaling) {
        cap_post_proc_param->is_need_scaling = 1;
    } else {
        cap_post_proc_param->is_need_scaling = 0;
    }

    if (!cap_post_proc_param->is_need_scaling) {
        CMR_LOGI("no scaling");
        return ret;
    }

    CMR_LOGI("cap_zoom_mode %ld, is_need_scaling %ld", prev_cxt->cap_zoom_mode,
             cap_post_proc_param->is_need_scaling);

    if (ZOOM_BY_CAP == prev_cxt->cap_zoom_mode ||
        ZOOM_POST_PROCESS_WITH_TRIM == prev_cxt->cap_zoom_mode) {
        rect.start_x = 0;
        rect.start_y = 0;

        if (IMG_ANGLE_90 == rot || IMG_ANGLE_270 == rot) {
            rect.width = prev_cxt->cap_org_size.height;
            rect.height = prev_cxt->cap_org_size.width;
        } else {
            rect.width = prev_cxt->cap_org_size.width;
            rect.height = prev_cxt->cap_org_size.height;
        }
    } else {
        switch (rot) {
        case IMG_ANGLE_MIRROR:
            rect.start_x = sensor_mode_info->trim_width -
                           sensor_mode_info->scaler_trim.start_x -
                           sensor_mode_info->scaler_trim.width;
            rect.start_y = sensor_mode_info->scaler_trim.start_y;
            rect.width = sensor_mode_info->scaler_trim.width;
            rect.height = sensor_mode_info->scaler_trim.height;
            break;

        case IMG_ANGLE_90:
            rect.start_x = sensor_mode_info->trim_height -
                           sensor_mode_info->scaler_trim.start_y -
                           sensor_mode_info->scaler_trim.height;
            rect.start_y = sensor_mode_info->scaler_trim.start_x;
            rect.width = sensor_mode_info->scaler_trim.height;
            rect.height = sensor_mode_info->scaler_trim.width;
            break;

        case IMG_ANGLE_180:
            rect.start_x = sensor_mode_info->trim_width -
                           sensor_mode_info->scaler_trim.start_x -
                           sensor_mode_info->scaler_trim.width;
            rect.start_y = sensor_mode_info->trim_height -
                           sensor_mode_info->scaler_trim.start_y -
                           sensor_mode_info->scaler_trim.height;
            rect.width = sensor_mode_info->scaler_trim.width;
            rect.height = sensor_mode_info->scaler_trim.height;
            break;

        case IMG_ANGLE_270:
            rect.start_x = sensor_mode_info->scaler_trim.start_y;
            rect.start_y = sensor_mode_info->trim_width -
                           sensor_mode_info->scaler_trim.start_x -
                           sensor_mode_info->scaler_trim.width;
            rect.width = sensor_mode_info->scaler_trim.height;
            rect.height = sensor_mode_info->scaler_trim.width;
            break;

        case IMG_ANGLE_0:
        default:
            rect.start_x = sensor_mode_info->scaler_trim.start_x;
            rect.start_y = sensor_mode_info->scaler_trim.start_y;
            rect.width = sensor_mode_info->scaler_trim.width;
            rect.height = sensor_mode_info->scaler_trim.height;
            break;
        }

        CMR_LOGI("src rect %d %d %d %d", rect.start_x, rect.start_y, rect.width,
                 rect.height);

        trim_sz.width = rect.width;
        trim_sz.height = rect.height;
        /*caculate trim rect*/

        if (ZOOM_INFO != zoom_param->mode) {
            ret = camera_get_trim_rect(&rect, zoom_param->zoom_level, &trim_sz);
        } else {
            ret = camera_get_trim_rect2(
                &rect, zoom_param->zoom_info.zoom_ratio,
                zoom_param->zoom_info.capture_aspect_ratio, rect.width,
                rect.height, IMG_ANGLE_0);
        }

        if (ret) {
            CMR_LOGE("get scale trim failed");
            return CMR_CAMERA_FAIL;
        }

        CMR_LOGI("after rect %d %d %d %d", rect.start_x, rect.start_y,
                 rect.width, rect.height);
    }

    CMR_LOGI("out rect %d %d %d %d", rect.start_x, rect.start_y, rect.width,
             rect.height);

    /*return scale rect*/
    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        cmr_copy(&cap_post_proc_param->scaler_src_rect[i], &rect,
                 sizeof(struct img_rect));
    }

    return ret;
}

static cmr_int prev_before_set_param(struct prev_handle *handle,
                                     cmr_u32 camera_id,
                                     enum preview_param_mode mode) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 channel_bits = 0;
    cmr_u32 sec = 0;
    cmr_u32 usec = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (mode >= PARAM_MODE_MAX) {
        CMR_LOGE("invalid mode");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];

    if (PREVIEWING != prev_cxt->prev_status) {
        CMR_LOGI("not in previewing, return directly");
        return CMR_CAMERA_SUCCESS; /*return directly without error*/
    }

    CMR_LOGD("mode %d, prev_status %ld, preview_eb %d, snapshot_eb %d", mode,
             prev_cxt->prev_status, prev_cxt->prev_param.preview_eb,
             prev_cxt->prev_param.snapshot_eb);

    if (PARAM_NORMAL == mode) {
        /*normal param, get timestamp to skip frame*/
        if (handle->ops.channel_get_cap_time) {
            ret = handle->ops.channel_get_cap_time(handle->oem_handle, &sec,
                                                   &usec);
        } else {
            CMR_LOGE("ops is null");
            return CMR_CAMERA_INVALID_PARAM;
        }
        prev_cxt->restart_timestamp = sec * 1000000000LL + usec * 1000;
        prev_cxt->video_restart_timestamp = prev_cxt->restart_timestamp;
    } else {
        /*zoom in or zoom out*/
        if (prev_cxt->prev_param.snapshot_eb &&
            PREV_CHN_BUSY == prev_cxt->cap_channel_status &&
            !prev_cxt->is_zsl_frm) {

            if (!handle->ops.channel_pause) {
                CMR_LOGE("ops channel_pause null");
                ret = CMR_CAMERA_INVALID_PARAM;
                goto exit;
            }

            /*pause cap channel*/
            ret = handle->ops.channel_pause(handle->oem_handle,
                                            prev_cxt->cap_channel_id, 1);
            if (ret) {
                CMR_LOGE("pause chn failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
            prev_cxt->cap_channel_status = PREV_CHN_IDLE;
        }
    }

exit:
    return ret;
}

static cmr_int prev_after_set_param(struct prev_handle *handle,
                                    cmr_u32 camera_id,
                                    enum preview_param_mode mode,
                                    enum img_skip_mode skip_mode,
                                    cmr_u32 skip_number) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 skip_num = 0;
    cmr_u32 frm_num = 0;
    struct cmr_path_capability capability;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (mode >= PARAM_MODE_MAX) {
        CMR_LOGE("invalid mode");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];

    CMR_LOGD(
        "mode %d, prev_status %ld, preview_eb %d, snapshot_eb %d, video_eb %d",
        mode, prev_cxt->prev_status, prev_cxt->prev_param.preview_eb,
        prev_cxt->prev_param.snapshot_eb, prev_cxt->prev_param.video_eb);

    if (PREVIEWING != prev_cxt->prev_status) {
        return CMR_CAMERA_SUCCESS; /*directly return without error*/
    }

    prev_cxt->skip_mode = skip_mode;
    prev_cxt->prev_skip_num = skip_number;

    if (PARAM_NORMAL == mode) {
        /*normal param*/
        pthread_mutex_lock(&handle->thread_cxt.prev_mutex);
        prev_cxt->restart_skip_cnt = 0;
        prev_cxt->restart_skip_en = 1;
        prev_cxt->video_restart_skip_cnt = 0;
        prev_cxt->video_restart_skip_en = 1;
        prev_cxt->cap_zsl_restart_skip_cnt = 0;
        prev_cxt->cap_zsl_restart_skip_en = 1;
        pthread_mutex_unlock(&handle->thread_cxt.prev_mutex);
    } else {
        /*zoom*/
        ret = prev_set_prev_param_lightly(handle, camera_id);
        if (ret) {
            CMR_LOGE("failed to update prev param when previewing");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        if (prev_cxt->prev_param.video_eb) {
            ret = prev_set_video_param_lightly(handle, camera_id);
            if (ret) {
                CMR_LOGE("failed to update video param when previewing");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }

        if (prev_cxt->prev_param.preview_eb &&
            prev_cxt->prev_param.snapshot_eb) {
            // struct preview_out_param out_param;
            /*update capture param*/
            // ret = prev_set_cap_param(handle, camera_id, 1, NULL);
            ret = prev_set_cap_param(handle, camera_id, 1, 1, NULL);
            if (ret) {
                CMR_LOGE("failed to update cap param when previewing");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }

            if (prev_cxt->is_zsl_frm) {
                ret = prev_set_zsl_param_lightly(handle, camera_id);
                if (ret) {
                    CMR_LOGE("failed to update zsl param when previewing");
                    ret = CMR_CAMERA_FAIL;
                    goto exit;
                }
            }

            if (!handle->ops.channel_path_capability) {
                CMR_LOGE("ops channel_path_capability is null");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }

            ret = handle->ops.channel_path_capability(handle->oem_handle,
                                                      &capability);
            if (ret) {
                CMR_LOGE("channel_path_capability failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
            if (!(prev_cxt->prev_param.video_eb &&
                  !capability.is_video_prev_diff) &&
                !prev_cxt->is_zsl_frm) {
                if (!handle->ops.channel_resume) {
                    CMR_LOGE("ops channel_resume null");
                    ret = CMR_CAMERA_INVALID_PARAM;
                    goto exit;
                }

                /*resume cap channel*/
                frm_num = -1;
                if (IMG_SKIP_HW == skip_mode) {
                    skip_num = skip_number;
                }

                ret = handle->ops.channel_resume(handle->oem_handle,
                                                 prev_cxt->cap_channel_id,
                                                 skip_num, 0, frm_num);
                if (ret) {
                    CMR_LOGE("resume chn failed");
                    ret = CMR_CAMERA_FAIL;
                    goto exit;
                }
                prev_cxt->cap_channel_status = PREV_CHN_BUSY;
            }
        }
    }

exit:
    CMR_LOGD("X");
    return ret;
}

cmr_int prev_set_preview_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                                cmr_uint src_phy_addr, cmr_uint src_vir_addr,
                                cmr_s32 fd) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 width, height, buffer_size, frame_size;
    struct buffer_cfg buf_cfg;
    cmr_uint rot_index = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (/*!src_phy_addr ||*/ !src_vir_addr) {
        CMR_LOGE("in parm error");
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->prev_mem_valid_num;
    width = prev_cxt->actual_prev_size.width;
    height = prev_cxt->actual_prev_size.height;

    buffer_size = width * height;
    frame_size = prev_cxt->prev_mem_size;

    if (valid_num >= PREV_FRM_CNT || valid_num < 0) {
        CMR_LOGE("cnt error valid_num %ld", valid_num);
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    prev_cxt->prev_fd_array[valid_num] = fd;
    prev_cxt->prev_phys_addr_array[valid_num] = src_phy_addr;
    prev_cxt->prev_virt_addr_array[valid_num] = src_vir_addr;
    prev_cxt->prev_frm[valid_num].buf_size = frame_size;
    prev_cxt->prev_frm[valid_num].addr_vir.addr_y =
        prev_cxt->prev_virt_addr_array[valid_num];
    prev_cxt->prev_frm[valid_num].addr_vir.addr_u =
        prev_cxt->prev_frm[valid_num].addr_vir.addr_y + buffer_size;
    prev_cxt->prev_frm[valid_num].addr_phy.addr_y =
        prev_cxt->prev_phys_addr_array[valid_num];
    prev_cxt->prev_frm[valid_num].addr_phy.addr_u =
        prev_cxt->prev_frm[valid_num].addr_phy.addr_y + buffer_size;
    prev_cxt->prev_frm[valid_num].fd = prev_cxt->prev_fd_array[valid_num];

    prev_cxt->prev_frm[valid_num].fmt = prev_cxt->prev_param.preview_fmt;
    prev_cxt->prev_frm[valid_num].size.width = prev_cxt->actual_prev_size.width;
    prev_cxt->prev_frm[valid_num].size.height =
        prev_cxt->actual_prev_size.height;
    prev_cxt->prev_mem_valid_num++;

    buf_cfg.channel_id = prev_cxt->prev_channel_id;
    buf_cfg.base_id = CMR_PREV_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = frame_size;
    buf_cfg.flag = BUF_FLAG_RUNNING;
    if (prev_cxt->prev_param.prev_rot) {
        if (CMR_CAMERA_SUCCESS ==
            prev_search_rot_buffer(prev_cxt, CAMERA_PREVIEW)) {
            rot_index = prev_cxt->prev_rot_index % PREV_ROT_FRM_ALLOC_CNT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->prev_rot_frm[rot_index].addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->prev_rot_frm[rot_index].addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->prev_rot_frm[rot_index].addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->prev_rot_frm[rot_index].addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->prev_rot_frm[rot_index].fd;
            ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_PREVIEW, rot_index,
                                           1);
            if (ret) {
                CMR_LOGE("prev_set_rot_buffer_flag failed");
                goto exit;
            }
            CMR_LOGI("rot_index %ld prev_rot_frm_is_lock %ld", rot_index,
                     prev_cxt->prev_rot_frm_is_lock[rot_index]);
        } else {
            CMR_LOGE("error no rot buffer");
            goto exit;
        }
    } else {
        buf_cfg.addr[0].addr_y = prev_cxt->prev_frm[valid_num].addr_phy.addr_y;
        buf_cfg.addr[0].addr_u = prev_cxt->prev_frm[valid_num].addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->prev_frm[valid_num].addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->prev_frm[valid_num].addr_vir.addr_u;
        buf_cfg.fd[0] = prev_cxt->prev_frm[valid_num].fd;
    }
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel_buff_cfg failed");
        goto exit;
    }

exit:
    CMR_LOGI("fd=0x%x, channel_id=0x%lx, valid_num=%ld camera_id = %ld",
             prev_cxt->prev_frm[valid_num].fd, prev_cxt->prev_channel_id,
             prev_cxt->prev_mem_valid_num, prev_cxt->camera_id);
    ATRACE_END();
    return ret;
}

cmr_int prev_pop_preview_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                                struct frm_info *data, cmr_u32 is_to_hal) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 i;
    struct camera_frame_type frame_type;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->prev_mem_valid_num;

    if (valid_num > PREV_FRM_CNT || valid_num <= 0) {
        CMR_LOGE("wrong valid_num %ld", valid_num);
        return CMR_CAMERA_INVALID_PARAM;
    }
    if ((prev_cxt->prev_frm[0].fd == (cmr_s32)data->fd) && valid_num > 0) {
        frame_type.y_phy_addr = prev_cxt->prev_frm[0].addr_phy.addr_y;
        frame_type.y_vir_addr = prev_cxt->prev_frm[0].addr_vir.addr_y;
        frame_type.fd = prev_cxt->prev_frm[0].fd;
        frame_type.type = PREVIEW_CANCELED_FRAME;

        CMR_LOGV("fd addr 0x%x addr 0x%lx", frame_type.fd,
                 frame_type.y_vir_addr);

        for (i = 0; i < (cmr_u32)(valid_num - 1); i++) {
            prev_cxt->prev_phys_addr_array[i] =
                prev_cxt->prev_phys_addr_array[i + 1];
            prev_cxt->prev_virt_addr_array[i] =
                prev_cxt->prev_virt_addr_array[i + 1];
            prev_cxt->prev_fd_array[i] = prev_cxt->prev_fd_array[i + 1];
            memcpy(&prev_cxt->prev_frm[i], &prev_cxt->prev_frm[i + 1],
                   sizeof(struct img_frm));
        }
        prev_cxt->prev_phys_addr_array[valid_num - 1] = 0;
        prev_cxt->prev_virt_addr_array[valid_num - 1] = 0;
        prev_cxt->prev_fd_array[valid_num - 1] = 0;
        cmr_bzero(&prev_cxt->prev_frm[valid_num - 1], sizeof(struct img_frm));
        prev_cxt->prev_mem_valid_num--;
        if (is_to_hal) {
            frame_type.timestamp = data->sec * 1000000000LL + data->usec * 1000;
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);
        }
    } else {
        CMR_LOGE("got wrong buf: id=%d, data->fd=0x%x, prev_frm[0].fd=0x%x, "
                 "valid_num=%ld",
                 camera_id, data->fd, prev_cxt->prev_frm[0].fd, valid_num);
        return CMR_CAMERA_INVALID_FRAME;
    }

exit:
    CMR_LOGI("fd=0x%x, channel_id=0x%x, valid_num=%ld, camera_id = %ld",
             data->fd, data->channel_id, prev_cxt->prev_mem_valid_num,
             prev_cxt->camera_id);
    ATRACE_END();
    return ret;
}

cmr_int prev_set_sensor_datatype_buffer(struct prev_handle *handle,
                                        cmr_u32 camera_id,
                                        cmr_uint src_phy_addr,
                                        cmr_uint src_vir_addr, cmr_s32 fd) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    struct buffer_cfg buf_cfg;
    cmr_uint rot_index = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (/*!src_phy_addr ||*/ !src_vir_addr) {
        CMR_LOGE("in parm error");
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->sensor_datatype_mem_valid_num;

    if (valid_num >= PREV_FRM_CNT || valid_num < 0) {
        CMR_LOGE("cnt error valid_num %ld", valid_num);
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    switch (prev_cxt->prev_param.sensor_datatype) {
    case SENSOR_REAL_DEPTH_ENABLE:
        prev_cxt->sensor_datatype_frm[valid_num].buf_size =
            CAMERA_DEPTH_META_SIZE;
        buf_cfg.length = CAMERA_DEPTH_META_SIZE;
        break;
    case SENSOR_EMBEDDED_INFO_ENABLE:
        prev_cxt->sensor_datatype_frm[valid_num].buf_size =
            CAMERA_EMBEDDED_INFO_META_SIZE;
        buf_cfg.length = CAMERA_EMBEDDED_INFO_META_SIZE;
        break;
    case SENSOR_DATATYPE_PDAF_ENABLE:
        prev_cxt->sensor_datatype_frm[valid_num].buf_size =
            prev_cxt->prev_param.datatype_size;
        buf_cfg.length = prev_cxt->prev_param.datatype_size;
        break;
    default:
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt->sensor_datatype_phys_addr_array[valid_num] = src_phy_addr;
    prev_cxt->sensor_datatype_virt_addr_array[valid_num] = src_vir_addr;
    prev_cxt->sensor_datatype_fd_array[valid_num] = fd;
    prev_cxt->sensor_datatype_frm[valid_num].addr_vir.addr_y =
        prev_cxt->sensor_datatype_virt_addr_array[valid_num];
    prev_cxt->sensor_datatype_frm[valid_num].addr_phy.addr_y =
        prev_cxt->sensor_datatype_phys_addr_array[valid_num];
    prev_cxt->sensor_datatype_frm[valid_num].fd =
        prev_cxt->sensor_datatype_fd_array[valid_num];
    prev_cxt->sensor_datatype_frm[valid_num].fmt = IMG_DATA_TYPE_RAW;

    buf_cfg.channel_id = prev_cxt->sensor_datatype_channel_id;
    buf_cfg.base_id = CMR_SENSOR_DATATYPE_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.flag = BUF_FLAG_RUNNING;

    buf_cfg.addr[0].addr_y =
        prev_cxt->sensor_datatype_frm[valid_num].addr_phy.addr_y;
    buf_cfg.addr[0].addr_u =
        prev_cxt->sensor_datatype_frm[valid_num].addr_phy.addr_u;
    buf_cfg.addr_vir[0].addr_y =
        prev_cxt->sensor_datatype_frm[valid_num].addr_vir.addr_y;
    buf_cfg.addr_vir[0].addr_u =
        prev_cxt->sensor_datatype_frm[valid_num].addr_vir.addr_u;
    buf_cfg.fd[0] = prev_cxt->sensor_datatype_frm[valid_num].fd;
    buf_cfg.fd[1] = prev_cxt->sensor_datatype_frm[valid_num].fd;
    prev_cxt->sensor_datatype_mem_valid_num++;

    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel_buff_cfg failed");
        goto exit;
    }

exit:
    CMR_LOGI("fd=0x%x, valid_num=%ld",
             prev_cxt->sensor_datatype_frm[valid_num].fd,
             prev_cxt->sensor_datatype_mem_valid_num);
    ATRACE_END();
    return ret;
}

cmr_int prev_pop_sensor_datatype_buffer(struct prev_handle *handle,
                                        cmr_u32 camera_id,
                                        struct frm_info *data,
                                        cmr_u32 is_to_hal) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 i;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->sensor_datatype_mem_valid_num;

    if (valid_num > PREV_FRM_CNT || valid_num <= 0) {
        CMR_LOGE("cnt error valid_num %ld", valid_num);
        goto exit;
    }
    if ((prev_cxt->sensor_datatype_frm[0].fd == (cmr_s32)data->fd) &&
        valid_num > 0) {
        for (i = 0; i < (cmr_u32)(valid_num - 1); i++) {
            prev_cxt->sensor_datatype_phys_addr_array[i] =
                prev_cxt->sensor_datatype_phys_addr_array[i + 1];
            prev_cxt->sensor_datatype_virt_addr_array[i] =
                prev_cxt->sensor_datatype_virt_addr_array[i + 1];
            prev_cxt->sensor_datatype_fd_array[i] =
                prev_cxt->sensor_datatype_fd_array[i + 1];
            memcpy(&prev_cxt->sensor_datatype_frm[i],
                   &prev_cxt->sensor_datatype_frm[i + 1],
                   sizeof(struct img_frm));
        }
        prev_cxt->sensor_datatype_phys_addr_array[valid_num - 1] = 0;
        prev_cxt->sensor_datatype_virt_addr_array[valid_num - 1] = 0;

        cmr_bzero(&prev_cxt->sensor_datatype_frm[valid_num - 1],
                  sizeof(struct img_frm));
        prev_cxt->sensor_datatype_mem_valid_num--;
    } else {
        ret = CMR_CAMERA_INVALID_FRAME;
        CMR_LOGE("error yaddr 0x%x uaddr 0x%x yaddr 0x%lx uaddr 0x%lx,  "
                 "prev_cxt->sensor_datatype_frm[0].fd 0x%x",
                 data->yaddr, data->uaddr,
                 prev_cxt->sensor_datatype_frm[0].addr_phy.addr_y,
                 prev_cxt->sensor_datatype_frm[0].addr_phy.addr_u,
                 prev_cxt->sensor_datatype_frm[0].fd);
        goto exit;
    }

exit:
    CMR_LOGI("fd=0x%x, valid_num=%ld", data->fd,
             prev_cxt->sensor_datatype_mem_valid_num);
    ATRACE_END();
    return ret;
}

cmr_int prev_set_pdaf_raw_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                                 cmr_uint src_phy_addr, cmr_uint src_vir_addr,
                                 cmr_s32 fd) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 width, height, frame_size;
    struct buffer_cfg buf_cfg;
    cmr_uint rot_index = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (/*!src_phy_addr ||*/ !src_vir_addr) {
        CMR_LOGE("in parm error");
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->pdaf_mem_valid_num;
    width = prev_cxt->pdaf_rect.width;
    height = prev_cxt->pdaf_rect.height;

    frame_size = prev_cxt->pdaf_mem_size;
    if (valid_num >= PDAF_FRM_CNT || valid_num < 0) {
        CMR_LOGE("cnt error valid_num %ld", valid_num);
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    prev_cxt->pdaf_fd_array[valid_num] = fd;
    prev_cxt->pdaf_phys_addr_array[valid_num] = src_phy_addr;
    prev_cxt->pdaf_virt_addr_array[valid_num] = src_vir_addr;
    prev_cxt->pdaf_frm[valid_num].buf_size = prev_cxt->pdaf_mem_size;
    prev_cxt->pdaf_frm[valid_num].addr_vir.addr_y =
        prev_cxt->pdaf_virt_addr_array[valid_num];
    prev_cxt->pdaf_frm[valid_num].addr_phy.addr_y =
        prev_cxt->pdaf_phys_addr_array[valid_num];
    prev_cxt->pdaf_frm[valid_num].fd = prev_cxt->pdaf_fd_array[valid_num];
    prev_cxt->pdaf_frm[valid_num].fmt =
        IMG_DATA_TYPE_RAW; // IMG_DATA_TYPE_PDAF_TYPE3;
    prev_cxt->pdaf_frm[valid_num].size.width = width;
    prev_cxt->pdaf_frm[valid_num].size.height = height;

    buf_cfg.channel_id = prev_cxt->pdaf_channel_id;
    buf_cfg.base_id = CMR_PDAF_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->pdaf_mem_size;
    buf_cfg.flag = BUF_FLAG_RUNNING;

    buf_cfg.addr[0].addr_y = prev_cxt->pdaf_frm[valid_num].addr_phy.addr_y;
    buf_cfg.addr[0].addr_u = prev_cxt->pdaf_frm[valid_num].addr_phy.addr_u;
    buf_cfg.addr_vir[0].addr_y = prev_cxt->pdaf_frm[valid_num].addr_vir.addr_y;
    buf_cfg.addr_vir[0].addr_u = prev_cxt->pdaf_frm[valid_num].addr_vir.addr_u;
    buf_cfg.fd[0] = prev_cxt->pdaf_frm[valid_num].fd;
    prev_cxt->pdaf_mem_valid_num++;

    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel_buff_cfg failed");
        goto exit;
    }

exit:
    CMR_LOGI("done cnt %ld width %d height %d addr_y 0x%lx, fd 0x%x",
             prev_cxt->pdaf_mem_valid_num, width, height,
             prev_cxt->pdaf_frm[valid_num].addr_vir.addr_y,
             prev_cxt->pdaf_frm[valid_num].fd);
    ATRACE_END();
    return ret;
}

cmr_int prev_pop_pdaf_raw_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct frm_info *data, cmr_u32 is_to_hal) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 i;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->pdaf_mem_valid_num;

    CMR_LOGI("addr 0x%x 0x%x fd 0x%x", data->yaddr_vir, data->uaddr_vir,
             data->fd);

    if (valid_num > PDAF_FRM_CNT || valid_num <= 0) {
        CMR_LOGE("cnt error valid_num %ld", valid_num);
        goto exit;
    }
    if ((prev_cxt->pdaf_frm[0].fd == (cmr_s32)data->fd) && valid_num > 0) {
        for (i = 0; i < (cmr_u32)(valid_num - 1); i++) {
            prev_cxt->pdaf_phys_addr_array[i] =
                prev_cxt->pdaf_phys_addr_array[i + 1];
            prev_cxt->pdaf_virt_addr_array[i] =
                prev_cxt->pdaf_virt_addr_array[i + 1];
            prev_cxt->pdaf_fd_array[i] = prev_cxt->pdaf_fd_array[i + 1];
            memcpy(&prev_cxt->pdaf_frm[i], &prev_cxt->pdaf_frm[i + 1],
                   sizeof(struct img_frm));
        }
        prev_cxt->pdaf_phys_addr_array[valid_num - 1] = 0;
        prev_cxt->pdaf_virt_addr_array[valid_num - 1] = 0;
        prev_cxt->pdaf_fd_array[valid_num - 1] = 0;
        cmr_bzero(&prev_cxt->pdaf_frm[valid_num - 1], sizeof(struct img_frm));
        prev_cxt->pdaf_mem_valid_num--;
    } else {
        ret = CMR_CAMERA_INVALID_FRAME;
        CMR_LOGE(
            "error yaddr 0x%lx uaddr 0x%lx,  prev_cxt->pdaf_frm[0].fd 0x%x",
            prev_cxt->pdaf_frm[0].addr_vir.addr_y,
            prev_cxt->pdaf_frm[0].addr_vir.addr_u, prev_cxt->pdaf_frm[0].fd);
        goto exit;
    }

exit:
    CMR_LOGI("done cnt %ld yaddr_vir 0x%x fd 0x%x",
             prev_cxt->pdaf_mem_valid_num, data->yaddr_vir, data->fd);
    ATRACE_END();
    return ret;
}

cmr_int prev_set_video_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                              cmr_uint src_phy_addr, cmr_uint src_vir_addr,
                              cmr_s32 fd) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 width, height, buffer_size, frame_size;
    struct buffer_cfg buf_cfg;
    cmr_uint rot_index = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!src_vir_addr) {
        CMR_LOGE("in parm error");
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->video_mem_valid_num;
    width = prev_cxt->actual_video_size.width;
    height = prev_cxt->actual_video_size.height;

    buffer_size = width * height;
    frame_size = prev_cxt->video_mem_size;

    prev_cxt->video_fd_array[valid_num] = fd;
    prev_cxt->video_phys_addr_array[valid_num] = src_phy_addr;
    prev_cxt->video_virt_addr_array[valid_num] = src_vir_addr;
    prev_cxt->video_frm[valid_num].buf_size = frame_size;
    prev_cxt->video_frm[valid_num].addr_vir.addr_y =
        prev_cxt->video_virt_addr_array[valid_num];
    prev_cxt->video_frm[valid_num].addr_vir.addr_u =
        prev_cxt->video_frm[valid_num].addr_vir.addr_y + buffer_size;
    prev_cxt->video_frm[valid_num].addr_phy.addr_y =
        prev_cxt->video_phys_addr_array[valid_num];
    prev_cxt->video_frm[valid_num].addr_phy.addr_u =
        prev_cxt->video_frm[valid_num].addr_phy.addr_y + buffer_size;
    prev_cxt->video_frm[valid_num].fd = prev_cxt->video_fd_array[valid_num];
    prev_cxt->video_frm[valid_num].fmt = prev_cxt->prev_param.preview_fmt;
    prev_cxt->video_frm[valid_num].size.width =
        prev_cxt->actual_video_size.width;
    prev_cxt->video_frm[valid_num].size.height =
        prev_cxt->actual_video_size.height;
    prev_cxt->video_mem_valid_num++;

    buf_cfg.channel_id = prev_cxt->video_channel_id;
    buf_cfg.base_id = CMR_VIDEO_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = frame_size;
    buf_cfg.flag = BUF_FLAG_RUNNING;

    if (prev_cxt->prev_param.prev_rot) {
        if (CMR_CAMERA_SUCCESS ==
            prev_search_rot_buffer(prev_cxt, CAMERA_VIDEO)) {
            rot_index = prev_cxt->video_rot_index % PREV_ROT_FRM_CNT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->video_rot_frm[rot_index].addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->video_rot_frm[rot_index].addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->video_rot_frm[rot_index].addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->video_rot_frm[rot_index].addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->video_rot_frm[rot_index].fd;
            ret =
                prev_set_rot_buffer_flag(prev_cxt, CAMERA_VIDEO, rot_index, 1);
            if (ret) {
                CMR_LOGE("prev_set_rot_buffer_flag failed");
                goto exit;
            }
            CMR_LOGI("rot_index %ld prev_rot_frm_is_lock %ld", rot_index,
                     prev_cxt->video_rot_frm_is_lock[rot_index]);
        } else {
            CMR_LOGE("error no rot buffer");
            goto exit;
        }
    } else {
        buf_cfg.addr[0].addr_y = prev_cxt->video_frm[valid_num].addr_phy.addr_y;
        buf_cfg.addr[0].addr_u = prev_cxt->video_frm[valid_num].addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->video_frm[valid_num].addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->video_frm[valid_num].addr_vir.addr_u;
        buf_cfg.fd[0] = prev_cxt->video_frm[valid_num].fd;
    }
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel_buff_cfg failed");
        goto exit;
    }

exit:
    CMR_LOGD("fd=0x%x, channel_id=0x%lx, valid_num=%ld, camera_id = %ld",
             prev_cxt->video_frm[valid_num].fd, prev_cxt->video_channel_id,
             prev_cxt->video_mem_valid_num, prev_cxt->camera_id);
    ATRACE_END();
    return ret;
}

cmr_int prev_pop_video_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                              struct frm_info *data, cmr_u32 is_to_hal) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 i;
    struct camera_frame_type frame_type;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->video_mem_valid_num;

    if (valid_num > PREV_FRM_CNT || valid_num <= 0) {
        CMR_LOGE("wrong valid_num %ld", valid_num);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if ((prev_cxt->video_frm[0].fd == (cmr_s32)data->fd) && valid_num > 0) {
        frame_type.y_phy_addr = prev_cxt->video_phys_addr_array[0];
        frame_type.y_vir_addr = prev_cxt->video_virt_addr_array[0];
        frame_type.fd = prev_cxt->video_fd_array[0];
        frame_type.type = PREVIEW_VIDEO_CANCELED_FRAME;

        for (i = 0; i < (cmr_u32)valid_num - 1; i++) {
            prev_cxt->video_phys_addr_array[i] =
                prev_cxt->video_phys_addr_array[i + 1];
            prev_cxt->video_virt_addr_array[i] =
                prev_cxt->video_virt_addr_array[i + 1];
            prev_cxt->video_fd_array[i] = prev_cxt->video_fd_array[i + 1];
            memcpy(&prev_cxt->video_frm[i], &prev_cxt->video_frm[i + 1],
                   sizeof(struct img_frm));
        }
        prev_cxt->video_phys_addr_array[valid_num - 1] = 0;
        prev_cxt->video_virt_addr_array[valid_num - 1] = 0;
        prev_cxt->video_fd_array[valid_num - 1] = 0;
        cmr_bzero(&prev_cxt->video_frm[valid_num - 1], sizeof(struct img_frm));
        prev_cxt->video_mem_valid_num--;
        if (is_to_hal) {
            frame_type.timestamp = data->sec * 1000000000LL + data->usec * 1000;
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);
        }
    } else {
        CMR_LOGE("got wrong buf: id=%d, data->fd=0x%x, video_frm[0].fd=0x%x, "
                 "valid_num=%ld",
                 camera_id, data->fd, prev_cxt->video_frm[0].fd, valid_num);
        return CMR_CAMERA_INVALID_FRAME;
    }

exit:
    CMR_LOGD("fd=0x%x, channel_id=0x%x, valid_num=%ld, camera_id = %ld",
             data->fd, data->channel_id, prev_cxt->video_mem_valid_num,
             prev_cxt->camera_id);
    ATRACE_END();
    return ret;
}

cmr_int prev_set_zsl_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_uint src_phy_addr, cmr_uint src_vir_addr,
                            cmr_s32 fd) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 width, height, buffer_size, frame_size;
    struct buffer_cfg buf_cfg;
    cmr_uint rot_index = 0;
    cmr_int zoom_post_proc = 0;
    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!src_vir_addr) {
        CMR_LOGE("in parm error");
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    prev_cxt = &handle->prev_cxt[camera_id];
    if (IDLE == prev_cxt->prev_status) {
        CMR_LOGI("don't need to set buffer");
        return ret;
    }
    if (PREV_CHN_IDLE == prev_cxt->cap_channel_status) {
        CMR_LOGI("cap_channel_status idle");
        return ret;
    }
    valid_num = prev_cxt->cap_zsl_mem_valid_num;
    if (ZOOM_POST_PROCESS == zoom_post_proc) {
        width = prev_cxt->cap_sn_size.width;
        height = prev_cxt->cap_sn_size.height;
    } else if (ZOOM_POST_PROCESS_WITH_TRIM == zoom_post_proc) {
        width = prev_cxt->max_size.width;
        height = prev_cxt->max_size.height;
    } else {
        width = prev_cxt->actual_pic_size.width;
        height = prev_cxt->actual_pic_size.height;
    }
    if (prev_cxt->is_reprocessing && cxt->is_multi_mode == MODE_3D_CAPTURE) {
        width = prev_cxt->cap_sn_size.width;
        height = prev_cxt->cap_sn_size.height;
    }
    buffer_size = width * height;
    frame_size = prev_cxt->cap_zsl_mem_size;

    prev_cxt->cap_zsl_fd_array[valid_num] = fd;
    prev_cxt->cap_zsl_phys_addr_array[valid_num] = src_phy_addr;
    prev_cxt->cap_zsl_virt_addr_array[valid_num] = src_vir_addr;
    prev_cxt->cap_zsl_frm[valid_num].buf_size = frame_size;
    prev_cxt->cap_zsl_frm[valid_num].addr_vir.addr_y =
        prev_cxt->cap_zsl_virt_addr_array[valid_num];
    prev_cxt->cap_zsl_frm[valid_num].addr_vir.addr_u =
        prev_cxt->cap_zsl_frm[valid_num].addr_vir.addr_y + buffer_size;
    prev_cxt->cap_zsl_frm[valid_num].addr_phy.addr_y =
        prev_cxt->cap_zsl_phys_addr_array[valid_num];
    prev_cxt->cap_zsl_frm[valid_num].addr_phy.addr_u =
        prev_cxt->cap_zsl_frm[valid_num].addr_phy.addr_y + buffer_size;
    prev_cxt->cap_zsl_frm[valid_num].fd = prev_cxt->cap_zsl_fd_array[valid_num];
    prev_cxt->cap_zsl_frm[valid_num].fmt = prev_cxt->cap_org_fmt;
    prev_cxt->cap_zsl_frm[valid_num].size.width = width;
    prev_cxt->cap_zsl_frm[valid_num].size.height = height;
    prev_cxt->cap_zsl_mem_valid_num++;

    buf_cfg.channel_id = prev_cxt->cap_channel_id;
    buf_cfg.base_id = CMR_CAP1_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = frame_size;
    buf_cfg.flag = BUF_FLAG_RUNNING;

    if (prev_cxt->prev_param.prev_rot) {
        if (CMR_CAMERA_SUCCESS ==
            prev_search_rot_buffer(prev_cxt, CAMERA_SNAPSHOT_ZSL)) {
            rot_index = prev_cxt->cap_zsl_rot_index % PREV_ROT_FRM_CNT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->video_rot_frm[rot_index].addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->cap_zsl_rot_frm[rot_index].addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->cap_zsl_rot_frm[rot_index].addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->cap_zsl_rot_frm[rot_index].addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->cap_zsl_rot_frm[rot_index].fd;
            ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_SNAPSHOT_ZSL,
                                           rot_index, 1);
            if (ret) {
                CMR_LOGE("prev_set_rot_buffer_flag failed");
                goto exit;
            }
            CMR_LOGD("rot_index %ld prev_rot_frm_is_lock %ld", rot_index,
                     prev_cxt->video_rot_frm_is_lock[rot_index]);
        } else {
            CMR_LOGE("error no rot buffer");
            goto exit;
        }
    } else {
        buf_cfg.addr[0].addr_y =
            prev_cxt->cap_zsl_frm[valid_num].addr_phy.addr_y;
        buf_cfg.addr[0].addr_u =
            prev_cxt->cap_zsl_frm[valid_num].addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->cap_zsl_frm[valid_num].addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->cap_zsl_frm[valid_num].addr_vir.addr_u;
        buf_cfg.fd[0] = prev_cxt->cap_zsl_frm[valid_num].fd;
    }

    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel_buff_cfg failed");
        goto exit;
    }

exit:
    CMR_LOGD("fd=0x%x, channel_id=0x%lx, valid_num=%ld, camera_id = %ld",
             prev_cxt->cap_zsl_frm[valid_num].fd, prev_cxt->cap_channel_id,
             prev_cxt->cap_zsl_mem_valid_num, prev_cxt->camera_id);
    ATRACE_END();
    return ret;
}

cmr_int prev_pop_zsl_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                            struct frm_info *data, cmr_u32 is_to_hal) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 i;
    struct camera_frame_type frame_type;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->cap_zsl_mem_valid_num;

    if (valid_num > ZSL_FRM_CNT || valid_num <= 0) {
        CMR_LOGE("wrong valid_num %ld", valid_num);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if ((prev_cxt->cap_zsl_frm[0].fd == (cmr_s32)data->fd) && valid_num > 0) {
        frame_type.y_phy_addr = prev_cxt->cap_zsl_phys_addr_array[0];
        frame_type.y_vir_addr = prev_cxt->cap_zsl_virt_addr_array[0];
        frame_type.fd = prev_cxt->cap_zsl_fd_array[0];
        frame_type.type = PREVIEW_ZSL_CANCELED_FRAME;

        for (i = 0; i < (cmr_u32)valid_num - 1; i++) {
            prev_cxt->cap_zsl_phys_addr_array[i] =
                prev_cxt->cap_zsl_phys_addr_array[i + 1];
            prev_cxt->cap_zsl_virt_addr_array[i] =
                prev_cxt->cap_zsl_virt_addr_array[i + 1];
            prev_cxt->cap_zsl_fd_array[i] = prev_cxt->cap_zsl_fd_array[i + 1];
            memcpy(&prev_cxt->cap_zsl_frm[i], &prev_cxt->cap_zsl_frm[i + 1],
                   sizeof(struct img_frm));
        }
        prev_cxt->cap_zsl_phys_addr_array[valid_num - 1] = 0;
        prev_cxt->cap_zsl_virt_addr_array[valid_num - 1] = 0;
        prev_cxt->cap_zsl_fd_array[valid_num - 1] = 0;
        cmr_bzero(&prev_cxt->cap_zsl_frm[valid_num - 1],
                  sizeof(struct img_frm));
        prev_cxt->cap_zsl_mem_valid_num--;
        if (is_to_hal) {
            frame_type.timestamp = data->sec * 1000000000LL + data->usec * 1000;
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);
        }
    } else {
        CMR_LOGE("got wrong buf: id=%d, data->fd=0x%x, cap_zsl_frm[0].fd=0x%x, "
                 "valid_num=%ld",
                 camera_id, data->fd, prev_cxt->cap_zsl_frm[0].fd, valid_num);
        return CMR_CAMERA_INVALID_FRAME;
    }

exit:
    CMR_LOGD("fd=0x%x, channel_id=0x%x, valid_num=%ld, camera_id = %ld",
             data->fd, data->channel_id, prev_cxt->cap_zsl_mem_valid_num,
             prev_cxt->camera_id);
    ATRACE_END();
    return ret;
}

cmr_uint prev_get_rot_val(cmr_uint rot_enum) {
    cmr_uint rot_val = 0;

    switch (rot_enum) {
    case IMG_ANGLE_0:
        rot_val = 0;
        break;

    case IMG_ANGLE_90:
        rot_val = 1;
        break;

    case IMG_ANGLE_180:
        rot_val = 2;
        break;

    case IMG_ANGLE_270:
        rot_val = 3;
        break;

    default:
        CMR_LOGE("uncorrect params!");
        break;
    }

    CMR_LOGI("in angle %ld, out val %ld", rot_enum, rot_val);

    return rot_val;
}

cmr_uint prev_get_rot_enum(cmr_uint rot_val) {
    cmr_uint rot_enum = IMG_ANGLE_0;

    switch (rot_val) {
    case 0:
        rot_enum = IMG_ANGLE_0;
        break;

    case 1:
        rot_enum = IMG_ANGLE_90;
        break;

    case 2:
        rot_enum = IMG_ANGLE_180;
        break;

    case 3:
        rot_enum = IMG_ANGLE_270;
        break;

    default:
        CMR_LOGE("uncorrect params!");
        break;
    }

    CMR_LOGI("in val %ld, out enum %ld", rot_val, rot_enum);

    return rot_enum;
}

cmr_uint prev_set_rot_buffer_flag(struct prev_context *prev_cxt, cmr_uint type,
                                  cmr_int index, cmr_uint flag) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_uint *frm_is_lock = NULL;

    if (!prev_cxt) {
        return ret;
    }

    if (PREVIEWING == prev_cxt->prev_status) {
        if (CAMERA_PREVIEW == type) {
            frm_is_lock = &prev_cxt->prev_rot_frm_is_lock[0];
        } else if (CAMERA_VIDEO == type) {
            frm_is_lock = &prev_cxt->video_rot_frm_is_lock[0];
        } else if (CAMERA_SNAPSHOT_ZSL == type) {
            frm_is_lock = &prev_cxt->cap_zsl_rot_frm_is_lock[0];
        } else {
            CMR_LOGW("ignored  prev_status %ld, index %ld",
                     prev_cxt->prev_status, index);
            ret = CMR_CAMERA_INVALID_STATE;
        }
    }
    if (!ret && (index >= 0 && index < PREV_ROT_FRM_ALLOC_CNT) &&
        (NULL != frm_is_lock)) {
        *(frm_is_lock + index) = flag;
    } else {
        CMR_LOGE("error index %ld", index);
        ret = CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGI("[prev_rot] done ret %ld flag %ld", ret, flag);
    return ret;
}

cmr_uint prev_search_rot_buffer(struct prev_context *prev_cxt, cmr_uint type) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_uint search_index = 0;
    cmr_uint count = 0;
    cmr_uint *rot_index = NULL;
    cmr_uint *frm_is_lock = NULL;

    if (!prev_cxt) {
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    if (PREVIEWING == prev_cxt->prev_status) {
        if (CAMERA_PREVIEW == type) {
            search_index = prev_cxt->prev_rot_index;
            rot_index = &prev_cxt->prev_rot_index;
            frm_is_lock = &prev_cxt->prev_rot_frm_is_lock[0];
        } else if (CAMERA_VIDEO == type) {
            search_index = prev_cxt->video_rot_index;
            rot_index = &prev_cxt->video_rot_index;
            frm_is_lock = &prev_cxt->video_rot_frm_is_lock[0];
        } else if (CAMERA_SNAPSHOT_ZSL == type) {
            search_index = prev_cxt->cap_zsl_rot_index;
            rot_index = &prev_cxt->cap_zsl_rot_index;
            frm_is_lock = &prev_cxt->cap_zsl_rot_frm_is_lock[0];
        } else {
            CMR_LOGW("ignored  prev_status %ld, type %ld",
                     prev_cxt->prev_status, type);
            ret = CMR_CAMERA_INVALID_STATE;
        }
    }
    if (!ret && (NULL != frm_is_lock)) {
        for (count = 0; count < PREV_ROT_FRM_ALLOC_CNT; count++) {
            search_index += count;
            search_index %= PREV_ROT_FRM_ALLOC_CNT;
            if (0 == *(frm_is_lock + search_index)) {
                *rot_index = search_index;
                CMR_LOGI("[prev_rot] find %ld", search_index);
                ret = CMR_CAMERA_SUCCESS;
                break;
            } else {
                ret = CMR_CAMERA_INVALID_PARAM;
                CMR_LOGV("[prev_rot] rot buffer %ld is locked", search_index);
            }
        }
    }
    /*	search_index = 0;//prev_cxt->prev_rot_index;

            for (count = 0; count < PREV_ROT_FRM_CNT; count++){
                    search_index = count;
                    //search_index %= PREV_ROT_FRM_CNT;
                    CMR_LOGI("[prev_rot] index %d lock %d",
                                    search_index,
                                    prev_cxt->prev_rot_frm_is_lock[search_index]);
                    if (!data) {
                            if (0 ==
       prev_cxt->prev_rot_frm_is_lock[search_index]) {
                                    ret = CMR_CAMERA_SUCCESS;
                                    prev_cxt->prev_rot_index = search_index;
                                    CMR_LOGI("[prev_rot] find %d",
       search_index);
                                    break;
                            } else {
                                    CMR_LOGI("[prev_rot] rot buffer %ld is
       locked", search_index);
                            }
                    } else {
                            CMR_LOGI("[prev_rot] index %d lock %d frame 0x%x
       rot_frm 0x%x",
                                    search_index,
                                    prev_cxt->prev_rot_frm_is_lock[search_index],
                                    data->yaddr,
                                    prev_cxt->prev_rot_frm[search_index].addr_phy.addr_y);
                            if (1 ==
       prev_cxt->prev_rot_frm_is_lock[search_index] && data->yaddr ==
       prev_cxt->prev_rot_frm[search_index].addr_phy.addr_y) {
                                    ret = CMR_CAMERA_SUCCESS;
                                    prev_cxt->prev_rot_index = search_index;
                                    CMR_LOGI("[prev_rot] match %d",
       search_index);
                                    break;
                            } else {
                                    CMR_LOGI("[prev_rot] no match rot buffer %ld
       is locked", search_index);
                            }
                    }
            }*/
    CMR_LOGI("[prev_rot] done ret %ld search_index %ld", ret, search_index);
    return ret;
}

cmr_uint prev_get_src_rot_buffer(struct prev_context *prev_cxt,
                                 struct frm_info *data, cmr_uint *index) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_uint count = 0;
    cmr_uint *frm_is_lock;
    struct img_frm *frm_ptr = NULL;

    if (!prev_cxt || !index) {
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    if (PREVIEWING == prev_cxt->prev_status) {
        if (IS_PREVIEW_FRM(data->frame_id)) {
            CMR_LOGI("PREVIEW");
            frm_is_lock = &prev_cxt->prev_rot_frm_is_lock[0];
            frm_ptr = &prev_cxt->prev_rot_frm[0];
        } else if (IS_VIDEO_FRM(data->frame_id)) {
            CMR_LOGI("VIDEO");
            frm_is_lock = &prev_cxt->video_rot_frm_is_lock[0];
            frm_ptr = &prev_cxt->video_rot_frm[0];
        } else if (IS_ZSL_FRM(data->frame_id)) {
            CMR_LOGI("ZSL");
            frm_is_lock = &prev_cxt->cap_zsl_rot_frm_is_lock[0];
            frm_ptr = &prev_cxt->cap_zsl_rot_frm[0];
        } else {
            CMR_LOGW("ignored  prev_status %ld, frame_id 0x%x",
                     prev_cxt->prev_status, data->frame_id);
            ret = CMR_CAMERA_INVALID_STATE;
        }
    }

    if (!ret && (frm_ptr != NULL)) {
        for (count = 0; count < PREV_ROT_FRM_ALLOC_CNT; count++) {
            CMR_LOGI("[prev_rot] fd 0x%x 0x%x %ld", (frm_ptr + count)->fd,
                     data->fd, *(frm_is_lock + count));
            if (1 == *(frm_is_lock + count) &&
                (cmr_s32)data->fd == (frm_ptr + count)->fd) {
                *index = count;
                //*rot_frm = *(frm_ptr + count);
                CMR_LOGI("[prev_rot] find %ld", count);
                ret = CMR_CAMERA_SUCCESS;
                break;
            } else {
                ret = CMR_CAMERA_INVALID_PARAM;
                CMR_LOGV("[prev_rot] rot buffer %ld is locked", count);
            }
        }
    }

    CMR_LOGI("[prev_rot] done ret %ld count %ld", ret, count);
    return ret;
}

cmr_uint prev_search_rot_video_buffer(struct prev_context *prev_cxt) {
    cmr_uint ret = CMR_CAMERA_FAIL;
    cmr_uint search_index;
    cmr_uint count = 0;

    if (!prev_cxt) {
        return ret;
    }
    search_index = prev_cxt->video_rot_index;

    for (count = 0; count < PREV_ROT_FRM_CNT; count++) {
        search_index += count;
        search_index %= PREV_ROT_FRM_CNT;
        if (0 == prev_cxt->video_rot_frm_is_lock[search_index]) {
            ret = CMR_CAMERA_SUCCESS;
            prev_cxt->video_rot_index = search_index;
            CMR_LOGI("[prev_rot] find %ld", search_index);
            break;
        } else {
            CMR_LOGV("[prev_rot] rot buffer %ld is locked", search_index);
        }
    }

    return ret;
}

cmr_uint prev_search_rot_zsl_buffer(struct prev_context *prev_cxt) {
    cmr_uint ret = CMR_CAMERA_FAIL;
    cmr_uint search_index;
    cmr_uint count = 0;

    if (!prev_cxt) {
        return ret;
    }
    search_index = prev_cxt->cap_zsl_rot_index;

    for (count = 0; count < PREV_ROT_FRM_CNT; count++) {
        search_index += count;
        search_index %= PREV_ROT_FRM_CNT;
        if (0 == prev_cxt->cap_zsl_rot_frm_is_lock[search_index]) {
            ret = CMR_CAMERA_SUCCESS;
            prev_cxt->cap_zsl_rot_index = search_index;
            CMR_LOGI("[prev_rot] find %ld", search_index);
            break;
        } else {
            CMR_LOGW("[prev_rot] rot buffer %ld is locked", search_index);
        }
    }

    return ret;
}

cmr_int prev_start_rotate(struct prev_handle *handle, cmr_u32 camera_id,
                          struct frm_info *data) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_u32 frm_id = 0;
    cmr_u32 rot_frm_id = 0;
    struct prev_context *prev_cxt = &handle->prev_cxt[camera_id];
    struct rot_param rot_param;
    struct cmr_op_mean op_mean;

    if (!handle->ops.start_rot) {
        CMR_LOGE("ops start_rot is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&rot_param, sizeof(struct rot_param));
    cmr_bzero(&op_mean, sizeof(struct cmr_op_mean));
    /*check preview status and frame id*/
    if (PREVIEWING == prev_cxt->prev_status) {
        ret = prev_get_src_rot_buffer(prev_cxt, data, (cmr_uint *)&rot_frm_id);
        if (ret) {
            CMR_LOGE("get src rot buffer failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        if (IS_PREVIEW_FRM(data->frame_id)) {
            frm_id = data->frame_id - CMR_PREV_ID_BASE;
            rot_param.dst_img = &prev_cxt->prev_frm[frm_id];
            rot_param.src_img = &prev_cxt->prev_rot_frm[rot_frm_id];
            rot_param.src_img->data_end = prev_cxt->prev_data_endian;
            rot_param.dst_img->data_end = prev_cxt->prev_data_endian;
        } else if (IS_VIDEO_FRM(data->frame_id)) {
            frm_id = data->frame_id - CMR_VIDEO_ID_BASE;
            rot_param.dst_img = &prev_cxt->video_frm[frm_id];
            rot_param.src_img = &prev_cxt->video_rot_frm[rot_frm_id];
            rot_param.src_img->data_end = prev_cxt->video_data_endian;
            rot_param.dst_img->data_end = prev_cxt->video_data_endian;
        } else if (IS_ZSL_FRM(data->frame_id)) {
            frm_id = data->frame_id - CMR_CAP1_ID_BASE;
            rot_param.dst_img = &prev_cxt->cap_zsl_frm[frm_id];
            rot_param.src_img = &prev_cxt->cap_zsl_rot_frm[rot_frm_id];
            rot_param.src_img->data_end = prev_cxt->cap_data_endian;
            rot_param.dst_img->data_end = prev_cxt->cap_data_endian;
        } else {
            CMR_LOGW("ignored  prev_status %ld, frame_id 0x%x",
                     prev_cxt->prev_status, data->frame_id);
            ret = CMR_CAMERA_INVALID_STATE;
            goto exit;
        }

        rot_param.angle = prev_cxt->prev_param.prev_rot;
        ;

        CMR_LOGI("frm_id %d, rot_frm_id %d", frm_id, rot_frm_id);

        op_mean.rot = rot_param.angle;

        ret = handle->ops.start_rot(handle->oem_handle, (cmr_handle)handle,
                                    rot_param.src_img, rot_param.dst_img,
                                    &op_mean);
        if (ret) {
            CMR_LOGE("rot failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

    } else {
        CMR_LOGW("ignored  prev_status %ld, frame_id 0x%x",
                 prev_cxt->prev_status, data->frame_id);
        ret = CMR_CAMERA_INVALID_STATE;
    }

exit:
    CMR_LOGI("out");
    return ret;
}

cmr_int prev_start_video_rotate(struct prev_handle *handle, cmr_u32 camera_id,
                                struct frm_info *data) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_u32 frm_id = 0;
    cmr_u32 rot_frm_id = 0;
    struct prev_context *prev_cxt = &handle->prev_cxt[camera_id];
    struct rot_param rot_param;
    struct cmr_op_mean op_mean;

    if (!handle->ops.start_rot) {
        CMR_LOGE("ops start_rot is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&rot_param, sizeof(struct rot_param));
    cmr_bzero(&op_mean, sizeof(struct cmr_op_mean));
    /*check preview status and frame id*/
    if (PREVIEWING == prev_cxt->prev_status && IS_VIDEO_FRM(data->frame_id)) {

        frm_id = data->frame_id - CMR_PREV_ID_BASE;
        rot_frm_id = prev_cxt->video_rot_index % PREV_ROT_FRM_CNT;

        CMR_LOGI("frm_id %d, rot_frm_id %d", frm_id, rot_frm_id);

        rot_param.angle = prev_cxt->prev_param.prev_rot;
        rot_param.src_img = &prev_cxt->video_frm[frm_id];
        rot_param.dst_img = &prev_cxt->video_rot_frm[rot_frm_id];
        rot_param.src_img->data_end = prev_cxt->video_data_endian;
        rot_param.dst_img->data_end = prev_cxt->video_data_endian;

        op_mean.rot = rot_param.angle;

        ret = handle->ops.start_rot(handle->oem_handle, (cmr_handle)handle,
                                    rot_param.src_img, rot_param.dst_img,
                                    &op_mean);
        if (ret) {
            CMR_LOGE("rot failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

    } else {
        CMR_LOGW("ignored  prev_status %ld, frame_id 0x%x",
                 prev_cxt->prev_status, data->frame_id);
        ret = CMR_CAMERA_INVALID_STATE;
    }

exit:
    CMR_LOGI("out");
    return ret;
}

cmr_int prev_start_zsl_rotate(struct prev_handle *handle, cmr_u32 camera_id,
                              struct frm_info *data) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_u32 frm_id = 0;
    cmr_u32 rot_frm_id = 0;
    struct prev_context *prev_cxt = &handle->prev_cxt[camera_id];
    struct rot_param rot_param;
    struct cmr_op_mean op_mean;

    if (!handle->ops.start_rot) {
        CMR_LOGE("ops start_rot is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&rot_param, sizeof(struct rot_param));
    cmr_bzero(&op_mean, sizeof(struct cmr_op_mean));
    /*check preview status and frame id*/
    if (PREVIEWING == prev_cxt->prev_status && IS_ZSL_FRM(data->frame_id)) {

        frm_id = data->frame_id - CMR_PREV_ID_BASE;
        rot_frm_id = prev_cxt->cap_zsl_rot_index % PREV_ROT_FRM_CNT;

        CMR_LOGI("frm_id %d, rot_frm_id %d", frm_id, rot_frm_id);

        rot_param.angle = prev_cxt->prev_param.cap_rot;
        rot_param.src_img = &prev_cxt->cap_zsl_frm[frm_id];
        rot_param.dst_img = &prev_cxt->cap_zsl_rot_frm[rot_frm_id];
        rot_param.src_img->data_end = prev_cxt->cap_data_endian;
        rot_param.dst_img->data_end = prev_cxt->cap_data_endian;

        op_mean.rot = rot_param.angle;

        ret = handle->ops.start_rot(handle->oem_handle, (cmr_handle)handle,
                                    rot_param.src_img, rot_param.dst_img,
                                    &op_mean);
        if (ret) {
            CMR_LOGE("rot failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

    } else {
        CMR_LOGW("ignored  prev_status %ld, frame_id 0x%x",
                 prev_cxt->prev_status, data->frame_id);
        ret = CMR_CAMERA_INVALID_STATE;
    }

exit:
    CMR_LOGI("out");
    return ret;
}

cmr_uint prev_get_prev_res_buffer_count(struct prev_context *prev_cxt) {
    cmr_uint count = 0;

    //for cts
    if ((prev_cxt->actual_pic_size.width >= UHD_WIDTH)
            && (prev_cxt->actual_video_size.width >= UHD_WIDTH)
            && (1== prev_cxt->prev_param.is_uhd_recording_mode)) {
        count = 2;
    } else {
        count = PREV_RESERVED_FRM_CNT;
    }

    return count;
}

cmr_uint prev_get_capzsl_res_buffer_count(struct prev_context *prev_cxt) {
    cmr_uint count = 0;

    //for cts
    if ((prev_cxt->actual_pic_size.width >= UHD_WIDTH)
            && (prev_cxt->actual_video_size.width >= UHD_WIDTH)
            && (1== prev_cxt->prev_param.is_uhd_recording_mode)) {
        count = 2;
    } else {
        count = CAP_ZSL_RESERVED_FRM_CNT;
    }

    return count;
}

cmr_int prev_get_cap_post_proc_param(struct prev_handle *handle,
                                     cmr_u32 camera_id, cmr_u32 encode_angle,
                                     struct snp_proc_param *out_param_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 cap_rot = 0;
    cmr_u32 cfg_cap_rot = 0;
    cmr_u32 is_cfg_rot_cap = 0;
    cmr_u32 tmp_refer_rot = 0;
    cmr_u32 tmp_req_rot = 0;
    cmr_u32 i = 0;
    cmr_u32 is_normal_cap = 0;
    struct img_size *org_pic_size = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!out_param_ptr) {
        CMR_LOGE("invalid param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    cap_rot = prev_cxt->prev_param.cap_rot;
    is_cfg_rot_cap = prev_cxt->prev_param.is_cfg_rot_cap;
    cfg_cap_rot = encode_angle;
    org_pic_size = &prev_cxt->prev_param.picture_size;

    if (!prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb) {
        is_normal_cap = 1;
    } else {
        is_normal_cap = 0;
    }

    CMR_LOGI("cap_rot %d, is_cfg_rot_cap %d, cfg_cap_rot %d, is_normal_cap %d",
             cap_rot, is_cfg_rot_cap, cfg_cap_rot, is_normal_cap);

    if (is_normal_cap) {
        if ((IMG_ANGLE_0 != cap_rot) ||
            (is_cfg_rot_cap && (IMG_ANGLE_0 != cfg_cap_rot))) {

            if (IMG_ANGLE_0 != cfg_cap_rot && IMG_ANGLE_180 != cfg_cap_rot) {
                prev_cxt->actual_pic_size.width =
                    prev_cxt->aligned_pic_size.height;
                prev_cxt->actual_pic_size.height =
                    prev_cxt->aligned_pic_size.width;

                prev_cxt->dealign_actual_pic_size.width = org_pic_size->height;
                prev_cxt->dealign_actual_pic_size.height = org_pic_size->width;
            } else if (IMG_ANGLE_180 != cap_rot) {
                prev_cxt->actual_pic_size.width =
                    prev_cxt->aligned_pic_size.width;
                prev_cxt->actual_pic_size.height =
                    prev_cxt->aligned_pic_size.height;

                prev_cxt->dealign_actual_pic_size.width = org_pic_size->width;
                prev_cxt->dealign_actual_pic_size.height = org_pic_size->height;
            } else {
                CMR_LOGI("default");
            }

            CMR_LOGI(
                "now actual_pic_size %d %d,  dealign_actual_pic_size %d %d",
                prev_cxt->actual_pic_size.width,
                prev_cxt->actual_pic_size.height,
                prev_cxt->dealign_actual_pic_size.width,
                prev_cxt->dealign_actual_pic_size.height);

            tmp_req_rot = prev_get_rot_val(cfg_cap_rot);
            tmp_refer_rot = prev_get_rot_val(cap_rot);
            tmp_req_rot += tmp_refer_rot;
            if (tmp_req_rot >= IMG_ANGLE_MIRROR) {
                tmp_req_rot -= IMG_ANGLE_MIRROR;
            }
            cap_rot = prev_get_rot_enum(tmp_req_rot);
        } else {
            prev_cxt->actual_pic_size.width = prev_cxt->aligned_pic_size.width;
            prev_cxt->actual_pic_size.height =
                prev_cxt->aligned_pic_size.height;
            prev_cxt->dealign_actual_pic_size.width = org_pic_size->width;
            prev_cxt->dealign_actual_pic_size.height = org_pic_size->height;
        }
    } else {
        if (is_cfg_rot_cap && (IMG_ANGLE_0 != cfg_cap_rot)) {

            if (IMG_ANGLE_0 != cfg_cap_rot && IMG_ANGLE_180 != cfg_cap_rot) {
                prev_cxt->actual_pic_size.width =
                    prev_cxt->aligned_pic_size.height;
                prev_cxt->actual_pic_size.height =
                    prev_cxt->aligned_pic_size.width;

                prev_cxt->dealign_actual_pic_size.width = org_pic_size->height;
                prev_cxt->dealign_actual_pic_size.height = org_pic_size->width;
            } else {
                prev_cxt->actual_pic_size.width =
                    prev_cxt->aligned_pic_size.width;
                prev_cxt->actual_pic_size.height =
                    prev_cxt->aligned_pic_size.height;
                prev_cxt->dealign_actual_pic_size.width = org_pic_size->width;
                prev_cxt->dealign_actual_pic_size.height = org_pic_size->height;
            }

            CMR_LOGI(
                "2 now actual_pic_size %d %d,  dealign_actual_pic_size %d %d",
                prev_cxt->actual_pic_size.width,
                prev_cxt->actual_pic_size.height,
                prev_cxt->dealign_actual_pic_size.width,
                prev_cxt->dealign_actual_pic_size.height);

        } else {
            prev_cxt->actual_pic_size.width = prev_cxt->aligned_pic_size.width;
            prev_cxt->actual_pic_size.height =
                prev_cxt->aligned_pic_size.height;
            prev_cxt->dealign_actual_pic_size.width = org_pic_size->width;
            prev_cxt->dealign_actual_pic_size.height = org_pic_size->height;
        }
        cap_rot = cfg_cap_rot;
    }

    CMR_LOGI("now cap_rot %d", cap_rot);

    /*if (prev_cxt->prev_param.video_eb) {
            cap_rot = 0;
    }*/
    ret = prev_get_scale_rect(handle, camera_id, cap_rot, out_param_ptr);
    if (ret) {
        CMR_LOGE("get scale rect failed");
    }

    out_param_ptr->rot_angle = cap_rot;
    out_param_ptr->channel_zoom_mode = prev_cxt->cap_zoom_mode;
    out_param_ptr->snp_size = prev_cxt->aligned_pic_size;
    out_param_ptr->actual_snp_size = prev_cxt->actual_pic_size;
    out_param_ptr->dealign_actual_snp_size = prev_cxt->dealign_actual_pic_size;
    out_param_ptr->max_size = prev_cxt->max_size;

    cmr_copy(&out_param_ptr->chn_out_frm[0], &prev_cxt->cap_frm[0],
             CMR_CAPTURE_MEM_SUM * sizeof(struct img_frm));

    cmr_copy(&out_param_ptr->mem[0], &prev_cxt->cap_mem[0],
             CMR_CAPTURE_MEM_SUM * sizeof(struct cmr_cap_mem));

    CMR_LOGI("rot_angle %ld, channel_zoom_mode %ld, is_need_scaling %ld",
             out_param_ptr->rot_angle, out_param_ptr->channel_zoom_mode,
             out_param_ptr->is_need_scaling);

    CMR_LOGI("cap_org_size %d, %d", prev_cxt->cap_org_size.width,
             prev_cxt->cap_org_size.height);

    CMR_LOGI(
        "snp_size %d %d, actual_snp_size %d %d, dealign_actual_pic_size %d %d",
        out_param_ptr->snp_size.width, out_param_ptr->snp_size.height,
        out_param_ptr->actual_snp_size.width,
        out_param_ptr->actual_snp_size.height,
        out_param_ptr->dealign_actual_snp_size.width,
        out_param_ptr->dealign_actual_snp_size.height);

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        CMR_LOGI(
            "chn_out_frm[%d], format %d, size %d %d, fd 0x%x, buf_size 0x%x", i,
            out_param_ptr->chn_out_frm[i].fmt,
            out_param_ptr->chn_out_frm[i].size.width,
            out_param_ptr->chn_out_frm[i].size.height,
            out_param_ptr->chn_out_frm[i].fd,
            out_param_ptr->chn_out_frm[i].buf_size);
    }

    return ret;
}

cmr_int prev_pause_cap_channel(struct prev_handle *handle, cmr_u32 camera_id,
                               struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 snapshot_enable = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    CMR_LOGI("in");

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!handle->ops.channel_pause) {
        CMR_LOGE("ops channel_start is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    snapshot_enable = prev_cxt->prev_param.snapshot_eb;
    CMR_LOGI("snapshot_eb %d, channel_id %d, %ld", snapshot_enable,
             data->channel_id, prev_cxt->cap_channel_id);

    if (snapshot_enable && (data->channel_id == prev_cxt->cap_channel_id)) {
        /*pause channel*/
        ret = handle->ops.channel_pause(handle->oem_handle,
                                        prev_cxt->cap_channel_id, 1);
        if (ret) {
            CMR_LOGE("channel_pause failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        prev_cxt->cap_zsl_mem_valid_num = 0;
    }

exit:
    CMR_LOGI("out, ret %ld", ret);
    return ret;
}

cmr_int prev_resume_cap_channel(struct prev_handle *handle, cmr_u32 camera_id,
                                struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 snapshot_enable = 0;
    cmr_u32 channel_bits = 0;
    struct preview_out_param out_param;
    struct buffer_cfg buf_cfg;
    cmr_u32 i;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    CMR_LOGI("in");

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!handle->ops.channel_resume) {
        CMR_LOGE("ops channel_resume is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    snapshot_enable = prev_cxt->prev_param.snapshot_eb;
    CMR_LOGI("snapshot_eb %d, channel_id %d, %ld", snapshot_enable,
             data->channel_id, prev_cxt->cap_channel_id);

    if (snapshot_enable && (data->channel_id == prev_cxt->cap_channel_id)) {
        if (prev_cxt->prev_param.preview_eb &&
            prev_cxt->prev_param.snapshot_eb) {
            /*zsl*/
            if (FRAME_CONTINUE == prev_cxt->prev_param.frame_ctrl) {
            } else {
                CMR_LOGE("wrong cap param!");
            }
        } else {
            if (1 != prev_cxt->prev_param.frame_count) {
                buf_cfg.channel_id = prev_cxt->cap_channel_id;
                buf_cfg.base_id = CMR_CAP0_ID_BASE;
                buf_cfg.count = CMR_CAPTURE_MEM_SUM;
                buf_cfg.flag = BUF_FLAG_INIT;
                buf_cfg.length = prev_cxt->actual_pic_size.width *
                                 prev_cxt->actual_pic_size.height * 3 / 2;
                for (i = 0; i < buf_cfg.count; i++) {
                    buf_cfg.addr[i].addr_y =
                        prev_cxt->cap_frm[i].addr_phy.addr_y;
                    buf_cfg.addr[i].addr_u =
                        prev_cxt->cap_frm[i].addr_phy.addr_u;
                    buf_cfg.addr_vir[i].addr_y =
                        prev_cxt->cap_frm[i].addr_vir.addr_y;
                    buf_cfg.addr_vir[i].addr_u =
                        prev_cxt->cap_frm[i].addr_vir.addr_u;
                    buf_cfg.fd[i] = prev_cxt->cap_frm[i].fd;
                }
                ret =
                    handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
                if (ret) {
                    CMR_LOGE("channel_buff_cfg failed");
                    ret = CMR_CAMERA_FAIL;
                    goto exit;
                }
            }
        }

        /*resume channel*/
        ret = handle->ops.channel_resume(
            handle->oem_handle, prev_cxt->cap_channel_id,
            prev_cxt->cap_skip_num, 0, prev_cxt->prev_param.frame_count);
        if (ret) {
            CMR_LOGE("channel_resume failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        cb_data_info.cb_type = PREVIEW_EVT_CB_RESUME;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }

exit:
    CMR_LOGI("out, ret %ld", ret);
    return ret;
}

cmr_int prev_restart_cap_channel(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 preview_enable = 0;
    cmr_u32 snapshot_enable = 0;
    cmr_u32 channel_id = 0;
    struct video_start_param video_param;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct img_data_end endian;
    struct buffer_cfg buf_cfg;
    cmr_u32 is_capture_zsl = 0;
    cmr_u32 i;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    CMR_LOGI("in");

    prev_cxt = &handle->prev_cxt[camera_id];

    preview_enable = prev_cxt->prev_param.preview_eb;
    snapshot_enable = prev_cxt->prev_param.snapshot_eb;
    CMR_LOGI("preview_eb%d, snapshot_eb %d, channel_id %d, %ld, isp_status %ld",
             preview_enable, snapshot_enable, data->channel_id,
             prev_cxt->cap_channel_id, prev_cxt->isp_status);

    if (prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb) {
        /*zsl*/
        if (FRAME_CONTINUE == prev_cxt->prev_param.frame_ctrl) {
            is_capture_zsl = 1;
        }
    }

    if (snapshot_enable && (data->channel_id == prev_cxt->cap_channel_id)) {

        /*reconfig the channel with the params saved before*/
        ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                      &prev_cxt->restart_chn_param, &channel_id,
                                      &endian);
        if (ret) {
            CMR_LOGE("channel config failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        CMR_LOGI("cap chn id is %ld", prev_cxt->cap_channel_id);
        prev_cxt->cap_channel_status = PREV_CHN_BUSY;

        /* for capture, not skip frame for now, for cts
         * testMandatoryOutputCombinations issue */
        if ((prev_cxt->skip_mode == IMG_SKIP_SW_KER) && 0) {
            /*config skip num buffer*/
            for (i = 0; i < prev_cxt->cap_skip_num; i++) {
                cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
                buf_cfg.channel_id = prev_cxt->cap_channel_id;
                if (is_capture_zsl) {
                    buf_cfg.base_id = CMR_CAP1_ID_BASE;
                } else {
                    buf_cfg.base_id = CMR_CAP0_ID_BASE;
                }
                buf_cfg.count = 1;
                buf_cfg.length = prev_cxt->cap_zsl_mem_size;
                buf_cfg.is_reserved_buf = 0;
                buf_cfg.flag = BUF_FLAG_INIT;
                buf_cfg.addr[0].addr_y =
                    prev_cxt->cap_zsl_reserved_frm[0].addr_phy.addr_y;
                buf_cfg.addr[0].addr_u =
                    prev_cxt->cap_zsl_reserved_frm[0].addr_phy.addr_u;
                buf_cfg.addr_vir[0].addr_y =
                    prev_cxt->cap_zsl_reserved_frm[0].addr_vir.addr_y;
                buf_cfg.addr_vir[0].addr_u =
                    prev_cxt->cap_zsl_reserved_frm[0].addr_vir.addr_u;
                buf_cfg.fd[0] = prev_cxt->cap_zsl_reserved_frm[0].fd;
                ret =
                    handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
                if (ret) {
                    CMR_LOGE("channel buff config failed");
                    ret = CMR_CAMERA_FAIL;
                    goto exit;
                }
            }
        }

        prev_cxt->restart_chn_param.buffer.channel_id = channel_id;
        ret = handle->ops.channel_buff_cfg(handle->oem_handle,
                                           &prev_cxt->restart_chn_param.buffer);
        if (ret) {
            CMR_LOGE("channel buff config failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        /*config reserved buffer*/
        cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
        buf_cfg.channel_id = prev_cxt->cap_channel_id;
        if (is_capture_zsl) {
            buf_cfg.base_id = CMR_CAP1_ID_BASE;
        } else {
            buf_cfg.base_id = CMR_CAP0_ID_BASE;
        }
        buf_cfg.count = CAP_ZSL_RESERVED_FRM_CNT;
        buf_cfg.length = prev_cxt->cap_zsl_mem_size;
        buf_cfg.is_reserved_buf = 1;
        buf_cfg.flag = BUF_FLAG_INIT;
        for (i = 0; i < CAP_ZSL_RESERVED_FRM_CNT; i++) {
            buf_cfg.addr[i].addr_y =
                prev_cxt->cap_zsl_reserved_frm[i].addr_phy.addr_y;
            buf_cfg.addr[i].addr_u =
                prev_cxt->cap_zsl_reserved_frm[i].addr_phy.addr_u;
            buf_cfg.addr_vir[i].addr_y =
                prev_cxt->cap_zsl_reserved_frm[i].addr_vir.addr_y;
            buf_cfg.addr_vir[i].addr_u =
                prev_cxt->cap_zsl_reserved_frm[i].addr_vir.addr_u;
            buf_cfg.fd[i] = prev_cxt->cap_zsl_reserved_frm[i].fd;
        }
        ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
        if (ret) {
            CMR_LOGE("channel buff config failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        ret = prev_start(handle, camera_id, 1, 0);
        if (ret) {
            CMR_LOGE("prev start failed");
            goto exit;
        }
    }

exit:
    CMR_LOGI("out, ret %ld", ret);
    return ret;
}

cmr_int prev_fd_open(struct prev_handle *handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct ipm_open_in in_param;
    struct ipm_open_out out_param;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    CMR_LOGI("is_support_fd %ld, is_fd_on %ld",
             prev_cxt->prev_param.is_support_fd, prev_cxt->prev_param.is_fd_on);

    if (!prev_cxt->prev_param.is_support_fd) {
        CMR_LOGD("not support fd");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (prev_cxt->fd_handle) {
        CMR_LOGI("fd inited already");
        return ret;
    }

    in_param.frame_cnt = 1;
    if ((IMG_ANGLE_90 == prev_cxt->prev_param.prev_rot) ||
        (IMG_ANGLE_270 == prev_cxt->prev_param.prev_rot)) {
        in_param.frame_size.width = prev_cxt->actual_prev_size.height;
        in_param.frame_size.height = prev_cxt->actual_prev_size.width;
        in_param.frame_rect.start_x = 0;
        in_param.frame_rect.start_y = 0;
        in_param.frame_rect.width = in_param.frame_size.height;
        in_param.frame_rect.height = in_param.frame_size.width;
    } else {
        in_param.frame_size.width = prev_cxt->actual_prev_size.width;
        in_param.frame_size.height = prev_cxt->actual_prev_size.height;
        in_param.frame_rect.start_x = 0;
        in_param.frame_rect.start_y = 0;
        in_param.frame_rect.width = in_param.frame_size.width;
        in_param.frame_rect.height = in_param.frame_size.height;
    }

    in_param.reg_cb = prev_fd_cb;

    ret = cmr_ipm_open(handle->ipm_handle, IPM_TYPE_FD, &in_param, &out_param,
                       &prev_cxt->fd_handle);
    if (ret) {
        CMR_LOGE("cmr_ipm_open failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    CMR_LOGD("fd_handle 0x%p", prev_cxt->fd_handle);

exit:
    CMR_LOGI("out, ret %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int prev_fd_close(struct prev_handle *handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;

    prev_cxt = &handle->prev_cxt[camera_id];

    CMR_LOGI("is_support_fd %ld, is_fd_on %ld",
             prev_cxt->prev_param.is_support_fd, prev_cxt->prev_param.is_fd_on);

    CMR_LOGV("fd_handle 0x%p", prev_cxt->fd_handle);
    if (prev_cxt->fd_handle) {
        ret = cmr_ipm_close(prev_cxt->fd_handle);
        prev_cxt->fd_handle = 0;
    }

    CMR_LOGV("ret %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int prev_fd_send_data(struct prev_handle *handle, cmr_u32 camera_id,
                          struct img_frm *frm) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct ipm_frame_in ipm_in_param;
    struct ipm_frame_out imp_out_param;

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!prev_cxt->fd_handle) {
        CMR_LOGE("fd closed");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGD("is_support_fd %ld, is_fd_on %ld",
             prev_cxt->prev_param.is_support_fd, prev_cxt->prev_param.is_fd_on);

    if (!prev_cxt->prev_param.is_support_fd || !prev_cxt->prev_param.is_fd_on) {
        CMR_LOGE("fd unsupport or closed");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ipm_in_param.src_frame = *frm;
    ipm_in_param.dst_frame = *frm;
    ipm_in_param.caller_handle = (void *)handle;
    ipm_in_param.private_data = (void *)((unsigned long)camera_id);

    ret = ipm_transfer_frame(prev_cxt->fd_handle, &ipm_in_param, NULL);
    if (ret) {
        CMR_LOGE("failed to transfer frame to ipm %ld", ret);
        goto exit;
    }

exit:
    CMR_LOGD("out, ret %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int prev_fd_cb(cmr_u32 class_type, struct ipm_frame_out *cb_param) {
    UNUSED(class_type);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = NULL;
    struct prev_context *prev_cxt = NULL;
    struct camera_frame_type frame_type;
    struct prev_cb_info cb_data_info;
    cmr_u32 camera_id = CAMERA_ID_MAX;
    cmr_u32 i = 0;

    if (!cb_param || !cb_param->caller_handle) {
        CMR_LOGE("error param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    handle = (struct prev_handle *)cb_param->caller_handle;
    camera_id = (cmr_u32)((unsigned long)cb_param->private_data);
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!prev_cxt->prev_param.is_support_fd || !prev_cxt->prev_param.is_fd_on) {
        CMR_LOGW("fd closed");
        return CMR_CAMERA_INVALID_PARAM;
    }

    /*copy face-detect info*/
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
    frame_type.width = cb_param->dst_frame.size.width;
    frame_type.height = cb_param->dst_frame.size.height;
    frame_type.face_num = (cmr_u32)cb_param->face_area.face_count;
    CMR_LOGD("face_num %d", frame_type.face_num);
    for (i = 0; i < frame_type.face_num; i++) {
        frame_type.face_info[i].face_id = cb_param->face_area.range[i].face_id;
        frame_type.face_info[i].sx = cb_param->face_area.range[i].sx;
        frame_type.face_info[i].sy = cb_param->face_area.range[i].sy;
        frame_type.face_info[i].srx = cb_param->face_area.range[i].srx;
        frame_type.face_info[i].sry = cb_param->face_area.range[i].sry;
        frame_type.face_info[i].ex = cb_param->face_area.range[i].ex;
        frame_type.face_info[i].ey = cb_param->face_area.range[i].ey;
        frame_type.face_info[i].elx = cb_param->face_area.range[i].elx;
        frame_type.face_info[i].ely = cb_param->face_area.range[i].ely;
        frame_type.face_info[i].brightness =
            cb_param->face_area.range[i].brightness;
        frame_type.face_info[i].angle = cb_param->face_area.range[i].angle;
        frame_type.face_info[i].pose       = cb_param->face_area.range[i].pose;
        frame_type.face_info[i].smile_level =
            cb_param->face_area.range[i].smile_level;
        frame_type.face_info[i].blink_level =
            cb_param->face_area.range[i].blink_level;
    }

    /*notify fd info directly*/
    cb_data_info.cb_type = PREVIEW_EVT_CB_FD;
    cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
    cb_data_info.frame_data = &frame_type;
    prev_cb_start(handle, &cb_data_info);

    return ret;
}

cmr_int prev_fd_ctrl(struct prev_handle *handle, cmr_u32 camera_id,
                     cmr_u32 on_off) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    CMR_LOGD(" %d", on_off);

    prev_cxt->prev_param.is_fd_on = on_off;

    if (0 == on_off) {
        prev_fd_close(handle, camera_id);
    } else {
        prev_fd_open(handle, camera_id);
    }

    return ret;
}

cmr_int prev_sensor_datatype_open(struct prev_handle *handle, cmr_u32 camera_id,
                                  struct sensor_data_info otp_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct ipm_open_in in_param;
    struct ipm_open_out out_param;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    CMR_LOGI("sensor_datatype %u", prev_cxt->prev_param.sensor_datatype);

    if (prev_cxt->prev_param.sensor_datatype < SENSOR_DATATYPE_DISABLED ||
        prev_cxt->prev_param.sensor_datatype > SENSOR_DATATYPE_MAX) {
        CMR_LOGD("not support sensor datatype");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    switch (prev_cxt->prev_param.sensor_datatype) {
    case SENSOR_REAL_DEPTH_ENABLE:
        if (prev_cxt->refocus_handle) {
            CMR_LOGI("refocus inited already");
            ret = CMR_CAMERA_FAIL;
            break;
        }

        in_param.frame_cnt = prev_cxt->prev_frm_cnt;
        if ((IMG_ANGLE_90 == prev_cxt->prev_param.prev_rot) ||
            (IMG_ANGLE_270 == prev_cxt->prev_param.prev_rot)) {
            in_param.frame_size.width = prev_cxt->actual_prev_size.height;
            in_param.frame_size.height = prev_cxt->actual_prev_size.width;
            in_param.frame_rect.start_x = 0;
            in_param.frame_rect.start_y = 0;
            in_param.frame_rect.width = in_param.frame_size.height;
            in_param.frame_rect.height = in_param.frame_size.width;
            in_param.otp_data.otp_size = 8192; // otp_data->size;//TBD
            in_param.otp_data.otp_ptr = otp_data.data_ptr; // TBD
        } else {
            in_param.frame_size.width = prev_cxt->actual_prev_size.width;
            in_param.frame_size.height = prev_cxt->actual_prev_size.height;
            in_param.frame_rect.start_x = 0;
            in_param.frame_rect.start_y = 0;
            in_param.frame_rect.width = in_param.frame_size.width;
            in_param.frame_rect.height = in_param.frame_size.height;
            in_param.otp_data.otp_size = 8192; // otp_data->size;//TBD
            in_param.otp_data.otp_ptr = otp_data.data_ptr; // TBD
        }
        in_param.reg_cb = prev_sensor_datatype_cb;
        ret = cmr_ipm_open(handle->ipm_handle, IPM_TYPE_REFOCUS, &in_param,
                           &out_param, &prev_cxt->refocus_handle);
        if (ret) {
            CMR_LOGE("cmr_ipm_open failed");
            ret = CMR_CAMERA_FAIL;
        }
        CMR_LOGD("refocus_handle 0x%p", prev_cxt->refocus_handle);
        break;
    /*
case SENSOR_EMBEDDED_INFO_ENABLE:
    break;
case SENSOR_DATATYPE_PDAF_ENABLE:
    break;
    */
    default:
        return CMR_CAMERA_INVALID_PARAM;
    }

exit:
    CMR_LOGI("out, ret %ld", ret);
    return ret;
}

cmr_int prev_sensor_datatype_close(struct prev_handle *handle,
                                   cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;

    prev_cxt = &handle->prev_cxt[camera_id];

    CMR_LOGI(" sensor_datatype %u", prev_cxt->prev_param.sensor_datatype);

    CMR_LOGV("sensor_datatype_handle 0x%p", prev_cxt->refocus_handle);
    switch (prev_cxt->prev_param.sensor_datatype) {
    case SENSOR_REAL_DEPTH_ENABLE:
        if (prev_cxt->refocus_handle) {
            ret = cmr_ipm_close(prev_cxt->refocus_handle);
            prev_cxt->refocus_handle = 0;
        }
        CMR_LOGV("refocus_handle 0x%p", prev_cxt->refocus_handle);
        break;

    /*
    case SENSOR_EMBEDDED_INFO_ENABLE:
            break;
    case SENSOR_DATATYPE_PDAF_ENABLE:
            break;
    */
    default:
        break;
    }

    CMR_LOGV("ret %ld", ret);
    return ret;
}

cmr_int prev_sensor_datatype_send_data(struct prev_handle *handle,
                                       cmr_u32 camera_id, struct img_frm *frm,
                                       struct frm_info *sensor_datatype_frm) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct ipm_frame_in ipm_in_param;
    struct ipm_frame_out imp_out_param;

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!prev_cxt->refocus_handle) {
        CMR_LOGE("refoucs closed");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGD("sensor_datatype %u frm %p addr_vir.addr_y 0x%lx, touchX "
             "%d,touchY %d, sensor_datatype_frm->yaddr_vir 0x%x",
             prev_cxt->prev_param.sensor_datatype, frm, frm->addr_vir.addr_y,
             prev_cxt->touch_info.touchX, prev_cxt->touch_info.touchY,
             sensor_datatype_frm->yaddr_vir);

    if (prev_cxt->prev_param.sensor_datatype < SENSOR_DATATYPE_DISABLED ||
        prev_cxt->prev_param.sensor_datatype > SENSOR_DATATYPE_MAX) {
        CMR_LOGE("sensor datatype unsupport or closed");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    switch (prev_cxt->prev_param.sensor_datatype) {
    case SENSOR_REAL_DEPTH_ENABLE:
        ipm_in_param.frame_cnt = prev_cxt->prev_frm_cnt;
        ipm_in_param.src_frame = *frm;
        ipm_in_param.dst_frame = *frm;
        ipm_in_param.touch_x = prev_cxt->touch_info.touchX;
        ipm_in_param.touch_y = prev_cxt->touch_info.touchY;
        ipm_in_param.depth_map.width = 480;  // TBD
        ipm_in_param.depth_map.height = 360; // TBD
        ipm_in_param.depth_map.depth_map_ptr =
            (void *)((unsigned long)sensor_datatype_frm->yaddr_vir); // TBD
        ipm_in_param.caller_handle = (void *)handle;
        ipm_in_param.private_data = (void *)((unsigned long)camera_id);

        ret = ipm_transfer_frame(prev_cxt->refocus_handle, &ipm_in_param, NULL);
        if (ret) {
            CMR_LOGE("failed to transfer frame to ipm %ld", ret);
        }
        break;
    case SENSOR_EMBEDDED_INFO_ENABLE:
        break;
    case SENSOR_DATATYPE_PDAF_ENABLE:
        break;
    default:
        return CMR_CAMERA_INVALID_PARAM;
    }

exit:
    CMR_LOGD("out, ret %ld", ret);
    return ret;
}

cmr_int prev_sensor_datatype_cb(cmr_u32 class_type,
                                struct ipm_frame_out *cb_param) {
    UNUSED(class_type);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = NULL;
    struct prev_context *prev_cxt = NULL;
    struct camera_frame_type frame_type;
    struct prev_cb_info cb_data_info;
    cmr_u32 camera_id = CAMERA_ID_MAX;
    cmr_u32 i = 0;

    if (!cb_param || !cb_param->caller_handle) {
        CMR_LOGE("error param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    handle = (struct prev_handle *)cb_param->caller_handle;
    camera_id = (cmr_u32)((unsigned long)cb_param->private_data);
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    if (prev_cxt->prev_param.sensor_datatype < SENSOR_DATATYPE_DISABLED ||
        prev_cxt->prev_param.sensor_datatype > SENSOR_DATATYPE_MAX) {
        CMR_LOGW("sensor datatype closed");
        return CMR_CAMERA_INVALID_PARAM;
    }

    /*notify frame*/
    ret = prev_pop_preview_buffer(handle, camera_id,
                                  prev_cxt->preview_bakcup_data, 0);
    if (ret) {
        CMR_LOGE("pop frm 0x%x err", prev_cxt->preview_bakcup_data->channel_id);
        return CMR_CAMERA_FAIL;
    }

    /*copy refoucs-detect info*/
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
    frame_type.width = cb_param->dst_frame.size.width;
    frame_type.height = cb_param->dst_frame.size.height;

    frame_type.y_phy_addr = prev_cxt->preview_bakcup_data->yaddr;
    frame_type.y_vir_addr = prev_cxt->preview_bakcup_data->yaddr_vir;
    frame_type.fd = prev_cxt->preview_bakcup_data->fd;
    frame_type.type = PREVIEW_FRAME;
    frame_type.timestamp = prev_cxt->preview_bakcup_data->sec * 1000000000LL +
                           prev_cxt->preview_bakcup_data->usec * 1000;

    /*notify refoucs info directly*/
    cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
    cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
    cb_data_info.frame_data = &frame_type;
    prev_cb_start(handle, &cb_data_info);
    // prev_cxt->preview_bakcup_frm  = NULL;
    ret = prev_pop_sensor_datatype_buffer(handle, camera_id,
                                          prev_cxt->sensor_datatype_data, 0);
    if (prev_cxt->sensor_datatype_data->yaddr_vir != 0)
        ret = prev_set_sensor_datatype_buffer(
            handle, camera_id, prev_cxt->sensor_datatype_data->yaddr,
            prev_cxt->sensor_datatype_data->yaddr_vir,
            prev_cxt->sensor_datatype_data->fd);

    return ret;
}

#if 0
cmr_int prev_sensor_datatype_ctrl(struct prev_handle *handle,
				cmr_u32 camera_id,
				cmr_u32 on_off)
{
	cmr_int                ret = CMR_CAMERA_SUCCESS;
	struct prev_context    *prev_cxt = NULL;

	CHECK_HANDLE_VALID(handle);
	CHECK_CAMERA_ID(camera_id);

	prev_cxt = &handle->prev_cxt[camera_id];

	CMR_LOGD(" %d", on_off);

	prev_cxt->prev_param.sensor_datatype = on_off;

	if (0 == on_off) {
		prev_sensor_datatype_close(handle, camera_id);
	} else {
		prev_sensor_datatype_open(handle, camera_id);
	}

	return ret;
}
#endif

cmr_int prev_capture_zoom_post_cap(struct prev_handle *handle, cmr_int *flag,
                                   cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_path_capability capability;
    struct prev_context *prev_cxt = NULL;

    if (!handle || !flag) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGV("in");

    prev_cxt = &handle->prev_cxt[camera_id];

    if (prev_cxt != NULL && (prev_cxt->prev_param.sprd_highiso_enabled ||
                             prev_cxt->prev_param.isp_to_dram ||
                             prev_cxt->prev_param.sprd_burstmode_enabled))
        *flag = ZOOM_POST_PROCESS;
    else {
        if (!handle->ops.channel_path_capability) {
            CMR_LOGE("ops channel_path_capability is null");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        ret = handle->ops.channel_path_capability(handle->oem_handle,
                                                  &capability);
        if (ret) {
            CMR_LOGE("channel_path_capability failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        *flag = capability.zoom_post_proc;
    }

    CMR_LOGV("out flag %ld", *flag);
exit:
    return ret;
}

cmr_int prev_set_preview_skip_frame_num(cmr_handle preview_handle,
                                        cmr_u32 camera_id, cmr_uint skip_num,
                                        cmr_uint has_preflashed) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    prev_cxt->prev_skip_num = prev_cxt->prev_frm_cnt + skip_num;
    prev_cxt->prev_preflash_skip_en = has_preflashed;

    return ret;
}

cmr_int prev_set_ae_time(cmr_handle preview_handle, cmr_u32 camera_id,
                         void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    cmr_uint ae_time = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    ae_time = *(cmr_uint *)data * 1000; // ns

    prev_cxt = &handle->prev_cxt[camera_id];
    prev_cxt->ae_time = ae_time;

    return ret;
}

cmr_int prev_set_preview_touch_info(cmr_handle preview_handle,
                                    cmr_u32 camera_id,
                                    struct touch_coordinate *touch_xy) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    prev_cxt->touch_info.touchX = (*touch_xy).touchX;
    prev_cxt->touch_info.touchY = (*touch_xy).touchY;
    CMR_LOGI("touchX %d touchY %d", prev_cxt->touch_info.touchX,
             prev_cxt->touch_info.touchY);

    return ret;
}

cmr_int cmr_preview_get_zoom_factor(cmr_handle preview_handle,
                                    cmr_u32 camera_id, float *zoom_factor) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_HANDLE_VALID(zoom_factor);
    CHECK_CAMERA_ID(camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];
    *zoom_factor = prev_cxt->prev_param.zoom_setting.zoom_info.zoom_ratio;
    CMR_LOGI("zoom_factor is %f ", *zoom_factor);
    return ret;
}
cmr_int prev_set_vcm_step(cmr_handle preview_handle, cmr_u32 camera_id,
                          void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    cmr_uint vcm_step = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    vcm_step = *(cmr_uint *)data;

    prev_cxt = &handle->prev_cxt[camera_id];
    prev_cxt->vcm_step = vcm_step;

    return ret;
}
cmr_int prev_is_need_scaling(cmr_handle preview_handle, cmr_u32 camera_id) {
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    cmr_int is_need_scaling = 1;
    cmr_u32 is_raw_capture = 0;
    char value[PROPERTY_VALUE_MAX];

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];

    property_get("persist.sys.camera.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }

    // yuv no scale condition
    if ((ZOOM_BY_CAP == prev_cxt->cap_zoom_mode) &&
        (prev_cxt->cap_org_size.width == prev_cxt->actual_pic_size.width) &&
        (prev_cxt->cap_org_size.height == prev_cxt->actual_pic_size.height)) {
        is_need_scaling = 0;
        return is_need_scaling;
    }

    // raw data no scale condition
    if ((is_raw_capture) && (ZOOM_POST_PROCESS == prev_cxt->cap_zoom_mode) &&
        (prev_cxt->cap_org_size.width == prev_cxt->actual_pic_size.width) &&
        (prev_cxt->cap_org_size.height == prev_cxt->actual_pic_size.height)) {
        is_need_scaling = 0;
        return is_need_scaling;
    }

    // no raw data no scale condition
    if ((!is_raw_capture) && (ZOOM_POST_PROCESS == prev_cxt->cap_zoom_mode) &&
        (prev_cxt->cap_org_size.width == prev_cxt->actual_pic_size.width) &&
        (prev_cxt->cap_org_size.height == prev_cxt->actual_pic_size.height) &&
        (prev_cxt->cap_org_size.width == prev_cxt->cap_sn_trim_rect.width) &&
        (prev_cxt->cap_org_size.height == prev_cxt->cap_sn_trim_rect.height)) {
        is_need_scaling = 0;
        return is_need_scaling;
    }

    CMR_LOGI("cap_zoom_mode %ld, cap_org_size %d %d, cap_sn_trim_rect %d %d, "
             "actual_pic_size %d %d, is_raw_capture %d, is_need_scaling %ld",
             prev_cxt->cap_zoom_mode, prev_cxt->cap_org_size.width,
             prev_cxt->cap_org_size.height, prev_cxt->cap_sn_trim_rect.width,
             prev_cxt->cap_sn_trim_rect.height, prev_cxt->actual_pic_size.width,
             prev_cxt->actual_pic_size.height, is_raw_capture, is_need_scaling);

    return is_need_scaling;
}

cmr_u32 prev_get_aligned_type(cmr_handle preview_handle, cmr_u32 camera_id) {
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 aligned_type = CAMERA_MEM_NO_ALIGNED;
    char value[PROPERTY_VALUE_MAX];
    cmr_u32 is_raw_capture = 0;
    cmr_u32 preview_enable = 0;
    cmr_u32 snapshot_enable = 0;
    cmr_u32 video_enable = 0;
    cmr_u32 sprd_zsl_enabled;
    struct img_size org_pic_size;
    cmr_u32 width, height = 0;
    cmr_u32 video_snapshot_type = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    org_pic_size = prev_cxt->prev_param.picture_size;
    width = org_pic_size.width;
    height = org_pic_size.height;
    preview_enable = prev_cxt->prev_param.preview_eb;
    snapshot_enable = prev_cxt->prev_param.snapshot_eb;
    video_enable = prev_cxt->prev_param.video_eb;
    sprd_zsl_enabled = prev_cxt->prev_param.sprd_zsl_enabled;
    video_snapshot_type = prev_cxt->prev_param.video_snapshot_type;

    property_get("persist.sys.camera.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }

    // condition scene and size for w/h aligned by 16
    if (snapshot_enable && !is_raw_capture) {
        if ((!preview_enable ||                      // non zsl takepicture
             (preview_enable && sprd_zsl_enabled) || // zsl takepicture
             (preview_enable && video_enable &&
              (video_snapshot_type ==
               VIDEO_SNAPSHOT_VIDEO)) || //  video snapshot takepicture use
                                         //  video buffer
             (preview_enable && video_enable &&
              sprd_zsl_enabled)) && // video snapshot takepicture
            ((height == 1944 && width == 2592) ||
             (height == 1836 && width == 3264) ||
             (height == 1080 && width == 1920) ||
             (height == 360 && width == 640))) {
            aligned_type = CAMERA_MEM_ALIGNED;
        }
    }

    if (prev_cxt->prev_param.refocus_mode != 0) {
        aligned_type = CAMERA_MEM_NO_ALIGNED;
    }

    CMR_LOGI("aligned_type %d", aligned_type);
    return aligned_type;
}
