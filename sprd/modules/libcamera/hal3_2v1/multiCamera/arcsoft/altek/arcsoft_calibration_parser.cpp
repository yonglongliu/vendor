/*******************************************************************************
Copyright(c) ArcSoft, All right reserved.

This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary
and confidential information.

The information and code contained in this file is only for authorized ArcSoft
employees to design, create, modify, or review.

DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER
AUTHORIZATION.

If you are not an intended recipient of this file, you must not copy,
distribute, modify, or take any action in reliance on it.

If you have received this file in error, please immediately notify ArcSoft and
permanently delete the original and any copy of any file and any printout
thereof.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include "arcsoft_calibration_parser.h"
#include "include/alParseOTP.h"

#define LIB_ALTEKPARSER_PATH "libalParseOTP.so"
#define MASTER_AF_INF_POSITION 0x0010
#define MASTER_AF_MACRO_POSITION 0x0012
#define SLAVE_AF_INF_POSITION 0x1010
#define SLAVE_AF_MACRO_POSITION 0x1012

#ifdef ANDROID
#define TAG "ARC_PARSER"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#endif

typedef struct {
    void *handle;
    alParseOTP_ERR_CODE (*alParseOTP_Func)(void *a_pInOTPBuf, int a_dInOTPSize,
                                           int a_dInVCMStep, int a_dInCamLayout,
                                           ALTEK_OUTPUT *a_tOutStruct);
    alParseOTP_ERR_CODE (*alParseOTP_VersionInfo_Get_Func)(void *a_pOutBuf,
                                                           int a_dInBufMaxSize);
} AltekParseOTP_t;

typedef struct _ArcCaliData {
    ArcParam arcParam;
    AltekParseOTP_t *pAltekAPI;
} ArcCaliData;

MRESULT Arc_CaliData_ZTE_FillAltekParam(ArcCaliData *pCaliData,
                                        ALTEK_OUTPUT *pAltekData);
int LoadAltekAPI(AltekParseOTP_t *pAltekAPI);
int UnloadAltekAPI(AltekParseOTP_t *pAltekAPI);
MRESULT Arc_CaliData_PrintAltek(ALTEK_OUTPUT *a_tOutStruct);

ARCCALI_API MRESULT Arc_CaliData_Init(MHandle *phHandle) {
    if (phHandle == NULL)
        return MERR_CAL_INVALID_ARGUMENT;

    ArcCaliData *pCaliData = (ArcCaliData *)malloc(sizeof(ArcCaliData));
    if (pCaliData == NULL) {
        return MERR_CAL_OUTOFMEMORY;
    }
    memset(pCaliData, 0, sizeof(sizeof(ArcCaliData)));
    *phHandle = pCaliData;

    pCaliData->pAltekAPI = (AltekParseOTP_t *)malloc(sizeof(AltekParseOTP_t));
    if (pCaliData->pAltekAPI == NULL) {
        LOGE("pAltekAPI malloc failed.");
    } else {
        memset(pCaliData->pAltekAPI, 0, sizeof(AltekParseOTP_t));
    }
    if (LoadAltekAPI(pCaliData->pAltekAPI) < 0) {
        LOGE("load arcsoft bokeh api failed.");
    }

    return MOK;
}

ARCCALI_API MRESULT Arc_CaliData_Uninit(MHandle *phHandle) {
    if (phHandle == NULL)
        return MERR_CAL_INVALID_ARGUMENT;

    ArcCaliData *pCaliData = (ArcCaliData *)(*phHandle);

    UnloadAltekAPI(pCaliData->pAltekAPI);
    if (pCaliData->pAltekAPI) {
        free(pCaliData->pAltekAPI);
        pCaliData->pAltekAPI = NULL;
    }

    free(pCaliData);
    *phHandle = NULL;
    return MOK;
}

ARCCALI_API MRESULT Arc_CaliData_ParseDualCamData(MHandle hHandle,
                                                  void *pDualCamData,
                                                  int nBufSize, int caliType,
                                                  void *pParam) {
    LOGI("enter Arc_CaliData_ParseDualCamData \n");
    LOGI("hHandle:%p pDualCamData:%p nBufSize:%d caliType:%d pParam:%p \n",
         hHandle, pDualCamData, nBufSize, caliType, pParam);

    if (hHandle == NULL || pDualCamData == NULL || nBufSize <= 0)
        return MERR_CAL_INVALID_ARGUMENT;

    if (caliType == cali_type_altek) {
        if (pParam == NULL)
            return MERR_CAL_INVALID_ARGUMENT;

        ArcCaliData *pCaliData = (ArcCaliData *)hHandle;

        char acVersion[256] = {0};
        pCaliData->pAltekAPI->alParseOTP_VersionInfo_Get_Func(acVersion, 256);
        LOGD("Altek alParseOTP_VersionInfo_Get: %s", acVersion);

        AltekParam *pAltekParam = (AltekParam *)pParam;
        ALTEK_OUTPUT a_tOutStruct;

        LOGD("fliu a_dInVCMStep: %d", pAltekParam->a_dInVCMStep);
        LOGD("Altek a_dInCamLayout %d", pAltekParam->a_dInCamLayout);

        alParseOTP_ERR_CODE err = pCaliData->pAltekAPI->alParseOTP_Func(
            pDualCamData, nBufSize, pAltekParam->a_dInVCMStep,
            pAltekParam->a_dInCamLayout, &a_tOutStruct);
        LOGD("Altek alParseOTP_Func: err: %d", err);

        if (err == alParseOTP_ERR_SUCCESS) {
            Arc_CaliData_ZTE_FillAltekParam(pCaliData, &a_tOutStruct);
            Arc_CaliData_PrintAltek(&a_tOutStruct);

            return MOK;
        } else {
            LOGE("alParseOTP error: %d", err);
        }
    }

    return MERR_CAL_INVALID_ARGUMENT;
}

ARCCALI_API MRESULT Arc_CaliData_ParseOTPData(MHandle hHandle, void *pOTPData,
                                              int nBufSize, int otpType,
                                              void *pParam) {
    if (hHandle == NULL || pOTPData == NULL || nBufSize <= 0)
        return MERR_CAL_INVALID_ARGUMENT;

    if (otp_type_zte == otpType) {
        if (nBufSize < SLAVE_AF_MACRO_POSITION) ///  check if buf is too small
            return MERR_CAL_INVALID_ARGUMENT;

        ArcCaliData *pCaliData = (ArcCaliData *)hHandle;
        ArcParam *pArcParam = &pCaliData->arcParam;

        unsigned char *pOTPAddress = (unsigned char *)pOTPData;

        short master_af_inf_pos = 0;
        short master_af_macro_pos = 0;
        master_af_inf_pos = pOTPAddress[MASTER_AF_INF_POSITION + 1] << 8 |
                            pOTPAddress[MASTER_AF_INF_POSITION];
        master_af_macro_pos = pOTPAddress[MASTER_AF_MACRO_POSITION + 1] << 8 |
                              pOTPAddress[MASTER_AF_MACRO_POSITION];

        short slave_af_inf_pos = 0;
        short slave_af_macro_pos = 0;
        slave_af_inf_pos = pOTPAddress[SLAVE_AF_INF_POSITION + 1] << 8 |
                           pOTPAddress[SLAVE_AF_INF_POSITION];
        slave_af_macro_pos = pOTPAddress[SLAVE_AF_MACRO_POSITION + 1] << 8 |
                             pOTPAddress[SLAVE_AF_MACRO_POSITION];

        pArcParam->left_afcode1 =
            master_af_macro_pos; // 12. 左镜头对焦在标定距离时的 AFcode 值
        pArcParam->left_afcode2 =
            master_af_inf_pos; // 13. 左镜头对焦在无穷远处的 AFcode 值

        pArcParam->right_afcode1 =
            slave_af_macro_pos; // 40. 右镜头对焦在标定距离时的 AFcode 值
        pArcParam->right_afcode2 =
            slave_af_inf_pos; // 41. 右镜头对焦在无穷远处的 AFcode 值
    }

    return MOK;
}

ARCCALI_API MRESULT Arc_CaliData_GetArcParam(MHandle hHandle, void *pArcParam,
                                             int bufSize) {
    if (hHandle == NULL || pArcParam == NULL)
        return MERR_CAL_INVALID_ARGUMENT;

    int ArcParamSize = sizeof(ArcParam);
    if (bufSize < ArcParamSize) {
        return MERR_CAL_INVALID_ARGUMENT;
    }

    ArcCaliData *pCaliData = (ArcCaliData *)hHandle;

    memcpy(pArcParam, (void *)&pCaliData->arcParam, sizeof(ArcParam));
    return MOK;
}

ARCCALI_API MRESULT Arc_CaliData_PrintArcParam(ArcParam *pParam) {
    if (pParam == NULL)
        return MERR_CAL_INVALID_ARGUMENT;

    LOGI("1  left_fx: %.3f\n", pParam->left_fx);
    LOGI("2  left_fy: %.3f\n", pParam->left_fy);
    LOGI("3  left_u: %.3f\n", pParam->left_u);
    LOGI("4  left_v: %.3f\n", pParam->left_v);
    LOGI("5  left_skew: %.3f\n", pParam->left_skew);
    LOGI("6  left_k1: %.3f\n", pParam->left_k1);
    LOGI("7  left_k2: %.3f\n", pParam->left_k2);
    LOGI("8  left_k3: %.3f\n", pParam->left_k3);
    LOGI("9  left_p1: %.3f\n", pParam->left_p1);
    LOGI("10 left_p2: %.3f\n", pParam->left_p2);
    LOGI("11 left_cal_distance: %.3f\n", pParam->left_cal_distance);
    LOGI("12 left_afcode1: %.3f\n", pParam->left_afcode1);
    LOGI("13 left_afcode2: %.3f\n", pParam->left_afcode2);
    LOGI("14 left_intf_f: %.3f\n", pParam->left_intf_f);
    LOGI("15 left_pixelsize: %.3f\n", pParam->left_pixelsize);
    LOGI("16 left_image_width: %.3f\n", pParam->left_image_width);
    LOGI("17 left_image_height: %.3f\n", pParam->left_image_height);
    LOGI("18 left_fov: %.3f\n", pParam->left_fov);
    LOGI("19 left_field_1: %.3f\n", pParam->left_field_1);
    LOGI("20 left_field_2: %.3f\n", pParam->left_field_2);
    LOGI("21 left_field_3: %.3f\n", pParam->left_field_3);
    LOGI("22 left_field_4: %.3f\n", pParam->left_field_4);
    LOGI("23 left_field_5: %.3f\n", pParam->left_field_5);
    LOGI("24 left_field_6: %.3f\n", pParam->left_field_6);
    LOGI("25 left_field_7: %.3f\n", pParam->left_field_7);
    LOGI("26 left_field_8: %.3f\n", pParam->left_field_8);
    LOGI("27 left_field_9: %.3f\n", pParam->left_field_9);
    LOGI("28 left_field_10: %.3f\n", pParam->left_field_10);

    LOGI("29 right_fx: %.3f\n", pParam->right_fx);
    LOGI("30 right_fy: %.3f\n", pParam->right_fy);
    LOGI("31 right_u: %.3f\n", pParam->right_u);
    LOGI("32 right_v: %.3f\n", pParam->right_v);
    LOGI("33 right_skew: %.3f\n", pParam->right_skew);
    LOGI("34 right_k1: %.3f\n", pParam->right_k1);
    LOGI("35 right_k2: %.3f\n", pParam->right_k2);
    LOGI("36 right_k3: %.3f\n", pParam->right_k3);
    LOGI("37 right_p1: %.3f\n", pParam->right_p1);
    LOGI("38 right_p2: %.3f\n", pParam->right_p2);
    LOGI("39 right_cal_distance: %.3f\n", pParam->right_cal_distance);
    LOGI("40 right_afcode1: %.3f\n", pParam->right_afcode1);
    LOGI("41 right_afcode2: %.3f\n", pParam->right_afcode2);
    LOGI("42 right_intf_f: %.3f\n", pParam->right_intf_f);
    LOGI("43 right_pixelsize: %.3f\n", pParam->right_pixelsize);
    LOGI("44 right_image_width: %.3f\n", pParam->right_image_width);
    LOGI("45 right_image_height: %.3f\n", pParam->right_image_height);
    LOGI("46 right_fov: %.3f\n", pParam->right_fov);
    LOGI("47 right_field_1: %.3f\n", pParam->right_field_1);
    LOGI("48 right_field_2: %.3f\n", pParam->right_field_2);
    LOGI("49 right_field_3: %.3f\n", pParam->right_field_3);
    LOGI("50 right_field_4: %.3f\n", pParam->right_field_4);
    LOGI("51 right_field_5: %.3f\n", pParam->right_field_5);
    LOGI("52 right_field_6: %.3f\n", pParam->right_field_6);
    LOGI("53 right_field_7: %.3f\n", pParam->right_field_7);
    LOGI("54 right_field_8: %.3f\n", pParam->right_field_8);
    LOGI("55 right_field_9: %.3f\n", pParam->right_field_9);
    LOGI("56 right_field_10: %.3f\n", pParam->right_field_10);

    LOGI("57 rx: %.3f\n", pParam->rx);
    LOGI("58 ry: %.3f\n", pParam->ry);
    LOGI("59 rz: %.3f\n", pParam->rz);
    LOGI("60 tx: %.3f\n", pParam->tx);
    LOGI("61 ty: %.3f\n", pParam->ty);
    LOGI("62 tz: %.3f\n", pParam->tz);
    LOGI("63 camera_layout1: %.3f\n", pParam->camera_layout1);
    LOGI("64 camera_layout2: %.3f\n", pParam->camera_layout2);

    FILE *fp = NULL;
    fp = fopen("/data/misc/cameraserver/dualcam_cali.data", "wb");
    if (fp) {
        fwrite(pParam, 1, sizeof(ArcParam), fp);
        fclose(fp);
    }

    return MOK;
}

int LoadAltekAPI(AltekParseOTP_t *pAltekAPI) {
    const char *error = NULL;

    if (pAltekAPI == NULL)
        return -1;

    pAltekAPI->handle = dlopen(LIB_ALTEKPARSER_PATH, RTLD_LAZY);

    pAltekAPI->alParseOTP_Func =
        (alParseOTP_ERR_CODE (*)(void *a_pInOTPBuf, int a_dInOTPSize,
                                 int a_dInVCMStep, int a_dInCamLayout,
                                 ALTEK_OUTPUT *a_tOutStruct))
            dlsym(pAltekAPI->handle, "alParseOTP");
    if (pAltekAPI->alParseOTP_Func == NULL) {
        error = dlerror();
        LOGE("sym alParseOTP failed.error = %s", error);
        return -1;
    }

    pAltekAPI->alParseOTP_VersionInfo_Get_Func =
        (alParseOTP_ERR_CODE (*)(void *a_pOutBuf, int a_dInBufMaxSize))dlsym(
            pAltekAPI->handle, "alParseOTP_VersionInfo_Get");
    if (pAltekAPI->alParseOTP_VersionInfo_Get_Func == NULL) {
        error = dlerror();
        LOGE("sym alParseOTP_VersionInfo_Get failed.error = %s", error);
        return -1;
    }

    return 0;
}

int UnloadAltekAPI(AltekParseOTP_t *pAltekAPI) {
    if (pAltekAPI == NULL)
        return -1;

    if (pAltekAPI->handle != NULL) {
        dlclose(pAltekAPI->handle);
        pAltekAPI->handle = NULL;
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////
MRESULT Arc_CaliData_ZTE_FillAltekParam(ArcCaliData *pCaliData,
                                        ALTEK_OUTPUT *pAltekData) {
    if (pCaliData == NULL || pAltekData == NULL)
        return MERR_CAL_INVALID_ARGUMENT;

    ArcParam *pArcParam = &pCaliData->arcParam;

    pArcParam->left_fx = pAltekData->fx_L; // 1. 左镜头水平方向焦距 fx
    pArcParam->left_fy = pAltekData->fy_L; // 2. 左镜头垂直方向焦距 fy
    pArcParam->left_u = pAltekData->u0_L;  // 3. 左镜头主点横坐标 u
    pArcParam->left_v = pAltekData->v0_L;  // 4. 左镜头主点纵坐标点 v

    pArcParam->left_skew = 0; // 5. 左镜头倾斜系数 skew
    pArcParam->left_k1 = 0;   // 6. 左镜头径向一阶畸变系数 k1
    pArcParam->left_k2 = 0;   // 7. 左镜头径向二阶畸变系数 k2
    pArcParam->left_k3 = 0;   // 8. 左镜头径向三阶畸变系数 k3
    pArcParam->left_p1 = 0;   // 9. 左镜头切向畸变系数 p1
    pArcParam->left_p2 = 0;   // 10. 左镜头切向畸变系数 p2

    pArcParam->left_cal_distance = 600; //  11. 左镜头标定距离(mm)
    // pArcParam->left_afcode1; // 12. 左镜头对焦在标定距离时的 AFcode 值
    // pArcParam->left_afcode2; // 13. 左镜头对焦在无穷远处的 AFcode 值
    pArcParam->left_intf_f = 3.57; // 14. 左镜头对焦在无穷远处的焦距(mm)
    pArcParam->left_pixelsize = 1.75; // 15. 左镜头传感器的单位像素物理大小(um)
    pArcParam->left_image_width = pAltekData->width_L; // 16. 左镜头标定图像的宽
    pArcParam->left_image_height =
        pAltekData->height_L; // 17. 左镜头标定图像的高
    // pArcParam->left_image_width = CALIBRATION_WIDTH; // 16.
    // 左镜头标定图像的宽
    // pArcParam->left_image_height = CALIBRATION_HEIGHT; // 17.
    // 左镜头标定图像的高

    pArcParam->left_fov = 78.4; // 18. 左镜头 FOV

    pArcParam->left_field_1 = 0.27;  // 19. 右镜头 0.1field 处畸变值
    pArcParam->left_field_2 = 0.63;  // 20. 右镜头 0.2field 处畸变值
    pArcParam->left_field_3 = 0.7;   // 21. 右镜头 0.3field 处畸变值
    pArcParam->left_field_4 = 0.7;   // 22. 右镜头 0.4field 处畸变值
    pArcParam->left_field_5 = 0.71;  // 23. 右镜头 0.5field 处畸变值
    pArcParam->left_field_6 = 0.67;  // 24. 右镜头 0.6field 处畸变值
    pArcParam->left_field_7 = 0.49;  // 25. 右镜头 0.7field 处畸变值
    pArcParam->left_field_8 = 0.05;  // 26. 右镜头 0.8field 处畸变值
    pArcParam->left_field_9 = -0.23; // 27. 右镜头 0.9field 处畸变值
    pArcParam->left_field_10 = 0.06; // 28. 左镜头 1.0field 处畸变值

    pArcParam->right_fx = pAltekData->fx_R; // 29. 右镜头水平方向焦距 fx
    pArcParam->right_fy = pAltekData->fy_R; // 30. 右镜头垂直方向焦距 fy

    pArcParam->right_u = pAltekData->u0_R; // 31. 右镜头主点横坐标 u
    pArcParam->right_v = pAltekData->v0_R; // 32. 右镜头主点纵坐标点 v

    pArcParam->right_skew = 0; // 33. 右镜头倾斜系数 skew
    pArcParam->right_k1 = 0;   // 34. 右镜头径向一阶畸变系数 k1
    pArcParam->right_k2 = 0;   // 35. 右镜头径向二阶畸变系数 k2
    pArcParam->right_k3 = 0;   // 36. 右镜头径向三阶畸变系数 k3
    pArcParam->right_p1 = 0;   // 37. 右镜头切向畸变系数 p1
    pArcParam->right_p2 = 0;   // 38. 右镜头切向畸变系数 p2

    pArcParam->right_cal_distance = 600; //  39. 右镜头标定距离(mm)
    // pArcParam->right_afcode1; // 40. 右镜头对焦在标定距离时的 AFcode 值
    // pArcParam->right_afcode2; // 41. 右镜头对焦在无穷远处的 AFcode 值
    pArcParam->right_intf_f = 2.0; // 42. 右镜头对焦在无穷远处的焦距(mm)
    pArcParam->right_pixelsize = 1.12; // 43. 右镜头传感器的单位像素物理大小(um)
    pArcParam->right_image_width =
        pAltekData->width_R; // 44. 右镜头标定图像的宽
    pArcParam->right_image_height =
        pAltekData->height_R; // 45. 右镜头标定图像的高
    // pArcParam->right_image_width = CALIBRATION_WIDTH; // 44.
    // 右镜头标定图像的宽
    // pArcParam->right_image_height = CALIBRATION_HEIGHT; // 45.
    // 右镜头标定图像的高

    pArcParam->right_fov = 83; // 46. 右镜头 FOV

    pArcParam->right_field_1 = 0.05; // 47. 左镜头 0.1field 处畸变值
    pArcParam->right_field_2 = 0.28; // 48. 左镜头 0.2field 处畸变值
    pArcParam->right_field_3 = 0.69; // 49. 左镜头 0.3field 处畸变值
    pArcParam->right_field_4 = 1.11; // 50. 左镜头 0.4field 处畸变值
    pArcParam->right_field_5 = 1.43; // 51. 左镜头 0.5field 处畸变值
    pArcParam->right_field_6 = 1.57; // 52. 左镜头 0.6field 处畸变值
    pArcParam->right_field_7 = 1.5;  // 53. 左镜头 0.7field 处畸变值
    pArcParam->right_field_8 = 1.42; // 54. 左镜头 0.8field 处畸变值
    pArcParam->right_field_9 = 1.5;  // 55. 左镜头 0.9field 处畸变值
    pArcParam->right_field_10 = 1.3; // 56. 左镜头 1.0field 处畸变值

    pArcParam->rx = pAltekData->Rx;      // 57. 外参旋转矢量 Rx
    pArcParam->ry = pAltekData->Ry;      // 58. 外参旋转矢量 Ry
    pArcParam->rz = pAltekData->Rz;      // 59. 外参旋转矢量 Rz
    pArcParam->tx = pAltekData->Tx * 10; // 60. X 方向上的相对移动Tx
    pArcParam->ty = pAltekData->Ty * 10; // 61. Y 方向上的相对移动Ty
    pArcParam->tz = pAltekData->Tz * 10; // 62. Z 方向上的相对移动Tz

    pArcParam->camera_layout1 = 0; // 63. 摄像头横纵排列方式, ArcSoft: 0 for
                                   // horizontal layout,  1 for vertical layout
    pArcParam->camera_layout2 = 0; // 64. 左右摄像头相对位置关系, ArcSoft: 1
                                   // main camera on left, 0 main camera on
                                   // right

    return MOK;
}

MRESULT Arc_CaliData_PrintAltek(ALTEK_OUTPUT *a_tOutStruct) {
    LOGI("Altek fx_L:%f", a_tOutStruct->fx_L);
    LOGI("Altek fy_L:%f", a_tOutStruct->fy_L);
    LOGI("Altek u0_L:%f", a_tOutStruct->u0_L);
    LOGI("Altek v0_L:%f", a_tOutStruct->v0_L);
    LOGI("Altek width_L:%f", a_tOutStruct->width_L);
    LOGI("Altek height_L:%f", a_tOutStruct->height_L);

    LOGI("Altek fx_R:%f", a_tOutStruct->fx_R);
    LOGI("Altek fy_R:%f", a_tOutStruct->fy_R);
    LOGI("Altek u0_R:%f", a_tOutStruct->u0_R);
    LOGI("Altek v0_R:%f", a_tOutStruct->v0_R);
    LOGI("Altek width_R:%f", a_tOutStruct->width_R);
    LOGI("Altek height_R:%f", a_tOutStruct->height_R);

    LOGI("Altek Rx:%f", a_tOutStruct->Rx);
    LOGI("Altek Ry:%f", a_tOutStruct->Ry);
    LOGI("Altek Rz:%f", a_tOutStruct->Rz);
    LOGI("Altek Tx:%f", a_tOutStruct->Tx);
    LOGI("Altek Ty:%f", a_tOutStruct->Ty);
    LOGI("Altek Tz:%f", a_tOutStruct->Tz);

    LOGI("Altek type1:%d", a_tOutStruct->type1);
    LOGI("Altek type2:%d", a_tOutStruct->type2);

    return MOK;
}
