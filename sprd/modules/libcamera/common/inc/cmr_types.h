/*
 *******************************************************************************
 * $Header$
 *
 *  Copyright (c) 2016-2025 Spreadtrum Inc. All rights reserved.
 *
 *  +-----------------------------------------------------------------+
 *  | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *  | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *  | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. |
 *  | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *  | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *  | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *  |                                                                 |
 *  | THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT   |
 *  | ANY PRIOR NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY |
 *  | SPREADTRUM INC.                                                 |
 *  +-----------------------------------------------------------------+
 *
 *
 *******************************************************************************
 */

#ifndef _CMR_TYPES_H_
#define _CMR_TYPES_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include <sys/types.h>
#include <utils/Log.h>

enum camera_mem_cb_type {
    CAMERA_PREVIEW = 0,
    CAMERA_SNAPSHOT,
    CAMERA_SNAPSHOT_ZSL,
    CAMERA_VIDEO,
    CAMERA_PREVIEW_RESERVED,
    CAMERA_SNAPSHOT_ZSL_RESERVED_MEM,
    CAMERA_SNAPSHOT_ZSL_RESERVED,
    CAMERA_VIDEO_RESERVED,
    CAMERA_ISP_LSC,
    CAMERA_ISP_BINGING4AWB,
    CAMERA_SNAPSHOT_PATH,
    CAMERA_ISP_FIRMWARE,
    CAMERA_SNAPSHOT_HIGHISO,
    CAMERA_ISP_RAW_DATA,
    CAMERA_ISP_ANTI_FLICKER,
    CAMERA_ISP_RAWAE,
    CAMERA_ISP_PREVIEW_Y,
    CAMERA_ISP_PREVIEW_YUV,
#if defined(CONFIG_CAMERA_ISP_DIR_2_1) || defined(CONFIG_CAMERA_ISP_DIR_2_4)
    CAMERA_DEPTH_MAP,
    CAMERA_DEPTH_MAP_RESERVED,
#else
    CAMERA_SENSOR_DATATYPE_MAP,          // CAMERA_DEPTH_MAP,
    CAMERA_SENSOR_DATATYPE_MAP_RESERVED, // CAMERA_DEPTH_MAP_RESERVED,
#endif
    CAMERA_PDAF_RAW,
    CAMERA_PDAF_RAW_RESERVED,
#if defined(CONFIG_CAMERA_ISP_DIR_2_1) || defined(CONFIG_CAMERA_ISP_DIR_2_4)
    CAMERA_ISP_STATIS,
    CAMERA_SNAPSHOT_3DNR,
    CAMERA_SNAPSHOT_3DNR_DST,
    CAMERA_PREVIEW_3DNR,
    CAMERA_PREVIEW_SCALE_3DNR,
#endif
    CAMERA_PREVIEW_DEPTH,
    CAMERA_PREVIEW_SW_OUT,
    CAMERA_SNAPSHOT_SW3DNR,
    CAMERA_SNAPSHOT_SW3DNR_PATH,
    CAMERA_MEM_CB_TYPE_MAX
};

typedef unsigned long cmr_uint;
typedef long cmr_int;
typedef uint64_t cmr_u64;
typedef int64_t cmr_s64;
typedef unsigned int cmr_u32;
typedef int cmr_s32;
typedef unsigned short cmr_u16;
typedef short cmr_s16;
typedef unsigned char cmr_u8;
typedef signed char cmr_s8;
typedef void *cmr_handle;

#ifndef bzero
#define bzero(p, len) memset(p, 0, len);
#endif

#ifndef UNUSED
#define UNUSED(x) (void) x
#endif
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

typedef cmr_int (*cmr_malloc)(cmr_u32 mem_type, cmr_handle oem_handle,
                              cmr_u32 *size, cmr_u32 *sum, cmr_uint *phy_addr,
                              cmr_uint *vir_addr, cmr_s32 *fd);
typedef cmr_int (*cmr_free)(cmr_u32 mem_type, cmr_handle oem_handle,
                            cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_s32 *fd,
                            cmr_u32 sum);
typedef cmr_int (*cmr_gpu_malloc)(cmr_u32 mem_type, cmr_handle oem_handle,
                                  cmr_u32 *size, cmr_u32 *sum,
                                  cmr_uint *phy_addr, cmr_uint *vir_addr,
                                  cmr_s32 *fd, void **handle, cmr_uint *width,
                                  cmr_uint *height);
#endif
