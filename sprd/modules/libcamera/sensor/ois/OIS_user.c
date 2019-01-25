/////////////////////////////////////////////////////////////////////////////
// File Name	: OIS_user.c
// Function		: User defined function.
// 				  These functions depend on user's circumstance.
//
// Rule         : Use TAB 4
//
// Copyright(c)	Rohm Co.,Ltd. All rights reserved
//
/***** ROHM Confidential ***************************************************/
#ifndef OIS_USER_C
#define OIS_USER_C
#endif

#include "OIS_head.h"
//#include "usb_func.h"
//#include "winbase.h"
#include "sensor_drv_u.h"

// Following Variables that depend on user's environment
// RHM_HT
// 2013.03.13	add
OIS_UWORD FOCUS_VAL = 0x0122; // Focus Value

// /////////////////////////////////////////////////////////
// VCOSET function
// ---------------------------------------------------------
// <Function>
//		To use external clock at CLK/PS, it need to set PLL.
//		After enabling PLL, more than 30ms wait time is required to
//change
// clock source.
//		So the below sequence has to be used:
// 		Input CLK/PS --> Call VCOSET0 --> Download Program/Coed --> Call
// VCOSET1
//
// <Input>
//		none
//
// <Output>
//		none
//
// =========================================================
void VCOSET0(SENSOR_HW_HANDLE handle) {

    OIS_UWORD CLK_PS = 24000; // Input Frequency [kHz] of CLK/PS terminal
                              // (Depend on your system)
    OIS_UWORD FVCO_1 = 27000; // Target Frequency [kHz]
    OIS_UWORD FREF = 25;      // Reference Clock Frequency [kHz]

    OIS_UWORD DIV_N = CLK_PS / FREF - 1; // calc DIV_N
    OIS_UWORD DIV_M = FVCO_1 / FREF - 1; // calc DIV_M

    I2C_OIS_per_write(0x62, DIV_N);  // Divider for internal reference clock
    I2C_OIS_per_write(0x63, DIV_M);  // Divider for internal PLL clock
    I2C_OIS_per_write(0x64, 0x4060); // Loop Filter

    I2C_OIS_per_write(0x60, 0x3011); // PLL
    I2C_OIS_per_write(0x65, 0x0080); //
    I2C_OIS_per_write(0x61, 0x8002); // VCOON
    I2C_OIS_per_write(0x61, 0x8003); // Circuit ON
    I2C_OIS_per_write(0x61, 0x8809); // PLL ON
}

void VCOSET1(SENSOR_HW_HANDLE handle) {
    //
    //     OIS_UWORD 	CLK_PS = 23880;
    //     // Input Frequency [kHz] of CLK/PS terminal (Depend on your system)
    //     RHM_HT 2013.05.09	Change 12M -> 6.75M
    //     OIS_UWORD 	FVCO_1 = 27000;
    //     // Target Frequency [kHz]
    //     OIS_UWORD 	FREF   = 25;
    //     // Reference Clock Frequency [kHz]
    //
    //     OIS_UWORD	DIV_N  = CLK_PS / FREF - 1; //
    //     calc DIV_N
    //     OIS_UWORD	DIV_M  = FVCO_1 / FREF - 1; //
    //     calc DIV_M
    //
    //     I2C_OIS_per_write( 0x62, DIV_N  );
    //     // Divider for internal reference clock
    //     I2C_OIS_per_write( 0x63, DIV_M  );
    //     // Divider for internal PLL clock
    //     I2C_OIS_per_write( 0x64, 0x4060 );
    //     // Loop Filter
    //
    //     I2C_OIS_per_write( 0x60, 0x3011 );
    //     // PLL
    //     I2C_OIS_per_write( 0x65, 0x0080 );
    //     //
    //     I2C_OIS_per_write( 0x61, 0x8002 );
    //     // VCOON
    //     I2C_OIS_per_write( 0x61, 0x8003 );
    //     // Circuit ON
    //     I2C_OIS_per_write( 0x61, 0x8809 );
    //     // PLL ON
    //
    //     Wait( 30 );
    //     // Wait for PLL lock

    I2C_OIS_per_write(0x05, 0x000C); // Prepare for PLL clock as master clock
    I2C_OIS_per_write(0x05, 0x000D); // Change to PLL clock
}

// struct msm_actuator_ctrl_t *g_i2c_ctrl;
/**
 *
 *
 */
void WR_I2C(SENSOR_HW_HANDLE handle, OIS_UBYTE slvadr, OIS_UBYTE size,
            OIS_UBYTE *dat) {

    if (4 == size) {
        OIS_UWORD addr = dat[0] << 8 | dat[1];
        OIS_UWORD data_uw = (dat[2] << 8) | dat[3];

        sensor_grc_write_i2c(slvadr, addr, data_uw, BITS_ADDR16_REG16);
        // SENSOR_LOGI("WR_I2C addr:0x%x ,data_uw:0x%x", addr, data_uw);
    }
}

OIS_UWORD RD_I2C(SENSOR_HW_HANDLE handle, OIS_UBYTE slvadr, OIS_UBYTE size,
                 OIS_UBYTE *dat) {
    OIS_UWORD read_data = 0;
    OIS_UWORD addr;

    if (2 == size) {
        addr = dat[0] << 8 | dat[1];
        read_data = sensor_grc_read_i2c(slvadr, addr, BITS_ADDR16_REG16);
    } else if (1 == size) {
        addr = dat[0];
        read_data = sensor_grc_read_i2c(slvadr, addr, BITS_ADDR8_REG8);
    }

    return read_data;
}

// *********************************************************
// Write Factory Adjusted data to the non-volatile memory
// ---------------------------------------------------------
// <Function>
//		Factory adjusted data are sotred somewhere
//		non-volatile memory.
//
// <Input>
//		_FACT_ADJ	Factory Adjusted data
//
// <Output>
//		none
//
// <Description>
//		You have to port your own system.
//
// *********************************************************
void store_FADJ_MEM_to_non_volatile_memory(SENSOR_HW_HANDLE handle,
                                           _FACT_ADJ param) {
    /* 	Write to the non-vollatile memory such as EEPROM or internal of the
     * CMOS sensor... */
}

// *********************************************************
// Read Factory Adjusted data from the non-volatile memory
// ---------------------------------------------------------
// <Function>
//		Factory adjusted data are sotred somewhere
//		non-volatile memory.  I2C master has to read these
//		data and store the data to the OIS controller.
//
// <Input>
//		none
//
// <Output>
//		_FACT_ADJ	Factory Adjusted data
//
// <Description>
//		You have to port your own system.
//
// *********************************************************

_FACT_ADJ get_FADJ_MEM_from_non_volatile_memory(SENSOR_HW_HANDLE handle) {
#if 1 // for module EEPROM correction data
      /* 	Read from the non-vollatile memory such as EEPROM or internal of the
       * CMOS sensor... */
    OIS_UWORD i;
    OIS_UBYTE read_data[38];
    OIS_UBYTE flag = 0;

    // flag=sensor_grc_read_i2c(0x50, 0x00, BITS_ADDR16_REG8);
    // SENSOR_LOGI("EEPROM 0x00:flag=%x", flag);
    // flag=sensor_grc_read_i2c(0x50, 0x01, BITS_ADDR16_REG8);
    // SENSOR_LOGI("EEPROM 0x01:flag=%x", flag);
    for (i = 0x2; i <= 0x27; i++) {
        read_data[i - 2] = sensor_grc_read_i2c(0x50, i, BITS_ADDR16_REG8);
    }

    FADJ_MEM.gl_CURDAT = read_data[0] | (read_data[1] << 8);
    FADJ_MEM.gl_HALOFS_X = read_data[2] | (read_data[3] << 8);
    FADJ_MEM.gl_HALOFS_Y = read_data[4] | (read_data[5] << 8);
    FADJ_MEM.gl_HX_OFS = read_data[6] | (read_data[7] << 8);
    FADJ_MEM.gl_HY_OFS = read_data[8] | (read_data[9] << 8);
    FADJ_MEM.gl_PSTXOF = read_data[10] | (read_data[11] << 8);
    FADJ_MEM.gl_PSTYOF = read_data[12] | (read_data[13] << 8);
    FADJ_MEM.gl_GX_OFS = read_data[14] | (read_data[15] << 8);
    FADJ_MEM.gl_GY_OFS = read_data[16] | (read_data[17] << 8);
    FADJ_MEM.gl_KgxHG = read_data[18] | (read_data[19] << 8);
    FADJ_MEM.gl_KgyHG = read_data[20] | (read_data[21] << 8);
    FADJ_MEM.gl_KGXG = read_data[22] | (read_data[23] << 8);
    FADJ_MEM.gl_KGYG = read_data[24] | (read_data[25] << 8);
    FADJ_MEM.gl_SFTHAL_X = read_data[26] | (read_data[27] << 8);
    FADJ_MEM.gl_SFTHAL_Y = read_data[28] | (read_data[29] << 8);
    FADJ_MEM.gl_TMP_X_ = read_data[30] | (read_data[31] << 8);
    FADJ_MEM.gl_TMP_Y_ = read_data[32] | (read_data[33] << 8);
    FADJ_MEM.gl_KgxH0 = read_data[34] | (read_data[35] << 8);
    FADJ_MEM.gl_KgyH0 = read_data[36] | (read_data[37] << 8);
    return FADJ_MEM;
#else
    return FADJ_MEM; // Note: This return data is for DEBUG.
#endif
}

// ==> RHM_HT 2013/04/15	Add for DEBUG
// *********************************************************
// Printf for DEBUG
// ---------------------------------------------------------
// <Function>
//
// <Input>
//		const char *format, ...
// 				Same as printf
//
// <Output>
//		none
//
// <Description>
//
// *********************************************************
int debug_print(const char *format, ...) { return 0; }
