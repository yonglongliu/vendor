/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "sns_cfg"

#include "sensor_drv_u.h"
#include "sensor_cfg.h"
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

// gc area
#ifdef GC0310
extern SENSOR_INFO_T g_GC0310_MIPI_yuv_info;
#endif
#ifdef GC2165
extern SENSOR_INFO_T g_gc2165_mipi_yuv_info;
#endif
#ifdef GC2375
extern SENSOR_INFO_T g_gc2375_mipi_raw_info;
#endif
#ifdef GC2375A
extern SENSOR_INFO_T g_gc2375a_mipi_raw_info;
#endif
#ifdef GC5005
extern SENSOR_INFO_T g_gc5005_mipi_raw_info;
#endif
#ifdef GC5024
extern SENSOR_INFO_T g_gc5024_mipi_raw_info;
#endif
#ifdef GC8024
extern SENSOR_INFO_T g_gc8024_mipi_raw_info;
#endif
#ifdef GC030A
extern SENSOR_INFO_T g_gc030a_mipi_raw_info;
#endif
#ifdef GC030A_F
extern SENSOR_INFO_T g_gc030af_mipi_raw_info;
#endif
#ifdef GC033A_CXT_F
extern SENSOR_INFO_T g_gc033af_cxt_mipi_raw_info;
#endif
#ifdef GC2385
extern SENSOR_INFO_T g_gc2385_mipi_raw_info;
#endif
#ifdef GC2385_CXT
extern SENSOR_INFO_T g_gc2385_cxt_mipi_raw_info;
#endif

// ov area
#ifdef OV2680
extern SENSOR_INFO_T g_ov2680_mipi_raw_info;
#endif
#ifdef OV2680_SBS
extern SENSOR_INFO_T g_ov2680_sbs_mipi_raw_info;
#endif
#ifdef OV5675
extern SENSOR_INFO_T g_ov5675_mipi_raw_info;
#endif
#ifdef OV5675_DUAL
extern SENSOR_INFO_T g_ov5675_dual_mipi_raw_info;
#endif
#ifdef OV8856_SHINE
extern SENSOR_INFO_T g_ov8856_shine_mipi_raw_info;
#endif
#ifdef OV8856
extern SENSOR_INFO_T g_ov8856_mipi_raw_info;
#endif
#ifdef OV8858
extern SENSOR_INFO_T g_ov8858_mipi_raw_info;
#endif
#ifdef OV13855
extern SENSOR_INFO_T g_ov13855_mipi_raw_info;
#endif
#ifdef OV13855_SUNNY
extern SENSOR_INFO_T g_ov13855_sunny_mipi_raw_info;
#endif
#ifdef OV13855A
extern SENSOR_INFO_T g_ov13855a_mipi_raw_info;
#endif
#ifdef OV13850R2A
extern SENSOR_INFO_T g_ov13850r2a_mipi_raw_info;
#endif

// imx 258
#ifdef IMX135
extern SENSOR_INFO_T g_imx135_mipi_raw_info;
#endif
#ifdef IMX230
extern SENSOR_INFO_T g_imx230_mipi_raw_info;
#endif
#ifdef IMX258
extern SENSOR_INFO_T g_imx258_mipi_raw_info;
#endif

// cista area
#ifdef C2390
extern SENSOR_INFO_T g_c2390_mipi_raw_info;
#endif
#ifdef C2580
extern SENSOR_INFO_T g_c2580_mipi_raw_info;
#endif

// sp area
#ifdef SP0A09
extern SENSOR_INFO_T g_sp0a09_mipi_raw_info;
#endif
#ifdef SP2509
extern SENSOR_INFO_T g_sp2509_mipi_raw_info;
#endif
#ifdef SP2509R
extern SENSOR_INFO_T g_sp2509r_mipi_raw_info;
#endif
#ifdef SP2509V
extern SENSOR_INFO_T g_sp2509v_mipi_raw_info;
#endif
#ifdef SP8407
extern SENSOR_INFO_T g_sp8407_mipi_raw_info;
#endif
#ifdef SP0A09_F
extern SENSOR_INFO_T g_sp0a09f_mipi_raw_info;
#endif
#ifdef SP0A09_CMK_F
extern SENSOR_INFO_T g_sp0a09f_cmk_mipi_raw_info;
#endif
#ifdef SP0A09_HLT_F
extern SENSOR_INFO_T g_sp0a09f_hlt_mipi_raw_info;
#endif
#ifdef SP0A09_ZRT_F
extern SENSOR_INFO_T g_sp0a09f_zrt_mipi_raw_info;
#endif
#ifdef SP0A09_CXT_F
extern SENSOR_INFO_T g_sp0a09f_cxt_mipi_raw_info;
#endif
#ifdef SP250A
extern SENSOR_INFO_T g_sp250a_mipi_raw_info;
#endif
#ifdef SP250A_CMK
extern SENSOR_INFO_T g_sp250a_cmk_mipi_raw_info;
#endif
#ifdef SP250A_HLT
extern SENSOR_INFO_T g_sp250a_hlt_mipi_raw_info;
#endif
#ifdef SP250A_ZRT
extern SENSOR_INFO_T g_sp250a_zrt_mipi_raw_info;
#endif
#ifdef SP250A_CXT
extern SENSOR_INFO_T g_sp250a_cxt_mipi_raw_info;
#endif
//samsung area
#ifdef S5K3L8XXM3
extern SENSOR_INFO_T g_s5k3l8xxm3_mipi_raw_info;
#endif
#ifdef S5K3L8XXM3Q
extern SENSOR_INFO_T g_s5k3l8xxm3q_mipi_raw_info;
#endif
#ifdef S5K3L8XXM3R
extern SENSOR_INFO_T g_s5k3l8xxm3r_mipi_raw_info;
#endif
#ifdef S5K3P8SM
extern SENSOR_INFO_T g_s5k3p8sm_mipi_raw_info;
#endif
#ifdef S5K4H8YX
extern SENSOR_INFO_T g_s5k4h8yx_mipi_raw_info;
#endif
#ifdef S5K5E2YA
extern SENSOR_INFO_T g_s5k5e2ya_mipi_raw_info;
#endif
#ifdef S5K5E8YX
extern SENSOR_INFO_T g_s5k5e8yx_mipi_raw_info;
#endif


extern otp_drv_entry_t imx258_drv_entry;
extern otp_drv_entry_t ov13855_drv_entry;
extern otp_drv_entry_t ov13855_sunny_drv_entry;
extern otp_drv_entry_t ov5675_sunny_drv_entry;
extern otp_drv_entry_t imx258_truly_drv_entry;
extern otp_drv_entry_t ov13855_altek_drv_entry;
extern otp_drv_entry_t s5k3l8xxm3_qtech_drv_entry;
extern otp_drv_entry_t s5k3p8sm_truly_drv_entry;
extern otp_drv_entry_t gc5024_common_drv_entry;
extern otp_drv_entry_t s5k3l8xxm3_reachtech_drv_entry;
extern otp_drv_entry_t gc2375_altek_drv_entry;
extern otp_drv_entry_t ov8856_cmk_drv_entry;
extern otp_drv_entry_t ov8858_cmk_drv_entry;
extern otp_drv_entry_t ov2680_cmk_drv_entry;
extern otp_drv_entry_t sp8407_otp_entry;
extern otp_drv_entry_t sp8407_cmk_otp_entry;
extern otp_drv_entry_t ov8856_shine_otp_entry;
extern otp_drv_entry_t s5k5e8yx_jd_otp_entry;

extern struct sns_af_drv_entry dw9800_drv_entry;
extern struct sns_af_drv_entry dw9714_drv_entry;
extern struct sns_af_drv_entry dw9714a_drv_entry;
extern struct sns_af_drv_entry dw9714p_drv_entry;
extern struct sns_af_drv_entry dw9718s_drv_entry;
extern struct sns_af_drv_entry bu64297gwz_drv_entry;
extern struct sns_af_drv_entry vcm_ak7371_drv_entry;
extern struct sns_af_drv_entry lc898214_drv_entry;
extern struct sns_af_drv_entry dw9763_drv_entry;
extern struct sns_af_drv_entry dw9763a_drv_entry;
extern struct sns_af_drv_entry vcm_zc524_drv_entry;
extern struct sns_af_drv_entry ad5823_drv_entry;
extern struct sns_af_drv_entry vm242_drv_entry;
extern struct sns_af_drv_entry dw9763r_drv_entry;
extern struct sns_af_drv_entry ces6301_drv_entry;

/**
 * NOTE: the interface can only be used by sensor ic.
 *       not for otp and vcm device.
 **/
const cmr_u8 camera_module_name_str[MODULE_MAX][20] = {
        [0] = "default",
        [MODULE_SUNNY] = "Sunny",
        [MODULE_TRULY] = "Truly",
        [MODULE_RTECH] = "ReachTech",
        [MODULE_QTECH] = "Qtech",
        [MODULE_ALTEK] = "Altek",/*5*/
        [MODULE_CMK] = "CameraKing",
        [MODULE_SHINE] = "Shine",
        [MODULE_DARLING] = "Darling",
        [MODULE_BROAD] = "Broad",
        [MODULE_DMEGC] = "DMEGC",/*10*/
        [MODULE_SEASONS] = "Seasons",
        [MODULE_SUNWIN] = " Sunwin",
        [MODULE_OFLIM] = "Oflim",
        [MODULE_HONGSHI] = "Hongshi",
        [MODULE_SUNNINESS] = "Sunniness", /*15*/
        [MODULE_RIYONG] = "Riyong",
        [MODULE_TONGJU] = "Tongju",
        [MODULE_A_KERR] = "A-kerr",
        [MODULE_LITEARRAY] = "LiteArray",
        [MODULE_HUAQUAN] = "Huaquan",
        [MODULE_KINGCOM] = "Kingcom",
        [MODULE_BOOYI] = "Booyi",
        [MODULE_LAIMU] = "Laimu",
        [MODULE_WDSEN] = "Wdsen",
        [MODULE_SUNRISE] = "Sunrise",

    /*add you camera module name following*/
};

/*---------------------------------------------------------------------------*
 **                         Constant Variables                                *
 **---------------------------------------------------------------------------*/

//{.sn_name = "imx258_mipi_raw", .sensor_info =  &g_imx258_mipi_raw_info,
// .af_dev_info = {.af_drv_entry = &dw9800_drv_entry, .af_work_mode = 0},
// &imx258_drv_entry},

const SENSOR_MATCH_T back_sensor_infor_tab[] = {
// gc area
#ifdef GC5005
    {MODULE_SUNNY, "gc5005", &g_gc5005_mipi_raw_info, {&dw9714_drv_entry, 0}, NULL},
#endif
#ifdef GC8024
    {MODULE_SUNNY, "gc8024", &g_gc8024_mipi_raw_info, {&dw9714_drv_entry, 0}, NULL},
#endif
#ifdef GC030A
    {MODULE_SUNNY, "gc030a", &g_gc030a_mipi_raw_info, {NULL, 0}, NULL},
#endif

#ifdef GC2385_CXT
    {MODULE_SUNNY, "gc2385_cxt", &g_gc2385_cxt_mipi_raw_info, {NULL, 0}, NULL},
#endif
 
// ov area
#ifdef OV8856_SHINE
    {MODULE_SUNNY, "ov8856_shine", &g_ov8856_shine_mipi_raw_info, {&dw9714p_drv_entry, 0}, &ov8856_shine_otp_entry},
#endif
#ifdef OV8856
    {MODULE_SUNNY, "ov8856", &g_ov8856_mipi_raw_info, {&dw9763a_drv_entry, 0}, &ov8856_cmk_drv_entry},
#endif
#ifdef OV8858
    {MODULE_SUNNY, "ov8858", &g_ov8858_mipi_raw_info, {&dw9763a_drv_entry, 0}, &ov8858_cmk_drv_entry},
#endif
#ifdef OV13855
#if defined(CONFIG_DUAL_MODULE)
    {MODULE_SUNNY, "ov13855", &g_ov13855_mipi_raw_info, {&vcm_zc524_drv_entry, 0}, &ov13855_sunny_drv_entry},
#else
    {MODULE_SUNNY, "ov13855", &g_ov13855_mipi_raw_info, {&dw9718s_drv_entry, 0}, &ov13855_drv_entry},
#endif
#endif
#ifdef OV13855_SUNNY
    {MODULE_SUNNY, "ov13855_sunny", &g_ov13855_sunny_mipi_raw_info, {&vcm_zc524_drv_entry, 0}, &ov13855_sunny_drv_entry},
#endif
#ifdef OV13855A
    {MODULE_SUNNY, "ov13855a", &g_ov13855a_mipi_raw_info, {&bu64297gwz_drv_entry, 0}, &ov13855_altek_drv_entry},
#endif

// imx area
#ifdef IMX258
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
    {MODULE_TRULY, "imx258", &g_imx258_mipi_raw_info, {&dw9800_drv_entry, 0}, &imx258_drv_entry},
#elif defined(CONFIG_CAMERA_ISP_DIR_3)
#ifdef CAMERA_SENSOR_BACK_I2C_SWITCH
    {MODULE_DARLING, "imx258", &g_imx258_mipi_raw_info, {&dw9763_drv_entry, 0}, NULL},
#else
    {MODULE_SUNNY ,"imx258", &g_imx258_mipi_raw_info, {&lc898214_drv_entry, 0}, &imx258_truly_drv_entry},
#endif
#endif
#endif
#ifdef IMX135
    {MODULE_SUNNY, "imx135", &g_imx135_mipi_raw_info, {&ad5823_drv_entry, 0}, NULL},
#endif
#ifdef IMX230
    {MODULE_SUNNY ,"imx230", &g_imx230_mipi_raw_info, {&dw9800_drv_entry, 0}, NULL},
#endif
// cista area
#ifdef C2390
    {MODULE_SUNNY, "c2390", &g_c2390_mipi_raw_info, {NULL, 0}, NULL},
#endif

// sp area
#ifdef SP2509
    {MODULE_SUNNY, "sp2509", &g_sp2509_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef SP250A_CMK
    {MODULE_SUNNY, "sp250a_cmk", &g_sp250a_cmk_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef SP250A_HLT
    {MODULE_SUNNY, "sp250a_hlt", &g_sp250a_hlt_mipi_raw_info, {NULL, 0}, NULL},
#endif
 
#ifdef SP250A_CXT
    {MODULE_SUNNY, "sp250a_cxt", &g_sp250a_cxt_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef SP250A_ZRT
    {MODULE_SUNNY, "sp250a_zrt", &g_sp250a_zrt_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef SP250A
    {MODULE_SUNNY, "sp250a", &g_sp250a_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef GC2385
    {MODULE_SUNNY, "gc2385", &g_gc2385_mipi_raw_info, {NULL, 0}, NULL},
#endif

#ifdef SP8407
#ifdef SBS_SENSOR_FRONT
    {MODULE_SUNNY, "sp8407", &g_sp8407_mipi_raw_info, {&dw9763_drv_entry, 0}, &sp8407_cmk_otp_entry},
#else
    {MODULE_SUNNY, "sp8407", &g_sp8407_mipi_raw_info, {&dw9763a_drv_entry, 0}, &sp8407_otp_entry},
#endif
#endif

// samsung area
#ifdef S5K3L8XXM3R
#if defined(CONFIG_DUAL_MODULE)
    {MODULE_SUNNY, "s5k3l8xxm3r", &g_s5k3l8xxm3r_mipi_raw_info, {&vm242_drv_entry, 0}, NULL},
#else
    {MODULE_SUNNY, "s5k3l8xxm3r", &g_s5k3l8xxm3r_mipi_raw_info, {&dw9763r_drv_entry, 0}, &s5k3l8xxm3_reachtech_drv_entry},
#endif
#endif
#ifdef S5K3L8XXM3
    {MODULE_SUNNY ,"s5k3l8xxm3", &g_s5k3l8xxm3_mipi_raw_info, {&vcm_ak7371_drv_entry, 0}, NULL},
#endif
#ifdef S5K3P8SM
    {MODULE_SUNNY ,"s5k3p8sm", &g_s5k3p8sm_mipi_raw_info, {&bu64297gwz_drv_entry, 0}, &s5k3p8sm_truly_drv_entry},
#endif
#ifdef S5K5E8YX
    {MODULE_SUNNY ,"s5k5e8yx", &g_s5k5e8yx_mipi_raw_info, {&ces6301_drv_entry, 0}, &s5k5e8yx_jd_otp_entry},
#endif

    {0, "0", NULL, {NULL, 0}, NULL}};

const SENSOR_MATCH_T front_sensor_infor_tab[] = {
// gc area
#ifdef GC033A_CXT_F
    {MODULE_SUNNY, "gc033af_cxt", &g_gc033af_cxt_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef GC2375
    {MODULE_SUNNY, "gc2375", &g_gc2375_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef GC5005
    {MODULE_SUNNY, "gc5005", &g_gc5005_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef GC5024
    {MODULE_SUNNY, "gc5024", &g_gc5024_mipi_raw_info, {NULL, 0}, &gc5024_common_drv_entry},
#endif

//ov area
#ifdef OV5675
    {MODULE_DARLING, "ov5675", &g_ov5675_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef OV8856
    {MODULE_SUNNY, "ov8856", &g_ov8856_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef OV8858
    {MODULE_SUNNY, "ov8858", &g_ov8858_mipi_raw_info, {&dw9763a_drv_entry, 0}, &ov8858_cmk_drv_entry},
#endif
#ifdef OV13850R2A
    {MODULE_SUNNY,  "ov13850r2a", &g_ov13850r2a_mipi_raw_info, {&dw9714a_drv_entry, 0}, NULL},
#endif

// sp area
#ifdef SP0A09
    {MODULE_SUNNY, "sp0a09", &g_sp0a09_mipi_raw_info, {NULL, 0}, NULL},
#endif


#ifdef SP0A09_CMK_F
    {MODULE_SUNNY, "sp0a09f_cmk", &g_sp0a09f_cmk_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef SP0A09_HLT_F
    {MODULE_SUNNY, "sp0a09f_hlt", &g_sp0a09f_hlt_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef SP0A09_CXT_F
    {MODULE_SUNNY, "sp0a09f_cxt", &g_sp0a09f_cxt_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef SP0A09_ZRT_F
    {MODULE_SUNNY, "sp0a09f_zrt", &g_sp0a09f_zrt_mipi_raw_info, {NULL, 0}, NULL},
#endif

#ifdef SP0A09_F
    {MODULE_SUNNY, "sp0a09f", &g_sp0a09f_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef GC030A_F
    {MODULE_SUNNY, "gc030af", &g_gc030af_mipi_raw_info, {NULL, 0}, NULL},
#endif

#ifdef SP8407
#ifdef CONFIG_FRONT_CAMERA_AUTOFOCUS
    {MODULE_SUNNY, "sp8407", &g_sp8407_mipi_raw_info, {&dw9763_drv_entry, 0}, &sp8407_cmk_otp_entry},
#else
    {MODULE_SUNNY, "sp8407", &g_sp8407_mipi_raw_info, {NULL, 0}, NULL},
#endif
#endif

// samsung area
#ifdef S5K3L8XXM3Q
    {MODULE_QTECH, "s5k3l8xxm3q", &g_s5k3l8xxm3q_mipi_raw_info, {NULL, 0}, &s5k3l8xxm3_qtech_drv_entry},
#endif
#ifdef S5K4H8YX
    {MODULE_SUNNY, "s5k4h8yx", &g_s5k4h8yx_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef S5K5E2YA
    {MODULE_SUNNY, "s5k5e2ya", &g_s5k5e2ya_mipi_raw_info, {&dw9714_drv_entry, 0}, NULL},
#endif

    {0, "0", NULL, {NULL, 0}, NULL}};

const SENSOR_MATCH_T back_ext_sensor_infor_tab[] = {
// ov area
#ifdef OV2680
    {MODULE_SUNNY, "ov2680", &g_ov2680_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef OV2680_SBS
    {MODULE_SUNNY, "ov2680_sbs", &g_ov2680_sbs_mipi_raw_info, {NULL, 0}, &ov2680_cmk_drv_entry},
#endif
#ifdef OV5675_DUAL
    {MODULE_SUNNY, "ov5675_dual", &g_ov5675_dual_mipi_raw_info, {NULL, 0}, &ov5675_sunny_drv_entry},
#endif

// gc area
#ifdef GC2165
    {MODULE_SUNNY, "gc2165", &g_gc2165_mipi_yuv_info, {NULL, 0}, NULL},
#endif
#ifdef GC2375A
    {MODULE_SUNNY, "gc2375a", &g_gc2375a_mipi_raw_info, {NULL, 0}, &gc2375_altek_drv_entry},
#endif

// cista area
#ifdef C2580
    {MODULE_SUNNY, "c2580", &g_c2580_mipi_raw_info, {NULL, 0}, NULL},
#endif

// sp area
#ifdef SP2509V
    {MODULE_SUNNY, "sp2509v", &g_sp2509v_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef SP2509R
    {MODULE_SUNNY, "sp2509r", &g_sp2509r_mipi_raw_info, {NULL, 0}, NULL},
#endif

    {0, "0", NULL, {NULL, 0}, NULL}};

const SENSOR_MATCH_T front_ext_sensor_infor_tab[] = {
// ov area
#ifdef OV2680_SBS
    {MODULE_SUNNY, "ov2680_sbs", &g_ov2680_sbs_mipi_raw_info, {NULL, 0}, &ov2680_cmk_drv_entry},
#endif

// cista area
#ifdef C2580
    {MODULE_SUNNY, "c2580", &g_c2580_mipi_raw_info, {NULL, 0}, NULL},
#endif

#ifdef SC_FPGA
//"ov5640_mipi_raw", &g_ov5640_mipi_raw_info
#endif

    {0, "0", NULL, {NULL, 0}, NULL}};

/*
* add for auto test for main and sub camera (raw yuv)
* 2014-02-07 freed wang end
*/
// SENSOR_MATCH_T *Sensor_GetInforTab(struct sensor_drv_context *sensor_cxt,
// SENSOR_ID_E sensor_id) {
SENSOR_MATCH_T *sensor_get_module_tab(cmr_int at_flag, cmr_u32 sensor_id) {
    SENSOR_MATCH_T *sensor_infor_tab_ptr = NULL;
    cmr_u32 index = 0;

    switch (sensor_id) {
    case SENSOR_MAIN:
        sensor_infor_tab_ptr = (SENSOR_MATCH_T *)&back_sensor_infor_tab;
        break;
    case SENSOR_SUB:
        sensor_infor_tab_ptr = (SENSOR_MATCH_T *)&front_sensor_infor_tab;
        break;
    case SENSOR_DEVICE2:
        sensor_infor_tab_ptr = (SENSOR_MATCH_T *)&back_ext_sensor_infor_tab;
        break;
    case SENSOR_DEVICE3:
        sensor_infor_tab_ptr = (SENSOR_MATCH_T *)&front_ext_sensor_infor_tab;
        break;
    default:
        break;
    }
    return (SENSOR_MATCH_T *)sensor_infor_tab_ptr;
}

// Sensor_GetInforTabLenght
cmr_u32 sensor_get_tab_length(cmr_int at_flag, cmr_u32 sensor_id) {
    cmr_u32 tab_lenght = 0;

    switch (sensor_id) {
    case SENSOR_MAIN:
        tab_lenght = ARRAY_SIZE(back_sensor_infor_tab);
        break;
    case SENSOR_SUB:
        tab_lenght = ARRAY_SIZE(front_sensor_infor_tab);
        break;
    case SENSOR_DEVICE2:
        tab_lenght = ARRAY_SIZE(back_ext_sensor_infor_tab);
        break;
    case SENSOR_DEVICE3:
        tab_lenght = ARRAY_SIZE(front_ext_sensor_infor_tab);
        break;
    default:
        break;
    }
    SENSOR_LOGI("module table length:%d", tab_lenght);
    return tab_lenght;
}

cmr_u32 sensor_get_match_index(cmr_int at_flag, cmr_u32 sensor_id) {
    cmr_u32 i = 0;
    cmr_u32 retValue = 0xFF;
    cmr_u32 mSnNum = 0;
    cmr_u32 sSnNum = 0;

    if (sensor_id == SENSOR_MAIN) {
        mSnNum = ARRAY_SIZE(back_sensor_infor_tab) - 1;
        SENSOR_LOGI("sensor sensorTypeMatch main is %d", mSnNum);
        for (i = 0; i < mSnNum; i++) {
            if (strcmp(back_sensor_infor_tab[i].sn_name,
                       CAMERA_SENSOR_TYPE_BACK) == 0) {
                SENSOR_LOGI("sensor sensor matched %dth  is %s", i,
                            CAMERA_SENSOR_TYPE_BACK);
                retValue = i;
                break;
            }
        }
    }
    if (sensor_id == SENSOR_SUB) {
        sSnNum = ARRAY_SIZE(front_sensor_infor_tab) - 1;
        SENSOR_LOGI("sensor sensorTypeMatch sub is %d", sSnNum);
        for (i = 0; i < sSnNum; i++) {
            if (strcmp(front_sensor_infor_tab[i].sn_name,
                       CAMERA_SENSOR_TYPE_FRONT) == 0) {
                SENSOR_LOGI("sensor sensor matched the %dth is %s", i,
                            CAMERA_SENSOR_TYPE_FRONT);
                retValue = i;
                break;
            }
        }
    }
    if (sensor_id == SENSOR_DEVICE2) {
        mSnNum = ARRAY_SIZE(back_ext_sensor_infor_tab) - 1;
        SENSOR_LOGI("sensor sensorTypeMatch main2 is %d", mSnNum);
        for (i = 0; i < mSnNum; i++) {
            if (strcmp(back_ext_sensor_infor_tab[i].sn_name,
                       CAMERA_SENSOR_TYPE_BACK_EXT) == 0) {
                SENSOR_LOGI("sensor sensor matched %dth is %s", i,
                            CAMERA_SENSOR_TYPE_BACK_EXT);
                retValue = i;
                break;
            }
        }
    }
    if (sensor_id == SENSOR_DEVICE3) {
        sSnNum = ARRAY_SIZE(front_ext_sensor_infor_tab) - 1;
        SENSOR_LOGI("sensor sensorTypeMatch sub2 is %d", sSnNum);
        for (i = 0; i < sSnNum; i++) {
            if (strcmp(front_ext_sensor_infor_tab[i].sn_name,
                       CAMERA_SENSOR_TYPE_FRONT_EXT) == 0) {
                SENSOR_LOGI("sensor sensor matched the %dth is %s", i,
                            CAMERA_SENSOR_TYPE_FRONT_EXT);
                retValue = i;
                break;
            }
        }
    }
    return retValue;
}
