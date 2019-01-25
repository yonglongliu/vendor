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

static enum Tfa98xx_Error tfa9912_specific(Tfa98xx_handle_t handle) {
	enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
	unsigned short value, xor;

	if (!tfa98xx_handle_is_open(handle))
		return Tfa98xx_Error_NotOpen;

	if ((handles_local[handle].rev & 0xff) != 0x13) {
		pr_err("This code is not for this device type: %x\n", handles_local[handle].rev);
		return Tfa98xx_Error_Bad_Parameter;
	}

	/* Unlock keys to write settings */
	error = reg_write(handle, 0x0F, 0x5A6B);
	error = reg_read(handle, 0xFB, &value);
	xor = value ^ 0x005A;
	error = reg_write(handle, 0xA0, xor);

	/* The optimal settings */
	if (handles_local[handle].rev == 0x1a13) {
		/* -----  version 1.25 ----- */
		tfa98xx_write_register16(handle, 0x00, 0x1245); //POR=0x0245
		tfa98xx_write_register16(handle, 0x01, 0x838a); //POR=0x83ca
		tfa98xx_write_register16(handle, 0x02, 0x2dc8); //POR=0x2828
		tfa98xx_write_register16(handle, 0x20, 0x089e); //POR=0x0890
		tfa98xx_write_register16(handle, 0x22, 0x543c); //POR=0x545c
		tfa98xx_write_register16(handle, 0x23, 0x0007); //POR=0x0006
		tfa98xx_write_register16(handle, 0x24, 0x0014); //POR=0x0000
		tfa98xx_write_register16(handle, 0x26, 0x0110); //POR=0x0010
		tfa98xx_write_register16(handle, 0x51, 0x0000); //POR=0x0080
		tfa98xx_write_register16(handle, 0x65, 0x0a9a); //POR=0x0a8b
	}

	return error;
}

static enum Tfa98xx_Error tfa9912_tfa_dsp_write_tables(Tfa98xx_handle_t handle, int sample_rate)
{
	unsigned char buffer[15] = {0};
	int size = 15 * sizeof(char);

	/* Write the fractional delay in the hardware register 'cs_frac_delay' */
	switch(sample_rate) {
		case 0:	/* 8kHz */
			TFA_SET_BF(handle, FRACTDEL, 40);
			break;
		case 1:	/* 11.025KHz */
			TFA_SET_BF(handle, FRACTDEL, 38);
			break;
		case 2:	/* 12kHz */
			TFA_SET_BF(handle, FRACTDEL, 37);
			break;
		case 3:	/* 16kHz */
			TFA_SET_BF(handle, FRACTDEL, 59);
			break;
		case 4:	/* 22.05KHz */
			TFA_SET_BF(handle, FRACTDEL, 56);
			break;
		case 5:	/* 24kHz */
			TFA_SET_BF(handle, FRACTDEL, 56);
			break;
		case 6:	/* 32kHz */
			TFA_SET_BF(handle, FRACTDEL, 52);
			break;
		case 7:	/* 44.1kHz */
			TFA_SET_BF(handle, FRACTDEL, 48);
			break;
		case 8:
		default:/* 48kHz */
			TFA_SET_BF(handle, FRACTDEL, 46);
			break;
	}

	/* First copy the msg_id to the buffer */
        buffer[0] = (uint8_t) 0;
        buffer[1] = (uint8_t) MODULE_FRAMEWORK + 128;
        buffer[2] = (uint8_t) FW_PAR_ID_SET_SENSES_DELAY;

        /* Required for all FS exept 8kHz (8kHz is all zero) */
	if(sample_rate != 0) {
		buffer[5] = 1;	/* Vdelay_P */
		buffer[8] = 0;	/* Idelay_P */
		buffer[11] = 1; /* Vdelay_S */
		buffer[14] = 0; /* Idelay_S */
	}

	/* send SetSensesDelay msg */
	return dsp_msg(handle, size, (char *)buffer);
}

/*
 * register device specifics functions
 */
void tfa9912_ops(struct tfa_device_ops *ops) {
	ops->tfa_init = tfa9912_specific;
	ops->tfa_dsp_write_tables=tfa9912_tfa_dsp_write_tables;
}

