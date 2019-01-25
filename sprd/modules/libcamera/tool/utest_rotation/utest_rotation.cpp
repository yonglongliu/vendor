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
#include <semaphore.h>
#include "sprd_ion.h"

#include "sprd_cpp.h"
#include "MemIon.h"

using namespace android;

#define UTEST_ROTATION_COUNTER 50
#define SAVE_ROTATION_OUTPUT_DATA 0
#define ERR(x...) fprintf(stderr, x)
#define INFO(x...) fprintf(stdout, x)

#define UTEST_ROTATION_EXIT_IF_ERR(n)                                          \
    do {                                                                       \
        if (n) {                                                               \
            ERR("utest rotate  error  . Line:%d ", __LINE__);                  \
            goto exit;                                                         \
        }                                                                      \
    } while (0)

enum file_number { NO_FILE = 0, ONE_FILE, TWO_FLIE, MAX_FILE_NUMBER };

struct utest_rotation_context {
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

    uint32_t width;
    uint32_t height;
    int angle;
    uint32_t format;
    enum file_number filenum;
};

static char utest_rotation_src_y_422_file[] = "/data/like/pic/src_y_422.raw";
static char utest_rotation_src_uv_422_file[] = "/data/like/pic/src_uv_422.raw";
static char utest_rotation_src_y_420_file[] = "/data/like/pic/src_y_420.raw";
static char utest_rotation_src_uv_420_file[] = "/data/like/pic/src_uv_420.raw";
static char utest_rotation_dst_y_file[] =
    "/data/like/pic/dst_y_%dx%d-angle%d-format%d_%d.raw";
static char utest_rotation_dst_uv_file[] =
    "/data/like/pic/dst_uv_%dx%d-angle%d-format%d_%d.raw";

static int utest_do_rotation(int fd, uint32_t width, uint32_t height,
                             uint32_t input_yaddr, uint32_t input_uvaddr,
                             uint32_t output_yaddr, uint32_t output_uvaddr,
                             unsigned int format, unsigned int angle) {
    int ret = 0;
    struct sprd_cpp_rot_cfg_parm rot_cfg;

    memset((void *)&rot_cfg, 0x00, sizeof(rot_cfg));
    rot_cfg.angle = angle;
    rot_cfg.format = format;
    rot_cfg.size.w = (uint16_t)width;
    rot_cfg.size.h = (uint16_t)height;
    rot_cfg.src_addr.y = input_yaddr;
    rot_cfg.src_addr.u = input_uvaddr;
    rot_cfg.src_addr.v = input_uvaddr;
    rot_cfg.dst_addr.y = output_yaddr;
    rot_cfg.dst_addr.u = output_uvaddr;
    rot_cfg.dst_addr.v = output_uvaddr;

    ret = ioctl(fd, SPRD_CPP_IO_START_ROT, &rot_cfg);
    if (ret) {
        ERR("utest_rotation camera_rotation fail. Line:%d", __LINE__);
        ret = -1;
    } else {
        ret = 0;
    }
    return ret;
}

static void usage(void) {
    INFO("Usage:\n");
    INFO("utest_rotation -if format -iw width -ih height -ia angle(90 180 270 "
         "-1) -icnt filenum(1 2)\n");
}

static unsigned int utest_rotation_angle_cvt(int angle) {
    unsigned int tmp_angle = 0;
    if (angle == 90) {
        tmp_angle = ROT_90;
    } else if (angle == 270) {
        tmp_angle = ROT_270;
    } else if (angle == 180) {
        tmp_angle = ROT_180;
    } else if (angle == -1) {
        tmp_angle = ROT_MIRROR;
    } else {
        ERR("utest rotate  error  . Line:%d ", __LINE__);
    }
    INFO("utest_rotation_angle_cvt %d \n", tmp_angle);
    return tmp_angle;
}

static int utest_rotation_param_set(
    struct utest_rotation_context *g_utest_rotation_cxt_ptr, int argc,
    char **argv) {
    int i = 0;
    struct utest_rotation_context *rotation_cxt_ptr = g_utest_rotation_cxt_ptr;

    if (argc < 9) {
        usage();
        return -1;
    }

    memset(rotation_cxt_ptr, 0, sizeof(utest_rotation_context));

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-if") == 0 && (i < argc - 1)) {
            if (strcmp(argv[i + 1], "yuv422") == 0)
                rotation_cxt_ptr->format = 0;
            else if (strcmp(argv[i + 1], "yuv420") == 0)
                rotation_cxt_ptr->format = 1;
            else
                ERR("error:picture format is error\n");

            i++;
        } else if (strcmp(argv[i], "-iw") == 0 && (i < argc - 1)) {
            rotation_cxt_ptr->width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-ih") == 0 && (i < argc - 1)) {
            rotation_cxt_ptr->height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-ia") == 0 && (i < argc - 1)) {
            rotation_cxt_ptr->angle = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-icnt") == 0 && (i < argc - 1)) {
            rotation_cxt_ptr->filenum = (enum file_number)(atoi(argv[++i]));
        } else {
            usage();
            return -1;
        }
    }

    return 0;
}

static int utest_mm_iommu_is_enabled(void) { return 0; }

static int utest_rotation_mem_alloc(
    struct utest_rotation_context *g_utest_rotation_cxt_ptr) {
    struct utest_rotation_context *rotation_cxt_ptr = g_utest_rotation_cxt_ptr;
    int s_mem_method = utest_mm_iommu_is_enabled();

    INFO("utest_rotation_mem_alloc %d\n", s_mem_method);
    /* alloc input y buffer */

    rotation_cxt_ptr->input_y_pmem_hp = new MemIon(
        "/dev/ion", rotation_cxt_ptr->width * rotation_cxt_ptr->height,
        MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);

    if (rotation_cxt_ptr->input_y_pmem_hp->getHeapID() < 0) {
        ERR("failed to alloc input_y pmem buffer.\n");
        return -1;
    }

    rotation_cxt_ptr->input_y_pmem_hp->get_phy_addr_from_ion(
        &rotation_cxt_ptr->input_y_physical_addr,
        &rotation_cxt_ptr->input_y_pmemory_size);

    rotation_cxt_ptr->input_y_virtual_addr =
        (unsigned char *)rotation_cxt_ptr->input_y_pmem_hp->getBase();

    ERR("ROT:src y :phy addr:0x%lx,vir addr:%p\n",
        rotation_cxt_ptr->input_y_physical_addr,
        rotation_cxt_ptr->input_y_virtual_addr);

    if (!rotation_cxt_ptr->input_y_physical_addr) {
        ERR("failed to alloc input_y pmem buffer:addr is null.\n");
        return -1;
    }
    memset(rotation_cxt_ptr->input_y_virtual_addr, 0x80,
           rotation_cxt_ptr->width * rotation_cxt_ptr->height);

    /* alloc input uv buffer */

    rotation_cxt_ptr->input_uv_pmem_hp = new MemIon(
        "/dev/ion", rotation_cxt_ptr->width * rotation_cxt_ptr->height,
        MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);

    if (rotation_cxt_ptr->input_uv_pmem_hp->getHeapID() < 0) {
        ERR("failed to alloc input_uv pmem buffer.\n");
        return -1;
    }

    rotation_cxt_ptr->input_uv_pmem_hp->get_phy_addr_from_ion(
        &rotation_cxt_ptr->input_uv_physical_addr,
        &rotation_cxt_ptr->input_uv_pmemory_size);

    rotation_cxt_ptr->input_uv_virtual_addr =
        (unsigned char *)rotation_cxt_ptr->input_uv_pmem_hp->getBase();

    ERR("ROT:src uv :phy addr:0x%lx,vir addr:%p\n",
        rotation_cxt_ptr->input_uv_physical_addr,
        rotation_cxt_ptr->input_uv_virtual_addr);

    if (!rotation_cxt_ptr->input_uv_physical_addr) {
        ERR("failed to alloc input_uv pmem buffer:addr is null.\n");
        return -1;
    }
    memset(rotation_cxt_ptr->input_uv_virtual_addr, 0x80,
           rotation_cxt_ptr->width * rotation_cxt_ptr->height);

    /* alloc outout y buffer */
    rotation_cxt_ptr->output_y_pmem_hp = new MemIon(
        "/dev/ion", rotation_cxt_ptr->width * rotation_cxt_ptr->height,
        MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);

    if (rotation_cxt_ptr->output_y_pmem_hp->getHeapID() < 0) {
        ERR("failed to alloc output_y pmem buffer.\n");
        return -1;
    }

    rotation_cxt_ptr->output_y_pmem_hp->get_phy_addr_from_ion(
        &rotation_cxt_ptr->output_y_physical_addr,
        &rotation_cxt_ptr->output_y_pmemory_size);

    rotation_cxt_ptr->output_y_virtual_addr =
        (unsigned char *)rotation_cxt_ptr->output_y_pmem_hp->getBase();

    ERR("ROT:dst y :phy addr:0x%lx,vir addr:%p\n",
        rotation_cxt_ptr->output_y_physical_addr,
        rotation_cxt_ptr->output_y_virtual_addr);
    if (!rotation_cxt_ptr->output_y_physical_addr) {
        ERR("failed to alloc output_y pmem buffer:addr is null.\n");
        return -1;
    }

    /* alloc outout uv buffer */
    rotation_cxt_ptr->output_uv_pmem_hp = new MemIon(
        "/dev/ion", rotation_cxt_ptr->width * rotation_cxt_ptr->height,
        MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);

    if (rotation_cxt_ptr->output_uv_pmem_hp->getHeapID() < 0) {
        ERR("failed to alloc output_uv pmem buffer.\n");
        return -1;
    }

    rotation_cxt_ptr->output_uv_pmem_hp->get_phy_addr_from_ion(
        &rotation_cxt_ptr->output_uv_physical_addr,
        &rotation_cxt_ptr->output_uv_pmemory_size);

    rotation_cxt_ptr->output_uv_virtual_addr =
        (unsigned char *)rotation_cxt_ptr->output_uv_pmem_hp->getBase();

    ERR("ROT:dst uv :phy addr:0x%lx,vir addr:%p\n",
        rotation_cxt_ptr->output_uv_physical_addr,
        rotation_cxt_ptr->output_uv_virtual_addr);
    if (!rotation_cxt_ptr->output_uv_physical_addr) {
        ERR("failed to alloc output_uv pmem buffer:addr is null.\n");
        return -1;
    }

    return 0;
}

static int utest_rotation_mem_release(
    struct utest_rotation_context *g_utest_rotation_cxt_ptr) {
    struct utest_rotation_context *rotation_cxt_ptr = g_utest_rotation_cxt_ptr;

    delete rotation_cxt_ptr->input_y_pmem_hp;

    delete rotation_cxt_ptr->input_uv_pmem_hp;

    delete rotation_cxt_ptr->output_y_pmem_hp;

    delete rotation_cxt_ptr->output_uv_pmem_hp;

    return 0;
}

static int utest_rotation_src_cfg(
    struct utest_rotation_context *g_utest_rotation_cxt_ptr) {
    FILE *fp = 0;
    struct utest_rotation_context *rotation_cxt_ptr = g_utest_rotation_cxt_ptr;

    if (rotation_cxt_ptr->format == 0)
        fp = fopen(utest_rotation_src_y_422_file, "r");
    else if (rotation_cxt_ptr->format == 1)
        fp = fopen(utest_rotation_src_y_420_file, "r");

    if (fp != NULL) {
        fread((void *)rotation_cxt_ptr->input_y_virtual_addr, 1,
              rotation_cxt_ptr->width * rotation_cxt_ptr->height, fp);
        fclose(fp);
    } else {
        ERR("utest_rotation_src_cfg fail : no input_y source file.\n");
        return -1;
    }

    if (rotation_cxt_ptr->format == 0)
        fp = fopen(utest_rotation_src_uv_422_file, "r");
    else if (rotation_cxt_ptr->format == 1)
        fp = fopen(utest_rotation_src_uv_420_file, "r");

    if (fp != NULL) {
        if (rotation_cxt_ptr->format == 0)
            fread((void *)rotation_cxt_ptr->input_uv_virtual_addr, 1,
                  rotation_cxt_ptr->width * rotation_cxt_ptr->height, fp);
        else if (rotation_cxt_ptr->format == 1)
            fread((void *)rotation_cxt_ptr->input_uv_virtual_addr, 1,
                  rotation_cxt_ptr->width * rotation_cxt_ptr->height / 2, fp);

        fclose(fp);
    } else {
        ERR("utest_rotation_src_cfg fail : no input_uv source file.\n");
        return -1;
    }

    return 0;
}

static int utest_rotation_save_raw_data(
    struct utest_rotation_context *g_utest_rotation_cxt_ptr, int angle) {

    FILE *fp = 0;
    char file_name[128] = "utest_rotation_output_temp.raw";
    struct utest_rotation_context *rotation_cxt_ptr = g_utest_rotation_cxt_ptr;

    sprintf(file_name, utest_rotation_dst_y_file, rotation_cxt_ptr->width,
            rotation_cxt_ptr->height, angle, rotation_cxt_ptr->format);
    fp = fopen(file_name, "wb");
    if (fp != NULL) {
        fwrite((void *)rotation_cxt_ptr->output_y_virtual_addr, 1,
               rotation_cxt_ptr->width * rotation_cxt_ptr->height, fp);
        fclose(fp);
    } else {
        ERR("utest_rotation_save_raw_data: failed to open save_file_y.\n");
        return -1;
    }

    sprintf(file_name, utest_rotation_dst_uv_file, rotation_cxt_ptr->width,
            rotation_cxt_ptr->height, angle, rotation_cxt_ptr->format);
    fp = fopen(file_name, "wb");
    if (fp != NULL) {
        if (rotation_cxt_ptr->format == 0)
            fwrite((void *)rotation_cxt_ptr->output_uv_virtual_addr, 1,
                   rotation_cxt_ptr->width * rotation_cxt_ptr->height, fp);
        else if (rotation_cxt_ptr->format == 1)
            fwrite((void *)rotation_cxt_ptr->output_uv_virtual_addr, 1,
                   rotation_cxt_ptr->width * rotation_cxt_ptr->height / 2, fp);
        fclose(fp);
    } else {
        ERR("utest_rotation_save_raw_data: failed to open save_file_uv.\n");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    int fd = 0;
    uint32_t i = 0;
    int64_t time_start = 0, time_end = 0;
    static struct utest_rotation_context utest_rotation_cxt;
    static struct utest_rotation_context *g_utest_rotation_cxt_ptr =
        &utest_rotation_cxt;
    struct utest_rotation_context *rotation_cxt_ptr = g_utest_rotation_cxt_ptr;
    unsigned int tmp_angle;

    if (utest_rotation_param_set(&utest_rotation_cxt, argc, argv))
        return -1;

    if (utest_rotation_mem_alloc(&utest_rotation_cxt))
        goto err;

    if (utest_rotation_src_cfg(&utest_rotation_cxt))
        goto err;

    for (i = 0; i < UTEST_ROTATION_COUNTER; i++) {

        fd = open("/dev/sprd_cpp", O_RDWR, 0);
        if (fd < 0) {
            ERR("open cpp device failed\n");
            goto err;
        }

        INFO("utest_rotation testing  start\n");
        time_start = systemTime();

        tmp_angle = utest_rotation_angle_cvt(rotation_cxt_ptr->angle);
        utest_do_rotation(fd, rotation_cxt_ptr->width, rotation_cxt_ptr->height,
                          rotation_cxt_ptr->input_y_physical_addr,
                          rotation_cxt_ptr->input_uv_physical_addr,
                          rotation_cxt_ptr->output_y_physical_addr,
                          rotation_cxt_ptr->output_uv_physical_addr,
                          rotation_cxt_ptr->format, tmp_angle);

        time_end = systemTime();
        INFO("utest_rotation testing  end time=%d, i = %x\n",
             (unsigned int)((time_end - time_start) / 1000000L), i);
        // usleep(30*1000);
        utest_rotation_save_raw_data(&utest_rotation_cxt,
                                     rotation_cxt_ptr->angle);

        close(fd);
    }

err:
    utest_rotation_mem_release(&utest_rotation_cxt);

    return 0;
}
