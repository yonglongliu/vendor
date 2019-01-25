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

#define LOG_TAG "cmr_rotate"

#include <fcntl.h>
#include <sys/ioctl.h>
#include "cmr_type.h"
#include "cmr_cvt.h"
#include "sprd_cpp.h"

static char rot_dev_name[50] = "/dev/sprd_cpp";

extern cmr_s32 cmr_grab_get_cpp_fd(cmr_handle grab_handle);

struct rot_file {
    cmr_int fd;
};

static unsigned int cmr_rot_fmt_cvt(cmr_u32 cmr_fmt) {
    unsigned int fmt = ROT_FMT_MAX;

    switch (cmr_fmt) {
    case IMG_DATA_TYPE_YUV422:
        fmt = ROT_YUV422;
        break;

    case IMG_DATA_TYPE_YUV420:
    case IMG_DATA_TYPE_YVU420:
        fmt = ROT_YUV420;
        break;

    case IMG_DATA_TYPE_RGB565:
        fmt = ROT_RGB565;
        break;

    case IMG_DATA_TYPE_RGB888:
        fmt = ROT_RGB888;
        break;

    default:
        break;
    }

    return fmt;
}

cmr_int cmr_rot_open(cmr_handle *rot_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct rot_file *file = NULL;
    cmr_int fd = -1;
    cmr_u32 val = 1;

    file = malloc(sizeof(struct rot_file));
    if (!file) {
        ret = -CMR_CAMERA_FAIL;
        goto open_out;
    }

    fd = open(rot_dev_name, O_RDWR, 0);
    if (fd < 0) {
        CMR_LOGE("Fail to open rotation device.");
        goto rot_free;
    }

    ret = ioctl(fd, SPRD_CPP_IO_OPEN_ROT, &val);
    if (ret)
        goto rot_free;

    file->fd = fd;

    *rot_handle = (cmr_handle)file;
    goto open_out;

rot_free:
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
    if (file)
        free(file);
    file = NULL;
open_out:

    return ret;
}

cmr_int cmr_rot(struct cmr_rot_param *rot_param) {
    struct sprd_cpp_rot_cfg_parm rot_cfg;
    cmr_int ret = CMR_CAMERA_SUCCESS;
    enum img_angle angle;
    struct img_frm *src_img;
    struct img_frm *dst_img;
    cmr_int fd;
    struct rot_file *file = NULL;

    if (!rot_param) {
        CMR_LOGE("Invalid Param!");
        ret = -CMR_CAMERA_FAIL;
        goto rot_exit;
    }

    file = (struct rot_file *)rot_param->handle;
    if (!file) {
        CMR_LOGE("Invalid Param rot_file !");
        ret = -CMR_CAMERA_FAIL;
        goto rot_exit;
    }

    fd = file->fd;
    if (fd < 0) {
        CMR_LOGE("Invalid Param handle!");
        ret = -CMR_CAMERA_FAIL;
        goto rot_exit;
    }

    angle = rot_param->angle;
    src_img = &rot_param->src_img;
    dst_img = &rot_param->dst_img;

    if (NULL == src_img || NULL == dst_img) {
        CMR_LOGE("Wrong parameter 0x%lx 0x%lx", (cmr_uint)src_img,
                 (cmr_uint)dst_img);
        ret = -CMR_CAMERA_FAIL;
        goto rot_exit;
    }

    CMR_LOGI("angle %ld, src fd 0x%x, w h %ld %ld, dst fd 0x%x", (cmr_int)angle,
             src_img->fd, (cmr_int)src_img->size.width,
             (cmr_int)src_img->size.height, dst_img->fd);

    if ((cmr_u32)angle < (cmr_u32)(IMG_ANGLE_90)) {
        CMR_LOGE("Wrong angle %ld", (cmr_int)angle);
        ret = -CMR_CAMERA_FAIL;
        goto rot_exit;
    }

    rot_cfg.format = cmr_rot_fmt_cvt(src_img->fmt);
    if (rot_cfg.format >= ROT_FMT_MAX) {
        CMR_LOGE("Unsupported format %d, %d", src_img->fmt, rot_cfg.format);
        ret = -CMR_CAMERA_FAIL;
        goto rot_exit;
    }

    rot_cfg.angle = angle - IMG_ANGLE_90 + ROT_90;
    rot_cfg.src_addr.y = (uint32_t)src_img->addr_phy.addr_y;
    rot_cfg.src_addr.u = (uint32_t)src_img->addr_phy.addr_u;
    rot_cfg.src_addr.v = (uint32_t)src_img->addr_phy.addr_v;
    rot_cfg.src_addr.mfd[0] = src_img->fd;
    rot_cfg.src_addr.mfd[1] = src_img->fd;
    rot_cfg.src_addr.mfd[2] = src_img->fd;
    rot_cfg.dst_addr.y = (uint32_t)dst_img->addr_phy.addr_y;
    rot_cfg.dst_addr.u = (uint32_t)dst_img->addr_phy.addr_u;
    rot_cfg.dst_addr.v = (uint32_t)dst_img->addr_phy.addr_v;
    rot_cfg.dst_addr.mfd[0] = dst_img->fd;
    rot_cfg.dst_addr.mfd[1] = dst_img->fd;
    rot_cfg.dst_addr.mfd[2] = dst_img->fd;
    rot_cfg.size.w = (cmr_u16)src_img->size.width;
    rot_cfg.size.h = (cmr_u16)src_img->size.height;
    rot_cfg.src_endian = src_img->data_end.uv_endian;
    rot_cfg.dst_endian = dst_img->data_end.uv_endian;

    ret = ioctl(fd, SPRD_CPP_IO_START_ROT, &rot_cfg);
    if (ret) {
        CMR_LOGE("cmr_rot camera_cpp_io_rotation fail. ret = %d", ret);
        ret = -CMR_CAMERA_FAIL;
        goto rot_exit;
    }

rot_exit:
    CMR_LOGI("X ret=%ld", ret);
    return ret;
}

cmr_int cmr_rot_close(cmr_handle rot_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct rot_file *file = (struct rot_file *)(rot_handle);

    CMR_LOGI("Start to close rotation device.");

    if (!file)
        goto out;

    if (-1 == file->fd) {
        CMR_LOGE("Invalid fd");
        ret = -CMR_CAMERA_FAIL;
        goto close_free;
    }

    /*    if (ret)
            ret = -CMR_CAMERA_FAIL;*/
    close(file->fd);

close_free:
    free(file);
out:

    CMR_LOGI("ret=%ld", ret);
    return ret;
}
