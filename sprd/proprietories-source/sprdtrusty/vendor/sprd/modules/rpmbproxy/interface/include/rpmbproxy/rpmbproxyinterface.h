/*
 * Copyright (C) 2017 spreadtrum.com
 *
 */

#pragma once

#include <stdint.h>


__BEGIN_DECLS

/**
 * rpmb_write() - Writes to rpmb at a given block index.
 * @buf: the buffer containing the data to write
 * @blk_ind: the index of block to write
 * @blk_cnt: the count of block to write.
 *
 * Return: the number of bytes written on success, negative error code on failure
 */
ssize_t rpmb_write(const unsigned char *buf, int blk_ind, int blk_cnt);

/**
 * rpmb_read() - read data from rpmb at a given block index.
 * @mmc_handle: mmc handle
 * @buf: the buffer to save the data which read from rpmb
 * @blk_ind: the index of block to read
 * @blk_cnt: the count of block to read. MMC_RPMB_DATA_SIZE(256) MAX_RPMB_BLOCK_CNT(2)
 *
 * Return: the number of bytes read on success, negative error code on failure
 */
ssize_t rpmb_read(unsigned char *buf, int blk_ind, int blk_cnt);


/**
 * init_rpmbproxy() - init rpmb proxy.
 *
 * zero is success negative error code on failure
 */
int init_rpmb();

/**
 * release_rpmbproxy() - release rpmb proxy.
 * @mmc_handle: mmc handle
 */
void release_rpmb();



__END_DECLS
