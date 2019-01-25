/*
 *Copyright 2014,2015 NXP Semiconductors
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
 

#include "tfa_dsp_fw.h"
#include "tfa_service.h"
#include "tfa_internal.h"

#include "tfa98xx_tfafieldnames.h"

static enum Tfa98xx_Error tfa9872_specific(Tfa98xx_handle_t handle)
{
	enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
	uint16_t MANAOOSC = 0x0140; /* version 17 */
	unsigned short value, xor;

	if (!tfa98xx_handle_is_open(handle))
		return Tfa98xx_Error_NotOpen;

	/* Unlock key 1 and 2 */
	error = reg_write(handle, 0x0F, 0x5A6B);
	error = reg_read(handle, 0xFB, &value);
	xor = value ^ 0x005A;
	error = reg_write(handle, 0xA0, xor);
	tfa98xx_key2(handle, 0); 

	/* ----- generated code start ----- */
	/* -----  version 25 ----- */
	tfa98xx_write_register16(handle, 0x00, 0x1801); //POR=0x0001
	tfa98xx_write_register16(handle, 0x02, 0x2dc8); //POR=0x2028
	tfa98xx_write_register16(handle, 0x20, 0x0890); //POR=0x2890
	tfa98xx_write_register16(handle, 0x22, 0x043c); //POR=0x045c
	tfa98xx_write_register16(handle, 0x51, 0x0000); //POR=0x0080
	tfa98xx_write_register16(handle, 0x52, 0x1a1c); //POR=0x7ae8
	tfa98xx_write_register16(handle, 0x58, 0x161c); //POR=0x101c
	tfa98xx_write_register16(handle, 0x61, 0x0198); //POR=0x0000
	tfa98xx_write_register16(handle, 0x65, 0x0a8b); //POR=0x0a9a
	tfa98xx_write_register16(handle, 0x70, 0x07f5); //POR=0x06e6
	tfa98xx_write_register16(handle, 0x74, 0xcc84); //POR=0xd823
	tfa98xx_write_register16(handle, 0x82, 0x01ed); //POR=0x000d
	tfa98xx_write_register16(handle, 0x83, 0x0014); //POR=0x0013
	tfa98xx_write_register16(handle, 0x84, 0x0021); //POR=0x0020
	tfa98xx_write_register16(handle, 0x85, 0x0001); //POR=0x0003
	/* ----- generated code end   ----- */

	/* Turn off the osc1m to save power: PLMA4928 */
	error = tfa_set_bf(handle, MANAOOSC, 1);

	return error;
}

static enum Tfa98xx_Error tfa9872_tfa_set_boost_trip_level(Tfa98xx_handle_t handle, int Re25C)
{
	enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
	uint16_t DCTRIP = 0x7444; /* version 17 */
	int trip_value;

	if(Re25C == 0) {
		pr_debug("\nWarning: No calibration values found: Boost trip level not adjusted! \n");
		return error;
	}

	/* Read trip level: The trip level is already set (if defined in cnt file) so we can just read the bitfield */
	trip_value = tfa_get_bf(handle, DCTRIP);
	pr_debug("\nCurrent calibration value is %d mOhm and the boost_trip_lvl is %d \n", Re25C, trip_value);

	if(Re25C > 0 && Re25C < 3700)
		trip_value = trip_value + 3;
	else if(Re25C >= 3700 && Re25C < 5000)
		trip_value = trip_value + 2;
	else if(Re25C >= 5000 && Re25C < 7000)
		trip_value = trip_value + 1;

	/* Set the boost trip level according to the new value */
	error = tfa_set_bf(handle, DCTRIP, (uint16_t)trip_value);
	pr_debug("New boost_trip_lvl is set to %d \n", trip_value);

	return error;
}

static enum Tfa98xx_Error tfa9872_tfa_adapt_boost_trigger_level(Tfa98xx_handle_t handle, int set_value)
{
	enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
	uint16_t DCTRIP = 0x7444; /* version 17 */

	/* Set the boost trip level according to the new value */
	error = tfa_set_bf(handle, DCTRIP, (uint16_t)set_value);

	return error;
}

/*
 * register device specifics functions
 */
void tfa9872_ops(struct tfa_device_ops *ops) {
	ops->tfa_init=tfa9872_specific;
	ops->tfa_set_boost_trip_level=tfa9872_tfa_set_boost_trip_level;
	ops->tfa_adapt_boost_trigger_level=tfa9872_tfa_adapt_boost_trigger_level;
}
