/*
 * Copyright (C) 2017 spreadtrum.com
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/major.h>
#include <linux/mmc/ioctl.h>

#include "log.h"
#include "rpmb.h"
#include "rpmb_ops.h"


#ifdef RPMB_DEBUG

static void print_buf(const char *prefix, const uint8_t *buf, size_t size)
{
    size_t i;

    printf("%s @%p [%zu]", prefix, buf, size);
    for (i = 0; i < size; i++) {
        if (i && i % 32 == 0)
            printf("\n%*s", (int) strlen(prefix), "");
        printf(" %02x", buf[i]);
    }
    printf("\n");
    fflush(stdout);
}

#endif


int rpmb_ops_send(int mmc_handle,
        void *reliable_write_buf, size_t reliable_write_size,
        void *write_buf, size_t write_size,
        void *read_buf, size_t read_size)
{
    int rc;
    struct {
        struct mmc_ioc_multi_cmd multi;
        struct mmc_ioc_cmd cmd_buf[3];
    } mmc = {};
    struct mmc_ioc_cmd *cmd = mmc.multi.cmds;

    memset(&mmc, 0, sizeof(mmc));
    if (reliable_write_size) {
        if ((reliable_write_size % MMC_BLOCK_SIZE) != 0 ||
             (NULL == reliable_write_buf)) {
            ALOGE("invalid reliable write size %u\n", reliable_write_size);
            return -EINVAL;
        }

        cmd->write_flag = MMC_WRITE_FLAG_RELW;
        cmd->opcode = MMC_WRITE_MULTIPLE_BLOCK;
        cmd->flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
        cmd->blksz = MMC_BLOCK_SIZE;
        cmd->blocks = reliable_write_size / MMC_BLOCK_SIZE;
        mmc_ioc_cmd_set_data((*cmd), reliable_write_buf);
#ifdef RPMB_DEBUG
        ALOGI("opcode: 0x%x, write_flag: 0x%x\n", cmd->opcode, cmd->write_flag);
        print_buf("request: ", reliable_write_buf, reliable_write_size);
#endif
        mmc.multi.num_of_cmds++;
        cmd++;
    }

    if (write_size) {
        if ((write_size % MMC_BLOCK_SIZE) != 0 || NULL == write_buf) {
            ALOGE("invalid write size %u\n", write_size);
            return -EINVAL;
        }

        cmd->write_flag = MMC_WRITE_FLAG_W;
        cmd->opcode = MMC_WRITE_MULTIPLE_BLOCK;
        cmd->flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
        cmd->blksz = MMC_BLOCK_SIZE;
        cmd->blocks = write_size / MMC_BLOCK_SIZE;
        mmc_ioc_cmd_set_data((*cmd), write_buf);
#ifdef RPMB_DEBUG
        ALOGI("opcode: 0x%x, write_flag: 0x%x\n", cmd->opcode, cmd->write_flag);
        print_buf("request: ", write_buf, write_size);
#endif
        mmc.multi.num_of_cmds++;
        cmd++;
    }

    if (read_size) {
        if (read_size % MMC_BLOCK_SIZE != 0 || NULL == read_buf) {
            ALOGE("%s: invalid read size %u\n", __func__, read_size);
            return -EINVAL;
        }

        cmd->write_flag = MMC_WRITE_FLAG_R;
        cmd->opcode = MMC_READ_MULTIPLE_BLOCK;
        cmd->flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC,
        cmd->blksz = MMC_BLOCK_SIZE;
        cmd->blocks = read_size / MMC_BLOCK_SIZE;
        mmc_ioc_cmd_set_data((*cmd), read_buf);
#ifdef RPMB_DEBUG
        ALOGI("opcode: 0x%x, write_flag: 0x%x\n", cmd->opcode, cmd->write_flag);
#endif
        mmc.multi.num_of_cmds++;
        cmd++;
    }

    rc = ioctl(mmc_handle, MMC_IOC_MULTI_CMD, &mmc.multi);
    if (rc < 0) {
        ALOGE("%s: mmc ioctl failed: %d, %s\n", __func__, rc, strerror(errno));
        return -EIO;
    }
#ifdef RPMB_DEBUG
    if (read_size)
        print_buf("response: ", read_buf, read_size);
#endif

    return 0;
}


int rpmb_ops_open(const char *rpmb_devname)
{
    int rc;

    rc = open(rpmb_devname, O_RDWR, 0);
    if (rc < 0) {
        ALOGE("unable (%d) to open rpmb device '%s': %s\n",
              errno, rpmb_devname, strerror(errno));
    }
    return rc;
}

void rpmb_ops_close(int mmc_handle)
{
    close(mmc_handle);
}
