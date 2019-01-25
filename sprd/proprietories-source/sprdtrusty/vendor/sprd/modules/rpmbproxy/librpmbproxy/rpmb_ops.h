/*
 * Copyright (C) 2017 spreadtrum.com
 *
 */
#pragma once

#include <stdint.h>

int rpmb_ops_open(const char *rpmb_devname);
void rpmb_ops_close(int mmc_handle);
int rpmb_ops_send(int mmc_handle,
        void *reliable_write_buf, size_t reliable_write_size,
        void *write_buf, size_t write_size,
        void *read_buf, size_t read_size);
