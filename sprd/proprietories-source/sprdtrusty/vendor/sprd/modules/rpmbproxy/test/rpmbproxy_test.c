/*
 * Copyright (C) 2017 spreadtrum.com
 */

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cutils/log.h>
#include <sys/time.h>

#include <rpmbproxyinterface.h>
#include "../librpmbproxy/rpmb_blk_rng.h"




static void print_buf(const char *prefix, const uint8_t *buf, size_t size)
{
    size_t i, j;

    fprintf(stderr, "%s", prefix);
    for (i = 0; i < size; i++) {
        if (i && i % 32 == 0) {
            fprintf(stderr,"\n");
            j = strlen(prefix);
            while (j--)
                fprintf(stderr, " ");
        }
        fprintf(stderr, " %02x", buf[i]);
    }
    fprintf(stderr,"\n");

}


int  main(void)
{
    uint8_t data_wr[MMC_BLOCK_SIZE];
    uint8_t data_rd[MMC_BLOCK_SIZE];
    uint16_t block_ind, block_count;
    int byte_cnt = 0;
    int ret;

    ret = init_rpmb();
    if (0 != ret) {
        fprintf(stderr, "init_rpmb error %d \n", ret);
        return ret;
    }

//clean rpmb
    memset(&data_wr, 0x0, sizeof(data_wr));
    uint16_t block_max_ind = RPMB_BLOCK_IND_END;  //the last block
    block_count = sizeof(data_wr) / RPMB_DATA_SIZE;
    for (uint16_t ind = RPMB_BLOCK_IND_START; ind <= block_max_ind; ind += block_count) {
        ret = rpmb_write(data_wr, ind, block_count);
        if (0 > ret) {
            fprintf(stderr, "clean rpmb block %d error\n", ind);
        }
    }
    fprintf(stderr, "clean rpmb block %d - %d \n", RPMB_BLOCK_IND_START, RPMB_BLOCK_IND_END);

/*
//write last block
    memset(&data_wr, 0x5a, sizeof(data_wr));
    block_ind = RPMB_BLOCK_IND_END;  //the last block
    block_count = sizeof(data_wr) / RPMB_DATA_SIZE;
    byte_cnt = rpmb_write(data_wr, block_ind, block_count);
    if (byte_cnt == block_count * RPMB_DATA_SIZE) {
        fprintf(stderr, "write rpmb blck %d, bytes %d success!\n", block_ind, byte_cnt);
    } else {
        fprintf(stderr, "write fail! return code %d \n", byte_cnt);
    }


//check write
    memset(&data_rd, 0x0, sizeof(data_rd));
    block_count = sizeof(data_rd) / RPMB_DATA_SIZE;
    block_ind = RPMB_BLOCK_IND_END;  //the last block
    byte_cnt = rpmb_read(data_rd, block_ind, block_count);
    if(byte_cnt > 0) {
        print_buf("last block:",data_rd,byte_cnt);
    } else {
        fprintf(stderr, "read fail! return code %d \n", byte_cnt);
    }
*/

    release_rpmb();

    return 0;

}
