#include <utils/Log.h>
#include "sensor.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

//#define OTP_AUTO_LOAD_LSC_SP8407

#define RG_TYPICAL_sp8407 0x011b // 0x0137
#define BG_TYPICAL_sp8407 0x0163 // 0x0139
#define R_TYPICAL_sp8407 0x0000
#define G_TYPICAL_sp8407 0x0000
#define B_TYPICAL_sp8407 0x0000
#define LSC_PARAM_QTY 240

struct otp_info_t {
    uint16_t flag;
    uint16_t module_id;
    uint16_t lens_id;
    uint16_t vcm_id;
    uint16_t vcm_driver_id;
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t rg_ratio_current;
    uint16_t bg_ratio_current;
    uint16_t rg_ratio_typical;
    uint16_t bg_ratio_typical;
    uint16_t r_current;
    uint16_t g_current;
    uint16_t b_current;
    uint16_t r_typical;
    uint16_t g_typical;
    uint16_t b_typical;
    uint16_t vcm_dac_start;
    uint16_t vcm_dac_inifity;
    uint16_t vcm_dac_macro;
    uint16_t lsc_param[LSC_PARAM_QTY];
};
static struct otp_info_t sp8407_otp_info;

static uint32_t sp8407_read_otp_info(cmr_handle handle, void *param_ptr) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    uint32_t rtn = SENSOR_SUCCESS;
    struct otp_info_t *otp_info =
        &sp8407_otp_info; //(struct otp_info_t *)param_ptr;
    otp_info->rg_ratio_typical = RG_TYPICAL_sp8407;
    otp_info->bg_ratio_typical = BG_TYPICAL_sp8407;
    otp_info->r_typical = R_TYPICAL_sp8407;
    otp_info->g_typical = G_TYPICAL_sp8407;
    otp_info->b_typical = B_TYPICAL_sp8407;

    /*TODO*/
    int otp_flag, addr, temp, i;
    // set 0x5001[3] to
    int temp1;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x01);
    temp1 = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x5001);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x5001,
                        (0x00 & 0x08) | (temp1 & (~0x08)));
    // read OTP into buffer
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3d84, 0xC0);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3d88,
                        0x70); // OTP start address
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3d89, 0x10);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3d8A,
                        0x72); // OTP end address
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3d8B, 0x0a);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3d81,
                        0x0a); // load otp into buffer
    usleep(10 * 1000);

    // OTP base information and WB calibration data
    otp_flag = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x7010);
    addr = 0;
    if ((otp_flag & 0xc0) == 0x40) {
        addr = 0x7011; // base address of info group 1
    } else if ((otp_flag & 0x30) == 0x10) {
        addr = 0x7019; // base address of info group 2
    }

    if (addr != 0) {
        otp_info->flag = 0xC0; // valid info and AWB in OTP
        otp_info->module_id = hw_sensor_read_reg(sns_drv_cxt->hw_handle, addr);
        otp_info->lens_id =
            hw_sensor_read_reg(sns_drv_cxt->hw_handle, addr + 1);
        otp_info->year = hw_sensor_read_reg(sns_drv_cxt->hw_handle, addr + 2);
        otp_info->month = hw_sensor_read_reg(sns_drv_cxt->hw_handle, addr + 3);
        otp_info->day = hw_sensor_read_reg(sns_drv_cxt->hw_handle, addr + 4);
        temp = hw_sensor_read_reg(sns_drv_cxt->hw_handle, addr + 7);
        otp_info->rg_ratio_current =
            (hw_sensor_read_reg(sns_drv_cxt->hw_handle, addr + 5) << 2) +
            ((temp >> 6) & 0x03);
        otp_info->bg_ratio_current =
            (hw_sensor_read_reg(sns_drv_cxt->hw_handle, addr + 6) << 2) +
            ((temp >> 4) & 0x03);
    } else {
        otp_info->flag = 0x00; // not info and AWB in OTP
        otp_info->module_id = 0;
        otp_info->lens_id = 0;
        otp_info->year = 0;
        otp_info->month = 0;
        otp_info->day = 0;
        otp_info->rg_ratio_current = 0;
        otp_info->bg_ratio_current = 0;
    }
#if 0
	// OTP VCM Calibration
	otp_flag = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x7021);
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7022; // base address of VCM Calibration group 1,0x22
	}else if((otp_flag & 0x30) == 0x10) {
		addr = 0x7025; // base address of VCM Calibration group 2,0x7025
	}
	if(addr != 0) {
		otp_info->flag |= 0x20;
		temp = hw_sensor_read_reg(sns_drv_cxt->hw_handle, addr + 2);
		otp_info->vcm_dac_start = (hw_sensor_read_reg(sns_drv_cxt->hw_handle, addr)<<2) | ((temp>>6) & 0x03);
		otp_info->vcm_dac_inifity=otp_info->vcm_dac_start;
		otp_info->vcm_dac_macro = (hw_sensor_read_reg(sns_drv_cxt->hw_handle, addr + 1) << 2) | ((temp>>4) & 0x03);
	}
	else {
		otp_info->vcm_dac_start = 0;
		otp_info->vcm_dac_inifity = 0;
		otp_info->vcm_dac_macro = 0;
	}
#endif
    // OTP Lenc Calibration
    otp_flag = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x7028);
    addr = 0;
    int checksum2 = 0;
    if ((otp_flag & 0xc0) == 0x40) {
        addr = 0x7029; // base address of Lenc Calibration group 1,
    } else if ((otp_flag & 0x30) == 0x10) {
        addr = 0x711a; // base address of Lenc Calibration group 2,
    }
    if (addr != 0) {
        for (i = 0; i < 240; i++) {
            otp_info->lsc_param[i] =
                hw_sensor_read_reg(sns_drv_cxt->hw_handle, addr + i);
            checksum2 += otp_info->lsc_param[i];
        }
        checksum2 = (checksum2) % 255 + 1;

        SENSOR_LOGI("checksum2=0x%x, %ld ", checksum2,
                    hw_sensor_read_reg(sns_drv_cxt->hw_handle, addr + 240));
        if (hw_sensor_read_reg(sns_drv_cxt->hw_handle, (addr + 240)) ==
            checksum2) {
            otp_info->flag |= 0x10;
        }
    } else {
        for (i = 0; i < 240; i++) {
            otp_info->lsc_param[i] = 0;
        }
    }
    char value[255];

    property_get("debug.camera.save.otp.raw.data", value, "0");
    FILE *fd = fopen("/data/misc/cameraserver/sp8407.otp.dump.bin", "wb+");
    for (i = 0x7010; i <= 0x720a; i++) {
        if (atoi(value) == 1) {
            cmr_u8 low_val = hw_sensor_read_reg(sns_drv_cxt->hw_handle, i);
            fwrite((char *)&low_val, 1, 1, fd);
        }
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, i,
                            0); // clear OTP buffer, recommended use continuous
                                // write to accelarate,0x720a
    }

    fclose(fd);
    // set 0x5001[3] to "1"
    temp1 = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x5001);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x5001,
                        (0x08 & 0x08) | (temp1 & (~0x08)));

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x00);

    /*print otp information*/
    // SENSOR_LOGI("___read otp info_________start\n");
    SENSOR_LOGI("flag=0x%x", otp_info->flag);
    SENSOR_LOGI("module_id=0x%x", otp_info->module_id);
    SENSOR_LOGI("lens_id=0x%x", otp_info->lens_id);
    SENSOR_LOGI("vcm_id=0x%x", otp_info->vcm_id);
    SENSOR_LOGI("vcm_id=0x%x", otp_info->vcm_id);
    SENSOR_LOGI("vcm_driver_id=0x%x", otp_info->vcm_driver_id);
    SENSOR_LOGI("data=%d-%d-%d", otp_info->year, otp_info->month,
                otp_info->day);
    SENSOR_LOGI("rg_ratio_current=0x%x", otp_info->rg_ratio_current);
    SENSOR_LOGI("bg_ratio_current=0x%x", otp_info->bg_ratio_current);
    SENSOR_LOGI("rg_ratio_typical=0x%x", otp_info->rg_ratio_typical);
    SENSOR_LOGI("bg_ratio_typical=0x%x", otp_info->bg_ratio_typical);
    SENSOR_LOGI("r_current=0x%x", otp_info->r_current);
    SENSOR_LOGI("g_current=0x%x", otp_info->g_current);
    SENSOR_LOGI("b_current=0x%x", otp_info->b_current);
    SENSOR_LOGI("r_typical=0x%x", otp_info->r_typical);
    SENSOR_LOGI("g_typical=0x%x", otp_info->g_typical);
    SENSOR_LOGI("b_typical=0x%x", otp_info->b_typical);
    // SENSOR_LOGI("vcm_dac_start=0x%x",otp_info->vcm_dac_start);
    // SENSOR_LOGI("vcm_dac_inifity=0x%x",otp_info->vcm_dac_inifity);
    // SENSOR_LOGI("vcm_dac_macro=0x%x",otp_info->vcm_dac_macro);
    // SENSOR_LOGI("___read otp info___________end\n");
    return rtn;
}

static uint32_t sp8407_update_awb(cmr_handle handle, void *param_ptr) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct otp_info_t *otp_info =
        &sp8407_otp_info; //(struct otp_info_t *)param_ptr;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    /*TODO*/
    int rg, bg, R_gain, G_gain, B_gain, Base_gain, temp, i;

    // apply OTP WB Calibration
    if (otp_info->flag & 0x40) {
        rg = otp_info->rg_ratio_current;
        bg = otp_info->bg_ratio_current;

        // calculate G gain
        R_gain = (otp_info->rg_ratio_typical * 1000) / rg;
        B_gain = (otp_info->bg_ratio_typical * 1000) / bg;
        G_gain = 1000;

        if (R_gain < 1000 || B_gain < 1000) {
            if (R_gain < B_gain)
                Base_gain = R_gain;
            else
                Base_gain = B_gain;
        } else {
            Base_gain = G_gain;
        }
        R_gain = 0x400 * R_gain / (Base_gain);
        B_gain = 0x400 * B_gain / (Base_gain);
        G_gain = 0x400 * G_gain / (Base_gain);

        SENSOR_LOGI("r_Gain=0x%x\n", R_gain);
        SENSOR_LOGI("g_Gain=0x%x\n", G_gain);
        SENSOR_LOGI("b_Gain=0x%x\n", B_gain);

        // update sensor WB gain
        if (R_gain > 0x400) {
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x5019, R_gain >> 8);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x501a,
                                R_gain & 0x00ff);
        }
        if (G_gain > 0x400) {
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x501b, G_gain >> 8);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x501c,
                                G_gain & 0x00ff);
        }
        if (B_gain > 0x400) {
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x501d, B_gain >> 8);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x501e,
                                B_gain & 0x00ff);
        }
    }

    return rtn;
}

#ifndef OTP_AUTO_LOAD_LSC_SP8407

static uint32_t sp8407_update_lsc(cmr_handle handle, void *param_ptr) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct otp_info_t *otp_info =
        &sp8407_otp_info; //(struct otp_info_t *)param_ptr;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    /*TODO*/
    int i = 0, temp = 0;
    if (otp_info->flag & 0x10) {
        SENSOR_LOGI("apply otp lsc \n");
        temp = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x5000);
        temp = 0x20 | temp;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x5000, temp);
        for (i = 0; i < 240; i++) {
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x5900 + i,
                                otp_info->lsc_param[i]);
        }
    }

    return rtn;
}

#endif
static uint32_t sp8407_update_otp(cmr_handle handle, void *param_ptr) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct otp_info_t *otp_info =
        &sp8407_otp_info; //(struct otp_info_t *)param_ptr;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    rtn = sp8407_update_awb(handle, param_ptr);
    if (rtn != SENSOR_SUCCESS) {
        SENSOR_LOGI("OTP awb appliy error!");
        return rtn;
    }

#ifndef OTP_AUTO_LOAD_LSC_SP8407

    rtn = sp8407_update_lsc(handle, param_ptr);
    if (rtn != SENSOR_SUCCESS) {
        SENSOR_LOGI("OTP lsc appliy error!");
        return rtn;
    }
#endif

    return rtn;
}
static uint32_t sp8407_read_otp(cmr_handle handle, void *param_ptr) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    sp8407_read_otp_info(handle, param_ptr);
    rtn = sp8407_update_otp(handle, param_ptr);
    SENSOR_LOGI("rtn=%d", rtn);
    return rtn;
}
