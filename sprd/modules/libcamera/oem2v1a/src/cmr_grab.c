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
#define LOG_TAG "cmr_grab"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include <cutils/trace.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "cmr_grab.h"
#include "sprd_img.h"
#include "cmr_oem.h"
//#include "ylog.h"

#define CMR_CHECK_FD                                                           \
    do {                                                                       \
        if (-1 == p_grab->fd) {                                                \
            CMR_LOGE("GRAB device not opened");                                \
            return -1;                                                         \
        }                                                                      \
    } while (0)

#define CMR_GRAB_DEV_NAME "/dev/sprd_image"

#define CMR_CPP_DEV_NAME "/dev/sprd_cpp"

#define CMR_CHECK_HANDLE                                                       \
    do {                                                                       \
        if (!p_grab) {                                                         \
            CMR_LOGE("Invalid handle");                                        \
            return -1;                                                         \
        }                                                                      \
    } while (0)

#if defined(CONFIG_CAMERA_POWER_OPTIMIZATION)
#define CAMERA_POWER_OPT_FLAG 1
#else
#define CAMERA_POWER_OPT_FLAG 0
#endif

static cmr_int cmr_grab_create_thread(cmr_handle grab_handle);
static cmr_int cmr_grab_kill_thread(cmr_handle grab_handle);
static void *cmr_grab_thread_proc(void *data);
static cmr_u32 cmr_grab_get_4cc(cmr_u32 img_type);
static cmr_u32 cmr_grab_get_img_type(cmr_u32 fourcc);
static cmr_u32 cmr_grab_get_data_endian(struct img_data_end *in_endian,
                                        struct img_data_end *out_endian);

cmr_int cmr_grap_free_grab(struct cmr_grab *p_grab) {
    cmr_u32 channel_id;

    if (p_grab == NULL) {
        return 0;
    }
    if (0 <= p_grab->fd) {
        close(p_grab->fd);
    }
    free((void *)p_grab);

    return 0;
}

cmr_int cmr_grab_init(struct grab_init_param *init_param_ptr,
                      cmr_handle *grab_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    cmr_u32 i = 0;
    cmr_u32 channel_id;
    struct cmr_grab *p_grab = NULL;
    struct sprd_img_res res;

    CMR_LOGI("E");

    cmr_bzero(&res, sizeof(res));
    p_grab = (struct cmr_grab *)malloc(sizeof(struct cmr_grab));
    if (!p_grab) {
        CMR_LOGE("No mem");
        return -1;
    }
    memset(p_grab, 0, sizeof(struct cmr_grab));
    p_grab->init_param = *init_param_ptr;
    p_grab->fd = open(CMR_GRAB_DEV_NAME, O_RDWR, 0);
    if (-1 == p_grab->fd) {
        CMR_LOGE("Failed to open dcam device.errno : %d", errno);
        fprintf(stderr, "Cannot open '%s': %d, %s\n", CMR_GRAB_DEV_NAME, errno,
                strerror(errno));
        cmr_grap_free_grab(p_grab);
        exit(EXIT_FAILURE);
    }
    CMR_LOGI("dcam_fd=0x%x", p_grab->fd);

    sem_init(&p_grab->close_sem, 0, 0);
    ret = pthread_mutex_init(&p_grab->cb_mutex, NULL);
    if (ret) {
        CMR_LOGE("Failed to init mutex : %d", errno);
        cmr_grap_free_grab(p_grab);
        exit(EXIT_FAILURE);
    }

    ret = pthread_mutex_init(&p_grab->dcam_mutex, NULL);
    if (ret) {
        CMR_LOGE("Failed to init dcam mutex : %d", errno);
        pthread_mutex_destroy(&p_grab->cb_mutex);
        cmr_grap_free_grab(p_grab);
        exit(EXIT_FAILURE);
    }

    ret = pthread_mutex_init(&p_grab->status_mutex, NULL);
    if (ret) {
        CMR_LOGE("Failed to init status mutex : %d", errno);
        pthread_mutex_destroy(&p_grab->cb_mutex);
        pthread_mutex_destroy(&p_grab->dcam_mutex);
        cmr_grap_free_grab(p_grab);
        exit(EXIT_FAILURE);
    }

    for (channel_id = 0; channel_id < CHN_MAX; channel_id++) {
        ret = pthread_mutex_init(&p_grab->path_mutex[channel_id], NULL);
        if (ret) {
            CMR_LOGE("Failed to init path_mutex %d : %d", channel_id, errno);
            if (channel_id > 0) {
                for (i = 0; i < channel_id; i++) {
                    pthread_mutex_destroy(&p_grab->path_mutex[i]);
                }
            }
            pthread_mutex_destroy(&p_grab->cb_mutex);
            pthread_mutex_destroy(&p_grab->dcam_mutex);
            pthread_mutex_destroy(&p_grab->status_mutex);
            cmr_grap_free_grab(p_grab);
            exit(EXIT_FAILURE);
        }
    }

    pthread_mutex_lock(&p_grab->dcam_mutex);
    if (0 == p_grab->mode_enable) {
        res.sensor_id = p_grab->init_param.sensor_id;
        CMR_LOGI("get dcam res w %d h %d sn id %d", res.width, res.height,
                 res.sensor_id);
        ret = ioctl(p_grab->fd, SPRD_IMG_IO_GET_DCAM_RES, &res);
        CMR_RTN_IF_ERR(ret);
        if (0 == res.flag) {
            CMR_LOGE("get dcam res failed!");
            pthread_mutex_unlock(&p_grab->dcam_mutex);
            cmr_grap_free_grab(p_grab);
            return -1;
        }
        p_grab->res = res.flag;
        p_grab->mode_enable = 1;
    }
    pthread_mutex_unlock(&p_grab->dcam_mutex);

    ret = cmr_grab_create_thread((cmr_handle)p_grab);
    if (0 != ret) {
        for (channel_id = 0; channel_id < CHN_MAX; channel_id++) {
            pthread_mutex_destroy(&p_grab->path_mutex[channel_id]);
        }
        pthread_mutex_destroy(&p_grab->cb_mutex);
        pthread_mutex_destroy(&p_grab->dcam_mutex);
        pthread_mutex_destroy(&p_grab->status_mutex);
        cmr_grap_free_grab(p_grab);
        exit(EXIT_FAILURE);
    }
    // pthread_debug_setname(p_grab->thread_handle, "grab%d",
    // p_grab->init_param.sensor_id);
    p_grab->grab_evt_cb = NULL;
    p_grab->stream_on_cb = NULL;
    p_grab->is_prev_trace = 0;
    p_grab->is_cap_trace = 0;
    p_grab->is_rt_refocus = 0;
    p_grab->isp_cb_enable = 1;
    memset(p_grab->chn_status, 0, sizeof(p_grab->chn_status));
    *grab_handle = (cmr_handle)p_grab;

exit:
    CMR_LOGI("X");
    ATRACE_END();
    return ret;
}

cmr_int cmr_grab_deinit(cmr_handle grab_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    cmr_u32 channel_id = 0;
    struct cmr_grab *p_grab;
    struct sprd_img_res res;
    cmr_bzero(&res, sizeof(res));

    p_grab = (struct cmr_grab *)grab_handle;

    CMR_CHECK_HANDLE;
    CMR_LOGI("E");

    /* thread should be killed before fd deinited */
    ret = cmr_grab_kill_thread(grab_handle);
    if (ret) {
        CMR_LOGE("Failed to kill the thread. errno : %d", errno);
        exit(EXIT_FAILURE);
    }

    /* then close fd */
    if (-1 != p_grab->fd) {
        CMR_LOGI("close fd");
        pthread_mutex_lock(&p_grab->dcam_mutex);
        if (0 != p_grab->mode_enable) {
            res.sensor_id = p_grab->init_param.sensor_id;
            res.flag = p_grab->res;
            ret = ioctl(p_grab->fd, SPRD_IMG_IO_PUT_DCAM_RES, &res);
            if (ret) {
                pthread_mutex_unlock(&p_grab->dcam_mutex);
                goto exit;
            }
            if (0 == res.flag) {
                CMR_LOGE("get dcam res failed!");
                pthread_mutex_unlock(&p_grab->dcam_mutex);
                return -1;
            }
            p_grab->mode_enable = 0;
        }
        pthread_mutex_unlock(&p_grab->dcam_mutex);
        if (-1 == close(p_grab->fd)) {
            fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
        p_grab->fd = -1;
        CMR_LOGI("p_grab->fd closed");
    }
    CMR_LOGI("mode_enable = %d", p_grab->mode_enable);
    pthread_mutex_lock(&p_grab->cb_mutex);
    p_grab->grab_evt_cb = NULL;
    p_grab->is_prev_trace = 0;
    p_grab->is_cap_trace = 0;
    p_grab->is_rt_refocus = 0;
    pthread_mutex_unlock(&p_grab->cb_mutex);
    pthread_mutex_destroy(&p_grab->cb_mutex);
    pthread_mutex_destroy(&p_grab->dcam_mutex);
    pthread_mutex_destroy(&p_grab->status_mutex);
    sem_destroy(&p_grab->close_sem);
    for (channel_id = 0; channel_id < CHN_MAX; channel_id++) {
        pthread_mutex_destroy(&p_grab->path_mutex[channel_id]);
    }
    free((void *)grab_handle);

exit:
    CMR_LOGI("X");
    ATRACE_END();
    return 0;
}

/*
 * get iommu status
 * return val:
 *    0:    has iommu;
 *    else: no iommu
 */
cmr_s32 cmr_grab_get_iommu_status(cmr_handle grab_handle) {
    cmr_s32 ret = 0;
    struct cmr_grab *p_grab;
    unsigned char has_iommu = 0;

    p_grab = (struct cmr_grab *)grab_handle;

    ret = ioctl(p_grab->fd, SPRD_IMG_IO_GET_IOMMU_STATUS, &has_iommu);
    if (ret) {
        CMR_LOGE("SPRD_IMG_IO_GET_IOMMU_STATUS failed");
        return ret;
    }

    if (has_iommu)
        ret = 0;
    else
        ret = -1;

    return ret;
}

void cmr_grab_evt_reg(cmr_handle grab_handle, cmr_evt_cb grab_event_cb) {
    struct cmr_grab *p_grab;

    p_grab = (struct cmr_grab *)grab_handle;
    if (!p_grab)
        return;

    pthread_mutex_lock(&p_grab->cb_mutex);
    p_grab->grab_evt_cb = grab_event_cb;
    pthread_mutex_unlock(&p_grab->cb_mutex);
}

void cmr_grab_isp_irq_proc_evt_reg(cmr_handle grab_handle,
                                   cmr_evt_cb isp_irq_proc_event_cb) {
    struct cmr_grab *p_grab;

    p_grab = (struct cmr_grab *)grab_handle;
    if (!p_grab)
        return;

    pthread_mutex_lock(&p_grab->cb_mutex);
    p_grab->isp_irq_proc_evt_cb = isp_irq_proc_event_cb;
    pthread_mutex_unlock(&p_grab->cb_mutex);
    return;
}

void cmr_grab_isp_statis_evt_reg(cmr_handle grab_handle,
                                 cmr_evt_cb isp_statis_event_cb) {
    struct cmr_grab *p_grab;

    p_grab = (struct cmr_grab *)grab_handle;
    if (!p_grab)
        return;

    pthread_mutex_lock(&p_grab->cb_mutex);
    p_grab->isp_statis_evt_cb = isp_statis_event_cb;
    pthread_mutex_unlock(&p_grab->cb_mutex);
    return;
}

void cmr_grab_post_ynr_evt_reg(cmr_handle grab_handle,
                               cmr_evt_cb grab_post_ynr_evt_cb) {
    struct cmr_grab *p_grab;

    p_grab = (struct cmr_grab *)grab_handle;
    if (!p_grab)
        return;
    pthread_mutex_lock(&p_grab->cb_mutex);
    p_grab->grab_post_ynr_evt_cb = grab_post_ynr_evt_cb;
    pthread_mutex_unlock(&p_grab->cb_mutex);
}

void cmr_grab_3dnr_evt_reg(cmr_handle grab_handle,
                           cmr_evt_cb grab_3dnr_event_cb) {
    struct cmr_grab *p_grab;

    p_grab = (struct cmr_grab *)grab_handle;
    if (!p_grab)
        return;

    pthread_mutex_lock(&p_grab->cb_mutex);
    p_grab->grab_3dnr_evt_cb = grab_3dnr_event_cb;
    pthread_mutex_unlock(&p_grab->cb_mutex);
}

cmr_int cmr_grab_if_cfg(cmr_handle grab_handle, struct sensor_if *sn_if) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    struct cmr_grab *p_grab;
    struct sprd_img_sensor_if sensor_if;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;

    sensor_if.if_type = sn_if->if_type;
    sensor_if.res[0] = IF_OPEN;
    sensor_if.img_fmt = sn_if->img_fmt;
    sensor_if.img_ptn = sn_if->img_ptn;
    sensor_if.frm_deci = sn_if->frm_deci;
    if (0 == sn_if->if_type) {
        /* CCIR interface */
        sensor_if.if_spec.ccir.v_sync_pol = sn_if->if_spec.ccir.v_sync_pol;
        sensor_if.if_spec.ccir.h_sync_pol = sn_if->if_spec.ccir.h_sync_pol;
        sensor_if.if_spec.ccir.pclk_pol = sn_if->if_spec.ccir.pclk_pol;
    } else {
        /* MIPI interface */
        sensor_if.if_spec.mipi.use_href = sn_if->if_spec.mipi.use_href;
        sensor_if.if_spec.mipi.bits_per_pxl = sn_if->if_spec.mipi.bits_per_pxl;
        sensor_if.if_spec.mipi.is_loose = sn_if->if_spec.mipi.is_loose;
        sensor_if.if_spec.mipi.lane_num = sn_if->if_spec.mipi.lane_num;

        if (CAMERA_POWER_OPT_FLAG) {
            CMR_LOGI("support power opt\n");
            sensor_if.if_spec.mipi.pclk = sn_if->if_spec.mipi.pclk;
        } else {
            CMR_LOGI("not support power opt\n");
            sensor_if.if_spec.mipi.pclk = 0xffff;
        }
    }

    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_SENSOR_IF, &sensor_if);

    CMR_LOGI("ret %ld, if type %d, mode %d, deci %d, status %d", ret,
             sensor_if.if_type, sensor_if.img_fmt, sensor_if.frm_deci,
             sensor_if.res[0]);

    ATRACE_END();
    return ret;
}

cmr_int cmr_grab_if_decfg(cmr_handle grab_handle, struct sensor_if *sn_if) {
    cmr_int ret = 0;
    struct cmr_grab *p_grab;
    struct sprd_img_sensor_if sensor_if;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;

    sensor_if.if_type = sn_if->if_type;
    sensor_if.res[0] = IF_CLOSE;

    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_SENSOR_IF, &sensor_if);

    CMR_LOGI("ret %ld, if type %d, status %d.", ret, sensor_if.if_type,
             sensor_if.res[0]);

    return ret;
}

cmr_int cmr_grab_sn_cfg(cmr_handle grab_handle, struct sn_cfg *config) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    // struct grab_streamparm   stream_parm;
    struct cmr_grab *p_grab;
    cmr_u32 mode;
    struct sprd_img_size size, sn_max_size;
    struct sprd_img_rect rect;
    struct sprd_img_sbs_info sbs_info;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;

    mode = 1; /* means multi-frame sample mode */

    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_MODE, &mode);
    CMR_RTN_IF_ERR(ret);

    size.w = config->sn_size.width;
    size.h = config->sn_size.height;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_SENSOR_SIZE, &size);
    if (ret) {
        CMR_LOGE("SPRD_IMG_IO_SET_SENSOR_SIZE failed");
    }

    CMR_LOGI("sn_trim x y w h %d, %d, %d, %d", config->sn_trim.start_x,
             config->sn_trim.start_y, config->sn_trim.width,
             config->sn_trim.height);

    rect.x = config->sn_trim.start_x;
    rect.y = config->sn_trim.start_y;
    rect.w = config->sn_trim.width;
    rect.h = config->sn_trim.height;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_SENSOR_TRIM, &rect);
    if (ret) {
        CMR_LOGE("SPRD_IMG_IO_SET_SENSOR_TRIM failed");
    }

    // for isp alloc memory use
    CMR_LOGI("sensor_max_size: w=%d, h=%d", config->sensor_max_size.width,
             config->sensor_max_size.height);
    sn_max_size.w = config->sensor_max_size.width;
    sn_max_size.h = config->sensor_max_size.height;

    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_SENSOR_MAX_SIZE, &sn_max_size);
    if (ret) {
        CMR_LOGE("SPRD_IMG_IO_SET_SENSOR_MAX_SIZE failed");
    }

    sbs_info.sbs_mode = config->sbs_mode;
    CMR_LOGI("sbs_mode %d", sbs_info.sbs_mode);
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SBS_MODE, &sbs_info);
    CMR_RTN_IF_ERR(ret);

exit:
    ATRACE_END();
    return ret;
}

static cmr_int cmr_grab_cap_cfg_common(cmr_handle grab_handle,
                                       struct cap_cfg *config,
                                       cmr_u32 channel_id,
                                       struct img_data_end *endian) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    cmr_u32 found = 0;
    cmr_u32 pxl_fmt;
    struct cmr_grab *p_grab;
    struct sprd_img_parm parm;
    struct sprd_img_get_fmt fmt_parm;
    struct sprd_img_format img_fmt;
    struct img_data_end data_endian;

    if (NULL == config)
        return -1;
    p_grab = (struct cmr_grab *)grab_handle;

    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;

    CMR_LOGI("chn_id=%d, regu_mode=%d, pdaf_ctrl=%d %d, deci_factor=%d, "
             "cfg_isp=%d, chn_skip_num=%d",
             channel_id, config->cfg.regular_desc.regular_mode,
             config->cfg.pdaf_ctrl.mode, config->cfg.pdaf_ctrl.phase_data_type,
             config->chn_deci_factor, config->buffer_cfg_isp,
             config->cfg.chn_skip_num);

    parm.channel_id = channel_id;
    parm.regular_desc = config->cfg.regular_desc;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_SHRINK, &parm);
    if (ret) {
        CMR_LOGE("SPRD_IMG_IO_SET_SHRINK failed, ret=%ld", ret);
        return ret;
    }

    parm.channel_id = channel_id;
    parm.pdaf_ctrl.mode = config->cfg.pdaf_ctrl.mode;
    parm.pdaf_ctrl.phase_data_type = config->cfg.pdaf_ctrl.phase_data_type;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_PDAF_CONTROL, &parm);
    if (ret) {
        CMR_LOGE("SPRD_IMG_IO_PDAF_CONTROL failed, ret=%ld", ret);
        return ret;
    }

    parm.channel_id = channel_id;
    parm.deci = config->chn_deci_factor;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_PATH_FRM_DECI, &parm);
    if (ret) {
        CMR_LOGE("SPRD_IMG_IO_PATH_FRM_DECI failed, ret=%ld", ret);
        return ret;
    }

    parm.channel_id = channel_id;
    parm.skip_num = config->cfg.chn_skip_num;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_PATH_SKIP_NUM, &parm);
    if (ret) {
        CMR_LOGE("SPRD_IMG_IO_SET_PATH_SKIP_NUM failed, ret=%ld", ret);
        return ret;
    }

    parm.crop_rect.x = config->cfg.src_img_rect.start_x;
    parm.crop_rect.y = config->cfg.src_img_rect.start_y;
    parm.crop_rect.w = config->cfg.src_img_rect.width;
    parm.crop_rect.h = config->cfg.src_img_rect.height;
    parm.reserved[0] = config->cfg.src_img_change;
    parm.reserved[1] = config->cfg.src_img_size.width;
    parm.reserved[2] = config->cfg.src_img_size.height;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_CROP, &parm);
    if (ret) {
        CMR_LOGE("SPRD_IMG_IO_SET_CROP failed, ret=%ld", ret);
        return ret;
    }
    CMR_LOGI("crop_rect in hex:%x,%x,%x,%x, crop_rect in dec:%d,%d,%d,%d",
             parm.crop_rect.x, parm.crop_rect.y, parm.crop_rect.w,
             parm.crop_rect.h, parm.crop_rect.x, parm.crop_rect.y,
             parm.crop_rect.w, parm.crop_rect.h);

    /* secondly,  check whether the output format described by
     * config->cfg[cfg_id] can be supported by the low layer */
    pxl_fmt = cmr_grab_get_4cc(config->cfg.dst_img_fmt);
    found = 0;
    fmt_parm.index = 0;
    while (0 == ioctl(p_grab->fd, SPRD_IMG_IO_GET_FMT, &fmt_parm)) {
        if (fmt_parm.fmt == pxl_fmt) {
            CMR_LOGV("FourCC 0x%x is supported by the low layer", pxl_fmt);
            found = 1;
            break;
        }
        fmt_parm.index++;
    }

    if (found) {
        bzero(&img_fmt, sizeof(struct sprd_img_format));
        img_fmt.channel_id = channel_id;
        img_fmt.width = config->cfg.dst_img_size.width;
        img_fmt.height = config->cfg.dst_img_size.height;
        img_fmt.fourcc = pxl_fmt; // fourecc
        img_fmt.need_isp = config->cfg.need_isp;
        img_fmt.flip_on = config->cfg.flip_on;
        img_fmt.buffer_cfg_isp = config->buffer_cfg_isp ? 0 : 1;
        if (endian == NULL) {
            img_fmt.is_lightly = 1;
        }
        pthread_mutex_lock(&p_grab->path_mutex[channel_id]);
        CMR_LOGI("SPRD_IMG_IO_CHECK_FMT fmt %d %d %d 0x%x %d flip:%d",
                 img_fmt.channel_id, img_fmt.width, img_fmt.height,
                 img_fmt.fourcc, img_fmt.need_isp, img_fmt.flip_on);
        ret = ioctl(p_grab->fd, SPRD_IMG_IO_CHECK_FMT, &img_fmt);
        CMR_LOGV("need binning, %d", img_fmt.need_binning);
        if (img_fmt.need_binning) {
            config->cfg.need_binning = 1;
        }
        if (0 == ret) {
            p_grab->chn_status[channel_id] = CHN_BUSY;
        } else if (ret > 0) {
            CMR_LOGI("need restart");
            ret = CMR_GRAB_RET_RESTART;
        }
        if (endian != NULL) {
            data_endian.y_endian = img_fmt.endian.y_endian;
            data_endian.uv_endian = img_fmt.endian.uv_endian;
            cmr_grab_get_data_endian(&data_endian, endian);
        }
        pthread_mutex_unlock(&p_grab->path_mutex[channel_id]);
    } else {
        CMR_LOGI("fourcc not founded dst_img_fmt=0x%x",
                 config->cfg.dst_img_fmt);
    }
exit:
    ATRACE_END();
    CMR_LOGV("ret %ld", ret);
    return ret;
}

cmr_int cmr_grab_cap_cfg(cmr_handle grab_handle, struct cap_cfg *config,
                         cmr_u32 *channel_id, struct img_data_end *endian) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    cmr_u32 pxl_fmt;
    struct cmr_grab *p_grab;
    struct sprd_img_parm parm;
    cmr_u32 ch_id;

    if (NULL == config || NULL == channel_id)
        return -1;
    memset(&parm, 0, sizeof(struct sprd_img_parm));
    p_grab = (struct cmr_grab *)grab_handle;

    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;

    CMR_LOGI("frm_num %d dst width %d dst height %d,slowmotion %d.",
             config->frm_num, config->cfg.dst_img_size.width,
             config->cfg.dst_img_size.height, config->cfg.slowmotion);

    CMR_LOGI("src_img_fmt %d dst_img_fmt %d.", config->cfg.src_img_fmt,
             config->cfg.dst_img_fmt);

    parm.dst_size.w = config->cfg.dst_img_size.width;
    parm.dst_size.h = config->cfg.dst_img_size.height;
    pxl_fmt = cmr_grab_get_4cc(config->cfg.dst_img_fmt);
    parm.pixel_fmt = pxl_fmt;
    parm.sensor_id = config->sensor_id;
    pxl_fmt = cmr_grab_get_4cc(config->cfg.src_img_fmt);
    parm.sn_fmt = pxl_fmt;
    parm.need_isp_tool = config->cfg.need_isp_tool;
    parm.need_isp = config->cfg.need_isp;
    parm.regular_desc = config->cfg.regular_desc;
    parm.rt_refocus = config->cfg.pdaf_ctrl.mode;
    parm.slowmotion = config->cfg.slowmotion;

    parm.crop_rect.x = config->cfg.src_img_rect.start_x;
    parm.crop_rect.y = config->cfg.src_img_rect.start_y;
    parm.crop_rect.w = config->cfg.src_img_rect.width;
    parm.crop_rect.h = config->cfg.src_img_rect.height;
    parm.reserved[0] = config->cfg.src_img_change;
    parm.reserved[1] = config->cfg.src_img_size.width;
    parm.reserved[2] = config->cfg.src_img_size.height;
    parm.scene_mode = config->cfg.sence_mode;

    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_OUTPUT_SIZE, &parm);
    CMR_RTN_IF_ERR(ret);

    ret = ioctl(p_grab->fd, SPRD_IMG_IO_GET_CH_ID, &ch_id);
    CMR_RTN_IF_ERR(ret);

    *channel_id = ch_id;
    ret = cmr_grab_cap_cfg_common(grab_handle, config, *channel_id, endian);

exit:
    CMR_LOGI("ret %ld", ret);
    ATRACE_END();
    return ret;
}

/*in cmr_grab_cap_cfg_lightly channel_id is in param*/
cmr_int cmr_grab_cap_cfg_lightly(cmr_handle grab_handle, struct cap_cfg *config,
                                 cmr_u32 channel_id) {
    struct cmr_grab *p_grab;

    if (NULL == config)
        return -1;
    p_grab = (struct cmr_grab *)grab_handle;

    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;

    CMR_LOGI("frm_num %d dst width %d dst height %d.", config->frm_num,
             config->cfg.dst_img_size.width, config->cfg.dst_img_size.height);

    return cmr_grab_cap_cfg_common(grab_handle, config, channel_id, NULL);
}

cmr_int cmr_grab_buff_cfg(cmr_handle grab_handle, struct buffer_cfg *buf_cfg) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    cmr_u32 i;
    struct cmr_grab *p_grab;
    struct sprd_img_parm parm;

    if (NULL == buf_cfg || buf_cfg->count > IMG_PATH_BUFFER_COUNT)
        return -1;
    p_grab = (struct cmr_grab *)grab_handle;

    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;

    CMR_LOGV("chn_id=%d, cnt=%d, base_id=0x%x ", buf_cfg->channel_id,
             buf_cfg->count, buf_cfg->base_id);

    /* firstly , set the base index for each channel */
    parm.frame_base_id = buf_cfg->base_id;
    parm.channel_id = buf_cfg->channel_id;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_FRM_ID_BASE, &parm);
    CMR_RTN_IF_ERR(ret);

    /* secondly , set the frame address */
    parm.channel_id = buf_cfg->channel_id;
    parm.is_reserved_buf = buf_cfg->is_reserved_buf;
    parm.buf_flag = buf_cfg->flag;
    parm.buffer_count = buf_cfg->count;
    parm.reserved[0] = buf_cfg->zsl_private;
    for (i = 0; i < buf_cfg->count; i++) {

        parm.frame_addr_array[i].y = buf_cfg->addr[i].addr_y;
        parm.frame_addr_array[i].u = buf_cfg->addr[i].addr_u;
        parm.frame_addr_array[i].v = buf_cfg->addr[i].addr_v;
        parm.frame_addr_vir_array[i].y = buf_cfg->addr_vir[i].addr_y;
        parm.frame_addr_vir_array[i].u = buf_cfg->addr_vir[i].addr_u;
        parm.frame_addr_vir_array[i].v = buf_cfg->addr_vir[i].addr_v;
        parm.fd_array[i] = buf_cfg->fd[i];
        parm.index = buf_cfg->index[i];
        CMR_LOGD("chn_id=%d, i=%d, fd=0x%x, y=0x%lx, u=0x%lx, reserved=%d",
                 buf_cfg->channel_id, i, buf_cfg->fd[i],
                 buf_cfg->addr[i].addr_y, buf_cfg->addr[i].addr_u,
                 buf_cfg->is_reserved_buf);
    }

    if (buf_cfg->count > 0) {
        ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_FRAME_ADDR, &parm);
        if (ret) {
            CMR_LOGE("Failed to QBuf i=%d, ret=%ld,", i, ret);
            goto exit;
        }
    }

exit:
    ATRACE_END();
    return ret;
}

cmr_int cmr_grab_cap_start(cmr_handle grab_handle, cmr_u32 skip_num) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    struct cmr_grab *p_grab;
    cmr_u32 num;
    cmr_u32 stream_on = 1;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;

    num = skip_num;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_CAP_SKIP_NUM, &num);
    CMR_RTN_IF_ERR(ret);

    ATRACE_BEGIN("dcam_stream_on");
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_STREAM_ON, &stream_on);
    if (ret) {
        CMR_LOGE("SPRD_IMG_IO_STREAM_ON failed");
        goto exit;
    }
    ATRACE_END();

    pthread_mutex_lock(&p_grab->status_mutex);
    p_grab->is_on = 1;
    pthread_mutex_unlock(&p_grab->status_mutex);

    ATRACE_BEGIN("sensor_stream_on");
    if (p_grab->stream_on_cb) {
        (*p_grab->stream_on_cb)(1, p_grab->init_param.oem_handle);
    }
    ATRACE_END();

exit:
    CMR_LOGI("ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int cmr_grab_cap_stop(cmr_handle grab_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    cmr_s32 i;
    struct cmr_grab *p_grab;
    cmr_u32 stream_on = 0;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;

    pthread_mutex_lock(&p_grab->status_mutex);
    p_grab->is_on = 0;
    pthread_mutex_unlock(&p_grab->status_mutex);

    for (i = 0; i < CHN_MAX; i++) {
        pthread_mutex_lock(&p_grab->path_mutex[i]);
    }
    if (p_grab->stream_on_cb) {
        (*p_grab->stream_on_cb)(0, p_grab->init_param.oem_handle);
    }

    ret = ioctl(p_grab->fd, SPRD_IMG_IO_STREAM_OFF, &stream_on);
    for (i = 0; i < CHN_MAX; i++) {
        p_grab->chn_status[i] = CHN_IDLE;
        pthread_mutex_unlock(&p_grab->path_mutex[i]);
    }

    CMR_LOGI("ret = %ld", ret);
    ATRACE_END();
    return ret;
}

// for sharkl2-like capture path
cmr_int cmr_grab_start_capture(cmr_handle grab_handle,
                               struct sprd_img_capture_param capture_param) {
    struct cmr_grab *p_grab;
    cmr_int ret = 0;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_START_CAPTURE, &capture_param);
    if (ret) {
        CMR_LOGE("SPRD_IMG_IO_START_CAPTURE failed");
    }
    CMR_LOGI("ret = %ld,capture_status %u,timestamp=%lld", ret,
             capture_param.type, capture_param.timestamp);

    return ret;
}

// for sharkl2-like capture path
cmr_int cmr_grab_stop_capture(cmr_handle grab_handle) {
    struct cmr_grab *p_grab;
    cmr_int ret = 0;
    cmr_u32 stop = 1;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_STOP_CAPTURE, &stop);
    if (ret) {
        CMR_LOGE("failed to stop offline path");
    }
    CMR_LOGI("ret = %ld", ret);

    return ret;
}

cmr_int cmr_grab_set_trace_flag(cmr_handle grab_handle, cmr_u32 trace_owner,
                                cmr_u32 val) {
    struct cmr_grab *p_grab;
    cmr_int ret = 0;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;

    if (PREV_TRACE == trace_owner) {
        p_grab->is_prev_trace = val;
    } else if (CAP_TRACE == trace_owner) {
        p_grab->is_cap_trace = val;
    } else {
        CMR_LOGE("unknown trace owner!");
    }
    return ret;
}

cmr_int cmr_grab_set_rt_refocus(cmr_handle grab_handle, cmr_u32 rt_refocus) {
    struct cmr_grab *p_grab;
    cmr_int ret = 0;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;

    p_grab->is_rt_refocus = rt_refocus;

    CMR_LOGI("rt_refocus %d", rt_refocus);

    return ret;
}

cmr_int cmr_grab_deinit_notice(cmr_handle grab_handle) {
    struct cmr_grab *p_grab;
    cmr_int ret = 0;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;

    p_grab->isp_cb_enable = 0;

    CMR_LOGD("isp_cb_enable %d", p_grab->isp_cb_enable);

    return ret;
}

cmr_int cmr_grab_cap_resume(cmr_handle grab_handle, cmr_u32 channel_id,
                            cmr_u32 skip_number, cmr_u32 deci_factor,
                            cmr_s32 frm_num) {
    UNUSED(skip_number);
    UNUSED(deci_factor);

    cmr_int ret = 0;
    struct cmr_grab *p_grab;
    cmr_u32 ch_id;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;
    CMR_LOGI("channel_id %d, frm_num %d, status %d", channel_id, frm_num,
             p_grab->chn_status[channel_id]);

    ch_id = channel_id;
    pthread_mutex_lock(&p_grab->path_mutex[channel_id]);
    p_grab->chn_status[channel_id] = CHN_BUSY;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_PATH_RESUME, &ch_id);
    pthread_mutex_unlock(&p_grab->path_mutex[channel_id]);

    return ret;
}

cmr_int cmr_grab_cap_pause(cmr_handle grab_handle, cmr_u32 channel_id,
                           cmr_u32 reconfig_flag) {
    cmr_int ret = 0;
    struct cmr_grab *p_grab;
    struct sprd_img_parm parm;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;
    CMR_LOGI("channel_id %d,reconfig_flag %d", channel_id, reconfig_flag);

    parm.channel_id = channel_id;
    parm.reserved[0] = reconfig_flag;
    pthread_mutex_lock(&p_grab->path_mutex[channel_id]);
    p_grab->chn_status[channel_id] = CHN_IDLE;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_PATH_PAUSE, &parm);
    pthread_mutex_unlock(&p_grab->path_mutex[channel_id]);

    return ret;
}

cmr_int cmr_grab_get_cap_time(cmr_handle grab_handle, cmr_u32 *sec,
                              cmr_u32 *usec) {
    cmr_int ret = 0;
    struct cmr_grab *p_grab = (struct cmr_grab *)grab_handle;
    struct sprd_img_time time;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;

    ret = ioctl(p_grab->fd, SPRD_IMG_IO_GET_TIME, &time);
    CMR_RTN_IF_ERR(ret);

    *sec = time.sec;
    *usec = time.usec;
    CMR_LOGI("sec=%d, usec=%d", *sec, *usec);

exit:
    return ret;
}

cmr_int cmr_grab_free_frame(cmr_handle grab_handle, cmr_u32 channel_id,
                            cmr_u32 index) {
    UNUSED(grab_handle);
    UNUSED(channel_id);
    UNUSED(index);

    cmr_int ret = 0;

    return ret;
}

cmr_int cmr_grab_scale_capability(cmr_handle grab_handle, cmr_u32 *width,
                                  cmr_u32 *sc_factor, cmr_u32 *sc_threshold) {
    cmr_int ret = 0;
    cmr_int cnt;
    struct cmr_grab *p_grab;
    struct sprd_img_read_op op;

    if (NULL == width || NULL == sc_factor) {
        CMR_LOGE("Wrong param, %p %p", width, sc_factor);
        return -ENODEV;
    }
    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;

    op.cmd = SPRD_IMG_GET_SCALE_CAP;
    cnt = read(p_grab->fd, &op, sizeof(struct sprd_img_read_op));
    if (cnt != sizeof(struct sprd_img_read_op))
        ret = cnt;

    *width = op.parm.reserved[0];
    *sc_factor = op.parm.reserved[1];
    *sc_threshold = op.parm.reserved[2];
    CMR_LOGI("width %d, sc_factor %d, sc_threshold %d", *width, *sc_factor,
             *sc_threshold);
    return ret;
}

cmr_int cmr_grab_path_capability(cmr_handle grab_handle,
                                 struct cmr_path_capability *capability) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    cmr_int cnt;
    struct cmr_grab *p_grab;
    struct sprd_img_read_op op;
    cmr_int i = 0;
    cmr_int yuv_cnt = 0;
    cmr_int trim_cnt = 0;

    if (NULL == capability) {
        CMR_LOGE("Wrong param, %p", capability);
        return -ENODEV;
    }
    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;

    memset(capability, 0, sizeof(struct cmr_path_capability));
    op.cmd = SPRD_IMG_GET_PATH_CAP;
    cnt = read(p_grab->fd, &op, sizeof(struct sprd_img_read_op));
    if (cnt != sizeof(struct sprd_img_read_op))
        ret = cnt;

    for (i = 0; i < (cmr_int)(op.parm.capability.count); i++) {
        if (op.parm.capability.path_info[i].support_yuv) {
            yuv_cnt++;
            if (p_grab->chn_status[i] == CHN_IDLE) {
                capability->yuv_available_cnt++;
            }
        }
        if (op.parm.capability.path_info[i].support_trim) {
            trim_cnt++;
        }
        if (op.parm.capability.path_info[i].is_scaleing_path &&
            p_grab->chn_status[i] == CHN_IDLE) {
            capability->hw_scale_available = 1;
        }
    }
    if (yuv_cnt > 2 && trim_cnt > 2) {
        capability->is_video_prev_diff = 1;
    }
    if (yuv_cnt > 2 && trim_cnt <= 2) {
        capability->capture_no_trim = 1;
    }

    if (1 == op.parm.capability.path_info[0].support_yuv) {
        if (0 == op.parm.capability.path_info[0].support_trim) {
            capability->zoom_post_proc = ZOOM_POST_PROCESS;
        } else {
            capability->zoom_post_proc = ZOOM_POST_PROCESS_WITH_TRIM;
        }
    } else {
        capability->zoom_post_proc = ZOOM_BY_CAP;
    }
    capability->capture_pause = 1;

    CMR_LOGV("video prev %d scale %d capture_no_trim %d capture_pause %d "
             "zoom_post_proc %d",
             capability->is_video_prev_diff, capability->hw_scale_available,
             capability->capture_no_trim, capability->capture_pause,
             capability->zoom_post_proc);

    ATRACE_END();
    return ret;
}

static cmr_s32 cmr_grab_evt_id(cmr_s32 isr_flag) {
    cmr_s32 ret = CMR_GRAB_MAX;

    switch (isr_flag) {
    case IMG_TX_DONE:
        ret = CMR_GRAB_TX_DONE;
        break;

    case IMG_NO_MEM:
        ret = CMR_GRAB_TX_NO_MEM;
        break;

    case IMG_TX_ERR:
        ret = CMR_GRAB_TX_ERROR;
        break;

    case IMG_CSI2_ERR:
        ret = CMR_GRAB_CSI2_ERR;
        break;

    case IMG_TIMEOUT:
        ret = CMR_GRAB_TIME_OUT;
        break;

    case IMG_CANCELED_BUF:
        ret = CMR_GRAB_CANCELED_BUF;
        break;

    default:
        CMR_LOGI("isr_flag 0x%x", isr_flag);
        break;
    }

    return ret;
}

static cmr_int cmr_grab_create_thread(cmr_handle grab_handle) {
    cmr_int ret = 0;
    pthread_attr_t attr;
    struct cmr_grab *p_grab;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    ret = pthread_create(&p_grab->thread_handle, &attr, cmr_grab_thread_proc,
                         (void *)grab_handle);
    pthread_setname_np(p_grab->thread_handle, "grab");
    pthread_attr_destroy(&attr);
    return ret;
}

static cmr_int cmr_grab_kill_thread(cmr_handle grab_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    cmr_int cnt;
    void *dummy;
    struct cmr_grab *p_grab;
    struct sprd_img_write_op op;
    int status;
    struct timespec ts;
    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;
    memset(&op, 0, sizeof(struct sprd_img_write_op));

    CMR_LOGI("E");

    op.cmd = SPRD_IMG_STOP_DCAM;
    op.sensor_id = p_grab->init_param.sensor_id;
    cnt = write(p_grab->fd, &op, sizeof(struct sprd_img_write_op));
    if (cnt == sizeof(struct sprd_img_write_op)) {
        CMR_LOGI("write OK");
        sem_wait(&p_grab->close_sem);
        CMR_LOGI("wait OK");
        ret = pthread_join(p_grab->thread_handle, &dummy);
        p_grab->thread_handle = 0;
    } else {
        CMR_LOGE("shoud not run to here");
        ret = cnt;
    }

    CMR_LOGI("X");
    ATRACE_END();
    return ret;
}

static void *cmr_grab_thread_proc(void *data) {
    cmr_s32 evt_id = -1;
    struct frm_info frame;
    cmr_u32 on_flag = 0;
    cmr_s32 frm_num = -1;
    cmr_s32 cnt;
    struct cmr_grab *p_grab;
    struct img_data_end endian;
    struct sprd_img_read_op op;
    struct sprd_img_res res;
    struct sprd_img_statis_info statis_info;
    struct sprd_irq_info irq_info;

    cmr_bzero(&res, sizeof(res));
    cmr_bzero(&statis_info, sizeof(statis_info));
    cmr_bzero(&irq_info, sizeof(irq_info));

    p_grab = (struct cmr_grab *)data;
    if (!p_grab)
        return NULL;
    if (-1 == p_grab->fd)
        return NULL;

    CMR_LOGI("E");

    struct camera_context *cxt =
        (struct camera_context *)p_grab->init_param.oem_handle;

    while (1) {
        cnt = sizeof(struct sprd_img_read_op);
        op.cmd = SPRD_IMG_GET_FRM_BUFFER;
        op.sensor_id = p_grab->init_param.sensor_id;
        if (cnt != read(p_grab->fd, &op, sizeof(struct sprd_img_read_op))) {
            CMR_LOGE("read failed");
            break;
        }

        if (IMG_TX_STOP == op.evt) {
            // stopped , to do release resource
            CMR_LOGI("TX Stopped, exit thread");
            break;
        } else if (IMG_SYS_BUSY == op.evt) {
            CMR_LOGI("continue");
            continue;
        } else {
            // normal irq
            if (op.parm.frame.irq_type == CAMERA_IRQ_IMG) {
                evt_id = cmr_grab_evt_id(op.evt);
                if (CMR_GRAB_MAX == evt_id) {
                    continue;
                }

                frame.channel_id = op.parm.frame.channel_id;

                CMR_LOGV("sensor_id %d, channel_id 0x%x, id 0x%x, evt_id 0x%x "
                         "sec %u usec %u fd 0x%x, yaddr_vir 0x%x",
                         p_grab->init_param.sensor_id, op.parm.frame.channel_id,
                         op.parm.frame.index, evt_id, op.parm.frame.sec,
                         op.parm.frame.usec, op.parm.frame.mfd,
                         op.parm.frame.yaddr_vir);

                frame.height = op.parm.frame.height;
                frame.frame_id = op.parm.frame.index;
                frame.frame_real_id = op.parm.frame.real_index;
                frame.sec = op.parm.frame.sec;
                frame.usec = op.parm.frame.usec;
                frame.monoboottime = op.parm.frame.monoboottime;
                frame.length = op.parm.frame.length;
                frame.base = op.parm.frame.frm_base_id;
                frame.fmt = cmr_grab_get_img_type(op.parm.frame.img_fmt);
                frame.yaddr = op.parm.frame.yaddr;
                frame.uaddr = op.parm.frame.uaddr;
                frame.vaddr = op.parm.frame.vaddr;
                frame.yaddr_vir = op.parm.frame.yaddr_vir;
                frame.uaddr_vir = op.parm.frame.uaddr_vir;
                frame.vaddr_vir = op.parm.frame.vaddr_vir;
                frame.fd = op.parm.frame.mfd;

                pthread_mutex_lock(&p_grab->status_mutex);
                on_flag = p_grab->is_on;
                pthread_mutex_unlock(&p_grab->status_mutex);
                if (on_flag) {
                    pthread_mutex_lock(&p_grab->cb_mutex);
                    if (p_grab->grab_evt_cb) {
                        (*p_grab->grab_evt_cb)(
                            evt_id, &frame,
                            (void *)p_grab->init_param.oem_handle);
                    }
                    pthread_mutex_unlock(&p_grab->cb_mutex);
                }
            } else if (op.parm.frame.irq_type == CAMERA_IRQ_STATIS) {
                evt_id = cmr_grab_evt_id(op.evt);
                if (CMR_GRAB_MAX == evt_id) {
                    continue;
                }
                statis_info.buf_size = op.parm.frame.buf_size;
                statis_info.phy_addr = op.parm.frame.phy_addr;
                statis_info.vir_addr = op.parm.frame.vir_addr;
                statis_info.addr_offset = op.parm.frame.addr_offset;
                statis_info.kaddr[0] = op.parm.frame.kaddr[0];
                statis_info.kaddr[1] = op.parm.frame.kaddr[1];
                statis_info.irq_property = op.parm.frame.irq_property;
                statis_info.mfd = op.parm.frame.mfd;
                statis_info.sec = op.parm.frame.sec;
                statis_info.usec = op.parm.frame.usec;
                statis_info.frame_id = op.parm.frame.frame_id;
                CMR_LOGV("got one frame statis buf_size 0x%x phy_addr 0x%x "
                         "vir_addr 0x%x irq_property 0x%x, "
                         "op.parm.frame.vir_addr = 0x%x, "
                         "op.parm.frame.addr_offset = 0x%x",
                         statis_info.buf_size, statis_info.phy_addr,
                         statis_info.vir_addr, statis_info.irq_property,
                         op.parm.frame.vir_addr, op.parm.frame.addr_offset);

                pthread_mutex_lock(&p_grab->cb_mutex);
                if (p_grab->isp_statis_evt_cb && p_grab->isp_cb_enable) {
                    (*p_grab->isp_statis_evt_cb)(
                        evt_id, &statis_info, (void *)cxt->isp_cxt.isp_handle);
                }
                pthread_mutex_unlock(&p_grab->cb_mutex);
            } else if (op.parm.frame.irq_type == CAMERA_IRQ_DONE) {
                irq_info.irq_property = op.parm.frame.irq_property;
                irq_info.sec = op.parm.frame.sec;
                irq_info.usec = op.parm.frame.usec;
                irq_info.frame_id = op.parm.frame.frame_id;

                pthread_mutex_lock(&p_grab->cb_mutex);
                if (p_grab->isp_irq_proc_evt_cb && p_grab->isp_cb_enable) {
                    (p_grab->isp_irq_proc_evt_cb)(
                        evt_id, &irq_info, (void *)cxt->isp_cxt.isp_handle);
                }
                pthread_mutex_unlock(&p_grab->cb_mutex);
            } else if (op.parm.frame.irq_type == CAMERA_IRQ_3DNR_DONE) {
                pthread_mutex_lock(&p_grab->cb_mutex);
                if (p_grab->grab_3dnr_evt_cb) {
                    (*p_grab->grab_3dnr_evt_cb)(
                        evt_id, &frame, (void *)p_grab->init_param.oem_handle);
                }
                pthread_mutex_unlock(&p_grab->cb_mutex);
            } else if (op.parm.frame.irq_type == CAMERA_IRQ_POST_YNR_DONE) {
                pthread_mutex_lock(&p_grab->cb_mutex);
                if (p_grab->grab_post_ynr_evt_cb && p_grab->isp_cb_enable) {
                    (p_grab->grab_post_ynr_evt_cb)(
                        evt_id, NULL, (void *)p_grab->init_param.oem_handle);
                }
                pthread_mutex_unlock(&p_grab->cb_mutex);
            }
        }
    }

    sem_post(&p_grab->close_sem);
    CMR_LOGI("X");
    return NULL;
}

static cmr_u32 cmr_grab_get_4cc(cmr_u32 img_type) {
    cmr_u32 ret_4cc;

    switch (img_type) {
    case IMG_DATA_TYPE_YUV422:
        ret_4cc = IMG_PIX_FMT_YUV422P;
        break;

    case IMG_DATA_TYPE_YUV420:
        ret_4cc = IMG_PIX_FMT_NV21;
        break;

    case IMG_DATA_TYPE_YVU420:
        ret_4cc = IMG_PIX_FMT_NV12;
        break;

    case IMG_DATA_TYPE_YUV420_3PLANE:
        ret_4cc = IMG_PIX_FMT_YUV420;
        break;

    case IMG_DATA_TYPE_RGB565:
        ret_4cc = IMG_PIX_FMT_RGB565;
        break;

    case IMG_DATA_TYPE_RAW:
        ret_4cc = IMG_PIX_FMT_GREY;
        break;

    case IMG_DATA_TYPE_PDAF_TYPE3:
        ret_4cc = IMG_PIX_FMT_PDA3;
        break;

    case IMG_DATA_TYPE_JPEG:
        ret_4cc = IMG_PIX_FMT_JPEG;
        break;

    default:
        ret_4cc = IMG_PIX_FMT_NV21;
        break;
    }

    CMR_LOGV("fmt %d", img_type);

    return ret_4cc;
}

static cmr_u32 cmr_grab_get_img_type(cmr_u32 fourcc) {
    cmr_u32 img_type;

    switch (fourcc) {
    case IMG_PIX_FMT_YUV422P:
        img_type = IMG_DATA_TYPE_YUV422;
        break;

    case IMG_PIX_FMT_NV21:
        img_type = IMG_DATA_TYPE_YUV420;
        break;

    case IMG_PIX_FMT_NV12:
        img_type = IMG_DATA_TYPE_YVU420;
        break;

    case IMG_PIX_FMT_YUV420:
        img_type = IMG_DATA_TYPE_YUV420_3PLANE;
        break;

    case IMG_PIX_FMT_RGB565:
        img_type = IMG_DATA_TYPE_RGB565;
        break;

    case IMG_PIX_FMT_GREY:
        img_type = IMG_DATA_TYPE_RAW;
        break;

    case IMG_PIX_FMT_JPEG:
        img_type = IMG_DATA_TYPE_JPEG;
        break;

    default:
        img_type = IMG_DATA_TYPE_YUV420;
        break;
    }

    CMR_LOGV("fmt %d", img_type);

    return img_type;
}

static cmr_u32 cmr_grab_get_data_endian(struct img_data_end *in_endian,
                                        struct img_data_end *out_endian) {
    if (NULL == in_endian || NULL == out_endian) {
        CMR_LOGE("Wrong param");
        return -ENODEV;
    }

    switch (in_endian->uv_endian) {
    case IMG_ENDIAN_LITTLE:
        out_endian->uv_endian = IMG_DATA_ENDIAN_2PLANE_UVUV;
        break;

    case IMG_ENDIAN_HALFBIG:
    case IMG_ENDIAN_BIG:
        out_endian->uv_endian = IMG_DATA_ENDIAN_2PLANE_VUVU;
        break;

    default:
        out_endian->uv_endian = IMG_DATA_ENDIAN_2PLANE_UVUV;
        break;
    }

    out_endian->y_endian = in_endian->y_endian;

    CMR_LOGI("y uv endian %d %d %d %d ", out_endian->y_endian,
             out_endian->uv_endian, in_endian->y_endian, in_endian->uv_endian);

    return 0;
}

cmr_u32 cmr_grab_get_dcam_endian(struct img_data_end *in_endian,
                                 struct img_data_end *out_endian) {
    if (NULL == in_endian || NULL == out_endian) {
        CMR_LOGE("Wrong param");
        return -ENODEV;
    }

    switch (in_endian->uv_endian) {
    case IMG_DATA_ENDIAN_2PLANE_UVUV:
        out_endian->uv_endian = IMG_ENDIAN_LITTLE;
        break;

    case IMG_DATA_ENDIAN_2PLANE_VUVU:
        out_endian->uv_endian = IMG_ENDIAN_HALFBIG;
        break;

    default:
        out_endian->uv_endian = IMG_ENDIAN_LITTLE;
        break;
    }

    out_endian->y_endian = in_endian->y_endian;

    CMR_LOGI("y uv endian %d %d %d %d ", out_endian->y_endian,
             out_endian->uv_endian, in_endian->y_endian, in_endian->uv_endian);

    return 0;
}

cmr_int cmr_grab_flash_cb(cmr_handle grab_handle,
                          struct grab_flash_opt *flash_opt) {
    cmr_int ret = 0;
    struct cmr_grab *p_grab;
    struct sprd_img_set_flash set_flash;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;

    bzero(&set_flash, sizeof(struct sprd_img_set_flash));
    if (FLASH_TORCH == flash_opt->flash_mode) {
#ifdef CONFIG_CAMERA_FLASH_LED_SWITCH
        set_flash.led1_ctrl = flash_opt->led1_enable;
#else
        set_flash.led0_ctrl = flash_opt->led0_enable;
#endif
    } else {
#ifdef CONFIG_CAMERA_FLASH_LED_0
        set_flash.led0_ctrl = flash_opt->led0_enable;
#endif
#ifdef CONFIG_CAMERA_FLASH_LED_1
        set_flash.led1_ctrl = flash_opt->led1_enable;
#endif
    }

    set_flash.led0_status = flash_opt->flash_mode;
    set_flash.led1_status = flash_opt->flash_mode;
    set_flash.flash_index = flash_opt->flash_index;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_FLASH, &set_flash);
    if (ret) {
        CMR_LOGE("error");
    }
    return ret;
}

cmr_int cmr_grab_stream_cb(cmr_handle grab_handle, grab_stream_on str_on) {
    cmr_int ret = 0;
    struct cmr_grab *p_grab;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    p_grab->stream_on_cb = str_on;

    return ret;
}

cmr_int cmr_grab_set_zoom_mode(cmr_handle grab_handle, cmr_u32 opt) {
    cmr_int ret = 0;
    struct cmr_grab *p_grab;
    cmr_u32 zoom;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;
    zoom = opt;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_SET_ZOOM_MODE, &zoom);
    if (ret) {
        CMR_LOGE("error");
    }
    return ret;
}

cmr_int cmr_grab_cfg_flash(cmr_handle grab_handle,
                           struct sprd_flash_cfg_param *cfg) {
    cmr_int ret = 0;
    struct cmr_grab *p_grab;

    p_grab = (struct cmr_grab *)grab_handle;
    CMR_CHECK_HANDLE;
    CMR_CHECK_FD;
    ret = ioctl(p_grab->fd, SPRD_IMG_IO_CFG_FLASH, cfg);
    if (ret) {
        CMR_LOGE("error");
    }
    return ret;
}
