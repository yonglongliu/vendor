/*
 *Copyright 2015 NXP Semiconductors
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *
 *http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 */
#ifndef HAL_REGISTERS_H
#define HAL_REGISTERS_H

#include "stdint.h"

uint8_t get_bf_address(char *bf_name);

uint16_t read_register(uint8_t slave_address, uint8_t subaddress);
int write_register(uint8_t slave_address, uint8_t subaddress, uint16_t value);

int16_t read_register_bf(uint8_t slave_address, char *bf_name);
int write_register_bf(uint8_t slave_address, char *bf_name, uint16_t value);


#endif // HAL_REGISTERS_H
