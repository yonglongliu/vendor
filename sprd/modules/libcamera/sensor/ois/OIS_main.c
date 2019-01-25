/////////////////////////////////////////////////////////////////////////////
// File Name	: OIS_main.c
// Function		: Main control function runnning on ISP.
//                ( But Just for example )
// Rule         : Use TAB 4
//
// Copyright(c)	Rohm Co.,Ltd. All rights reserved
//
/***** ROHM Confidential ***************************************************/
#ifndef OIS_MAIN_C
#define OIS_MAIN_C
#endif

//#include <kernel/stdio.h>
#include "OIS_head.h"
//#include <linux/kernel.h>
#include "OIS_main.h"

#define POSE_UP_HORIZONTAL 0
#define POSE_DOWN_HORIZONTAL 0
// GLOBAL variable ( Upper Level Host Set this Global variables )
////////////////////////////////////////////////////////////////////////////////
OIS_UWORD BOOT_MODE = _FACTORY_; // Execute Factory Adjust or not
// This main routine ( below ) polling this variable.
#define AF_REQ 0x8000
#define SCENE_REQ_ON 0x4000
#define SCENE_REQ_OFF 0x2000
#define POWERDOWN 0x1000
#define INITIAL_VAL 0x0000
OIS_UWORD OIS_REQUEST = INITIAL_VAL; // OIS control register.

OIS_UWORD OIS_SCENE = _SCENE_D_A_Y_1; // OIS Scene parameter.
// Upper Level Host can change the SCENE parameter.
// ==> RHM_HT 2013.03.04	Change type (OIS_UWORD -> double)
double OIS_PIXEL[2]; // Just Only use for factory adjustment.
// <== RHM_HT 2013.03.04
ADJ_STS OIS_MAIN_STS = ADJ_ERR; // Status register of this main routine.
                                // RHM_HT 2013/04/15	Change "typedef" and
                                // initial value.

static unsigned char ois_mode = 0;
static unsigned char pre_ois_mode = 0;

// MAIN
////////////////////////////////////////////////////////////////////////////////

int ois_main(SENSOR_HW_HANDLE handle) {

    _FACT_ADJ fadj;
    OIS_UWORD ret;

    // Factory Adjustment data
    SENSOR_LOGI("=================ois_main===============\n");
    //------------------------------------------------------
    // Get Factory adjusted data
    //------------------------------------------------------

    fadj = get_FADJ_MEM_from_non_volatile_memory(
        handle); // Initialize by Factory adjusted value.

    //------------------------------------------------------
    // Source Power and Negate the Power save pin
    //------------------------------------------------------
    SENSOR_LOGI("hope_call POWER_UP_AND_PS_DISABLE\n");
    // POWER_UP_AND_PS_DISABLE( );// Depend on your system

    //------------------------------------------------------
    // PLL setting to use external CLK
    //------------------------------------------------------
    VCOSET0(handle);

    //------------------------------------------------------
    // Download Program and Coefficient
    //------------------------------------------------------
    SENSOR_LOGI("hope_call func_PROGRAM_DOWNLOAD\n");
    OIS_MAIN_STS = func_PROGRAM_DOWNLOAD(handle); // Program Download
    // if ( OIS_MAIN_STS <= ADJ_ERR ) return OIS_MAIN_STS;

    SENSOR_LOGI("hope_call func_COEF_DOWNLOAD\n");
    func_COEF_DOWNLOAD(handle, 0); // Download Coefficient

    //------------------------------------------------------
    // Change Clock to external pin CLK_PS
    //------------------------------------------------------
    VCOSET1(handle);
    SET_FADJ_PARAM(handle, &FADJ_MEM);

    //------------------------------------------------------
    // Issue DSP start command.
    //------------------------------------------------------
    I2C_OIS_spcl_cmnd(handle, 1, _cmd_8C_EI); // DSP calculation START

// ret = I2C_OIS_mem__read( _M_OIS_STS );
// SENSOR_LOGI(" read mem 0x84f7 :: %x", ret);   //0X0104
//------------------------------------------------------
// Set calibration data
//------------------------------------------------------
#if 0
		SENSOR_LOGI("gl_CURDAT = 0x%x\n", FADJ_MEM.gl_CURDAT);
		SENSOR_LOGI("gl_HALOFS_X = 0x%x\n", FADJ_MEM.gl_HALOFS_X);
		SENSOR_LOGI("gl_HALOFS_Y = 0x%x\n", FADJ_MEM.gl_HALOFS_Y);
		SENSOR_LOGI("gl_HX_OFS = 0x%x\n", FADJ_MEM.gl_HX_OFS);
		SENSOR_LOGI("gl_HY_OFS = 0x%x\n", FADJ_MEM.gl_HY_OFS);
		SENSOR_LOGI("gl_PSTXOF = 0x%x\n", FADJ_MEM.gl_PSTXOF);
		SENSOR_LOGI("gl_PSTYOF = 0x%x\n", FADJ_MEM.gl_PSTYOF);
		SENSOR_LOGI("gl_GX_OFS = 0x%x\n", FADJ_MEM.gl_GX_OFS);
		SENSOR_LOGI("gl_GY_OFS = 0x%x\n", FADJ_MEM.gl_GY_OFS);
		SENSOR_LOGI("gl_KgxHG  = 0x%x\n", FADJ_MEM.gl_KgxHG);
		SENSOR_LOGI("gl_KgyHG  = 0x%x\n", FADJ_MEM.gl_KgyHG);
		SENSOR_LOGI("gl_KGXG   = 0x%x\n", FADJ_MEM.gl_KGXG);
		SENSOR_LOGI("gl_KGYG   = 0x%x\n", FADJ_MEM.gl_KGYG);
		SENSOR_LOGI("gl_SFTHAL_X = 0x%x\n", FADJ_MEM.gl_SFTHAL_X);
		SENSOR_LOGI("gl_SFTHAL_Y = 0x%x\n", FADJ_MEM.gl_SFTHAL_Y);
		SENSOR_LOGI("gl_TMP_X_ = 0x%x\n", FADJ_MEM.gl_TMP_X_);		// RHM_HT 2013/05/23	Added
		SENSOR_LOGI("gl_TMP_Y_ = 0x%x\n", FADJ_MEM.gl_TMP_Y_);		// RHM_HT 2013/05/23	Added
		SENSOR_LOGI("gl_KgxH0 = 0x%x\n", FADJ_MEM.gl_KgxH0); 	// RHM_HT 2013/05/23	Added
		SENSOR_LOGI("gl_KgyH0 = 0x%x\n", FADJ_MEM.gl_KgyH0); 	// RHM_HT 2013/05/23	Added
#endif
    //------------------------------------------------------
    // Set default AF dac and scene parameter for OIS
    //------------------------------------------------------
    // I2C_OIS_F0123_wr_( 0x90,0x00, 0x0130 );					// AF Control ( Value
    // is
    // example )
    // func_SET_SCENE_PARAM_for_NewGYRO_Fil( _SCENE_TEST___, 1, 0, 0, &fadj );
    // // Set default SCENE ( Just example )

    func_SET_SCENE_PARAM(handle, _SCENE_SPORT_3, 1, 0, 0,
                         &FADJ_MEM); // Set default SCENE ( Just example )
    ois_mode = _SCENE_SPORT_3;
    // ret = I2C_OIS_mem__read( _M_EQCTL );
    // SENSOR_LOGI(" read mem 0x847f :: %x", ret);  // ois open :0x0d0d close
    // 0x0c0c

    return ADJ_OK;
}

/*
*name:  ois_pre_open
*function: check for OIS I2C work
*
*/

uint32_t ois_pre_open(SENSOR_HW_HANDLE handle) {
    OIS_UWORD ret;

    SENSOR_LOGI("ois_pre_open()\n");
    ret = I2C_OIS_per__read(handle, 0);
    if (0x735 != ret) {
        SENSOR_LOGI(" [hope] is 0x735   err ret =%x\n", ret); // 0x0735
        return ADJ_ERR;
    }
    SENSOR_LOGI(" [hope] is 0x735    ret =%x\n", ret); // 0x0735

    I2C_OIS_per_write(0x20, 0x1234);
    ret = I2C_OIS_per__read(handle, 0x20);

    if (0x1234 != ret) {
        SENSOR_LOGI(" [hope] is 0x1234   err ret =%x\n", ret); // 0x1234
        return ADJ_ERR;
    }
    SENSOR_LOGI("[hope] is 0x1234 ret =%x\n", ret); // 0x1234

    return ADJ_OK;
}

uint32_t OpenOIS(SENSOR_HW_HANDLE handle) {
    SENSOR_LOGI("OpenOIS");
    ois_main(handle);
    return 0;
}

uint32_t CloseOIS(SENSOR_HW_HANDLE handle) {
    OIS_UWORD u16_dat;

    SENSOR_LOGI("CloseOIS");
    u16_dat = I2C_OIS_mem__read(_M_EQCTL);
    u16_dat = (u16_dat & 0xFEFE);
    I2C_OIS_mem_write(_M_EQCTL, u16_dat);
    SENSOR_LOGI("SET : EQCTL:%.4x", u16_dat);
    ois_mode = 0;
    return 0;
}

uint32_t SetOisMode(SENSOR_HW_HANDLE handle, unsigned char mode) {

    SENSOR_LOGI(" SetOisMode %d\n", mode);
    pre_ois_mode = ois_mode;
    ois_mode = mode;

    if (ois_mode == pre_ois_mode) {
        SENSOR_LOGI(" mode not change %d", mode);
        return 0;
    }
    if (0 == mode)
        CloseOIS(handle);

    func_SET_SCENE_PARAM(handle, mode, 1, 0, 0, &FADJ_MEM);

    return 0;
}

unsigned char GetOisMode(SENSOR_HW_HANDLE handle) { return ois_mode; }
// lens
unsigned short OisLensRead(SENSOR_HW_HANDLE handle, unsigned short cmd) {
    cmd = I2C_OIS_F0123__rd(handle);
    SENSOR_LOGI("OisLensRead = %x", cmd);
    return cmd;
}

uint32_t OisLensWrite(SENSOR_HW_HANDLE handle, unsigned short cmd) {
    I2C_OIS_F0123_wr_(handle, 0x90, 0x00, cmd);
    SENSOR_LOGI("OisLensWrite %x", cmd);
    return 0;
}

uint32_t OIS_write_af(SENSOR_HW_HANDLE handle, uint32_t param) {
    OIS_UWORD target_code = param & 0x3FF;

    SENSOR_LOGI("write target_code = %d", target_code);
    I2C_OIS_F0123_wr_(handle, 0x90, 0x00, target_code);
    // target_code=I2C_OIS_F0123__rd();
    // SENSOR_LOGI("read target_code = %x",target_code);
    return 0;
}

uint32_t Ois_get_pose_dis(SENSOR_HW_HANDLE handle, uint32_t *up2h,
                          uint32_t *h2down) {
    *up2h = POSE_UP_HORIZONTAL;
    *h2down = POSE_DOWN_HORIZONTAL;

    return 0;
}
