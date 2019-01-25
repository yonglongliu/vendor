/*
 * Copyright (C) 2008 The Android Open Source Project
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cutils/log.h>
//#include "cmr_common.h"
#include <semaphore.h>
#include <linux/ion.h>
#include "sprd_ion.h"
//#include <binder/MemoryHeapIon.h>
#include <linux/types.h>
#include <asm/ioctl.h>
#include "sprd_cpp.h"
#include "MemIon.h"

using namespace android;

#define UTEST_SCALING_COUNTER 0xFFFFFFFF
#define SAVE_SCALING_OUTPUT_DATA 0
#define ERR(x...) fprintf(stderr, x)
#define INFO(x...) fprintf(stdout, x)

#define UTEST_SCALING_EXIT_IF_ERR(n)                                           \
    do {                                                                       \
        if (n) {                                                               \
            ERR("utest scale  error  . Line:%d ", __LINE__);                   \
            goto exit;                                                         \
        }                                                                      \
    } while (0)

struct utest_scaling_context {
    MemIon *input_y_pmem_hp;
    uint32_t input_y_pmemory_size;
    unsigned long input_y_physical_addr;
    unsigned char *input_y_virtual_addr;

    MemIon *input_uv_pmem_hp;
    uint32_t input_uv_pmemory_size;
    unsigned long input_uv_physical_addr;
    unsigned char *input_uv_virtual_addr;

    MemIon *output_y_pmem_hp;
    uint32_t output_y_pmemory_size;
    unsigned long output_y_physical_addr;
    unsigned char *output_y_virtual_addr;

    MemIon *output_uv_pmem_hp;
    uint32_t output_uv_pmemory_size;
    unsigned long output_uv_physical_addr;
    unsigned char *output_uv_virtual_addr;

    uint32_t input_width;
    uint32_t input_height;
    uint32_t output_width;
    uint32_t output_height;
    int file_num;
    uint32_t input_format;
    uint32_t output_format;
};

static char utest_scaling_src_y_422_file[] = "/data/like/pic/src_y_422.raw";
static char utest_scaling_src_uv_422_file[] = "/data/like/pic/src_uv_422.raw";
static char utest_scaling_src_y_420_file[] = "/data/like/pic/src_y_420.raw";
static char utest_scaling_src_uv_420_file[] = "/data/like/pic/src_uv_420.raw";

static char utest_scaling_dst_y_file[] =
    "/data/like/pic/utest_scaling_dst_y_%dx%d_format_%d_%d.raw";
static char utest_scaling_dst_uv_file[] =
    "/data/like/pic/utest_scaling_dst_uv_%dx%d_format_%d_%d.raw";

static int utest_do_scaling(int fd, uint32_t input_width, uint32_t input_height,
                            uint32_t input_yaddr, uint32_t intput_uvaddr,
                            uint32_t input_fmt, uint32_t output_fmt,
                            uint32_t output_width, uint32_t output_height,
                            uint32_t output_yaddr, uint32_t output_uvaddr) {
    int ret = 0;
    struct sprd_cpp_rect src_rect;

    //	struct scale_frame scale_frm;

    struct sprd_cpp_scale_cfg_parm frame_params;

    memset((void *)&frame_params, 0x00, sizeof(struct sprd_cpp_scale_cfg_parm));

    // input params
    frame_params.input_size.w = input_width;
    frame_params.input_size.h = input_height;
    frame_params.input_rect.x = 0;
    frame_params.input_rect.y = 0;
    frame_params.input_rect.w = input_width;
    frame_params.input_rect.h = input_height;
    frame_params.input_format = input_fmt;
    frame_params.input_addr.y = input_yaddr;
    frame_params.input_addr.u = intput_uvaddr;
    frame_params.input_addr.v = intput_uvaddr;
    frame_params.input_endian.y_endian = 0;
    frame_params.input_endian.uv_endian = 0;

    // output params
    frame_params.output_size.w = output_width;
    frame_params.output_size.h = output_height;
    frame_params.output_format = output_fmt;
    frame_params.output_addr.y = output_yaddr;
    frame_params.output_addr.u = output_uvaddr;
    frame_params.output_addr.v = output_uvaddr;
    frame_params.output_endian.y_endian = 0;
    frame_params.output_endian.uv_endian = 0;

    ret = ioctl(fd, SPRD_CPP_IO_START_SCALE, &frame_params);

    if (ret) {
        ERR("utes_scaling camera_scaling fail. Line:%d\n", __LINE__);
        ret = -1;
    } else {
        ret = 0;
        INFO("utest_scaling: do_scaling OK\n");
    }

    return ret;
}

static void usage(void) {
    INFO("Usage:\n");
    INFO("utest_scaling -if format -of format -iw input_width -ih input_height "
         "-ow out_width -oh out_height -num file_num\n");
}

static int
utest_scaling_param_set(struct utest_scaling_context *g_utest_scaling_cxt_ptr,
                        int argc, char **argv) {
    int i = 0;
    struct utest_scaling_context *scaling_cxt_ptr = g_utest_scaling_cxt_ptr;

    if (argc < 9) {
        usage();
        return -1;
    }

    memset(scaling_cxt_ptr, 0, sizeof(utest_scaling_context));

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-if") == 0 && (i < argc - 1)) {
            if (strcmp(argv[i + 1], "yuv422") == 0)
                scaling_cxt_ptr->input_format = 2;
            else if (strcmp(argv[i + 1], "yuv420") == 0)
                scaling_cxt_ptr->input_format = 0;
            else
                ERR("error:picture format is error\n");

            i++;
        } else if (strcmp(argv[i], "-of") == 0 && (i < argc - 1)) {
            if (strcmp(argv[i + 1], "yuv422") == 0)
                scaling_cxt_ptr->output_format = 2;
            else if (strcmp(argv[i + 1], "yuv420") == 0)
                scaling_cxt_ptr->output_format = 0;
            else
                ERR("error:picture format is error\n");

            i++;
        } else if (strcmp(argv[i], "-iw") == 0 && (i < argc - 1)) {
            scaling_cxt_ptr->input_width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-ih") == 0 && (i < argc - 1)) {
            scaling_cxt_ptr->input_height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-ow") == 0 && (i < argc - 1)) {
            scaling_cxt_ptr->output_width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-oh") == 0 && (i < argc - 1)) {
            scaling_cxt_ptr->output_height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-num") == 0 && (i < argc - 1)) {
            scaling_cxt_ptr->file_num = atoi(argv[++i]);
        } else {
            usage();
            return -1;
        }
    }

    INFO("utest_scaling_set: input_size=(%d, %d), output_size=(%d, %d), "
         "cnt=%d\n",
         scaling_cxt_ptr->input_width, scaling_cxt_ptr->input_height,
         scaling_cxt_ptr->output_width, scaling_cxt_ptr->output_height,
         scaling_cxt_ptr->file_num);

    INFO("utest_scaling_set: scaling OK\n");

    return 0;
}

static int utest_mm_iommu_is_enabled(void) { return 0; }

static int
utest_scaling_mem_alloc(struct utest_scaling_context *g_utest_scaling_cxt_ptr) {
    struct utest_scaling_context *scaling_cxt_ptr = g_utest_scaling_cxt_ptr;

    /* alloc input y buffer */
    scaling_cxt_ptr->input_y_pmem_hp =
        new MemIon("/dev/ion",
                   scaling_cxt_ptr->input_width * scaling_cxt_ptr->input_height,
                   MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);

    if (scaling_cxt_ptr->input_y_pmem_hp->getHeapID() < 0) {
        ERR("failed to alloc input_y pmem buffer.\n");
        return -1;
    }

    scaling_cxt_ptr->input_y_pmem_hp->get_phy_addr_from_ion(
        &scaling_cxt_ptr->input_y_physical_addr,
        &scaling_cxt_ptr->input_y_pmemory_size);

    scaling_cxt_ptr->input_y_virtual_addr =
        (unsigned char *)scaling_cxt_ptr->input_y_pmem_hp->getBase();
    if (!scaling_cxt_ptr->input_y_physical_addr) {
        ERR("failed to alloc input_y pmem buffer:addr is null.\n");
        return -1;
    }
    INFO("LIKE:src y phy addr :0x%lx, virtual addr:%p\n",
         scaling_cxt_ptr->input_y_physical_addr,
         scaling_cxt_ptr->input_y_virtual_addr);

    memset(scaling_cxt_ptr->input_y_virtual_addr, 0x80,
           scaling_cxt_ptr->input_width * scaling_cxt_ptr->input_height);

    /* alloc input uv buffer */
    scaling_cxt_ptr->input_uv_pmem_hp =
        new MemIon("/dev/ion",
                   scaling_cxt_ptr->input_width * scaling_cxt_ptr->input_height,
                   MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);

    if (scaling_cxt_ptr->input_uv_pmem_hp->getHeapID() < 0) {
        ERR("failed to alloc input_uv pmem buffer.\n");
        return -1;
    }

    scaling_cxt_ptr->input_uv_pmem_hp->get_phy_addr_from_ion(
        &scaling_cxt_ptr->input_uv_physical_addr,
        &scaling_cxt_ptr->input_uv_pmemory_size);

    scaling_cxt_ptr->input_uv_virtual_addr =
        (unsigned char *)scaling_cxt_ptr->input_uv_pmem_hp->getBase();
    if (!scaling_cxt_ptr->input_uv_physical_addr) {
        ERR("failed to alloc input_uv pmem buffer:addr is null.\n");
        return -1;
    }
    INFO("LIKE:src uv phy addr :0x%lx, virtual addr:%p\n",
         scaling_cxt_ptr->input_uv_physical_addr,
         scaling_cxt_ptr->input_uv_virtual_addr);

    memset(scaling_cxt_ptr->input_uv_virtual_addr, 0x80,
           scaling_cxt_ptr->input_width * scaling_cxt_ptr->input_height);

    /* alloc outout y buffer */
    scaling_cxt_ptr->output_y_pmem_hp =
        new MemIon("/dev/ion", scaling_cxt_ptr->output_width *
                                   scaling_cxt_ptr->output_height,
                   MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);

    if (scaling_cxt_ptr->output_y_pmem_hp->getHeapID() < 0) {
        ERR("failed to alloc output_y pmem buffer.\n");
        return -1;
    }

    scaling_cxt_ptr->output_y_pmem_hp->get_phy_addr_from_ion(
        &scaling_cxt_ptr->output_y_physical_addr,
        &scaling_cxt_ptr->output_y_pmemory_size);

    scaling_cxt_ptr->output_y_virtual_addr =
        (unsigned char *)scaling_cxt_ptr->output_y_pmem_hp->getBase();
    if (!scaling_cxt_ptr->output_y_physical_addr) {
        ERR("failed to alloc output_y pmem buffer:addr is null.\n");
        return -1;
    }

    INFO("LIKE:dst y phy addr :0x%lx, virtual addr:%p\n",
         scaling_cxt_ptr->output_y_physical_addr,
         scaling_cxt_ptr->output_y_virtual_addr);

    /* alloc outout uv buffer */
    scaling_cxt_ptr->output_uv_pmem_hp =
        new MemIon("/dev/ion", scaling_cxt_ptr->output_width *
                                   scaling_cxt_ptr->output_height,
                   MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    if (scaling_cxt_ptr->output_uv_pmem_hp->getHeapID() < 0) {
        ERR("failed to alloc output_uv pmem buffer.\n");
        return -1;
    }

    scaling_cxt_ptr->output_uv_pmem_hp->get_phy_addr_from_ion(
        &scaling_cxt_ptr->output_uv_physical_addr,
        &scaling_cxt_ptr->output_uv_pmemory_size);

    scaling_cxt_ptr->output_uv_virtual_addr =
        (unsigned char *)scaling_cxt_ptr->output_uv_pmem_hp->getBase();
    if (!scaling_cxt_ptr->output_uv_physical_addr) {
        ERR("failed to alloc output_uv pmem buffer:addr is null.\n");
        return -1;
    }

    INFO("LIKE:dst uv phy addr :0x%lx, virtual addr:%p\n",
         scaling_cxt_ptr->output_uv_physical_addr,
         scaling_cxt_ptr->output_uv_virtual_addr);

    INFO("utest_scaling: alloc memory OK\n");

    return 0;
}

static int utest_scaling_mem_release(
    struct utest_scaling_context *g_utest_scaling_cxt_ptr) {
    struct utest_scaling_context *scaling_cxt_ptr = g_utest_scaling_cxt_ptr;

    delete scaling_cxt_ptr->input_y_pmem_hp;

    delete scaling_cxt_ptr->input_uv_pmem_hp;

    delete scaling_cxt_ptr->output_y_pmem_hp;

    delete scaling_cxt_ptr->output_uv_pmem_hp;

    return 0;
}

static int
utest_scaling_src_cfg(struct utest_scaling_context *g_utest_scaling_cxt_ptr) {
    FILE *fp = 0;
    struct utest_scaling_context *scaling_cxt_ptr = g_utest_scaling_cxt_ptr;

    INFO("utest scaling read src image start\n");

    if (scaling_cxt_ptr->file_num == 2) {
        /* get input_y src */
        if (scaling_cxt_ptr->input_format == 2)
            fp = fopen(utest_scaling_src_y_422_file, "r");
        else if (scaling_cxt_ptr->input_format == 0)
            fp = fopen(utest_scaling_src_y_420_file, "r");
        if (fp != NULL) {
            fread((void *)scaling_cxt_ptr->input_y_virtual_addr, 1,
                  scaling_cxt_ptr->input_width * scaling_cxt_ptr->input_height,
                  fp);
            fclose(fp);
        } else {
            ERR("utest_scaling_src_cfg fail : no input_y source file.\n");
            return -1;
        }

        /* get input_uv src */
        if (scaling_cxt_ptr->input_format == 2) {
            fp = fopen(utest_scaling_src_uv_422_file, "r");
            if (fp != NULL) {
                fread((void *)scaling_cxt_ptr->input_uv_virtual_addr, 1,
                      scaling_cxt_ptr->input_width *
                          scaling_cxt_ptr->input_height,
                      fp);
                fclose(fp);
            } else {
                ERR("utest_scaling_src_cfg fail : no input_uv source file.\n");
                return -1;
            }
        } else if (scaling_cxt_ptr->input_format == 0) {
            fp = fopen(utest_scaling_src_uv_420_file, "r");
            if (fp != NULL) {
                fread((void *)scaling_cxt_ptr->input_uv_virtual_addr, 1,
                      scaling_cxt_ptr->input_width *
                          scaling_cxt_ptr->input_height / 2,
                      fp);
                fclose(fp);
            } else {
                ERR("utest_scaling_src_cfg fail : no input_uv source file.\n");
                return -1;
            }
        }
    } else {
        ERR("utest_scaling_src_cfg fail :  file number is wrong.\n");
    }

    INFO("utest scaling read src image OK\n");
    return 0;
}

static int utest_scaling_save_raw_data(
    struct utest_scaling_context *g_utest_scaling_cxt_ptr) {

    FILE *fp = 0;
    char file_name[128] = "utest_scaling_output_temp.raw";
    struct utest_scaling_context *scaling_cxt_ptr = g_utest_scaling_cxt_ptr;

    sprintf(file_name, utest_scaling_dst_y_file, scaling_cxt_ptr->output_width,
            scaling_cxt_ptr->output_height, scaling_cxt_ptr->input_format,
            scaling_cxt_ptr->output_format);
    fp = fopen(file_name, "wb");
    if (fp != NULL) {
        fwrite((void *)scaling_cxt_ptr->output_y_virtual_addr, 1,
               scaling_cxt_ptr->output_width * scaling_cxt_ptr->output_height,
               fp);
        fclose(fp);
    } else {
        ERR("utest_scaling_save_raw_data: failed to open save_file_y.\n");
        return -1;
    }

    sprintf(file_name, utest_scaling_dst_uv_file, scaling_cxt_ptr->output_width,
            scaling_cxt_ptr->output_height, scaling_cxt_ptr->input_format,
            scaling_cxt_ptr->output_format);
    fp = fopen(file_name, "wb");
    if (fp != NULL) {
        if (scaling_cxt_ptr->output_format == 2)
            fwrite((void *)scaling_cxt_ptr->output_uv_virtual_addr, 1,
                   scaling_cxt_ptr->output_width *
                       scaling_cxt_ptr->output_height,
                   fp);
        else if (scaling_cxt_ptr->output_format == 0)
            fwrite((void *)scaling_cxt_ptr->output_uv_virtual_addr, 1,
                   scaling_cxt_ptr->output_width *
                       scaling_cxt_ptr->output_height / 2,
                   fp);

        fclose(fp);
    } else {
        ERR("utest_scaling_save_raw_data: failed to open save_file_uv.\n");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    int fd = 0;
    uint32_t i = 0;
    int64_t time_start = 0, time_end = 0;
    static struct utest_scaling_context utest_scaling_cxt;
    static struct utest_scaling_context *g_utest_scaling_cxt_ptr =
        &utest_scaling_cxt;
    struct utest_scaling_context *scaling_cxt_ptr = g_utest_scaling_cxt_ptr;

    if (utest_scaling_param_set(&utest_scaling_cxt, argc, argv))
        return -1;

    if (utest_scaling_mem_alloc(&utest_scaling_cxt))
        goto err;

    if (utest_scaling_src_cfg(&utest_scaling_cxt))
        goto err;

    fd = open("/dev/sprd_cpp", O_RDWR, 0);
    for (i = 0; i < 100; i++) {
        INFO("utest_scaling_testing  start\n");
        time_start = systemTime();

        utest_do_scaling(
            fd, scaling_cxt_ptr->input_width, scaling_cxt_ptr->input_height,
            scaling_cxt_ptr->input_y_physical_addr,
            scaling_cxt_ptr->input_uv_physical_addr,
            scaling_cxt_ptr->input_format, scaling_cxt_ptr->output_format,
            scaling_cxt_ptr->output_width, scaling_cxt_ptr->output_height,
            scaling_cxt_ptr->output_y_physical_addr,
            scaling_cxt_ptr->output_uv_physical_addr);

        time_end = systemTime();
        INFO("utest_scaling testing:  calc time=%d, i = %x\n",
             (unsigned int)((time_end - time_start) / 1000000L), i);
        // usleep(30*1000);
        utest_scaling_save_raw_data(&utest_scaling_cxt);
    }
    close(fd);

err:
    utest_scaling_mem_release(&utest_scaling_cxt);

    return 0;
}
