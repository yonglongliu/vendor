/*
 * Copyright 2016, WATCHDATA
 */

#ifndef _wd_GPIO_H_
#define _wd_GPIO_H_

#include "sw_types.h"

int wd_gpio_open(void);
int wd_gpio_close(void);
int wd_gpio_request(uint32_t offset);
int wd_gpio_free(uint32_t offset);
int wd_gpio_direction_output(uint32_t offset,uint32_t value);
#endif 
