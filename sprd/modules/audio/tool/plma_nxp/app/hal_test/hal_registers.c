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
#include <stdint.h>
#include <NXP_I2C.h>

extern unsigned short tfatype;

NXP_I2C_Error_t hal_last_error = NXP_I2C_Ok;

#define TFATYPE tfatype

/*
 * return register from bitfield
 */
uint8_t get_bf_address(char *bf_name) {
	uint16_t bf = tfaContBfEnum(bf_name, TFATYPE);
	return (bf>>8)&0xff;
}
/*
 * tfa_ functions copied from tfa_dsp.c
 */
static uint16_t tfa_get_bf_value(const uint16_t bf, const uint16_t reg_value)
{
	uint16_t msk, value;

	/*
	 * bitfield enum:
	 * - 0..3  : len
	 * - 4..7  : pos
	 * - 8..15 : address
	 */
	uint8_t len = bf & 0x0f;
	uint8_t pos = (bf >> 4) & 0x0f;

	msk = ((1<<(len+1))-1)<<pos;
	value = (reg_value & msk) >> pos;

	return value;
}
static int tfa_set_bf_value(const uint16_t bf, const uint16_t bf_value, uint16_t *p_reg_value)
{
	uint16_t regvalue, msk;

	/*
	 * bitfield enum:
	 * - 0..3  : len
	 * - 4..7  : pos
	 * - 8..15 : address
	 */
	uint8_t len = bf & 0x0f;
	uint8_t pos = (bf >> 4) & 0x0f;

	regvalue = *p_reg_value;

	msk = ((1<<(len+1))-1)<<pos;
	regvalue &= ~msk;
	regvalue |= bf_value<<pos;

	*p_reg_value = regvalue;

	return 0;
}


uint16_t read_register(uint8_t slave_address, uint8_t subaddress)
{
uint8_t write_data[1]; /* subaddress */
uint8_t read_buffer[2]; /* 2 data bytes */
uint16_t value;

    write_data[0] = subaddress;

    hal_last_error = NXP_I2C_WriteRead(slave_address<<1,
    sizeof(write_data), write_data, sizeof(read_buffer), read_buffer);
    value = (read_buffer[0] << 8) + read_buffer[1];

return value;
}

int write_register(uint8_t slave_address, uint8_t subaddress, uint16_t value)
{
uint8_t write_data[1+2]; /* subaddress */

    write_data[0] = subaddress;
    write_data[1] = value >> 8;
    write_data[2] = value ;

    hal_last_error = NXP_I2C_WriteRead(slave_address<<1,
              sizeof(write_data), write_data, 0, 0);

    return  NXP_I2C_Ok != hal_last_error;
}
uint16_t read_register_bf(uint8_t slave_address, char *bf_name){
uint16_t bitfield,reg;//, value;

    bitfield = tfaContBfEnum(bf_name, TFATYPE);
    if ( bitfield == 0xffff) {
    	hal_last_error = NXP_I2C_UnsupportedValue;
    	return 0xdead;
    }
    reg =  read_register(slave_address, (bitfield >> 8) & 0xff);

    return tfa_get_bf_value(bitfield, reg);
}
int write_register_bf(uint8_t slave_address, char *bf_name, uint16_t value){
uint16_t bitfield, regvalue, oldvalue, msk;
uint8_t len, pos, address;

    bitfield = tfaContBfEnum(bf_name, TFATYPE);
    if ( bitfield == 0xffff) {
    	hal_last_error = NXP_I2C_UnsupportedValue;
    	return 1;
    }
    //tfa_set_bf_value(bitfield, value, &regvalue);
    /*
     * bitfield enum:
     * - 0..3  : len-1
     * - 4..7  : pos
     * - 8..15 : address
     */
    len = (bitfield & 0x0f) +1;
    pos = (bitfield >> 4) & 0x0f;
    address = (bitfield >> 8) & 0xff;

    regvalue = read_register(slave_address, address);
    if (NXP_I2C_Ok != hal_last_error)
    	return 0xdead;

    oldvalue = regvalue;
	msk = ((1<<len)-1)<<pos;
	regvalue &= ~msk;
	regvalue |= value<<pos;

    /* Only write when the current register value is not the same as the new value */
    if(oldvalue != regvalue)
    	return write_register(slave_address, (bitfield >> 8) & 0xff, regvalue);
    return 0;
}
