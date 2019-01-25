
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

#ifndef ARCSOFT_CALIBRATION_PARSER_H_
#define ARCSOFT_CALIBRATION_PARSER_H_

#include "amcomdef.h"
#include "merror.h"

#ifdef _WIN32
#ifdef PICOBJECTDLL_EXPORTS
#define ARCCALI_API __declspec(dllexport)
#else
#define ARCCALI_API
#endif
#else
#if defined(__GNUC__) && (__GNUC__ >= 4)
#define ARCCALI_API __attribute__((visibility("default")))
#else
#define ARCCALI_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// Get calibration data from other's calibration
#define MERR_CAL_BASE 0XF000
#define MERR_CAL_INVALID_ARGUMENT (MERR_CAL_BASE + 1)
#define MERR_CAL_OUTOFMEMORY (MERR_CAL_BASE + 2)

// #define CALIBRATION_WIDTH	960
// #define CALIBRATION_HEIGHT  1280

typedef struct _ArcParam {
    double left_fx;           // 1. 左镜头水平方向焦距 fx
    double left_fy;           // 2. 左镜头垂直方向焦距 fy
    double left_u;            // 3. 左镜头主点横坐标 u
    double left_v;            // 4. 左镜头主点纵坐标点 v
    double left_skew;         // 5. 左镜头倾斜系数 skew
    double left_k1;           // 6. 左镜头径向一阶畸变系数 k1
    double left_k2;           // 7. 左镜头径向二阶畸变系数 k2
    double left_k3;           // 8. 左镜头径向三阶畸变系数 k3
    double left_p1;           // 9. 左镜头切向畸变系数 p1
    double left_p2;           // 10. 左镜头切向畸变系数 p2
    double left_cal_distance; //  11. 左镜头标定距离(mm)
    double left_afcode1; // 12. 左镜头对焦在标定距离时的 AFcode 值
    double left_afcode2; // 13. 左镜头对焦在无穷远处的 AFcode 值
    double left_intf_f;  // 14. 左镜头对焦在无穷远处的焦距(mm)
    double left_pixelsize; // 15. 左镜头传感器的单位像素物理大小(um)
    double left_image_width;   // 16. 左镜头标定图像的宽
    double left_image_height;  // 17. 左镜头标定图像的高
    double left_fov;           // 18. 左镜头 FOV
    double left_field_1;       // 19. 左镜头 0.1field 处畸变值
    double left_field_2;       // 20. 左镜头 0.2field 处畸变值
    double left_field_3;       // 21. 左镜头 0.3field 处畸变值
    double left_field_4;       // 22. 左镜头 0.4field 处畸变值
    double left_field_5;       // 23. 左镜头 0.5field 处畸变值
    double left_field_6;       // 24. 左镜头 0.6field 处畸变值
    double left_field_7;       // 25. 左镜头 0.7field 处畸变值
    double left_field_8;       // 26. 左镜头 0.8field 处畸变值
    double left_field_9;       // 27. 左镜头 0.9field 处畸变值
    double left_field_10;      // 28. 左镜头 1.0field 处畸变值
    double right_fx;           // 29. 右镜头水平方向焦距 fx
    double right_fy;           // 30. 右镜头垂直方向焦距 fy
    double right_u;            // 31. 右镜头主点横坐标 u
    double right_v;            // 32. 右镜头主点纵坐标点 v
    double right_skew;         // 33. 右镜头倾斜系数 skew
    double right_k1;           // 34. 右镜头径向一阶畸变系数 k1
    double right_k2;           // 35. 右镜头径向二阶畸变系数 k2
    double right_k3;           // 36. 右镜头径向三阶畸变系数 k3
    double right_p1;           // 37. 右镜头切向畸变系数 p1
    double right_p2;           // 38. 右镜头切向畸变系数 p2
    double right_cal_distance; //  39. 右镜头标定距离(mm)
    double right_afcode1; // 40. 右镜头对焦在标定距离时的 AFcode 值
    double right_afcode2; // 41. 右镜头对焦在无穷远处的 AFcode 值
    double right_intf_f;  // 42. 右镜头对焦在无穷远处的焦距(mm)
    double right_pixelsize; // 43. 右镜头传感器的单位像素物理大小(um)
    double right_image_width;  // 44. 右镜头标定图像的宽
    double right_image_height; // 45. 右镜头标定图像的高
    double right_fov;          // 46. 右镜头 FOV
    double right_field_1;      // 47. 右镜头 0.1field 处畸变值
    double right_field_2;      // 48. 右镜头 0.2field 处畸变值
    double right_field_3;      // 49. 右镜头 0.3field 处畸变值
    double right_field_4;      // 50. 右镜头 0.4field 处畸变值
    double right_field_5;      // 51. 右镜头 0.5field 处畸变值
    double right_field_6;      // 52. 右镜头 0.6field 处畸变值
    double right_field_7;      // 53. 右镜头 0.7field 处畸变值
    double right_field_8;      // 54. 右镜头 0.8field 处畸变值
    double right_field_9;      // 55. 右镜头 0.9field 处畸变值
    double right_field_10;     // 56. 右镜头 1.0field 处畸变值
    double rx;                 // 57. 外参旋转矢量 Rx
    double ry;                 // 58. 外参旋转矢量 Ry
    double rz;                 // 59. 外参旋转矢量 Rz
    double tx;                 // 60. X 方向上的相对移动Tx
    double ty;                 // 61. Y 方向上的相对移动Ty
    double tz;                 // 62. Z 方向上的相对移动Tz
    double camera_layout1;     // 63. 摄像头横纵排列方式
    double camera_layout2;     // 64. 左右摄像头相对位置关系
} ArcParam;

#define otp_type_zte 1
#define cali_type_altek 1

typedef struct _AltekParam {
    int a_dInVCMStep;
    int a_dInCamLayout;
} AltekParam;

MRESULT Arc_CaliData_Init(MHandle *phHandle);
MRESULT Arc_CaliData_Uninit(MHandle *phHandle);
MRESULT Arc_CaliData_ParseDualCamData(MHandle hHandle, void *pDualCamData,
                                      int nBufSize, int caliType, void *pParam);
MRESULT Arc_CaliData_ParseOTPData(MHandle hHandle, void *pOTPData, int nBufSize,
                                  int otpType, void *pParam);
MRESULT Arc_CaliData_GetArcParam(MHandle hHandle, void *pArcParam,
                                 int bufSize); /// buf size in bytes
MRESULT Arc_CaliData_PrintArcParam(ArcParam *pParam);

#ifdef __cplusplus
}
#endif

#endif /// ARCSOFT_CALIBRATION_PARSER_H_