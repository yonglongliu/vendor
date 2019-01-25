/*
 * Copyright 2016, WATCHDATA
 */

#ifndef _wd_SPI_H_
#define _wd_SPI_H_

#include "sw_types.h"

struct spi_device {
	uint32_t		max_speed_hz;
	uint8_t			chip_select;
	uint8_t			mode;
	uint8_t			bits_per_word;
};

int test_bus_driver(void);
int wd_spi_open(void);
int wd_spi_close(void);
int wd_spi_read(uint32_t spi_num,void *pbuf,size_t count,uint8_t size);
int wd_spi_write(uint32_t spi_num,void *pbuf,size_t count,uint8_t size);
int wd_spi_setup(uint32_t spi_num,struct spi_device *spi);
int wd_spi_get_dev(uint32_t spi_num,struct spi_device *spi);
int wd_spi_rd_wr(uint32_t spi_num,void *rbuf,void *wbuf,size_t count,uint8_t size);

#endif 
